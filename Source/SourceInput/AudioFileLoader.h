#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "Audio/AudioTypes.h"
#include <memory>

namespace grainhex {

/**
 * Loads audio files off the audio thread.
 * Supports WAV, AIFF, FLAC.
 * Decodes to float and normalizes mono/stereo.
 */
class AudioFileLoader
{
public:
    AudioFileLoader();

    struct LoadResult
    {
        std::shared_ptr<juce::AudioBuffer<float>> buffer;
        SampleMetadata metadata;
        bool success = false;
        juce::String errorMessage;
    };

    // Load file synchronously (call from worker/UI thread, NOT audio thread)
    LoadResult loadFile(const juce::File& file);

    // Supported format check
    bool isFormatSupported(const juce::File& file) const;
    juce::String getSupportedFormatsWildcard() const;

private:
    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileLoader)
};

} // namespace grainhex
