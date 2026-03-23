#include "Granular/Interpolator.h"
#include <algorithm>
#include <cmath>

namespace grainhex {

float Interpolator::cubicInterpolate(const float* data, int64_t numSamples, double position)
{
    if (data == nullptr || numSamples <= 0)
        return 0.0f;

    // Integer part and fractional part
    int64_t idx = static_cast<int64_t>(std::floor(position));
    float frac = static_cast<float>(position - static_cast<double>(idx));

    // Get 4 samples: y0 (idx-1), y1 (idx), y2 (idx+1), y3 (idx+2)
    // Clamp at boundaries
    auto clampedRead = [data, numSamples](int64_t i) -> float
    {
        i = std::clamp(i, int64_t(0), numSamples - 1);
        return data[i];
    };

    float y0 = clampedRead(idx - 1);
    float y1 = clampedRead(idx);
    float y2 = clampedRead(idx + 1);
    float y3 = clampedRead(idx + 2);

    // Catmull-Rom spline
    float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c = -0.5f * y0 + 0.5f * y2;
    float d = y1;

    return ((a * frac + b) * frac + c) * frac + d;
}

} // namespace grainhex
