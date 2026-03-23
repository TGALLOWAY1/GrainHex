#include "SourceInput/RootNoteDetector.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace grainhex {

PitchInfo RootNoteDetector::detect(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (buffer.getNumSamples() < kYinWindowSize * 2)
        return {}; // Too short to analyze

    // Mix to mono if stereo
    const int numSamples = buffer.getNumSamples();
    std::vector<float> mono(static_cast<size_t>(numSamples));

    if (buffer.getNumChannels() >= 2)
    {
        for (int i = 0; i < numSamples; ++i)
            mono[static_cast<size_t>(i)] = (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f;
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            mono[static_cast<size_t>(i)] = buffer.getSample(0, i);
    }

    // Analyze a segment from the middle of the sample for most stable pitch
    int segmentStart = (numSamples - kYinWindowSize * 2) / 2;
    segmentStart = std::max(0, segmentStart);
    int segmentLength = std::min(kYinWindowSize * 2, numSamples - segmentStart);

    float confidence = 0.0f;
    float frequency = yinDetect(mono.data() + segmentStart, segmentLength, sampleRate, confidence);

    return frequencyToPitchInfo(frequency, confidence);
}

PitchInfo RootNoteDetector::frequencyToPitchInfo(float frequency, float confidence)
{
    PitchInfo info;
    info.frequency = frequency;
    info.confidence = confidence;

    if (frequency <= 0.0f || confidence < 0.3f)
    {
        info.midiNote = -1;
        info.centsOffset = 0.0f;
        return info;
    }

    // MIDI note from frequency: 69 + 12 * log2(freq / 440)
    float midiFloat = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    info.midiNote = static_cast<int>(std::round(midiFloat));
    info.centsOffset = (midiFloat - static_cast<float>(info.midiNote)) * 100.0f;

    // Clamp to valid MIDI range
    if (info.midiNote < 0) info.midiNote = 0;
    if (info.midiNote > 127) info.midiNote = 127;

    return info;
}

float RootNoteDetector::yinDetect(const float* data, int numSamples, double sampleRate, float& outConfidence)
{
    // YIN algorithm implementation
    // Step 1: Difference function
    // Step 2: Cumulative mean normalized difference function
    // Step 3: Absolute threshold
    // Step 4: Parabolic interpolation

    const int halfWindow = numSamples / 2;
    if (halfWindow < 2)
    {
        outConfidence = 0.0f;
        return 0.0f;
    }

    std::vector<float> diff(static_cast<size_t>(halfWindow));
    std::vector<float> cmndf(static_cast<size_t>(halfWindow));

    // Step 1: Difference function
    diff[0] = 0.0f;
    for (int tau = 1; tau < halfWindow; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfWindow; ++j)
        {
            float delta = data[j] - data[j + tau];
            sum += delta * delta;
        }
        diff[static_cast<size_t>(tau)] = sum;
    }

    // Step 2: Cumulative mean normalized difference function
    cmndf[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < halfWindow; ++tau)
    {
        runningSum += diff[static_cast<size_t>(tau)];
        if (runningSum > 0.0f)
            cmndf[static_cast<size_t>(tau)] = diff[static_cast<size_t>(tau)] * static_cast<float>(tau) / runningSum;
        else
            cmndf[static_cast<size_t>(tau)] = 1.0f;
    }

    // Step 3: Absolute threshold — find first dip below threshold
    int tauEstimate = -1;

    // Compute valid tau range from frequency bounds
    int minTau = static_cast<int>(sampleRate / kMaxFrequency);
    int maxTau = static_cast<int>(sampleRate / kMinFrequency);
    minTau = std::max(2, minTau);
    maxTau = std::min(halfWindow - 1, maxTau);

    for (int tau = minTau; tau < maxTau; ++tau)
    {
        if (cmndf[static_cast<size_t>(tau)] < kYinThreshold)
        {
            // Find the local minimum
            while (tau + 1 < maxTau &&
                   cmndf[static_cast<size_t>(tau + 1)] < cmndf[static_cast<size_t>(tau)])
            {
                ++tau;
            }
            tauEstimate = tau;
            break;
        }
    }

    // If no dip found below threshold, find the global minimum
    if (tauEstimate < 0)
    {
        float minVal = 1.0f;
        for (int tau = minTau; tau < maxTau; ++tau)
        {
            if (cmndf[static_cast<size_t>(tau)] < minVal)
            {
                minVal = cmndf[static_cast<size_t>(tau)];
                tauEstimate = tau;
            }
        }

        if (minVal > 0.5f)
        {
            // Not confident at all
            outConfidence = 0.0f;
            return 0.0f;
        }
    }

    if (tauEstimate < 1)
    {
        outConfidence = 0.0f;
        return 0.0f;
    }

    // Step 4: Parabolic interpolation for sub-sample accuracy
    float betterTau = static_cast<float>(tauEstimate);
    if (tauEstimate > 0 && tauEstimate < halfWindow - 1)
    {
        float s0 = cmndf[static_cast<size_t>(tauEstimate - 1)];
        float s1 = cmndf[static_cast<size_t>(tauEstimate)];
        float s2 = cmndf[static_cast<size_t>(tauEstimate + 1)];

        float denom = 2.0f * (2.0f * s1 - s2 - s0);
        if (std::abs(denom) > 1e-9f)
            betterTau = static_cast<float>(tauEstimate) + (s0 - s2) / denom;
    }

    outConfidence = 1.0f - cmndf[static_cast<size_t>(tauEstimate)];
    outConfidence = std::max(0.0f, std::min(1.0f, outConfidence));

    float frequency = static_cast<float>(sampleRate) / betterTau;
    return frequency;
}

} // namespace grainhex
