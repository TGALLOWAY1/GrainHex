#pragma once

#include "Granular/WindowFunctions.h"
#include <cstdint>
#include <random>

namespace grainhex {

enum class DirectionMode
{
    Forward,
    Reverse,
    Random
};

enum class PitchQuantizeScale
{
    Off,
    Chromatic,
    Major,
    Minor
};

// Parameters resolved for a single grain at spawn time
struct GrainSpawnParams
{
    double sourcePosition = 0.0;    // Absolute sample position in source
    int grainLengthSamples = 0;     // Grain duration in samples
    double playbackIncrement = 1.0; // Pitch as rate multiplier
    float panLeft = 1.0f;
    float panRight = 1.0f;
    WindowShape windowShape = WindowShape::Hann;
    bool reverse = false;
};

class GrainScheduler
{
public:
    GrainScheduler();

    // Set parameters (called from UI thread via atomics, or directly)
    void setGrainSize(float sizeMs);       // Grain duration in ms
    void setGrainCount(int count);          // Target concurrent grain count
    void setPosition(float normPos);        // 0..1 normalized source position
    void setSpray(float sprayAmount);       // 0..1 randomized offset
    void setPitchSemitones(float semitones); // Pitch shift in semitones
    void setPitchQuantize(PitchQuantizeScale scale);
    void setWindowShape(WindowShape shape);
    void setDirection(DirectionMode direction);
    void setSpread(float spreadAmount);     // 0..1 stereo spread

    // Call once per audio block to advance internal state
    void prepareBlock(int numSamples, double sampleRate, int64_t sourceLength);

    // Call per-sample within block to check if a grain should spawn
    bool shouldSpawnGrain();

    // Get resolved parameters for the next grain to spawn
    GrainSpawnParams getSpawnParams(double sampleRate, int64_t sourceLength);

private:
    float quantizePitch(float semitones) const;

    // Parameters
    float grainSizeMs = 80.0f;
    int grainCount = 8;
    float position = 0.5f;
    float spray = 0.0f;
    float pitchSemitones = 0.0f;
    PitchQuantizeScale pitchQuantizeScale = PitchQuantizeScale::Off;
    WindowShape windowShape = WindowShape::Hann;
    DirectionMode directionMode = DirectionMode::Forward;
    float spread = 0.0f;

    // Internal scheduling state
    int samplesSinceLastSpawn = 0;
    int spawnIntervalSamples = 0;
    double currentSampleRate = 44100.0;

    // Random number generator (seeded once, deterministic per session)
    std::mt19937 rng;
    std::uniform_real_distribution<float> uniformDist { 0.0f, 1.0f };
};

} // namespace grainhex
