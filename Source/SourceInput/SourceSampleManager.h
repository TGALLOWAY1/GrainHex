#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Audio/AudioTypes.h"
#include "SourceInput/AudioFileLoader.h"
#include "SourceInput/RootNoteDetector.h"
#include <memory>
#include <functional>

namespace grainhex {

/**
 * Manages the current source sample.
 * Handles async loading, metadata, and root note detection.
 * Thread-safe: loading happens on a background thread,
 * the buffer is atomically published.
 */
class SourceSampleManager : private juce::Thread
{
public:
    using LoadCallback = std::function<void(bool success, const juce::String& message)>;

    SourceSampleManager();
    ~SourceSampleManager() override;

    // Load a file asynchronously. Callback fires on the message thread.
    void loadFileAsync(const juce::File& file, LoadCallback callback);

    // Current source access (thread-safe)
    std::shared_ptr<juce::AudioBuffer<float>> getCurrentBuffer() const;
    SampleMetadata getMetadata() const;
    bool hasSample() const;

    // Format support check
    bool isFormatSupported(const juce::File& file) const;
    juce::String getSupportedFormatsWildcard() const;

private:
    void run() override; // Background loading thread

    AudioFileLoader fileLoader;
    RootNoteDetector rootNoteDetector;

    std::shared_ptr<juce::AudioBuffer<float>> currentBuffer;
    SampleMetadata currentMetadata;
    mutable juce::CriticalSection metadataLock;

    // Pending load request
    juce::File pendingFile;
    LoadCallback pendingCallback;
    juce::CriticalSection pendingLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceSampleManager)
};

} // namespace grainhex
