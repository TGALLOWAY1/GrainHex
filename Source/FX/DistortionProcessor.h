#pragma once

#include "Audio/ParameterSmoother.h"
#include <juce_core/juce_core.h>
#include <atomic>
#include <cmath>

namespace grainhex {

enum class DistortionMode
{
    SoftClip,
    HardClip,
    Wavefold
};

/**
 * Distortion / waveshaper processor.
 * Modes: soft clip (tanh), hard clip, wavefold.
 * Parameters: drive (1.0–20.0), mix (0.0–1.0 dry/wet).
 * Audio-thread safe — no allocations in process().
 */
class DistortionProcessor
{
public:
    DistortionProcessor()
    {
        driveSmoother.reset(1.0f);
        mixSmoother.reset(0.0f);
    }

    void setSampleRate(double sampleRate)
    {
        driveSmoother.setSmoothingTime(0.005f, sampleRate);
        mixSmoother.setSmoothingTime(0.005f, sampleRate);
    }

    void setEnabled(bool enabled) { bypassed.store(!enabled, std::memory_order_relaxed); }
    bool isEnabled() const { return !bypassed.load(std::memory_order_relaxed); }

    void setMode(DistortionMode m) { mode.store(static_cast<int>(m), std::memory_order_relaxed); }
    DistortionMode getMode() const { return static_cast<DistortionMode>(mode.load(std::memory_order_relaxed)); }

    void setDrive(float d) { driveSmoother.setTargetValue(juce::jlimit(1.0f, 20.0f, d)); }
    void setMix(float m) { mixSmoother.setTargetValue(juce::jlimit(0.0f, 1.0f, m)); }

    void process(float* left, float* right, int numSamples)
    {
        if (bypassed.load(std::memory_order_relaxed))
            return;

        auto currentMode = static_cast<DistortionMode>(mode.load(std::memory_order_relaxed));

        for (int i = 0; i < numSamples; ++i)
        {
            float drive = driveSmoother.getNextValue();
            float mix = mixSmoother.getNextValue();

            float dryL = left[i];
            float dryR = right[i];

            float wetL = applyDistortion(dryL * drive, currentMode);
            float wetR = applyDistortion(dryR * drive, currentMode);

            left[i]  = dryL * (1.0f - mix) + wetL * mix;
            right[i] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    static float applyDistortion(float sample, DistortionMode m)
    {
        switch (m)
        {
            case DistortionMode::SoftClip:
                return std::tanh(sample);

            case DistortionMode::HardClip:
                return juce::jlimit(-1.0f, 1.0f, sample);

            case DistortionMode::Wavefold:
            {
                // Triangle wavefolder: fold signal back when exceeding [-1, 1]
                float folded = sample;
                while (folded > 1.0f || folded < -1.0f)
                {
                    if (folded > 1.0f)
                        folded = 2.0f - folded;
                    else if (folded < -1.0f)
                        folded = -2.0f - folded;
                }
                return folded;
            }
        }
        return sample;
    }

    std::atomic<bool> bypassed { true };
    std::atomic<int> mode { static_cast<int>(DistortionMode::SoftClip) };
    ParameterSmoother driveSmoother;
    ParameterSmoother mixSmoother;
};

} // namespace grainhex
