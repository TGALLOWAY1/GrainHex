#pragma once

#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>

namespace grainhex {

enum class LFOShape
{
    Sine,
    Triangle,
    Square,
    SampleAndHold
};

/**
 * LFO with phase accumulator.
 * Outputs normalized value in [-1, 1].
 * Shapes: sine, triangle, square, sample-and-hold (random).
 * Rate: free Hz or tempo-synced against a UI-provided BPM.
 * Audio-thread safe.
 */
class LFO
{
public:
    LFO() = default;

    void setSampleRate(double sr) { sampleRate = sr; }

    void setEnabled(bool e) { enabled.store(e, std::memory_order_relaxed); }
    bool isEnabled() const { return enabled.load(std::memory_order_relaxed); }

    void setRate(float hz) { rate.store(juce::jlimit(0.01f, 30.0f, hz), std::memory_order_relaxed); }
    void setDepth(float d) { depth.store(juce::jlimit(0.0f, 1.0f, d), std::memory_order_relaxed); }
    void setShape(LFOShape s) { shape.store(static_cast<int>(s), std::memory_order_relaxed); }
    void setTempoSync(bool shouldSync, float newBpm, float noteDivision)
    {
        tempoSyncEnabled.store(shouldSync, std::memory_order_relaxed);
        bpm.store(juce::jlimit(20.0f, 300.0f, newBpm), std::memory_order_relaxed);
        syncDivision.store(juce::jmax(0.125f, noteDivision), std::memory_order_relaxed);
    }

    LFOShape getShape() const { return static_cast<LFOShape>(shape.load(std::memory_order_relaxed)); }
    float getDepth() const { return depth.load(std::memory_order_relaxed); }
    bool isTempoSynced() const { return tempoSyncEnabled.load(std::memory_order_relaxed); }
    float getRate() const { return rate.load(std::memory_order_relaxed); }
    float getSyncDivision() const { return syncDivision.load(std::memory_order_relaxed); }
    float getBPM() const { return bpm.load(std::memory_order_relaxed); }
    float getEffectiveRateHz() const
    {
        if (!tempoSyncEnabled.load(std::memory_order_relaxed))
            return rate.load(std::memory_order_relaxed);

        const auto currentBpm = bpm.load(std::memory_order_relaxed);
        const auto division = juce::jmax(0.125f, syncDivision.load(std::memory_order_relaxed));
        return currentBpm / (60.0f * division);
    }

    // Reset phase (e.g., on note trigger)
    void resetPhase() { phase = 0.0; }

    // Advance one sample and return scaled output [-depth, +depth]
    float tick()
    {
        if (!enabled.load(std::memory_order_relaxed))
            return 0.0f;

        const float currentRate = getEffectiveRateHz();
        float currentDepth = depth.load(std::memory_order_relaxed);
        auto currentShape = static_cast<LFOShape>(shape.load(std::memory_order_relaxed));

        float raw = computeShape(currentShape);

        // Advance phase
        phase += static_cast<double>(currentRate) / sampleRate;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            // Update S&H on cycle boundary
            if (currentShape == LFOShape::SampleAndHold)
                shValue = randomFloat();
        }

        return raw * currentDepth;
    }

    // Get current raw value without advancing (for UI display)
    float getCurrentValue() const { return lastOutput; }

private:
    float computeShape(LFOShape s)
    {
        float p = static_cast<float>(phase);
        float out = 0.0f;

        switch (s)
        {
            case LFOShape::Sine:
                out = std::sin(p * juce::MathConstants<float>::twoPi);
                break;

            case LFOShape::Triangle:
                out = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
                break;

            case LFOShape::Square:
                out = (p < 0.5f) ? 1.0f : -1.0f;
                break;

            case LFOShape::SampleAndHold:
                out = shValue;
                break;
        }

        lastOutput = out;
        return out;
    }

    static float randomFloat()
    {
        // Simple fast random in [-1, 1]
        static uint32_t seed = 12345;
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(seed)) / static_cast<float>(INT32_MAX);
    }

    std::atomic<bool> enabled { false };
    std::atomic<float> rate { 1.0f };   // Hz
    std::atomic<float> depth { 0.5f };
    std::atomic<int> shape { static_cast<int>(LFOShape::Sine) };
    std::atomic<bool> tempoSyncEnabled { false };
    std::atomic<float> bpm { 120.0f };
    std::atomic<float> syncDivision { 1.0f }; // quarter-note units (1.0 = quarter, 0.5 = eighth)

    double phase = 0.0;
    double sampleRate = 44100.0;
    float shValue = 0.0f;       // Current S&H value
    float lastOutput = 0.0f;
};

} // namespace grainhex
