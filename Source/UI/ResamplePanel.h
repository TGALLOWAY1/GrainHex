#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "Resample/ResampleEngine.h"
#include "Resample/WavExporter.h"
#include "UI/GrainHexLookAndFeel.h"
#include <functional>

namespace grainhex {

/**
 * ResamplePanel — UI for the resample workflow.
 *
 * Shows:
 * - Resample button with capture length control
 * - Linear history of up to 8 mini waveform thumbnails
 * - Click-to-revert on history entries
 * - Undo button
 * - Export controls (bit depth selector + export button)
 * - Drag-and-drop out from history thumbnails
 */
class ResamplePanel : public juce::Component
{
public:
    ResamplePanel();
    ~ResamplePanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Update from ResampleEngine state
    void updateFromEngine(ResampleEngine& engine);

    // Set capture progress (0..1) for progress bar
    void setCaptureProgress(float progress);

    // Callbacks
    std::function<void()> onResample;                        // Resample button clicked
    std::function<void(int index)> onRevertTo;               // History entry clicked
    std::function<void()> onUndo;                            // Undo button clicked
    std::function<void(int index)> onExport;                 // Export specific entry
    std::function<void()> onExportCurrent;                   // Export current state
    std::function<void()> onClearHistory;                     // Clear history

    float getCaptureLengthSeconds() const;
    WavExporter::BitDepth getBitDepth() const;

private:
    /**
     * Mini waveform thumbnail component for history entries.
     */
    class HistoryThumbnail : public juce::Component
    {
    public:
        HistoryThumbnail();
        void paint(juce::Graphics& g) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

        void setEntry(const ResampleHistoryEntry* entry, int index, bool isCurrent);

        std::function<void(int)> onClick;
        std::function<void(int)> onExport;
        std::function<void(int, const juce::MouseEvent&)> onDragStart;

    private:
        const ResampleHistoryEntry* historyEntry = nullptr;
        int entryIndex = -1;
        bool current = false;
        bool hovered = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HistoryThumbnail)
    };

    // UI components
    juce::TextButton resampleButton { "Resample" };
    juce::TextButton undoButton { "Undo" };
    juce::TextButton exportButton { "Export WAV" };
    juce::TextButton clearButton { "Clear" };

    juce::Slider captureLengthSlider;
    juce::Label captureLengthLabel { {}, "Length" };

    juce::ComboBox bitDepthSelector;
    juce::Label bitDepthLabel { {}, "Bit Depth" };

    juce::Label historyLabel { {}, "Resample History" };
    juce::Label progressLabel;

    // History thumbnails (fixed pool of 8)
    std::array<HistoryThumbnail, 8> thumbnails;
    int visibleThumbnails = 0;

    float captureProgress = 0.0f;
    juce::Rectangle<int> historyBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResamplePanel)
};

} // namespace grainhex
