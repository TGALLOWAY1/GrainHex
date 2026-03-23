#pragma once

#include <array>
#include <cmath>

namespace grainhex {

enum class WindowShape
{
    Hann,
    Triangle,
    Trapezoid
};

class WindowFunctions
{
public:
    static constexpr int TableSize = 1024;

    // Get a windowed amplitude for a given phase (0.0 = start, 1.0 = end)
    static float getSample(WindowShape shape, float phase);

private:
    struct Tables
    {
        std::array<float, TableSize> hann;
        std::array<float, TableSize> triangle;
        std::array<float, TableSize> trapezoid;

        Tables();
    };

    static const Tables& getTables();
};

} // namespace grainhex
