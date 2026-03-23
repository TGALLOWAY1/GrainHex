#include "Granular/GrainVoice.h"
#include "Granular/Interpolator.h"

namespace grainhex {

void GrainVoice::start(double startPos, int lengthSamples, double increment,
                        float panL, float panR, WindowShape window, bool rev)
{
    active = true;
    sourceStartPosition = startPos;
    grainLengthSamples = lengthSamples;
    samplesRendered = 0;
    playbackIncrement = increment;
    panLeft = panL;
    panRight = panR;
    windowShape = window;
    reverse = rev;

    if (reverse)
        playbackCursor = startPos + static_cast<double>(lengthSamples);
    else
        playbackCursor = startPos;
}

void GrainVoice::renderSample(const float* sourceL, const float* sourceR,
                                int64_t sourceLength, float& outL, float& outR)
{
    if (!active || grainLengthSamples <= 0)
        return;

    // Window envelope based on progress through grain
    float windowPhase = static_cast<float>(samplesRendered) / static_cast<float>(grainLengthSamples);
    float envelope = WindowFunctions::getSample(windowShape, windowPhase);

    // Read source with cubic interpolation
    float sampleL = Interpolator::cubicInterpolate(sourceL, sourceLength, playbackCursor);
    float sampleR = (sourceR != nullptr)
        ? Interpolator::cubicInterpolate(sourceR, sourceLength, playbackCursor)
        : sampleL;

    // Apply envelope and pan, add to output
    outL += sampleL * envelope * panLeft;
    outR += sampleR * envelope * panRight;

    // Advance cursor
    if (reverse)
        playbackCursor -= playbackIncrement;
    else
        playbackCursor += playbackIncrement;

    ++samplesRendered;

    // Deactivate when grain is complete
    if (samplesRendered >= grainLengthSamples)
        active = false;
}

} // namespace grainhex
