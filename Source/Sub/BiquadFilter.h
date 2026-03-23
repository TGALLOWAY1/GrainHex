#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace grainhex {

/**
 * Lightweight biquad filter for crossover isolation.
 * Supports highpass and lowpass modes.
 * Coefficients computed on parameter change, not per-sample.
 */
class BiquadFilter
{
public:
    enum class Type { LowPass, HighPass };

    BiquadFilter() = default;

    void reset()
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    void setCoefficients(Type type, float cutoffHz, float q, double sampleRate)
    {
        if (sampleRate <= 0.0 || cutoffHz <= 0.0f)
            return;

        float w0 = juce::MathConstants<float>::twoPi * cutoffHz / static_cast<float>(sampleRate);
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * q);

        switch (type)
        {
            case Type::LowPass:
            {
                float a0 = 1.0f + alpha;
                b0 = ((1.0f - cosw0) / 2.0f) / a0;
                b1 = (1.0f - cosw0) / a0;
                b2 = b0;
                a1 = (-2.0f * cosw0) / a0;
                a2 = (1.0f - alpha) / a0;
                break;
            }
            case Type::HighPass:
            {
                float a0 = 1.0f + alpha;
                b0 = ((1.0f + cosw0) / 2.0f) / a0;
                b1 = -(1.0f + cosw0) / a0;
                b2 = b0;
                a1 = (-2.0f * cosw0) / a0;
                a2 = (1.0f - alpha) / a0;
                break;
            }
        }
    }

    float processSample(float input)
    {
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;

        return output;
    }

private:
    // Coefficients (normalized)
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // State
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
};

} // namespace grainhex
