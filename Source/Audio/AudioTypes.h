#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

namespace grainhex {

// Detected pitch info shared between threads via atomic snapshot
struct PitchInfo
{
    float frequency = 0.0f;
    float confidence = 0.0f;
    int midiNote = -1;
    float centsOffset = 0.0f;

    juce::String getNoteName() const
    {
        if (midiNote < 0 || confidence < 0.5f)
            return "---";

        static const char* noteNames[] = {
            "C", "C#", "D", "D#", "E", "F",
            "F#", "G", "G#", "A", "A#", "B"
        };
        int octave = (midiNote / 12) - 1;
        int note = midiNote % 12;
        return juce::String(noteNames[note]) + juce::String(octave);
    }
};

// Sample metadata captured on load
struct SampleMetadata
{
    juce::String filename;
    double originalSampleRate = 0.0;
    int numChannels = 0;
    int64_t numSamples = 0;
    double lengthSeconds = 0.0;
    PitchInfo rootNote;
};

// Transport state
enum class TransportState
{
    Stopped,
    Playing
};

// Loop region in samples
struct LoopRegion
{
    int64_t startSample = 0;
    int64_t endSample = 0;

    bool isValid() const { return endSample > startSample; }
    int64_t lengthInSamples() const { return endSample - startSample; }
};

} // namespace grainhex
