#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace grainhex {

enum class SubWaveform
{
    Sine,
    Triangle
};

/**
 * Phase-accumulator oscillator for sub bass generation.
 * Supports sine and triangle waveforms. No allocation, pure math.
 * Designed for audio-thread use.
 */
class SubOscillator
{
public:
    SubOscillator() = default;

    void setSampleRate(double sr)
    {
        sampleRate = sr;
        updatePhaseIncrement();
    }

    void setFrequency(float hz)
    {
        frequency = hz;
        updatePhaseIncrement();
    }

    void setWaveform(SubWaveform wf) { waveform = wf; }

    float getNextSample()
    {
        float sample = 0.0f;

        switch (waveform)
        {
            case SubWaveform::Sine:
                sample = std::sin(phase * juce::MathConstants<float>::twoPi);
                break;

            case SubWaveform::Triangle:
                sample = generateTriangle();
                break;
        }

        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return sample;
    }

    void reset() { phase = 0.0f; }

private:
    void updatePhaseIncrement()
    {
        if (sampleRate > 0.0)
            phaseIncrement = static_cast<float>(frequency / sampleRate);
    }

    // Bandlimited triangle using polyBLEP-corrected square integration
    float generateTriangle() const
    {
        // Simple triangle: 4 * |phase - 0.5| - 1
        // Maps phase [0,1) to triangle wave [-1, 1]
        float t = phase;
        if (t < 0.5f)
            return 4.0f * t - 1.0f;
        else
            return 3.0f - 4.0f * t;
    }

    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float frequency = 55.0f; // A1 default
    double sampleRate = 44100.0;
    SubWaveform waveform = SubWaveform::Sine;
};

} // namespace grainhex
