#include "Granular/WindowFunctions.h"
#include <algorithm>

namespace grainhex {

WindowFunctions::Tables::Tables()
{
    for (int i = 0; i < TableSize; ++i)
    {
        float phase = static_cast<float>(i) / static_cast<float>(TableSize - 1);

        // Hann: 0.5 * (1 - cos(2*pi*phase))
        hann[static_cast<size_t>(i)] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * phase));

        // Triangle: rises linearly to 1.0 at midpoint, falls back to 0.0
        triangle[static_cast<size_t>(i)] = (phase < 0.5f)
            ? (phase * 2.0f)
            : (2.0f - phase * 2.0f);

        // Trapezoid: flat top from 0.25 to 0.75, linear ramps at edges
        if (phase < 0.25f)
            trapezoid[static_cast<size_t>(i)] = phase * 4.0f;
        else if (phase > 0.75f)
            trapezoid[static_cast<size_t>(i)] = (1.0f - phase) * 4.0f;
        else
            trapezoid[static_cast<size_t>(i)] = 1.0f;
    }
}

const WindowFunctions::Tables& WindowFunctions::getTables()
{
    static const Tables tables;
    return tables;
}

float WindowFunctions::getSample(WindowShape shape, float phase)
{
    phase = std::clamp(phase, 0.0f, 1.0f);

    const auto& tables = getTables();
    float indexFloat = phase * static_cast<float>(TableSize - 1);
    int index = static_cast<int>(indexFloat);
    float frac = indexFloat - static_cast<float>(index);

    // Clamp index
    index = std::clamp(index, 0, TableSize - 2);

    const auto& table = (shape == WindowShape::Hann)      ? tables.hann
                      : (shape == WindowShape::Triangle)   ? tables.triangle
                      :                                      tables.trapezoid;

    // Linear interpolation between table entries
    return table[static_cast<size_t>(index)] * (1.0f - frac)
         + table[static_cast<size_t>(index + 1)] * frac;
}

} // namespace grainhex
