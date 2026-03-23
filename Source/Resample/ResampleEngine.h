#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "Audio/AudioTypes.h"
#include "SourceInput/RootNoteDetector.h"
#include <memory>
#include <vector>
#include <functional>
#include <atomic>

namespace grainhex {

/**
 * One entry in the resample history.
 * Stores the captured audio, waveform preview data, and metadata.
 */
struct ResampleHistoryEntry
{
    int id = 0;
    std::shared_ptr<juce::AudioBuffer<float>> buffer;
    double sampleRate = 44100.0;
    int iterationIndex = 0;
    juce::int64 timestamp = 0;
    bool exported = false;
    PitchInfo rootNote;

    // Downsampled waveform preview (min/max pairs for fast drawing)
    std::vector<float> previewMin;
    std::vector<float> previewMax;
    static constexpr int previewWidth = 120;

    int getNumSamples() const { return buffer ? buffer->getNumSamples() : 0; }
    double getLengthSeconds() const { return buffer ? static_cast<double>(buffer->getNumSamples()) / sampleRate : 0.0; }
};

/**
 * ResampleEngine — captures audio output, manages linear history (max 8),
 * supports revert, undo, WAV export, and drag-and-drop out.
 *
 * Capture flow:
 *   1. UI calls startCapture(lengthInSeconds)
 *   2. AudioEngine feeds samples via feedCapture()
 *   3. When capture completes, callback fires on message thread
 *   4. Captured audio becomes new source via reload callback
 */
class ResampleEngine
{
public:
    static constexpr int maxHistoryEntries = 8;

    using CaptureCompleteCallback = std::function<void(const ResampleHistoryEntry& entry)>;

    ResampleEngine();
    ~ResampleEngine() = default;

    // --- Capture control (call from UI thread) ---
    void startCapture(double sampleRate, double lengthInSeconds, CaptureCompleteCallback callback);
    void cancelCapture();
    bool isCapturing() const { return capturing.load(std::memory_order_relaxed); }
    float getCaptureProgress() const;

    // --- Audio thread interface ---
    // Feed captured audio from the audio callback. Must be lock-free.
    void feedCapture(const float* left, const float* right, int numSamples);

    // --- History management (UI thread) ---
    int getHistorySize() const;
    const ResampleHistoryEntry* getHistoryEntry(int index) const;
    const ResampleHistoryEntry* getCurrentEntry() const;
    int getCurrentIndex() const { return currentIndex; }

    // Revert to a specific history entry (returns the entry to reload)
    const ResampleHistoryEntry* revertTo(int index);

    // Single-step undo (returns the entry to reload, or nullptr if nothing to undo)
    const ResampleHistoryEntry* undo();

    // Clear all history
    void clearHistory();

    // Mark an entry as exported
    void markExported(int index);

    // Check if oldest entry was never exported (for overflow warning)
    bool oldestNeverExported() const;

private:
    void buildPreview(ResampleHistoryEntry& entry);
    void finaliseCapture();

    // History storage
    std::vector<ResampleHistoryEntry> history;
    int currentIndex = -1;
    int nextId = 1;

    // Capture state (audio-thread safe via atomics)
    std::atomic<bool> capturing { false };
    std::shared_ptr<juce::AudioBuffer<float>> captureBuffer;
    std::atomic<int> captureWritePos { 0 };
    int captureLength = 0;
    double captureSampleRate = 44100.0;
    CaptureCompleteCallback captureCallback;

    // Root note detection for resampled audio
    RootNoteDetector rootNoteDetector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResampleEngine)
};

} // namespace grainhex
