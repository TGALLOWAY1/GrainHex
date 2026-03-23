#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <array>
#include <functional>
#include <map>

namespace grainhex {

/**
 * MidiManager handles MIDI note input for granular pitch control,
 * CC learn for parameter mapping, and device selection.
 *
 * MIDI note input: maps relative to source root note.
 * CC learn: right-click a knob, move a CC to bind. Session persistence.
 * No MPE, no aftertouch in V1.
 */
class MidiManager
{
public:
    static constexpr int kMaxCCMappings = 32;

    // CC mapping entry
    struct CCMapping
    {
        int ccNumber = -1;
        int paramId = -1;   // Application-defined parameter ID
        float minValue = 0.0f;
        float maxValue = 1.0f;
    };

    // Callback types
    using NoteCallback = std::function<void(int midiNote, float velocity, bool isNoteOn)>;
    using CCCallback = std::function<void(int paramId, float normalizedValue)>;
    using CCLearnCallback = std::function<void(int ccNumber)>; // Fired when CC learned

    MidiManager() = default;

    // Process a block of MIDI messages (called from audio thread)
    void processMidiMessages(const juce::MidiBuffer& midiMessages)
    {
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();

            if (msg.isNoteOn())
            {
                lastNoteOn.store(msg.getNoteNumber(), std::memory_order_relaxed);
                lastVelocity.store(msg.getFloatVelocity(), std::memory_order_relaxed);
                noteHeld.store(true, std::memory_order_relaxed);
                midiActivity.store(true, std::memory_order_relaxed);

                if (noteCallback)
                    noteCallback(msg.getNoteNumber(), msg.getFloatVelocity(), true);
            }
            else if (msg.isNoteOff())
            {
                if (msg.getNoteNumber() == lastNoteOn.load(std::memory_order_relaxed))
                    noteHeld.store(false, std::memory_order_relaxed);

                midiActivity.store(true, std::memory_order_relaxed);

                if (noteCallback)
                    noteCallback(msg.getNoteNumber(), 0.0f, false);
            }
            else if (msg.isController())
            {
                int cc = msg.getControllerNumber();
                float value = msg.getControllerValue() / 127.0f;

                midiActivity.store(true, std::memory_order_relaxed);

                // If in learn mode, capture this CC
                if (learnMode.load(std::memory_order_relaxed))
                {
                    learnedCC.store(cc, std::memory_order_relaxed);
                    learnMode.store(false, std::memory_order_relaxed);
                    if (ccLearnCallback)
                        ccLearnCallback(cc);
                }

                // Check CC mappings and fire callback
                for (int i = 0; i < kMaxCCMappings; ++i)
                {
                    if (ccMappings[i].ccNumber == cc && ccMappings[i].paramId >= 0)
                    {
                        float mapped = ccMappings[i].minValue + value * (ccMappings[i].maxValue - ccMappings[i].minValue);
                        if (ccCallback)
                            ccCallback(ccMappings[i].paramId, mapped);
                    }
                }
            }
        }
    }

    // Set source root note for relative pitch mapping
    void setSourceRootNote(int midiNote) { sourceRootNote.store(midiNote, std::memory_order_relaxed); }

    // Get pitch offset in semitones from source root (for granular pitch)
    float getPitchOffsetFromRoot(int midiNote) const
    {
        return static_cast<float>(midiNote - sourceRootNote.load(std::memory_order_relaxed));
    }

    // Note state queries (thread-safe)
    bool isNoteHeld() const { return noteHeld.load(std::memory_order_relaxed); }
    int getLastNote() const { return lastNoteOn.load(std::memory_order_relaxed); }
    float getLastVelocity() const { return lastVelocity.load(std::memory_order_relaxed); }

    // MIDI activity indicator (resets after read)
    bool consumeActivity()
    {
        return midiActivity.exchange(false, std::memory_order_relaxed);
    }

    // CC Learn
    void startCCLearn() { learnMode.store(true, std::memory_order_relaxed); }
    void cancelCCLearn() { learnMode.store(false, std::memory_order_relaxed); }
    bool isLearning() const { return learnMode.load(std::memory_order_relaxed); }
    int getLearnedCC() const { return learnedCC.load(std::memory_order_relaxed); }

    // CC Mapping management
    bool addCCMapping(int ccNumber, int paramId, float minVal = 0.0f, float maxVal = 1.0f)
    {
        for (int i = 0; i < kMaxCCMappings; ++i)
        {
            if (ccMappings[i].ccNumber < 0)
            {
                ccMappings[i] = { ccNumber, paramId, minVal, maxVal };
                return true;
            }
        }
        return false;
    }

    bool removeCCMapping(int paramId)
    {
        for (int i = 0; i < kMaxCCMappings; ++i)
        {
            if (ccMappings[i].paramId == paramId)
            {
                ccMappings[i] = {};
                return true;
            }
        }
        return false;
    }

    void clearAllMappings()
    {
        for (int i = 0; i < kMaxCCMappings; ++i)
            ccMappings[i] = {};
    }

    // Callbacks (set from UI/main thread before audio starts)
    void setNoteCallback(NoteCallback cb) { noteCallback = std::move(cb); }
    void setCCCallback(CCCallback cb) { ccCallback = std::move(cb); }
    void setCCLearnCallback(CCLearnCallback cb) { ccLearnCallback = std::move(cb); }

private:
    std::atomic<int> lastNoteOn { -1 };
    std::atomic<float> lastVelocity { 0.0f };
    std::atomic<bool> noteHeld { false };
    std::atomic<bool> midiActivity { false };
    std::atomic<int> sourceRootNote { 60 }; // C4 default

    // CC learn state
    std::atomic<bool> learnMode { false };
    std::atomic<int> learnedCC { -1 };

    // Fixed-size CC mapping table
    std::array<CCMapping, kMaxCCMappings> ccMappings {};

    // Callbacks
    NoteCallback noteCallback;
    CCCallback ccCallback;
    CCLearnCallback ccLearnCallback;
};

} // namespace grainhex
