#include "Sub/SubEngine.h"

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
}

void SubEngine::processBlock(float* outL, float* outR, int numSamples)
{
    if (!subEnabled.load(std::memory_order_relaxed))
        return;

    // Update waveform if changed
    oscillator.setWaveform(static_cast<SubWaveform>(waveform.load(std::memory_order_relaxed)));

    for (int i = 0; i < numSamples; ++i)
    {
        // Per-sample frequency update for smooth pitch transitions
        float freq = frequencySmoother.getNextValue();
        oscillator.setFrequency(freq);

        float level = levelSmoother.getNextValue();
        float sample = oscillator.getNextSample() * level;

        // Sub is mono — add equally to both channels
        outL[i] += sample;
        if (outR != outL)
            outR[i] += sample;
    }
}

} // namespace grainhex
