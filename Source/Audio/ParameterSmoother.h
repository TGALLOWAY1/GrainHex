#pragma once

#include <cmath>

namespace grainhex {

// Simple one-pole smoother for audio-rate parameter changes
class ParameterSmoother
{
public:
    ParameterSmoother() = default;

    void reset(float value)
    {
        currentValue = value;
        targetValue = value;
    }

    void setTargetValue(float target)
    {
        targetValue = target;
    }

    void setSmoothingTime(float timeInSeconds, double sampleRate)
    {
        if (timeInSeconds <= 0.0f || sampleRate <= 0.0)
        {
            coefficient = 1.0f;
            return;
        }
        // One-pole coefficient for ~63% convergence in given time
        coefficient = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * timeInSeconds));
    }

    float getNextValue()
    {
        currentValue += coefficient * (targetValue - currentValue);
        return currentValue;
    }

    float getCurrentValue() const { return currentValue; }
    bool isSmoothing() const { return std::abs(targetValue - currentValue) > 1e-6f; }

private:
    float currentValue = 0.0f;
    float targetValue = 0.0f;
    float coefficient = 1.0f;
};

} // namespace grainhex
