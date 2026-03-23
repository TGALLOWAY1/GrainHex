#pragma once

#include "Audio/ParameterSmoother.h"
#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>

namespace grainhex {

enum class FilterMode
{
    LowPass,
    HighPass,
    BandPass
};

/**
 * Multi-mode SVF (state-variable filter) processor.
 * Modes: LP, HP, BP. Parameters: cutoff, resonance, envelope amount.
 * SVF chosen for smooth parameter modulation without instability.
 * Audio-thread safe.
 */
class FilterProcessor
{
public:
    FilterProcessor()
    {
        cutoffSmoother.reset(8000.0f);
        resoSmoother.reset(0.0f);
    }

    void setSampleRate(double sr)
    {
        sampleRate = sr;
        cutoffSmoother.setSmoothingTime(0.005f, sr);
        resoSmoother.setSmoothingTime(0.005f, sr);
    }

    void setEnabled(bool enabled) { bypassed.store(!enabled, std::memory_order_relaxed); }
    bool isEnabled() const { return !bypassed.load(std::memory_order_relaxed); }

    void setMode(FilterMode m) { mode.store(static_cast<int>(m), std::memory_order_relaxed); }
    FilterMode getMode() const { return static_cast<FilterMode>(mode.load(std::memory_order_relaxed)); }

    void setCutoff(float hz) { cutoffSmoother.setTargetValue(juce::jlimit(20.0f, 20000.0f, hz)); }
    void setResonance(float q) { resoSmoother.setTargetValue(juce::jlimit(0.0f, 1.0f, q)); }

    // Envelope amount: how much the envelope modulates cutoff (in semitones, +-96)
    void setEnvelopeAmount(float amount) { envelopeAmount.store(juce::jlimit(-96.0f, 96.0f, amount), std::memory_order_relaxed); }
    float getEnvelopeAmount() const { return envelopeAmount.load(std::memory_order_relaxed); }

    // Called per-sample by ModulationEngine to apply envelope offset
    void setEnvelopeModulation(float envValue)
    {
        currentEnvMod = envValue;
    }

    void process(float* left, float* right, int numSamples)
    {
        if (bypassed.load(std::memory_order_relaxed))
            return;

        auto currentMode = static_cast<FilterMode>(mode.load(std::memory_order_relaxed));
        float envAmt = envelopeAmount.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            float baseCutoff = cutoffSmoother.getNextValue();
            float reso = resoSmoother.getNextValue();

            // Apply envelope modulation to cutoff (in semitones)
            float modCutoff = baseCutoff;
            if (std::abs(envAmt) > 0.01f)
            {
                float semitoneOffset = envAmt * currentEnvMod;
                modCutoff = baseCutoff * std::pow(2.0f, semitoneOffset / 12.0f);
                modCutoff = juce::jlimit(20.0f, 20000.0f, modCutoff);
            }

            // SVF coefficients
            float g = std::tan(juce::MathConstants<float>::pi * modCutoff / static_cast<float>(sampleRate));
            float k = 2.0f - 2.0f * reso; // Q mapping: reso 0->Q=0.5, reso 1->Q=inf
            float a1 = 1.0f / (1.0f + g * (g + k));
            float a2 = g * a1;
            float a3 = g * a2;

            // Left channel
            {
                float v3 = left[i] - ic2eqL;
                float v1 = a1 * ic1eqL + a2 * v3;
                float v2 = ic2eqL + a2 * ic1eqL + a3 * v3;
                ic1eqL = 2.0f * v1 - ic1eqL;
                ic2eqL = 2.0f * v2 - ic2eqL;

                switch (currentMode)
                {
                    case FilterMode::LowPass:  left[i] = v2; break;
                    case FilterMode::HighPass:  left[i] = left[i] - k * v1 - v2; break;
                    case FilterMode::BandPass:  left[i] = v1; break;
                }
            }

            // Right channel
            {
                float v3 = right[i] - ic2eqR;
                float v1 = a1 * ic1eqR + a2 * v3;
                float v2 = ic2eqR + a2 * ic1eqR + a3 * v3;
                ic1eqR = 2.0f * v1 - ic1eqR;
                ic2eqR = 2.0f * v2 - ic2eqR;

                switch (currentMode)
                {
                    case FilterMode::LowPass:  right[i] = v2; break;
                    case FilterMode::HighPass:  right[i] = right[i] - k * v1 - v2; break;
                    case FilterMode::BandPass:  right[i] = v1; break;
                }
            }
        }
    }

    void reset()
    {
        ic1eqL = ic2eqL = 0.0f;
        ic1eqR = ic2eqR = 0.0f;
    }

private:
    std::atomic<bool> bypassed { true };
    std::atomic<int> mode { static_cast<int>(FilterMode::LowPass) };
    std::atomic<float> envelopeAmount { 48.0f }; // Default: 48 semitones (4 octaves)

    ParameterSmoother cutoffSmoother;
    ParameterSmoother resoSmoother;

    // SVF state
    float ic1eqL = 0.0f, ic2eqL = 0.0f;
    float ic1eqR = 0.0f, ic2eqR = 0.0f;

    float currentEnvMod = 0.0f; // Current envelope modulation value (0..1)
    double sampleRate = 44100.0;
};

} // namespace grainhex
