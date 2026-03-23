#pragma once

#include "FX/DistortionProcessor.h"
#include "FX/FilterProcessor.h"

namespace grainhex {

/**
 * EffectsChain routes distortion -> filter on granular output only.
 * Sub bypasses this chain entirely.
 */
class EffectsChain
{
public:
    EffectsChain() = default;

    void setSampleRate(double sampleRate)
    {
        distortion.setSampleRate(sampleRate);
        filter.setSampleRate(sampleRate);
    }

    // Process granular output through effects (distortion -> filter)
    void process(float* left, float* right, int numSamples)
    {
        distortion.process(left, right, numSamples);
        filter.process(left, right, numSamples);
    }

    DistortionProcessor& getDistortion() { return distortion; }
    FilterProcessor& getFilter() { return filter; }

private:
    DistortionProcessor distortion;
    FilterProcessor filter;
};

} // namespace grainhex
