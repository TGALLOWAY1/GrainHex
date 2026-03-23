#include "Resample/ResampleEngine.h"
#include <algorithm>
#include <cmath>

namespace grainhex {

ResampleEngine::ResampleEngine()
{
    history.reserve(maxHistoryEntries);
}

void ResampleEngine::startCapture(double sampleRate, double lengthInSeconds, CaptureCompleteCallback callback)
{
    if (capturing.load(std::memory_order_relaxed))
        return;

    captureSampleRate = sampleRate;
    captureLength = static_cast<int>(sampleRate * lengthInSeconds);
    if (captureLength <= 0)
        return;

    // Pre-allocate capture buffer (stereo)
    captureBuffer = std::make_shared<juce::AudioBuffer<float>>(2, captureLength);
    captureBuffer->clear();
    captureWritePos.store(0, std::memory_order_relaxed);
    captureCallback = std::move(callback);

    // Enable capture — audio thread will start writing
    capturing.store(true, std::memory_order_release);
}

void ResampleEngine::cancelCapture()
{
    capturing.store(false, std::memory_order_relaxed);
    captureBuffer = nullptr;
    captureCallback = nullptr;
}

float ResampleEngine::getCaptureProgress() const
{
    if (!capturing.load(std::memory_order_relaxed) || captureLength <= 0)
        return 0.0f;
    return static_cast<float>(captureWritePos.load(std::memory_order_relaxed)) / static_cast<float>(captureLength);
}

void ResampleEngine::feedCapture(const float* left, const float* right, int numSamples)
{
    if (!capturing.load(std::memory_order_acquire))
        return;

    auto buf = captureBuffer;
    if (buf == nullptr)
        return;

    int pos = captureWritePos.load(std::memory_order_relaxed);
    const int remaining = captureLength - pos;
    const int toCopy = std::min(numSamples, remaining);

    if (toCopy <= 0)
        return;

    // Copy audio data into capture buffer
    float* destL = buf->getWritePointer(0) + pos;
    float* destR = buf->getWritePointer(1) + pos;

    if (left != nullptr)
        std::memcpy(destL, left, sizeof(float) * static_cast<size_t>(toCopy));
    if (right != nullptr)
        std::memcpy(destR, right, sizeof(float) * static_cast<size_t>(toCopy));

    pos += toCopy;
    captureWritePos.store(pos, std::memory_order_relaxed);

    // Check if capture is complete
    if (pos >= captureLength)
    {
        capturing.store(false, std::memory_order_release);

        // Finalise on message thread
        juce::MessageManager::callAsync([this]() { finaliseCapture(); });
    }
}

void ResampleEngine::finaliseCapture()
{
    if (captureBuffer == nullptr)
        return;

    // Create history entry
    ResampleHistoryEntry entry;
    entry.id = nextId++;
    entry.buffer = captureBuffer;
    entry.sampleRate = captureSampleRate;
    entry.iterationIndex = static_cast<int>(history.size());
    entry.timestamp = juce::Time::currentTimeMillis();
    entry.exported = false;

    // Detect root note
    entry.rootNote = rootNoteDetector.detect(*captureBuffer, captureSampleRate);

    // Build waveform preview
    buildPreview(entry);

    // Handle overflow: if at max, warn and remove oldest
    if (static_cast<int>(history.size()) >= maxHistoryEntries)
    {
        history.erase(history.begin());
    }

    history.push_back(std::move(entry));
    currentIndex = static_cast<int>(history.size()) - 1;

    // Clean up capture state
    captureBuffer = nullptr;

    // Fire callback
    if (captureCallback)
    {
        auto cb = captureCallback;
        captureCallback = nullptr;
        cb(history.back());
    }
}

void ResampleEngine::buildPreview(ResampleHistoryEntry& entry)
{
    if (entry.buffer == nullptr || entry.buffer->getNumSamples() == 0)
        return;

    const int numSamples = entry.buffer->getNumSamples();
    const int width = ResampleHistoryEntry::previewWidth;
    const float samplesPerPixel = static_cast<float>(numSamples) / static_cast<float>(width);

    entry.previewMin.resize(static_cast<size_t>(width));
    entry.previewMax.resize(static_cast<size_t>(width));

    const float* data = entry.buffer->getReadPointer(0);

    for (int i = 0; i < width; ++i)
    {
        int start = static_cast<int>(static_cast<float>(i) * samplesPerPixel);
        int end = static_cast<int>(static_cast<float>(i + 1) * samplesPerPixel);
        end = std::min(end, numSamples);

        float mn = 1.0f;
        float mx = -1.0f;

        for (int s = start; s < end; ++s)
        {
            float v = data[s];
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }

        entry.previewMin[static_cast<size_t>(i)] = mn;
        entry.previewMax[static_cast<size_t>(i)] = mx;
    }
}

int ResampleEngine::getHistorySize() const
{
    return static_cast<int>(history.size());
}

const ResampleHistoryEntry* ResampleEngine::getHistoryEntry(int index) const
{
    if (index < 0 || index >= static_cast<int>(history.size()))
        return nullptr;
    return &history[static_cast<size_t>(index)];
}

const ResampleHistoryEntry* ResampleEngine::getCurrentEntry() const
{
    return getHistoryEntry(currentIndex);
}

const ResampleHistoryEntry* ResampleEngine::revertTo(int index)
{
    if (index < 0 || index >= static_cast<int>(history.size()))
        return nullptr;
    currentIndex = index;
    return &history[static_cast<size_t>(index)];
}

const ResampleHistoryEntry* ResampleEngine::undo()
{
    if (currentIndex <= 0)
        return nullptr;
    currentIndex--;
    return &history[static_cast<size_t>(currentIndex)];
}

void ResampleEngine::clearHistory()
{
    history.clear();
    currentIndex = -1;
}

void ResampleEngine::markExported(int index)
{
    if (index >= 0 && index < static_cast<int>(history.size()))
        history[static_cast<size_t>(index)].exported = true;
}

bool ResampleEngine::oldestNeverExported() const
{
    if (history.empty())
        return false;
    return !history.front().exported && static_cast<int>(history.size()) >= maxHistoryEntries;
}

} // namespace grainhex
