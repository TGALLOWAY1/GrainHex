#pragma once

#include "Sub/SubOscillator.h"
#include "Audio/ParameterSmoother.h"
#include <atomic>
#include <array>

namespace grainhex {

/**
 * SubEngine owns the sub oscillator, level control, and rendering.
 * Phase 3 Step 1: Fixed-frequency sub with level control, muted by default.
 * Designed for audio-thread use — no allocation in processBlock.
 */
class SubEngine
{
public:
    static constexpr int kMaxBlockSize = 2048;

    SubEngine();

    void setSampleRate(double sampleRate);

    // Render sub into output buffers (additive — caller should clear first)
    void processBlock(float* outL, float* outR, int numSamples);

    // Thread-safe parameter setters (called from UI thread)
    void setEnabled(bool enabled) { subEnabled.store(enabled, std::memory_order_relaxed); }
    void setLevel(float level) { levelSmoother.setTargetValue(level); }
    void setFrequency(float hz) { frequencySmoother.setTargetValue(hz); }
    void setWaveform(SubWaveform wf) { waveform.store(static_cast<int>(wf), std::memory_order_relaxed); }

    bool isEnabled() const { return subEnabled.load(std::memory_order_relaxed); }

private:
    SubOscillator oscillator;

    std::atomic<bool> subEnabled { false };
    std::atomic<int> waveform { static_cast<int>(SubWaveform::Sine) };

    ParameterSmoother levelSmoother;
    ParameterSmoother frequencySmoother;

    double currentSampleRate = 44100.0;
};

} // namespace grainhex
