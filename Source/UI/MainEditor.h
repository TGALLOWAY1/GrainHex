#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "Audio/AudioEngine.h"
#include "SourceInput/SourceSampleManager.h"
#include "UI/WaveformView.h"
#include "UI/GranularPanel.h"

namespace grainhex {

/**
 * Main editor component — owns the full UI layout.
 * Phase 1: waveform, transport controls, sample info, drag-and-drop zone.
 */
class MainEditor : public juce::Component,
                   public juce::FileDragAndDropTarget,
                   public juce::DragAndDropContainer
{
public:
    MainEditor(AudioEngine& engine, SourceSampleManager& sampleManager);
    ~MainEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void loadFile(const juce::File& file);
    void updateStatusLabel(const juce::String& text);

    AudioEngine& audioEngine;
    SourceSampleManager& sampleManager;

    void pushGranularParams();

    // UI components
    WaveformView waveformView;
    GranularPanel granularPanel;

    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton sineTestButton { "Sine Test" };
    juce::TextButton loadButton { "Load Sample" };
    juce::ToggleButton loopButton { "Loop" };
    juce::ToggleButton granularToggle { "Granular" };

    juce::Slider volumeSlider;
    juce::Label volumeLabel { {}, "Vol" };

    juce::Label statusLabel;
    juce::Label rootNoteLabel;
    juce::Label sampleInfoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainEditor)
};

} // namespace grainhex
