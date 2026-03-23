#include "Audio/AudioEngine.h"
#include <cmath>

namespace grainhex {

AudioEngine::AudioEngine()
{
    volumeSmoother.reset(0.8f);
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::initialise()
{
    // Enable MIDI input
    auto result = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (result.isNotEmpty())
    {
        DBG("AudioEngine: Failed to initialise audio device: " + result);
        // Try with larger buffer as fallback
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.bufferSize = 512;
        deviceManager.setAudioDeviceSetup(setup, true);
    }

    // Enable all available MIDI inputs
    auto midiDevices = juce::MidiInput::getAvailableDevices();
    for (auto& dev : midiDevices)
    {
        if (!deviceManager.isMidiInputDeviceEnabled(dev.identifier))
            deviceManager.setMidiInputDeviceEnabled(dev.identifier, true);
    }

    deviceManager.addAudioCallback(this);
}

void AudioEngine::shutdown()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    deviceSampleRate = device->getCurrentSampleRate();
    volumeSmoother.setSmoothingTime(0.01f, deviceSampleRate);
    subEngine.setSampleRate(deviceSampleRate);
    effectsChain.setSampleRate(deviceSampleRate);
    modulationEngine.setSampleRate(deviceSampleRate);
    sinePhase = 0.0;
    DBG("AudioEngine: Device started at " + juce::String(deviceSampleRate) + " Hz, buffer "
        + juce::String(device->getCurrentBufferSizeSamples()));
}

void AudioEngine::audioDeviceStopped()
{
    DBG("AudioEngine: Device stopped");
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* /*inputChannelData*/,
    int /*numInputChannels*/,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& /*context*/)
{
    // Clear output first
    for (int ch = 0; ch < numOutputChannels; ++ch)
        juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    if (sineTestEnabled.load(std::memory_order_relaxed))
    {
        renderSineTest(outputChannelData, numOutputChannels, numSamples);
        return;
    }

    if (granularEnabled.load(std::memory_order_relaxed))
    {
        renderGranular(outputChannelData, numOutputChannels, numSamples);
    }
    else if (transportState.load(std::memory_order_relaxed) == TransportState::Playing)
    {
        renderSamplePlayback(outputChannelData, numOutputChannels, numSamples);
    }

    // Mono lock: sum stereo to mono for consistent bass monitoring
    if (monoLocked.load(std::memory_order_relaxed) && numOutputChannels >= 2)
    {
        float* outL = outputChannelData[0];
        float* outR = outputChannelData[1];
        for (int i = 0; i < numSamples; ++i)
        {
            float mono = (outL[i] + outR[i]) * 0.5f;
            outL[i] = mono;
            outR[i] = mono;
        }
    }
}

void AudioEngine::renderSineTest(float* const* outputChannelData, int numOutputChannels, int numSamples)
{
    const double phaseIncrement = sineFrequency * 2.0 * juce::MathConstants<double>::pi / deviceSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = static_cast<float>(std::sin(sinePhase)) * 0.3f;
        sinePhase += phaseIncrement;
        if (sinePhase >= 2.0 * juce::MathConstants<double>::pi)
            sinePhase -= 2.0 * juce::MathConstants<double>::pi;

        for (int ch = 0; ch < numOutputChannels; ++ch)
            outputChannelData[ch][i] = sample;
    }
}

void AudioEngine::renderSamplePlayback(float* const* outputChannelData, int numOutputChannels, int numSamples)
{
    auto buffer = std::atomic_load(&sourceBuffer);
    if (buffer == nullptr || buffer->getNumSamples() == 0)
        return;

    const int sourceChannels = buffer->getNumChannels();
    const int64_t sourceLength = buffer->getNumSamples();

    const int64_t regionStart = loopStart.load(std::memory_order_relaxed);
    const int64_t regionEnd = loopEnd.load(std::memory_order_relaxed);
    const bool looping = loopEnabled.load(std::memory_order_relaxed);

    int64_t pos = playheadPosition.load(std::memory_order_relaxed);

    // Determine effective play region
    const int64_t effectiveStart = (regionEnd > regionStart) ? regionStart : 0;
    const int64_t effectiveEnd = (regionEnd > regionStart) ? regionEnd : sourceLength;

    // Playback rate ratio for sample rate conversion
    const double rateRatio = sourceSampleRate / deviceSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        if (pos < effectiveStart || pos >= effectiveEnd)
        {
            if (looping)
                pos = effectiveStart;
            else
            {
                transportState.store(TransportState::Stopped, std::memory_order_relaxed);
                break;
            }
        }

        float vol = volumeSmoother.getNextValue();

        // Simple nearest-sample playback (Phase 1; interpolated playback comes in Phase 2)
        int64_t readPos = pos;
        if (readPos >= 0 && readPos < sourceLength)
        {
            for (int ch = 0; ch < numOutputChannels; ++ch)
            {
                int srcCh = (ch < sourceChannels) ? ch : 0; // mono->stereo: duplicate
                outputChannelData[ch][i] = buffer->getSample(srcCh, static_cast<int>(readPos)) * vol;
            }
        }

        // Advance with rate conversion
        pos += static_cast<int64_t>(std::round(rateRatio));
        if (pos >= effectiveEnd)
        {
            if (looping)
                pos = effectiveStart;
            else
            {
                transportState.store(TransportState::Stopped, std::memory_order_relaxed);
                break;
            }
        }
    }

    playheadPosition.store(pos, std::memory_order_relaxed);
}

