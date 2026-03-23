#include "UI/ResamplePanel.h"

namespace grainhex {

// --- HistoryThumbnail ---

ResamplePanel::HistoryThumbnail::HistoryThumbnail()
{
}

void ResamplePanel::HistoryThumbnail::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    auto bgColour = current ? juce::Colour(0xff1a3a2a) : juce::Colour(0xff1a1a2a);
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    auto borderColour = current ? juce::Colour(0xff16c784) : juce::Colour(0xff333355);
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, current ? 2.0f : 1.0f);

    if (historyEntry == nullptr)
        return;

    // Draw waveform preview
    auto waveArea = bounds.reduced(4.0f, 12.0f);
    float centreY = waveArea.getCentreY();

    g.setColour(current ? juce::Colour(0xff16c784).withAlpha(0.8f) : juce::Colour(0xff6666aa));

    const int numPoints = static_cast<int>(historyEntry->previewMin.size());
    if (numPoints > 0)
    {
        float xScale = waveArea.getWidth() / static_cast<float>(numPoints);
        float yScale = waveArea.getHeight() * 0.5f;

        for (int i = 0; i < numPoints; ++i)
        {
            float x = waveArea.getX() + static_cast<float>(i) * xScale;
            float minY = centreY - historyEntry->previewMin[static_cast<size_t>(i)] * yScale;
            float maxY = centreY - historyEntry->previewMax[static_cast<size_t>(i)] * yScale;
            g.drawVerticalLine(static_cast<int>(x), std::min(minY, maxY), std::max(minY, maxY));
        }
    }

    // Iteration label
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(10.0f);
    g.drawText("#" + juce::String(entryIndex + 1), bounds.reduced(4.0f, 2.0f),
               juce::Justification::topLeft);

    // Root note label
    if (historyEntry->rootNote.midiNote >= 0)
    {
        g.drawText(historyEntry->rootNote.getNoteName(), bounds.reduced(4.0f, 2.0f),
                   juce::Justification::topRight);
    }

    // Duration
    g.drawText(juce::String(historyEntry->getLengthSeconds(), 1) + "s",
               bounds.reduced(4.0f, 2.0f), juce::Justification::bottomLeft);

    // Exported indicator
    if (historyEntry->exported)
    {
        g.setColour(juce::Colour(0xff16c784).withAlpha(0.6f));
        g.setFont(9.0f);
        g.drawText("EXP", bounds.reduced(4.0f, 2.0f), juce::Justification::bottomRight);
    }
}

void ResamplePanel::HistoryThumbnail::mouseDown(const juce::MouseEvent& e)
{
    if (historyEntry == nullptr)
        return;

    if (e.mods.isPopupMenu() && onExport)
    {
        onExport(entryIndex);
        return;
    }

    if (onClick)
        onClick(entryIndex);
}

void ResamplePanel::HistoryThumbnail::mouseDrag(const juce::MouseEvent& e)
{
    if (historyEntry == nullptr)
        return;

    if (e.getDistanceFromDragStart() > 5 && onDragStart)
        onDragStart(entryIndex, e);
}

void ResamplePanel::HistoryThumbnail::setEntry(const ResampleHistoryEntry* entry, int index, bool isCurrent)
{
    historyEntry = entry;
    entryIndex = index;
    current = isCurrent;
    repaint();
}

// --- ResamplePanel ---

ResamplePanel::ResamplePanel()
{
    // Resample button
    addAndMakeVisible(resampleButton);
    resampleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff16c784));
    resampleButton.onClick = [this] { if (onResample) onResample(); };

    // Undo button
    addAndMakeVisible(undoButton);
    undoButton.onClick = [this] { if (onUndo) onUndo(); };

    // Export button
    addAndMakeVisible(exportButton);
    exportButton.onClick = [this] { if (onExportCurrent) onExportCurrent(); };

    // Clear button
    addAndMakeVisible(clearButton);
    clearButton.onClick = [this] { if (onClearHistory) onClearHistory(); };

    // Capture length slider
    addAndMakeVisible(captureLengthSlider);
    captureLengthSlider.setRange(0.5, 30.0, 0.5);
    captureLengthSlider.setValue(4.0, juce::dontSendNotification);
    captureLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    captureLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    captureLengthSlider.setTextValueSuffix("s");
    addAndMakeVisible(captureLengthLabel);
    captureLengthLabel.setJustificationType(juce::Justification::centredRight);
    captureLengthLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Bit depth selector
    addAndMakeVisible(bitDepthSelector);
    bitDepthSelector.addItem("16-bit", 1);
    bitDepthSelector.addItem("24-bit", 2);
    bitDepthSelector.addItem("32-float", 3);
    bitDepthSelector.setSelectedId(2, juce::dontSendNotification); // Default 24-bit
    addAndMakeVisible(bitDepthLabel);
    bitDepthLabel.setJustificationType(juce::Justification::centredRight);
    bitDepthLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // History label
    addAndMakeVisible(historyLabel);
    historyLabel.setFont(juce::Font(14.0f).boldened());
    historyLabel.setColour(juce::Label::textColourId, juce::Colour(0xff16c784));

    // Progress label
    addAndMakeVisible(progressLabel);
    progressLabel.setJustificationType(juce::Justification::centred);
    progressLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);

    // History thumbnails
    for (auto& thumb : thumbnails)
    {
        addChildComponent(thumb);
        thumb.onClick = [this](int index) { if (onRevertTo) onRevertTo(index); };
        thumb.onExport = [this](int index) { if (onExport) onExport(index); };
        thumb.onDragStart = [this](int index, const juce::MouseEvent&)
        {
            // Drag-and-drop out: handled by parent DragAndDropContainer
            if (onExport)
                onExport(index);
        };
    }
}

void ResamplePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Panel background
    g.setColour(juce::Colour(0xff111122));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Border
    g.setColour(juce::Colour(0xff333355));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    // Capture progress bar
    if (captureProgress > 0.0f && captureProgress < 1.0f)
    {
        auto progressArea = bounds.reduced(10.0f).removeFromBottom(4.0f);
        g.setColour(juce::Colour(0xff333355));
        g.fillRoundedRectangle(progressArea, 2.0f);
        g.setColour(juce::Colour(0xff16c784));
        g.fillRoundedRectangle(progressArea.withWidth(progressArea.getWidth() * captureProgress), 2.0f);
    }
}

void ResamplePanel::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Top row: title + resample button + undo
    auto topRow = area.removeFromTop(28);
    historyLabel.setBounds(topRow.removeFromLeft(160));
    topRow.removeFromLeft(10);
    resampleButton.setBounds(topRow.removeFromLeft(90));
    topRow.removeFromLeft(5);
    undoButton.setBounds(topRow.removeFromLeft(55));
    topRow.removeFromLeft(5);
    clearButton.setBounds(topRow.removeFromLeft(55));

    area.removeFromTop(6);

    // Controls row: capture length + bit depth + export
    auto controlRow = area.removeFromTop(26);
    captureLengthLabel.setBounds(controlRow.removeFromLeft(45));
    controlRow.removeFromLeft(4);
    captureLengthSlider.setBounds(controlRow.removeFromLeft(180));
    controlRow.removeFromLeft(15);
    bitDepthLabel.setBounds(controlRow.removeFromLeft(55));
    controlRow.removeFromLeft(4);
    bitDepthSelector.setBounds(controlRow.removeFromLeft(90));
    controlRow.removeFromLeft(10);
    exportButton.setBounds(controlRow.removeFromLeft(90));

    // Progress label
    progressLabel.setBounds(controlRow);

    area.removeFromTop(8);

    // History thumbnails — horizontal strip
    auto historyArea = area;
    int thumbWidth = (historyArea.getWidth() - 7 * 4) / 8; // 4px gap between thumbnails
    int thumbHeight = historyArea.getHeight();

    for (int i = 0; i < 8; ++i)
    {
        auto thumbBounds = historyArea.removeFromLeft(thumbWidth);
        thumbnails[static_cast<size_t>(i)].setBounds(thumbBounds.withHeight(thumbHeight));
        if (i < 7)
            historyArea.removeFromLeft(4);
    }
}

void ResamplePanel::updateFromEngine(ResampleEngine& engine)
{
    int count = engine.getHistorySize();
    int current = engine.getCurrentIndex();
    visibleThumbnails = count;

    for (int i = 0; i < 8; ++i)
    {
        if (i < count)
        {
            thumbnails[static_cast<size_t>(i)].setEntry(engine.getHistoryEntry(i), i, i == current);
            thumbnails[static_cast<size_t>(i)].setVisible(true);
        }
        else
        {
            thumbnails[static_cast<size_t>(i)].setEntry(nullptr, -1, false);
            thumbnails[static_cast<size_t>(i)].setVisible(false);
        }
    }

    // Update button states
    undoButton.setEnabled(current > 0);
    exportButton.setEnabled(count > 0);
    clearButton.setEnabled(count > 0);
}

void ResamplePanel::setCaptureProgress(float progress)
{
    captureProgress = progress;

    if (progress > 0.0f && progress < 1.0f)
    {
        resampleButton.setEnabled(false);
        progressLabel.setText("Capturing " + juce::String(static_cast<int>(progress * 100.0f)) + "%",
                             juce::dontSendNotification);
    }
    else
    {
        resampleButton.setEnabled(true);
        progressLabel.setText("", juce::dontSendNotification);
    }

    repaint();
}

float ResamplePanel::getCaptureLengthSeconds() const
{
    return static_cast<float>(captureLengthSlider.getValue());
}

WavExporter::BitDepth ResamplePanel::getBitDepth() const
{
    switch (bitDepthSelector.getSelectedId())
    {
        case 1: return WavExporter::BitDepth::Bit16;
        case 3: return WavExporter::BitDepth::Float32;
        default: return WavExporter::BitDepth::Bit24;
    }
}

} // namespace grainhex
