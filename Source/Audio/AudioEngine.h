#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Audio/AudioTypes.h"
#include "Audio/ParameterSmoother.h"
#include <atomic>
#include <memory>

namespace grainhex {

class SourceSampleManager;

/**
 * AudioEngine owns the audio device and callback.
 * Phase 1: plays loaded samples with looping.
 * Includes a sine test mode for verifying the audio path.
 */
class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void initialise();
    void shutdown();

    // AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // Source sample
    void setSourceSample(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate);

    // Transport
    void play();
    void stop();
    TransportState getTransportState() const;

    // Loop
    void setLoopRegion(int64_t startSample, int64_t endSample);
    LoopRegion getLoopRegion() const;
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const;

    // Playhead position for UI (atomic read)
    int64_t getPlayheadPosition() const;

    // Sine test mode
    void setSineTestEnabled(bool enabled);
    bool isSineTestEnabled() const;

    // Volume
    void setMasterVolume(float volume);

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

private:
    void renderSineTest(float* const* outputChannelData, int numOutputChannels, int numSamples);
    void renderSamplePlayback(float* const* outputChannelData, int numOutputChannels, int numSamples);

    juce::AudioDeviceManager deviceManager;

    // Source sample (shared_ptr for safe swap)
    std::shared_ptr<juce::AudioBuffer<float>> sourceBuffer;
    double sourceSampleRate = 44100.0;
    double deviceSampleRate = 44100.0;

    // Transport
    std::atomic<TransportState> transportState { TransportState::Stopped };
    std::atomic<int64_t> playheadPosition { 0 };

    // Loop
    std::atomic<int64_t> loopStart { 0 };
    std::atomic<int64_t> loopEnd { 0 };
    std::atomic<bool> loopEnabled { true };

    // Sine test
    std::atomic<bool> sineTestEnabled { false };
    double sinePhase = 0.0;
    static constexpr double sineFrequency = 440.0;

    // Master volume
    ParameterSmoother volumeSmoother;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

} // namespace grainhex
