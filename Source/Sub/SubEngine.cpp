#include "Sub/SubEngine.h"
#include <cmath>

namespace grainhex {

SubEngine::SubEngine()
{
    levelSmoother.reset(0.0f); // Muted by default
    frequencySmoother.reset(55.0f); // A1 default
}

void SubEngine::setSampleRate(double sampleRate)
{
    currentSampleRate = sampleRate;
    oscillator.setSampleRate(sampleRate);
    levelSmoother.setSmoothingTime(0.01f, sampleRate);
    frequencySmoother.setSmoothingTime(0.05f, sampleRate);

    // Reset filter state and recompute coefficients
    granularHPL.reset();
    granularHPR.reset();
    subLP.reset();
    filtersNeedUpdate = true;
    updateFilterCoefficients();

    pitchDetector.setSampleRate(sampleRate);
}

void SubEngine::setSmoothingSpeed(SmoothingSpeed speed)
{
    float timeSeconds = 0.05f; // Medium default
    switch (speed)
    {
        case SmoothingSpeed::Slow:   timeSeconds = 0.1f;  break;
        case SmoothingSpeed::Medium: timeSeconds = 0.05f; break;
        case SmoothingSpeed::Fast:   timeSeconds = 0.01f; break;
    }
    frequencySmoother.setSmoothingTime(timeSeconds, currentSampleRate);
}

float SubEngine::midiNoteToFrequency(int midiNote)
{
    return 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
}

void SubEngine::setGranularHPFreq(float hz)
{
    granularHPFreq.store(hz, std::memory_order_relaxed);
    filtersNeedUpdate = true;
}

void SubEngine::setSubLPFreq(float hz)
{
    subLPFreq.store(hz, std::memory_order_relaxed);
    filtersNeedUpdate = true;
}

void SubEngine::updateFilterCoefficients()
{
    if (!filtersNeedUpdate)
        return;

    float hpFreq = granularHPFreq.load(std::memory_order_relaxed);
    float lpFreq = subLPFreq.load(std::memory_order_relaxed);

    granularHPL.setCoefficients(BiquadFilter::Type::HighPass, hpFreq, kFilterQ, currentSampleRate);
    granularHPR.setCoefficients(BiquadFilter::Type::HighPass, hpFreq, kFilterQ, currentSampleRate);
    subLP.setCoefficients(BiquadFilter::Type::LowPass, lpFreq, kFilterQ, currentSampleRate);

    filtersNeedUpdate = false;
}

void SubEngine::updateSubFrequencyFromPitch()
{
    auto mode = static_cast<SubTuningMode>(tuningMode.load(std::memory_order_relaxed));

    if (mode == SubTuningMode::Manual)
    {
        int note = manualMidiNote.load(std::memory_order_relaxed);
        int offset = octaveOffset.load(std::memory_order_relaxed);
        int targetNote = note + (offset * 12);
        targetNote = std::max(0, std::min(127, targetNote));
        frequencySmoother.setTargetValue(midiNoteToFrequency(targetNote));
        return;
    }

    // Auto mode: derive frequency from detected pitch
    PitchInfo pitch = pitchDetector.getLastStablePitch();
    if (pitch.frequency <= 0.0f || pitch.midiNote < 0)
        return; // No valid pitch yet, keep current frequency

    int offset = octaveOffset.load(std::memory_order_relaxed);
    auto snapMode = static_cast<PitchSnapMode>(pitchSnapMode.load(std::memory_order_relaxed));

    float targetFreq;
    if (snapMode == PitchSnapMode::Strict)
    {
        // Snap to nearest semitone, then apply octave offset
        int targetNote = pitch.midiNote + (offset * 12);
        targetNote = std::max(0, std::min(127, targetNote));
        targetFreq = midiNoteToFrequency(targetNote);
    }
    else
    {
        // Loose: follow exact frequency with octave offset
        targetFreq = pitch.frequency * std::pow(2.0f, static_cast<float>(offset));
    }

    // Clamp to audible sub range
    targetFreq = std::max(16.0f, std::min(250.0f, targetFreq));
    frequencySmoother.setTargetValue(targetFreq);
}

void SubEngine::applyGranularHP(float* left, float* right, int numSamples)
{
    if (!subEnabled.load(std::memory_order_relaxed))
        return;

    updateFilterCoefficients();

    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = granularHPL.processSample(left[i]);
        if (right != left)
            right[i] = granularHPR.processSample(right[i]);
    }
}

void SubEngine::processBlock(float* outL, float* outR, int numSamples)
{
    if (!subEnabled.load(std::memory_order_relaxed))
        return;

    updateFilterCoefficients();

    // Update sub frequency from pitch detection / manual selection
    updateSubFrequencyFromPitch();

    // Update waveform if changed
    oscillator.setWaveform(static_cast<SubWaveform>(waveform.load(std::memory_order_relaxed)));

    for (int i = 0; i < numSamples; ++i)
    {
        // Per-sample frequency update for smooth pitch transitions
        float freq = frequencySmoother.getNextValue();
        oscillator.setFrequency(freq);

        float level = levelSmoother.getNextValue();
        float sample = oscillator.getNextSample() * level;

        // Apply LP filter to sub output
        sample = subLP.processSample(sample);

        // Sub is mono — add equally to both channels
        outL[i] += sample;
        if (outR != outL)
            outR[i] += sample;
    }
}

void SubEngine::feedPitchDetector(const float* left, const float* right, int numSamples)
{
    // Mix to mono for pitch analysis
    int count = std::min(numSamples, kMaxBlockSize);
    for (int i = 0; i < count; ++i)
        monoMixBuffer[static_cast<size_t>(i)] = (left[i] + right[i]) * 0.5f;

    pitchDetector.feedSamples(monoMixBuffer.data(), count);

    // Run detection and update atomic snapshot
    PitchInfo pitch = pitchDetector.detect();
    detectedFrequency.store(pitch.frequency, std::memory_order_relaxed);
    detectedConfidence.store(pitch.confidence, std::memory_order_relaxed);
    detectedMidiNote.store(pitch.midiNote, std::memory_order_relaxed);
    detectedCentsOffset.store(pitch.centsOffset, std::memory_order_relaxed);
}

PitchInfo SubEngine::getDetectedPitch() const
{
    PitchInfo info;
    info.frequency = detectedFrequency.load(std::memory_order_relaxed);
    info.confidence = detectedConfidence.load(std::memory_order_relaxed);
    info.midiNote = detectedMidiNote.load(std::memory_order_relaxed);
    info.centsOffset = detectedCentsOffset.load(std::memory_order_relaxed);
    return info;
}

} // namespace grainhex
