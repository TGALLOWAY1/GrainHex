#include "Sub/PitchDetector.h"
#include "SourceInput/RootNoteDetector.h"
#include <cmath>
#include <algorithm>

namespace grainhex {

PitchDetector::PitchDetector()
{
    ringBuffer.fill(0.0f);
    diffBuffer.fill(0.0f);
    cmndfBuffer.fill(0.0f);
}

void PitchDetector::setSampleRate(double sr)
{
    sampleRate = sr;
    writePos = 0;
    samplesAccumulated = 0;
    ringBuffer.fill(0.0f);
    lastStablePitch = {};
}

void PitchDetector::feedSamples(const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        ringBuffer[static_cast<size_t>(writePos)] = data[i];
        writePos = (writePos + 1) % kBufferSize;
    }
    samplesAccumulated += numSamples;
}

PitchInfo PitchDetector::detect()
{
    // Don't analyze until we have enough samples
    if (samplesAccumulated < kYinWindowSize)
        return lastStablePitch;

    float confidence = 0.0f;
    float frequency = yinDetect(confidence);

    PitchInfo result = RootNoteDetector::frequencyToPitchInfo(frequency, confidence);

    // Hold-last-note behavior: if confidence is too low, keep previous stable pitch
    if (confidence >= kConfidenceThreshold && frequency > 0.0f)
    {
        lastStablePitch = result;
    }

    return lastStablePitch;
}

float PitchDetector::yinDetect(float& outConfidence)
{
    // Linearize ring buffer: copy last kYinWindowSize samples into contiguous view
    // The most recent samples end at writePos
    // We need samples from (writePos - kYinWindowSize) to (writePos - 1)

    // Create a contiguous view of the most recent kYinWindowSize samples
    std::array<float, kYinWindowSize> linearBuffer;
    int readStart = (writePos - kYinWindowSize + kBufferSize) % kBufferSize;

    for (int i = 0; i < kYinWindowSize; ++i)
    {
        linearBuffer[static_cast<size_t>(i)] =
            ringBuffer[static_cast<size_t>((readStart + i) % kBufferSize)];
    }

    const int halfWindow = kYinWindowSize / 2;

    // Step 1: Difference function
    diffBuffer[0] = 0.0f;
    for (int tau = 1; tau < halfWindow; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfWindow; ++j)
        {
            float delta = linearBuffer[static_cast<size_t>(j)]
                        - linearBuffer[static_cast<size_t>(j + tau)];
            sum += delta * delta;
        }
        diffBuffer[static_cast<size_t>(tau)] = sum;
    }

    // Step 2: Cumulative mean normalized difference function
    cmndfBuffer[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < halfWindow; ++tau)
    {
        runningSum += diffBuffer[static_cast<size_t>(tau)];
        if (runningSum > 0.0f)
            cmndfBuffer[static_cast<size_t>(tau)] =
                diffBuffer[static_cast<size_t>(tau)] * static_cast<float>(tau) / runningSum;
        else
            cmndfBuffer[static_cast<size_t>(tau)] = 1.0f;
    }

    // Step 3: Absolute threshold — find first dip below threshold
    int minTau = std::max(2, static_cast<int>(sampleRate / kMaxFrequency));
    int maxTau = std::min(halfWindow - 1, static_cast<int>(sampleRate / kMinFrequency));

    int tauEstimate = -1;

    for (int tau = minTau; tau < maxTau; ++tau)
    {
        if (cmndfBuffer[static_cast<size_t>(tau)] < kYinThreshold)
        {
            // Find the local minimum
            while (tau + 1 < maxTau &&
                   cmndfBuffer[static_cast<size_t>(tau + 1)] < cmndfBuffer[static_cast<size_t>(tau)])
            {
                ++tau;
            }
            tauEstimate = tau;
            break;
        }
    }

    // Fallback: global minimum if no dip below threshold
    if (tauEstimate < 0)
    {
        float minVal = 1.0f;
        for (int tau = minTau; tau < maxTau; ++tau)
        {
            if (cmndfBuffer[static_cast<size_t>(tau)] < minVal)
            {
                minVal = cmndfBuffer[static_cast<size_t>(tau)];
                tauEstimate = tau;
            }
        }

        if (minVal > 0.5f)
        {
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
        float s0 = cmndfBuffer[static_cast<size_t>(tauEstimate - 1)];
        float s1 = cmndfBuffer[static_cast<size_t>(tauEstimate)];
        float s2 = cmndfBuffer[static_cast<size_t>(tauEstimate + 1)];

        float denom = 2.0f * (2.0f * s1 - s2 - s0);
        if (std::abs(denom) > 1e-9f)
            betterTau = static_cast<float>(tauEstimate) + (s0 - s2) / denom;
    }

    outConfidence = 1.0f - cmndfBuffer[static_cast<size_t>(tauEstimate)];
    outConfidence = std::max(0.0f, std::min(1.0f, outConfidence));

    return static_cast<float>(sampleRate) / betterTau;
}

} // namespace grainhex
