#pragma once

#include "Modulation/LFO.h"
#include "Modulation/ADSREnvelope.h"
#include <array>
#include <atomic>
#include <functional>
#include <vector>

namespace grainhex {

// Modulation targets — the parameters that can be modulated
enum class ModTarget
{
    None = 0,
    FilterCutoff,
    GrainSize,
    GrainPosition,
    GrainSpray,
    GrainPitch,
    GrainSpread,
    DistortionDrive,
    Count
};

// Modulation source types
enum class ModSource
{
    None,
    LFO,
    Envelope
};

// A single modulation assignment
struct ModAssignment
{
    ModSource source = ModSource::None;
    ModTarget target = ModTarget::None;
    float depth = 1.0f; // Per-target scaling [-1, 1]
};

/**
 * ModulationEngine owns 1 LFO and 1 ADSR envelope.
 * Manages modulation assignments and computes per-sample modulation values.
 * V1: max 8 simultaneous assignments (no matrix).
 */
class ModulationEngine
{
public:
    static constexpr int kMaxAssignments = 8;

    ModulationEngine() = default;

    void setSampleRate(double sampleRate)
    {
        lfo.setSampleRate(sampleRate);
        envelope.setSampleRate(sampleRate);
    }

    LFO& getLFO() { return lfo; }
    ADSREnvelope& getEnvelope() { return envelope; }

    // Assignment management (called from UI thread)
    bool addAssignment(ModSource source, ModTarget target, float depth)
    {
        for (int i = 0; i < kMaxAssignments; ++i)
        {
            auto& a = assignments[i];
            if (a.source == ModSource::None)
            {
                a.source = source;
                a.target = target;
                a.depth = depth;
                assignmentCount.store(countActiveAssignments(), std::memory_order_relaxed);
                return true;
            }
        }
        return false; // Full
    }

    bool removeAssignment(ModSource source, ModTarget target)
    {
        for (int i = 0; i < kMaxAssignments; ++i)
        {
            auto& a = assignments[i];
            if (a.source == source && a.target == target)
            {
                a.source = ModSource::None;
                a.target = ModTarget::None;
                a.depth = 1.0f;
                assignmentCount.store(countActiveAssignments(), std::memory_order_relaxed);
                return true;
            }
        }
        return false;
    }

    void setAssignmentDepth(ModSource source, ModTarget target, float depth)
    {
        for (int i = 0; i < kMaxAssignments; ++i)
        {
            auto& a = assignments[i];
            if (a.source == source && a.target == target)
            {
                a.depth = depth;
                return;
            }
        }
    }

    bool hasAssignment(ModSource source, ModTarget target) const
    {
        for (int i = 0; i < kMaxAssignments; ++i)
            if (assignments[i].source == source && assignments[i].target == target)
                return true;
        return false;
    }

    bool targetHasModulation(ModTarget target) const
    {
        for (int i = 0; i < kMaxAssignments; ++i)
            if (assignments[i].target == target && assignments[i].source != ModSource::None)
                return true;
        return false;
    }

    int getAssignmentCount() const { return assignmentCount.load(std::memory_order_relaxed); }

    const std::array<ModAssignment, kMaxAssignments>& getAssignments() const { return assignments; }

    // Called once per sample from audio thread.
    // Returns modulation value for a given target (sum of all sources assigned to it).
    float getModulationValue(ModTarget target, float lfoVal, float envVal) const
    {
        float total = 0.0f;
        for (int i = 0; i < kMaxAssignments; ++i)
        {
            const auto& a = assignments[i];
            if (a.target == target && a.source != ModSource::None)
            {
                float sourceVal = (a.source == ModSource::LFO) ? lfoVal : envVal;
                total += sourceVal * a.depth;
            }
        }
        return total;
    }

    // Tick both sources and return their raw values
    struct ModValues { float lfo; float env; };
    ModValues tick()
    {
        float l = lfo.tick();
        float e = envelope.tick();
        return { l, e };
    }

private:
    int countActiveAssignments() const
    {
        int count = 0;
        for (int i = 0; i < kMaxAssignments; ++i)
            if (assignments[i].source != ModSource::None)
                ++count;
        return count;
    }

    LFO lfo;
    ADSREnvelope envelope;
    std::array<ModAssignment, kMaxAssignments> assignments {};
    std::atomic<int> assignmentCount { 0 };
};

} // namespace grainhex
