#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Audio/AudioTypes.h"

namespace grainhex {

/**
 * Offline pitch detection using YIN algorithm.
 * Analyzes a sample buffer and returns the detected root note.
 * Run on a worker thread, NOT the audio thread.
 */
class RootNoteDetector
{
public:
    RootNoteDetector() = default;

    // Analyze a buffer and detect the fundamental pitch.
    // Uses a segment from the middle of the buffer for stability.
    PitchInfo detect(const juce::AudioBuffer<float>& buffer, double sampleRate);

    // Convert frequency to MIDI note + cents
    static PitchInfo frequencyToPitchInfo(float frequency, float confidence);

private:
    // YIN pitch detection on a mono signal segment
    float yinDetect(const float* data, int numSamples, double sampleRate, float& outConfidence);

    static constexpr int kYinWindowSize = 4096;
    static constexpr float kYinThreshold = 0.15f;
    static constexpr float kMinFrequency = 20.0f;
    static constexpr float kMaxFrequency = 2000.0f;
};

} // namespace grainhex
