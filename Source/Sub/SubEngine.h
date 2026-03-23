#pragma once

#include "Sub/SubOscillator.h"
#include "Sub/BiquadFilter.h"
#include "Audio/ParameterSmoother.h"
#include <atomic>
#include <array>

namespace grainhex {

/**
 * SubEngine owns the sub oscillator, crossover filters, level control, and rendering.
 * Designed for audio-thread use — no allocation in processBlock.
 */
class SubEngine
{
public:
    static constexpr int kMaxBlockSize = 2048;

    // Default crossover frequencies
    static constexpr float kDefaultGranularHP = 100.0f;
    static constexpr float kDefaultSubLP = 200.0f;
    static constexpr float kFilterQ = 0.707f; // Butterworth

    SubEngine();

    void setSampleRate(double sampleRate);

    // Apply HP filter to granular output (called before sub mix)
    void applyGranularHP(float* left, float* right, int numSamples);

    // Render sub into output buffers (additive — caller should clear first)
    void processBlock(float* outL, float* outR, int numSamples);

    // Thread-safe parameter setters (called from UI thread)
    void setEnabled(bool enabled) { subEnabled.store(enabled, std::memory_order_relaxed); }
    void setLevel(float level) { levelSmoother.setTargetValue(level); }
    void setFrequency(float hz) { frequencySmoother.setTargetValue(hz); }
    void setWaveform(SubWaveform wf) { waveform.store(static_cast<int>(wf), std::memory_order_relaxed); }

    void setGranularHPFreq(float hz);
    void setSubLPFreq(float hz);

    bool isEnabled() const { return subEnabled.load(std::memory_order_relaxed); }

private:
    void updateFilterCoefficients();

    SubOscillator oscillator;

    std::atomic<bool> subEnabled { false };
    std::atomic<int> waveform { static_cast<int>(SubWaveform::Sine) };

    ParameterSmoother levelSmoother;
    ParameterSmoother frequencySmoother;

    // Crossover filters
    BiquadFilter granularHPL; // HP for left granular channel
    BiquadFilter granularHPR; // HP for right granular channel
    BiquadFilter subLP;       // LP for sub output (mono)

    std::atomic<float> granularHPFreq { kDefaultGranularHP };
    std::atomic<float> subLPFreq { kDefaultSubLP };
    bool filtersNeedUpdate = true;

    double currentSampleRate = 44100.0;
};

} // namespace grainhex