void AudioEngine::renderGranular(float* const* outputChannelData, int numOutputChannels, int numSamples)
{
    // Use first two channels for stereo granular output
    float* outL = (numOutputChannels >= 1) ? outputChannelData[0] : nullptr;
    float* outR = (numOutputChannels >= 2) ? outputChannelData[1] : nullptr;

    if (outL == nullptr)
        return;

    // If mono output, use same buffer for both
    if (outR == nullptr)
        outR = outL;

    granularEngine.processBlock(outL, outR, numSamples, deviceSampleRate);

    // Feed pitch detector with granular output BEFORE effects/sub mix (avoids feedback)
    subEngine.feedPitchDetector(outL, outR, numSamples);

    // Apply modulation (LFO + envelope) per-sample to effects parameters
    applyModulation(outL, outR, numSamples);

    // Effects chain: distortion -> filter (granular only, sub bypasses)
    effectsChain.process(outL, outR, numSamples);

    // Apply HP filter to granular output (removes low end before sub mix)
    subEngine.applyGranularHP(outL, outR, numSamples);

    // Render sub oscillator (additive mix with granular output)
    subEngine.processBlock(outL, outR, numSamples);

    // Apply master volume
    for (int i = 0; i < numSamples; ++i)
    {
        float vol = volumeSmoother.getNextValue();
        outL[i] *= vol;
        if (outR != outL)
            outR[i] *= vol;
    }

    // Feed resample capture with final mixed output (post-volume)
    resampleEngine.feedCapture(outL, outR, numSamples);

    // Copy to remaining channels
    for (int ch = 2; ch < numOutputChannels; ++ch)
        juce::FloatVectorOperations::copy(outputChannelData[ch], outL, numSamples);
}

void AudioEngine::applyModulation(float* /*outL*/, float* /*outR*/, int numSamples)
{
    // Tick modulation sources for each sample in the block
    // and apply the aggregate modulation to the filter envelope input
    for (int i = 0; i < numSamples; ++i)
    {
        auto modValues = modulationEngine.tick();

        // Apply envelope modulation to filter (default routing)
        float filterEnvMod = modulationEngine.getModulationValue(
            ModTarget::FilterCutoff, modValues.lfo, modValues.env);

        // Clamp and pass to filter
        effectsChain.getFilter().setEnvelopeModulation(
            juce::jlimit(0.0f, 1.0f, filterEnvMod + modValues.env));
    }
}

void AudioEngine::setGranularEnabled(bool enabled)
{
    granularEnabled.store(enabled, std::memory_order_relaxed);
}

bool AudioEngine::isGranularEnabled() const
{
    return granularEnabled.load(std::memory_order_relaxed);
}

void AudioEngine::setSourceSample(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate)
{
    sourceSampleRate = sampleRate;
    std::atomic_store(&sourceBuffer, buffer);

    // Forward to granular engine
    granularEngine.setSourceBuffer(buffer, sampleRate);

    // Reset transport
    playheadPosition.store(0, std::memory_order_relaxed);
    loopStart.store(0, std::memory_order_relaxed);
    if (buffer != nullptr)
        loopEnd.store(buffer->getNumSamples(), std::memory_order_relaxed);
    else
        loopEnd.store(0, std::memory_order_relaxed);
}

void AudioEngine::play()
{
    if (transportState.load() == TransportState::Stopped)
    {
        // If at end, restart from loop start
        auto pos = playheadPosition.load();
        auto end = loopEnd.load();
        if (pos >= end)
            playheadPosition.store(loopStart.load());
    }
    transportState.store(TransportState::Playing, std::memory_order_relaxed);
}

void AudioEngine::stop()
{
    transportState.store(TransportState::Stopped, std::memory_order_relaxed);
}

TransportState AudioEngine::getTransportState() const
{
    return transportState.load(std::memory_order_relaxed);
}

void AudioEngine::setLoopRegion(int64_t startSample, int64_t endSample)
{
    loopStart.store(startSample, std::memory_order_relaxed);
    loopEnd.store(endSample, std::memory_order_relaxed);
}

LoopRegion AudioEngine::getLoopRegion() const
{
    return { loopStart.load(std::memory_order_relaxed),
             loopEnd.load(std::memory_order_relaxed) };
}

void AudioEngine::setLoopEnabled(bool enabled)
{
    loopEnabled.store(enabled, std::memory_order_relaxed);
}

bool AudioEngine::isLoopEnabled() const
{
    return loopEnabled.load(std::memory_order_relaxed);
}

int64_t AudioEngine::getPlayheadPosition() const
{
    return playheadPosition.load(std::memory_order_relaxed);
}

void AudioEngine::setSineTestEnabled(bool enabled)
{
    sineTestEnabled.store(enabled, std::memory_order_relaxed);
}

bool AudioEngine::isSineTestEnabled() const
{
    return sineTestEnabled.load(std::memory_order_relaxed);
}

void AudioEngine::setMasterVolume(float volume)
{
    volumeSmoother.setTargetValue(juce::jlimit(0.0f, 1.0f, volume));
}

void AudioEngine::setMonoLock(bool enabled)
{
    monoLocked.store(enabled, std::memory_order_relaxed);
}

bool AudioEngine::isMonoLocked() const
{
    return monoLocked.load(std::memory_order_relaxed);
}

} // namespace grainhex
