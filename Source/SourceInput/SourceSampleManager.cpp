#include "SourceInput/SourceSampleManager.h"

namespace grainhex {

SourceSampleManager::SourceSampleManager()
    : juce::Thread("SampleLoader")
{
}

SourceSampleManager::~SourceSampleManager()
{
    stopThread(5000);
}

void SourceSampleManager::loadFileAsync(const juce::File& file, LoadCallback callback)
{
    {
        const juce::ScopedLock sl(pendingLock);
        pendingFile = file;
        pendingCallback = std::move(callback);
    }

    // If thread is already running, it will pick up the new request
    if (!isThreadRunning())
        startThread(juce::Thread::Priority::normal);
    else
        notify(); // Wake up if waiting
}

void SourceSampleManager::run()
{
    juce::File fileToLoad;
    LoadCallback callback;

    {
        const juce::ScopedLock sl(pendingLock);
        fileToLoad = pendingFile;
        callback = pendingCallback;
        pendingFile = {};
        pendingCallback = nullptr;
    }

    if (!fileToLoad.existsAsFile() || callback == nullptr)
        return;

    // Load and decode
    auto result = fileLoader.loadFile(fileToLoad);

    if (result.success)
    {
        // Run root note detection
        result.metadata.rootNote = rootNoteDetector.detect(*result.buffer, result.metadata.originalSampleRate);

        // Publish the new buffer and metadata
        {
            const juce::ScopedLock sl(metadataLock);
            currentMetadata = result.metadata;
        }
        std::atomic_store(&currentBuffer, result.buffer);

        juce::MessageManager::callAsync([callback, meta = result.metadata]()
        {
            callback(true, "Loaded: " + meta.filename
                     + " | Root: " + meta.rootNote.getNoteName()
                     + " (" + juce::String(meta.rootNote.confidence * 100.0f, 0) + "% confidence)");
        });
    }
    else
    {
        juce::MessageManager::callAsync([callback, err = result.errorMessage]()
        {
            callback(false, err);
        });
    }
}

std::shared_ptr<juce::AudioBuffer<float>> SourceSampleManager::getCurrentBuffer() const
{
    return std::atomic_load(&currentBuffer);
}

SampleMetadata SourceSampleManager::getMetadata() const
{
    const juce::ScopedLock sl(metadataLock);
    return currentMetadata;
}

bool SourceSampleManager::hasSample() const
{
    return std::atomic_load(&currentBuffer) != nullptr;
}

bool SourceSampleManager::isFormatSupported(const juce::File& file) const
{
    return fileLoader.isFormatSupported(file);
}

juce::String SourceSampleManager::getSupportedFormatsWildcard() const
{
    return fileLoader.getSupportedFormatsWildcard();
}

} // namespace grainhex
