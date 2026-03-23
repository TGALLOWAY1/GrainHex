#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "Audio/AudioEngine.h"
#include "SourceInput/SourceSampleManager.h"
#include "UI/GrainHexLookAndFeel.h"
#include "UI/WaveformView.h"
#include "UI/GranularPanel.h"
#include "UI/SubPanel.h"
#include "UI/EffectsPanel.h"
#include "UI/ModulationPanel.h"
#include "UI/ResamplePanel.h"
#include "UI/SampleBrowser.h"

namespace grainhex {

/**
 * Main editor component — owns the full UI layout.
 * Phase 6: polished PRD layout with sample browser and footer.
 *
 * Layout:
 *   TOP:           waveform + sample browser + transport
 *   CENTER:        granular controls
 *   BOTTOM-LEFT:   sub tuner
 *   BOTTOM-RIGHT:  resample history
 *   RIGHT SIDEBAR: effects + modulation
 *   FOOTER:        master level, mono lock, export, status
 */
class MainEditor : public juce::Component,
                   public juce::FileDragAndDropTarget,
                   public juce::DragAndDropContainer,
                   private juce::Timer
{
public:
    MainEditor(AudioEngine& engine, SourceSampleManager& sampleManager);
    ~MainEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void loadFile(const juce::File& file);
    void loadFactorySample(const FactorySample& sample);
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

    // Look and feel
    GrainHexLookAndFeel lookAndFeel;

    // === TOP SECTION ===
    WaveformView waveformView;
    SampleBrowser sampleBrowser;

    // Transport
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton loadButton { "Load" };
    juce::ToggleButton loopButton { "Loop" };
    juce::ToggleButton granularToggle { "Granular" };

    // Info
    juce::Label rootNoteLabel;
    juce::Label sampleInfoLabel;
    juce::Label midiActivityLabel;

    // === CENTER ===
    GranularPanel granularPanel;

    // === BOTTOM-LEFT ===
    SubPanel subPanel;

    // === BOTTOM-RIGHT ===
    ResamplePanel resamplePanel;

    // === RIGHT SIDEBAR ===
    EffectsPanel effectsPanel;
    ModulationPanel modulationPanel;

    // === FOOTER ===
    juce::Slider volumeSlider;
    juce::Label volumeLabel { {}, "Master" };
    juce::ToggleButton monoLockToggle { "Mono" };
    juce::TextButton exportButton { "Export WAV" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainEditor)
};

} // namespace grainhex
