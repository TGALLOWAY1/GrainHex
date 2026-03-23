#include "Granular/GrainScheduler.h"
#include <cmath>
#include <algorithm>

namespace grainhex {

// Major scale intervals in semitones: 0, 2, 4, 5, 7, 9, 11
static const float majorScale[] = { 0, 2, 4, 5, 7, 9, 11 };
// Minor scale intervals: 0, 2, 3, 5, 7, 8, 10
static const float minorScale[] = { 0, 2, 3, 5, 7, 8, 10 };

static float snapToScale(float semitones, const float* scale, int scaleSize)
{
    // Decompose into octave and semitone within octave
    float octaves = std::floor(semitones / 12.0f);
    float withinOctave = semitones - octaves * 12.0f;
    if (withinOctave < 0.0f) withinOctave += 12.0f;

    // Find nearest scale degree
    float bestDist = 100.0f;
    float bestNote = 0.0f;
    for (int i = 0; i < scaleSize; ++i)
    {
        float dist = std::abs(withinOctave - scale[i]);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestNote = scale[i];
        }
    }

    return octaves * 12.0f + bestNote;
}

GrainScheduler::GrainScheduler()
    : rng(42) // Fixed seed for reproducibility
{
}

void GrainScheduler::setGrainSize(float sizeMs) { grainSizeMs = std::max(1.0f, sizeMs); }
void GrainScheduler::setGrainCount(int count) { grainCount = std::max(1, count); }
void GrainScheduler::setPosition(float normPos) { position = std::clamp(normPos, 0.0f, 1.0f); }
void GrainScheduler::setSpray(float sprayAmount) { spray = std::clamp(sprayAmount, 0.0f, 1.0f); }
void GrainScheduler::setPitchSemitones(float semitones) { pitchSemitones = std::clamp(semitones, -24.0f, 24.0f); }
void GrainScheduler::setPitchQuantize(PitchQuantizeScale scale) { pitchQuantizeScale = scale; }
void GrainScheduler::setWindowShape(WindowShape shape) { windowShape = shape; }
void GrainScheduler::setDirection(DirectionMode direction) { directionMode = direction; }
void GrainScheduler::setSpread(float spreadAmount) { spread = std::clamp(spreadAmount, 0.0f, 1.0f); }

void GrainScheduler::prepareBlock(int /*numSamples*/, double sampleRate, int64_t /*sourceLength*/)
{
    currentSampleRate = sampleRate;

    // Spawn interval: distribute grains evenly across grain duration
    // spawnInterval = grainSizeSamples / grainCount
    int grainSizeSamples = std::max(1, static_cast<int>(grainSizeMs * 0.001 * sampleRate));
    spawnIntervalSamples = std::max(1, grainSizeSamples / grainCount);
}

bool GrainScheduler::shouldSpawnGrain()
{
    ++samplesSinceLastSpawn;
    if (samplesSinceLastSpawn >= spawnIntervalSamples)
    {
        samplesSinceLastSpawn = 0;
        return true;
    }
    return false;
}

GrainSpawnParams GrainScheduler::getSpawnParams(double sampleRate, int64_t sourceLength)
{
    GrainSpawnParams params;

    // Grain length in samples
    params.grainLengthSamples = std::max(1, static_cast<int>(grainSizeMs * 0.001 * sampleRate));

    // Base position in samples
    double basePos = position * static_cast<double>(sourceLength);

    // Apply spray: random offset within spray range
    if (spray > 0.0f)
    {
        float sprayRange = spray * static_cast<float>(sourceLength) * 0.25f; // Max 25% of source
        float offset = (uniformDist(rng) * 2.0f - 1.0f) * sprayRange;
        basePos += static_cast<double>(offset);
    }

    // Clamp to valid range
    basePos = std::clamp(basePos, 0.0, static_cast<double>(sourceLength - 1));
    params.sourcePosition = basePos;

    // Pitch -> playback increment
    float effectivePitch = quantizePitch(pitchSemitones);
    params.playbackIncrement = std::pow(2.0, static_cast<double>(effectivePitch) / 12.0);

    // Direction
    switch (directionMode)
    {
        case DirectionMode::Forward:
            params.reverse = false;
            break;
        case DirectionMode::Reverse:
            params.reverse = true;
            break;
        case DirectionMode::Random:
            params.reverse = (uniformDist(rng) > 0.5f);
            break;
    }

    // Stereo spread: 0 = center (equal L/R), 1 = full random panning
    if (spread <= 0.0f)
    {
        params.panLeft = 0.707f;  // Equal power center
        params.panRight = 0.707f;
    }
    else
    {
        float pan = 0.5f + (uniformDist(rng) - 0.5f) * spread; // 0..1
        // Equal-power panning
        params.panLeft = std::cos(pan * static_cast<float>(M_PI) * 0.5f);
        params.panRight = std::sin(pan * static_cast<float>(M_PI) * 0.5f);
    }

    params.windowShape = windowShape;

    return params;
}

float GrainScheduler::quantizePitch(float semitones) const
{
    switch (pitchQuantizeScale)
    {
        case PitchQuantizeScale::Chromatic:
            return std::round(semitones);
        case PitchQuantizeScale::Major:
            return snapToScale(semitones, majorScale, 7);
        case PitchQuantizeScale::Minor:
            return snapToScale(semitones, minorScale, 7);
        case PitchQuantizeScale::Off:
        default:
            return semitones;
    }
}

} // namespace grainhex
