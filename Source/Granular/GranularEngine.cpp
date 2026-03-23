#include "Granular/GranularEngine.h"
#include <algorithm>

namespace grainhex {

GranularEngine::GranularEngine()
{
    // All grains start inactive
    for (auto& grain : grainPool)
        grain.active = false;

    // Force window table initialization at construction (not during audio callback)
    WindowFunctions::getSample(WindowShape::Hann, 0.0f);
}

void GranularEngine::setSourceBuffer(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate)
{
    sourceSampleRate = sampleRate;
    std::atomic_store(&sourceBuffer, buffer);
}

void GranularEngine::processBlock(float* outL, float* outR, int numSamples, double deviceSampleRate)
{
    auto buffer = std::atomic_load(&sourceBuffer);
    if (buffer == nullptr || buffer->getNumSamples() == 0)
        return;

    const int64_t sourceLength = buffer->getNumSamples();
    const int sourceChannels = buffer->getNumChannels();
    const float* srcL = buffer->getReadPointer(0);
    const float* srcR = (sourceChannels >= 2) ? buffer->getReadPointer(1) : nullptr;

    // Push atomic params to scheduler
    scheduler.setGrainSize(paramGrainSize.load(std::memory_order_relaxed));
    scheduler.setGrainCount(paramGrainCount.load(std::memory_order_relaxed));
    scheduler.setPosition(paramPosition.load(std::memory_order_relaxed));
    scheduler.setSpray(paramSpray.load(std::memory_order_relaxed));
    scheduler.setPitchSemitones(paramPitch.load(std::memory_order_relaxed));
    scheduler.setPitchQuantize(static_cast<PitchQuantizeScale>(paramPitchQuantize.load(std::memory_order_relaxed)));
    scheduler.setWindowShape(static_cast<WindowShape>(paramWindowShape.load(std::memory_order_relaxed)));
    scheduler.setDirection(static_cast<DirectionMode>(paramDirection.load(std::memory_order_relaxed)));
    scheduler.setSpread(paramSpread.load(std::memory_order_relaxed));

    // Prepare scheduler for this block
    scheduler.prepareBlock(numSamples, deviceSampleRate, sourceLength);

    // Render sample-by-sample
    for (int i = 0; i < numSamples; ++i)
    {
        // Check if we should spawn a new grain
        if (scheduler.shouldSpawnGrain())
        {
            auto params = scheduler.getSpawnParams(deviceSampleRate, sourceLength);

            // Find an inactive grain slot
            for (int g = 0; g < MaxGrains; ++g)
            {
                if (!grainPool[static_cast<size_t>(g)].active)
                {
                    grainPool[static_cast<size_t>(g)].start(
                        params.sourcePosition,
                        params.grainLengthSamples,
                        params.playbackIncrement,
                        params.panLeft,
                        params.panRight,
                        params.windowShape,
                        params.reverse);

                    // Update high-water mark
                    if (g >= highestActiveIndex)
                        highestActiveIndex = g + 1;
                    break;
                }
            }
        }

        // Render active grains (only scan up to highest active index)
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (int g = 0; g < highestActiveIndex; ++g)
        {
            auto& grain = grainPool[static_cast<size_t>(g)];
            if (grain.active)
                grain.renderSample(srcL, srcR, sourceLength, sampleL, sampleR);
        }

        outL[i] += sampleL;
        outR[i] += sampleR;
    }

    // Shrink high-water mark at end of block
    while (highestActiveIndex > 0 && !grainPool[static_cast<size_t>(highestActiveIndex - 1)].active)
        --highestActiveIndex;
}

void GranularEngine::setGrainSize(float sizeMs) { paramGrainSize.store(sizeMs, std::memory_order_relaxed); }
void GranularEngine::setGrainCount(int count) { paramGrainCount.store(count, std::memory_order_relaxed); }
void GranularEngine::setPosition(float normPos) { paramPosition.store(normPos, std::memory_order_relaxed); }
void GranularEngine::setSpray(float amount) { paramSpray.store(amount, std::memory_order_relaxed); }
void GranularEngine::setPitchSemitones(float semitones) { paramPitch.store(semitones, std::memory_order_relaxed); }
void GranularEngine::setPitchQuantize(PitchQuantizeScale scale) { paramPitchQuantize.store(static_cast<int>(scale), std::memory_order_relaxed); }
void GranularEngine::setWindowShape(WindowShape shape) { paramWindowShape.store(static_cast<int>(shape), std::memory_order_relaxed); }
void GranularEngine::setDirection(DirectionMode direction) { paramDirection.store(static_cast<int>(direction), std::memory_order_relaxed); }
void GranularEngine::setSpread(float amount) { paramSpread.store(amount, std::memory_order_relaxed); }

std::vector<float> GranularEngine::getActiveGrainPositions() const
{
    // Called from UI thread — collect positions of active grains
    auto buffer = std::atomic_load(&sourceBuffer);
    if (buffer == nullptr || buffer->getNumSamples() == 0)
        return {};

    std::vector<float> positions;
    positions.reserve(MaxGrains);

    float sourceLen = static_cast<float>(buffer->getNumSamples());

    int scanLimit = std::min(highestActiveIndex, MaxGrains);
    for (int g = 0; g < scanLimit; ++g)
    {
        if (grainPool[static_cast<size_t>(g)].active)
        {
            float norm = static_cast<float>(grainPool[static_cast<size_t>(g)].playbackCursor) / sourceLen;
            positions.push_back(std::clamp(norm, 0.0f, 1.0f));
        }
    }

    return positions;
}

int GranularEngine::getActiveGrainCount() const
{
    int count = 0;
    int scanLimit = std::min(highestActiveIndex, MaxGrains);
    for (int g = 0; g < scanLimit; ++g)
        if (grainPool[static_cast<size_t>(g)].active)
            ++count;
    return count;
}

} // namespace grainhex
