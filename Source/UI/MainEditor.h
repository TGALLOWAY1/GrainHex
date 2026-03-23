#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "Audio/AudioEngine.h"
#include "SourceInput/SourceSampleManager.h"
#include "UI/WaveformView.h"
#include "UI/GranularPanel.h"
#include "UI/SubPanel.h"
#include "UI/EffectsPanel.h"
#include "UI/ModulationPanel.h"
#include "UI/ResamplePanel.h"

namespace grainhex {

/**
 * Main editor component — owns the full UI layout.
 * Phase 4: adds effects, modulation, and MIDI panels.
 */
class MainEditor : public juce::Component,
                   public juce::FileDragAndDropTarget,
                   public juce::DragAndDropContainer,
                   private juce::Timer
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
    void pushSubParams();
    void pushEffectsParams();
    void pushModulationParams();
    void handleResample();
    void handleRevertTo(int index);
    void handleUndo();
    void handleExportEntry(int index);
    void handleExportCurrent();
    void reloadFromHistory(const ResampleHistoryEntry* entry);
    void timerCallback() override;

    // UI components
    WaveformView waveformView;
    GranularPanel granularPanel;
    SubPanel subPanel;
    EffectsPanel effectsPanel;
    ModulationPanel modulationPanel;
    ResamplePanel resamplePanel;

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

    // MIDI activity indicator
    juce::Label midiActivityLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainEditor)
};

} // namespace grainhex
