#pragma once

#include <cstdint>

namespace grainhex {

class Interpolator
{
public:
    // 4-point Catmull-Rom cubic interpolation
    // position is a fractional sample index into data[0..numSamples-1]
    // Clamps at boundaries for safety.
    static float cubicInterpolate(const float* data, int64_t numSamples, double position);
};

} // namespace grainhex
