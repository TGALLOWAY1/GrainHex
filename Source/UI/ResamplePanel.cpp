#include "UI/ResamplePanel.h"

namespace grainhex {

// --- HistoryThumbnail ---

ResamplePanel::HistoryThumbnail::HistoryThumbnail()
{
    setRepaintsOnMouseActivity(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ResamplePanel::HistoryThumbnail::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto bgColour = current ? juce::Colour(Theme::bgElevated) : juce::Colour(Theme::bgControl);
    if (hovered)
        bgColour = bgColour.brighter(0.08f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, Theme::cornerRadius);

    auto borderColour = current ? juce::Colour(Theme::accentGreen)
                                : hovered ? juce::Colour(Theme::borderActive)
                                          : juce::Colour(Theme::border);
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds.reduced(0.5f), Theme::cornerRadius, current ? 2.0f : 1.0f);

    if (historyEntry == nullptr)
        return;

    auto waveArea = bounds.reduced(8.0f, 14.0f);
    waveArea.removeFromBottom(24.0f);
    const auto centreY = waveArea.getCentreY();

    g.setColour(current ? juce::Colour(Theme::accentGreen).withAlpha(0.9f)
                        : juce::Colour(Theme::textDim).withAlpha(0.8f));

    const auto numPoints = static_cast<int>(historyEntry->previewMin.size());
    if (numPoints > 0)
    {
        const auto xScale = waveArea.getWidth() / static_cast<float>(numPoints);
        const auto yScale = waveArea.getHeight() * 0.5f;

        for (int point = 0; point < numPoints; ++point)
        {
            const auto x = waveArea.getX() + static_cast<float>(point) * xScale;
            const auto minY = centreY - historyEntry->previewMin[static_cast<size_t>(point)] * yScale;
            const auto maxY = centreY - historyEntry->previewMax[static_cast<size_t>(point)] * yScale;
            g.drawVerticalLine(static_cast<int>(x), std::min(minY, maxY), std::max(minY, maxY));
        }
    }

    auto metaArea = bounds.reduced(8.0f, 6.0f);
    metaArea.removeFromTop(static_cast<int>(waveArea.getBottom() - bounds.getY()) + 6);

    g.setFont(juce::Font(Theme::fontSmall).boldened());
    g.setColour(juce::Colour(Theme::textBright));
    g.drawText("#" + juce::String(entryIndex + 1), metaArea.removeFromTop(12), juce::Justification::centredLeft);

    g.setFont(juce::Font(Theme::fontMetadata));
    g.setColour(juce::Colour(Theme::textDim));
    g.drawText(juce::String(historyEntry->getLengthSeconds(), 1) + "s",
               metaArea.removeFromTop(11), juce::Justification::centredLeft);

    if (historyEntry->rootNote.midiNote >= 0)
    {
        g.setColour(juce::Colour(Theme::textNormal));
        g.drawText(historyEntry->rootNote.getNoteName(), metaArea, juce::Justification::centredRight);
    }
}

void ResamplePanel::HistoryThumbnail::mouseEnter(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    hovered = true;
    repaint();
}

void ResamplePanel::HistoryThumbnail::mouseExit(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    hovered = false;
    repaint();
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
    addAndMakeVisible(resampleButton);
    resampleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Theme::buttonPrimary));
    resampleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(Theme::bgDarkest));
    resampleButton.onClick = [this] { if (onResample) onResample(); };

    addAndMakeVisible(undoButton);
    undoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Theme::buttonUtility));
    undoButton.onClick = [this] { if (onUndo) onUndo(); };

    addAndMakeVisible(exportButton);
    exportButton.setVisible(false);
    exportButton.onClick = [this] { if (onExportCurrent) onExportCurrent(); };

    addAndMakeVisible(clearButton);
    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Theme::buttonDestructive));
    clearButton.onClick = [this] { if (onClearHistory) onClearHistory(); };

    addAndMakeVisible(captureLengthSlider);
    captureLengthSlider.setRange(0.5, 30.0, 0.5);
    captureLengthSlider.setValue(4.0, juce::dontSendNotification);
    captureLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    captureLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
    captureLengthSlider.setTextValueSuffix("s");

    addAndMakeVisible(captureLengthLabel);
    captureLengthLabel.setJustificationType(juce::Justification::centredRight);
    captureLengthLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textNormal));
    captureLengthLabel.setFont(juce::Font(Theme::fontLabel));

    addAndMakeVisible(bitDepthSelector);
    bitDepthSelector.addItem("16-bit", 1);
    bitDepthSelector.addItem("24-bit", 2);
    bitDepthSelector.addItem("32-float", 3);
    bitDepthSelector.setSelectedId(2, juce::dontSendNotification);

    addAndMakeVisible(bitDepthLabel);
    bitDepthLabel.setJustificationType(juce::Justification::centredRight);
    bitDepthLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textNormal));
    bitDepthLabel.setFont(juce::Font(Theme::fontLabel));

    addAndMakeVisible(historyLabel);
    historyLabel.setText("History", juce::dontSendNotification);
    historyLabel.setFont(juce::Font(Theme::fontSmall).boldened());
    historyLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textDim));

    addAndMakeVisible(progressLabel);
    progressLabel.setJustificationType(juce::Justification::centredRight);
    progressLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textBright));
    progressLabel.setFont(juce::Font(Theme::fontMetadata));

    for (auto& thumb : thumbnails)
    {
        addChildComponent(thumb);
        thumb.onClick = [this](int index) { if (onRevertTo) onRevertTo(index); };
        thumb.onExport = [this](int index) { if (onExport) onExport(index); };
        thumb.onDragStart = [this](int index, const juce::MouseEvent&)
        {
            if (onExport)
                onExport(index);
        };
    }
}

void ResamplePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colour(Theme::bgPanel));
    g.fillRoundedRectangle(bounds, Theme::cornerRadius);

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(bounds.reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);

    g.setColour(juce::Colour(Theme::accentGreen));
    g.setFont(juce::Font(Theme::fontSectionHead).boldened());
    g.drawText("RESAMPLE", 12, 6, 100, 18, juce::Justification::centredLeft);

    auto historyArea = historyBounds;

    g.setColour(juce::Colour(Theme::bgControl));
    g.fillRoundedRectangle(historyArea.toFloat(), Theme::cornerRadius);
    g.setColour(juce::Colour(Theme::borderSubtle));
    g.drawRoundedRectangle(historyArea.toFloat().reduced(0.5f), Theme::cornerRadius, 1.0f);

    if (visibleThumbnails == 0)
    {
        g.setColour(juce::Colour(Theme::textDim));
        g.setFont(juce::Font(Theme::fontValue));
        g.drawText("Resample to start building history",
                   historyArea.reduced(14).toFloat().toNearestInt(),
                   juce::Justification::centred);
    }

    if (captureProgress > 0.0f && captureProgress < 1.0f)
    {
        auto progressArea = historyArea.removeFromBottom(4).reduced(14, 0).toFloat();
        g.setColour(juce::Colour(Theme::border));
        g.fillRoundedRectangle(progressArea, 2.0f);
        g.setColour(juce::Colour(Theme::accentGreen));
        g.fillRoundedRectangle(progressArea.withWidth(progressArea.getWidth() * captureProgress), 2.0f);
    }
}

void ResamplePanel::resized()
{
    auto area = getLocalBounds().reduced(Theme::panelPadding);
    area.removeFromTop(24);

    auto controlRow = area.removeFromTop(28);
    resampleButton.setBounds(controlRow.removeFromLeft(98));
    controlRow.removeFromLeft(Theme::controlGap);
    captureLengthLabel.setBounds(controlRow.removeFromLeft(46));
    controlRow.removeFromLeft(4);
    captureLengthSlider.setBounds(controlRow.removeFromLeft(160));
    controlRow.removeFromLeft(Theme::sectionGap);
    bitDepthLabel.setBounds(controlRow.removeFromLeft(48));
    controlRow.removeFromLeft(4);
    bitDepthSelector.setBounds(controlRow.removeFromLeft(92));
    progressLabel.setBounds(controlRow);

    area.removeFromTop(Theme::sectionGap);
    historyLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(4);

    auto actionsRow = area.removeFromBottom(28);
    undoButton.setBounds(actionsRow.removeFromLeft(72));
    clearButton.setBounds(actionsRow.removeFromRight(72));

    area.removeFromBottom(Theme::controlGap);
    historyBounds = area;

    const int gap = Theme::controlGap;
    const int thumbCount = 8;
    const int thumbWidth = (historyBounds.getWidth() - gap * (thumbCount - 1) - 20) / thumbCount;
    const int thumbHeight = historyBounds.getHeight() - 20;
    auto thumbArea = historyBounds.reduced(10, 10);

    for (int index = 0; index < 8; ++index)
    {
        const int x = thumbArea.getX() + index * (thumbWidth + gap);
        thumbnails[static_cast<size_t>(index)].setBounds(x, thumbArea.getY(), thumbWidth, thumbHeight);
    }
}

void ResamplePanel::updateFromEngine(ResampleEngine& engine)
{
    const auto count = engine.getHistorySize();
    const auto current = engine.getCurrentIndex();
    visibleThumbnails = count;

    for (int index = 0; index < 8; ++index)
    {
        if (index < count)
        {
            thumbnails[static_cast<size_t>(index)].setEntry(engine.getHistoryEntry(index), index, index == current);
            thumbnails[static_cast<size_t>(index)].setVisible(true);
        }
        else
        {
            thumbnails[static_cast<size_t>(index)].setEntry(nullptr, -1, false);
            thumbnails[static_cast<size_t>(index)].setVisible(false);
        }
    }

    undoButton.setEnabled(current > 0);
    clearButton.setEnabled(count > 0);
    repaint();
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
