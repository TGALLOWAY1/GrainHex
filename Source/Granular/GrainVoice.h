#pragma once

#include "Granular/WindowFunctions.h"
#include <cstdint>

namespace grainhex {

struct GrainVoice
{
    bool active = false;

    double sourceStartPosition = 0.0;  // Start position in source (samples)
    double playbackCursor = 0.0;       // Current fractional read position
    int grainLengthSamples = 0;        // Total grain duration in samples
    int samplesRendered = 0;           // How many samples rendered so far
    double playbackIncrement = 1.0;    // Rate: 1.0 = original, 2.0 = octave up
    float panLeft = 1.0f;             // Left channel gain
    float panRight = 1.0f;            // Right channel gain
    WindowShape windowShape = WindowShape::Hann;
    bool reverse = false;

    // Initialize and activate this grain
    void start(double startPos, int lengthSamples, double increment,
               float panL, float panR, WindowShape window, bool rev);

    // Render one sample of this grain into outL/outR (additive)
    // sourceL/sourceR: channel pointers from source buffer
    // sourceLength: total samples in source
    void renderSample(const float* sourceL, const float* sourceR,
                      int64_t sourceLength, float& outL, float& outR);
};

} // namespace grainhex
