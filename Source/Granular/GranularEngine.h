#pragma once

#include "Granular/GrainVoice.h"
#include "Granular/GrainScheduler.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace grainhex {

class GranularEngine
{
public:
    static constexpr int MaxGrains = 256;

    GranularEngine();

    // Set the source buffer (called from message thread)
    void setSourceBuffer(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate);

    // Main render method (called from audio thread)
    void processBlock(float* outL, float* outR, int numSamples, double deviceSampleRate);

    // Parameter setters (thread-safe via atomics)
    void setGrainSize(float sizeMs);
    void setGrainCount(int count);
    void setPosition(float normPos);
    void setSpray(float amount);
    void setPitchSemitones(float semitones);
    void setPitchQuantize(PitchQuantizeScale scale);
    void setWindowShape(WindowShape shape);
    void setDirection(DirectionMode direction);
    void setSpread(float amount);

    // For UI visualization — returns normalized positions of active grains
    std::vector<float> getActiveGrainPositions() const;

    // Stats
    int getActiveGrainCount() const;

private:
    // Fixed-capacity grain pool (no allocation during rendering)
    std::array<GrainVoice, MaxGrains> grainPool;

    GrainScheduler scheduler;

    // Source buffer (shared_ptr for safe swap)
    std::shared_ptr<juce::AudioBuffer<float>> sourceBuffer;
    double sourceSampleRate = 44100.0;

    // Atomic parameters for thread-safe updates
    std::atomic<float> paramGrainSize { 80.0f };
    std::atomic<int> paramGrainCount { 8 };
    std::atomic<float> paramPosition { 0.5f };
    std::atomic<float> paramSpray { 0.0f };
    std::atomic<float> paramPitch { 0.0f };
    std::atomic<int> paramPitchQuantize { 0 }; // PitchQuantizeScale as int
    std::atomic<int> paramWindowShape { 0 };    // WindowShape as int
    std::atomic<int> paramDirection { 0 };      // DirectionMode as int
    std::atomic<float> paramSpread { 0.0f };

    // Mutex for grain position reads (lightweight, non-blocking try_lock from audio)
    mutable std::mutex grainPositionMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularEngine)
};

} // namespace grainhex
