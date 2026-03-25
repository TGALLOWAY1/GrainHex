#include "UI/SubPanel.h"

namespace grainhex {

static void styleRotarySlider(juce::Slider& slider, double min, double max, double defaultVal, double interval = 0.0)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    slider.setRange(min, max, interval);
    slider.setValue(defaultVal, juce::dontSendNotification);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(Theme::accentPurple));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(Theme::border));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(Theme::textNormal));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

static void styleLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colour(Theme::textNormal));
    label.setFont(juce::Font(Theme::fontLabel));
}

static void styleComboBox(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(Theme::bgControl));
    box.setColour(juce::ComboBox::textColourId, juce::Colour(Theme::textNormal));
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(Theme::border));
}

SubPanel::SubPanel()
{
    addAndMakeVisible(enableToggle);
    enableToggle.setButtonText("Enabled");
    enableToggle.setToggleState(false, juce::dontSendNotification);
    enableToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(Theme::accentPurple));
    enableToggle.onClick = [this]
    {
        updateContentAlpha();
        parameterChanged();
        repaint();
    };

    styleRotarySlider(levelSlider, 0.0, 1.0, 0.7, 0.01);
    addAndMakeVisible(levelSlider);
    levelSlider.onValueChange = [this] { parameterChanged(); };

    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Triangle", 2);
    waveformBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(waveformBox);
    addAndMakeVisible(waveformBox);
    waveformBox.onChange = [this] { parameterChanged(); };

    modeBox.addItem("Auto", 1);
    modeBox.addItem("Manual", 2);
    modeBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(modeBox);
    addAndMakeVisible(modeBox);
    modeBox.onChange = [this]
    {
        updateModeVisibility();
        resized();
        parameterChanged();
    };

    snapModeBox.addItem("Strict", 1);
    snapModeBox.addItem("Loose", 2);
    snapModeBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(snapModeBox);
    addAndMakeVisible(snapModeBox);
    snapModeBox.onChange = [this] { parameterChanged(); };

    smoothingBox.addItem("Slow", 1);
    smoothingBox.addItem("Medium", 2);
    smoothingBox.addItem("Fast", 3);
    smoothingBox.setSelectedId(2, juce::dontSendNotification);
    styleComboBox(smoothingBox);
    addAndMakeVisible(smoothingBox);
    smoothingBox.onChange = [this] { parameterChanged(); };

    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int itemId = 1;
    for (int octave = 0; octave <= 3; ++octave)
        for (int note = 0; note < 12; ++note)
            noteBox.addItem(juce::String(noteNames[note]) + juce::String(octave), itemId++);
    noteBox.setSelectedId(25, juce::dontSendNotification);
    styleComboBox(noteBox);
    addAndMakeVisible(noteBox);
    noteBox.onChange = [this] { parameterChanged(); };

    octaveOffsetBox.addItem("-2", 1);
    octaveOffsetBox.addItem("-1", 2);
    octaveOffsetBox.addItem("0", 3);
    octaveOffsetBox.setSelectedId(2, juce::dontSendNotification);
    styleComboBox(octaveOffsetBox);
    addAndMakeVisible(octaveOffsetBox);
    octaveOffsetBox.onChange = [this] { parameterChanged(); };

    styleRotarySlider(granularHPSlider, 20.0, 300.0, 100.0, 1.0);
    granularHPSlider.setTextValueSuffix(" Hz");
    granularHPSlider.setSkewFactorFromMidPoint(100.0);
    addAndMakeVisible(granularHPSlider);
    granularHPSlider.onValueChange = [this] { parameterChanged(); };

    styleRotarySlider(subLPSlider, 40.0, 400.0, 200.0, 1.0);
    subLPSlider.setTextValueSuffix(" Hz");
    subLPSlider.setSkewFactorFromMidPoint(200.0);
    addAndMakeVisible(subLPSlider);
    subLPSlider.onValueChange = [this] { parameterChanged(); };

    granularHPLabel.setText("Grain Highpass", juce::dontSendNotification);
    subLPLabel.setText("Sub Lowpass", juce::dontSendNotification);

    for (auto* label : { &levelLabel, &waveformLabel, &modeLabel, &snapModeLabel,
                          &smoothingLabel, &noteLabel, &octaveOffsetLabel,
                          &granularHPLabel, &subLPLabel })
    {
        styleLabel(*label);
        addAndMakeVisible(label);
    }

    addAndMakeVisible(pitchNoteLabel);
    pitchNoteLabel.setJustificationType(juce::Justification::centred);
    pitchNoteLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::accentPurple));
    pitchNoteLabel.setFont(juce::Font(Theme::fontTitle + 12.0f).boldened());
    pitchNoteLabel.setText("---", juce::dontSendNotification);

    updateModeVisibility();
    updateContentAlpha();
}

void SubPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::bgPanel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius);

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);

    g.setColour(juce::Colour(Theme::accentPurple));
    g.setFont(juce::Font(Theme::fontSectionHead).boldened());
    g.drawText("SUB TUNER", 12, 6, 120, 18, juce::Justification::centredLeft);

    auto drawGroup = [&](juce::Rectangle<int> bounds, const juce::String& title)
    {
        g.setColour(juce::Colour(Theme::bgControl));
        g.fillRoundedRectangle(bounds.toFloat(), Theme::cornerRadius);
        g.setColour(juce::Colour(Theme::borderSubtle));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), Theme::cornerRadius, 1.0f);
        g.setColour(juce::Colour(Theme::textDim));
        g.setFont(juce::Font(Theme::fontSmall).boldened());
        g.drawText(title, bounds.getX() + 10, bounds.getY() + 8, bounds.getWidth() - 20, 14,
                   juce::Justification::centredLeft);
    };

    const auto contentAlpha = enableToggle.getToggleState() ? 1.0f : 0.4f;
    juce::Graphics::ScopedSaveState savedState(g);
    g.setOpacity(contentAlpha);

    drawGroup(pitchDisplayBounds, "Pitch Display");
    drawGroup(levelGroupBounds, "Level + Wave");
    drawGroup(modeGroupBounds, "Pitch + Mode");
    drawGroup(crossoverGroupBounds, "Crossover");

    auto display = pitchDisplayBounds.reduced(16, 18);
    display.removeFromTop(16);

    auto noteCard = display.removeFromTop(78).withSizeKeepingCentre(120, 72);
    g.setColour(juce::Colour(Theme::bgDarkest));
    g.fillRoundedRectangle(noteCard.toFloat(), 10.0f);
    g.setColour(juce::Colour(Theme::borderActive));
    g.drawRoundedRectangle(noteCard.toFloat().reduced(0.5f), 10.0f, 1.0f);

    auto centsText = (pitchConfidence > 0.0f ? juce::String(pitchCents >= 0.0f ? "+" : "")
                                               + juce::String(static_cast<int>(std::round(pitchCents))) + "ct"
                                             : juce::String("--"));
    g.setColour(juce::Colour(Theme::textDim));
    g.setFont(juce::Font(Theme::fontValue));
    g.drawText(centsText, noteCard.getX(), noteCard.getBottom() - 22, noteCard.getWidth(), 16,
               juce::Justification::centred);

    auto centsBar = display.removeFromTop(18);
    const auto barBounds = centsBar.withTrimmedLeft(8).withTrimmedRight(8).withHeight(8);
    g.setColour(juce::Colour(Theme::bgDarkest));
    g.fillRoundedRectangle(barBounds.toFloat(), 4.0f);
    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(barBounds.toFloat(), 4.0f, 1.0f);
    g.setColour(juce::Colour(Theme::textMuted));
    g.drawLine(static_cast<float>(barBounds.getCentreX()), static_cast<float>(barBounds.getY()),
               static_cast<float>(barBounds.getCentreX()), static_cast<float>(barBounds.getBottom()), 1.0f);

    if (pitchConfidence > 0.3f)
    {
        const auto centsNorm = juce::jlimit(-50.0f, 50.0f, pitchCents) / 50.0f;
        const auto indicatorX = static_cast<float>(barBounds.getCentreX()) + centsNorm * (static_cast<float>(barBounds.getWidth()) * 0.5f - 6.0f);
        g.setColour(juce::Colour(Theme::accentPurple));
        g.fillRoundedRectangle(indicatorX - 4.0f, static_cast<float>(barBounds.getY()) + 1.0f, 8.0f,
                               static_cast<float>(barBounds.getHeight()) - 2.0f, 4.0f);
    }

    auto confidenceBounds = display.withTrimmedTop(8).removeFromTop(16);
    g.setColour(juce::Colour(Theme::textDim));
    g.setFont(juce::Font(Theme::fontMetadata));
    g.drawText("Confidence", confidenceBounds.removeFromTop(10), juce::Justification::centredLeft);
    auto confidenceBar = confidenceBounds.removeFromTop(5).withWidth(juce::jmax(12, confidenceBounds.getWidth() - 10));
    g.setColour(juce::Colour(Theme::bgDarkest));
    g.fillRoundedRectangle(confidenceBar.toFloat(), 2.5f);
    g.setColour(juce::Colour(Theme::accentGreen).withAlpha(0.85f));
    g.fillRoundedRectangle(confidenceBar.withWidth(static_cast<int>(static_cast<float>(confidenceBar.getWidth()) * pitchConfidence)).toFloat(), 2.5f);
    g.setColour(juce::Colour(Theme::textBright));
    g.drawText(juce::String(static_cast<int>(pitchConfidence * 100.0f)) + "%", confidenceBar.translated(0, 6),
               juce::Justification::centredLeft);
}

void SubPanel::resized()
{
    auto area = getLocalBounds().reduced(Theme::panelPadding);
    area.removeFromTop(24);

    auto toggleRow = area.removeFromTop(20);
    enableToggle.setBounds(toggleRow.removeFromLeft(86));
    area.removeFromTop(Theme::sectionGap);

    auto leftColumn = area.removeFromLeft(static_cast<int>(area.getWidth() * 0.42f));
    area.removeFromLeft(Theme::sectionGap);
    auto rightColumn = area;

    pitchDisplayBounds = leftColumn;
    auto displayLayout = pitchDisplayBounds.reduced(16, 18);
    displayLayout.removeFromTop(16);
    auto noteCard = displayLayout.removeFromTop(78).withSizeKeepingCentre(120, 72);
    pitchNoteLabel.setBounds(noteCard.withTrimmedBottom(18));

    auto topRight = rightColumn.removeFromTop(static_cast<int>(rightColumn.getHeight() * 0.58f));
    levelGroupBounds = topRight.removeFromLeft(static_cast<int>(topRight.getWidth() * 0.38f));
    topRight.removeFromLeft(Theme::sectionGap);
    modeGroupBounds = topRight;
    rightColumn.removeFromTop(Theme::sectionGap);
    crossoverGroupBounds = rightColumn;

    const int labelH = 14;
    const int comboH = 24;
    const int knobSliderH = Theme::knobSize + 18;

    {
        auto levelArea = levelGroupBounds.reduced(10, 10);
        levelArea.removeFromTop(18);
        levelLabel.setBounds(levelArea.removeFromTop(labelH));
        levelSlider.setBounds(levelArea.removeFromTop(knobSliderH));
        levelArea.removeFromTop(2);
        waveformLabel.setBounds(levelArea.removeFromTop(labelH));
        waveformBox.setBounds(levelArea.removeFromTop(comboH));
    }

    {
        auto modeArea = modeGroupBounds.reduced(10, 10);
        modeArea.removeFromTop(18);
        modeLabel.setBounds(modeArea.removeFromTop(labelH));
        modeBox.setBounds(modeArea.removeFromTop(comboH));
        modeArea.removeFromTop(6);

        auto upperRow = modeArea.removeFromTop(labelH + comboH + 4);
        auto lowerRow = modeArea.removeFromTop(labelH + comboH + 4);
        const int halfWidth = (upperRow.getWidth() - Theme::controlGap) / 2;

        if (modeBox.getSelectedId() == 1)
        {
            auto snapArea = upperRow.removeFromLeft(halfWidth);
            upperRow.removeFromLeft(Theme::controlGap);
            auto smoothArea = upperRow;

            snapModeLabel.setBounds(snapArea.removeFromTop(labelH));
            snapModeBox.setBounds(snapArea.removeFromTop(comboH));
            smoothingLabel.setBounds(smoothArea.removeFromTop(labelH));
            smoothingBox.setBounds(smoothArea.removeFromTop(comboH));

            noteLabel.setBounds(juce::Rectangle<int>());
            noteBox.setBounds(juce::Rectangle<int>());
            octaveOffsetLabel.setBounds(juce::Rectangle<int>());
            octaveOffsetBox.setBounds(juce::Rectangle<int>());
        }
        else
        {
            auto noteArea = upperRow.removeFromLeft(halfWidth);
            upperRow.removeFromLeft(Theme::controlGap);
            auto octaveArea = upperRow;

            noteLabel.setBounds(noteArea.removeFromTop(labelH));
            noteBox.setBounds(noteArea.removeFromTop(comboH));
            octaveOffsetLabel.setBounds(octaveArea.removeFromTop(labelH));
            octaveOffsetBox.setBounds(octaveArea.removeFromTop(comboH));

            snapModeLabel.setBounds(juce::Rectangle<int>());
            snapModeBox.setBounds(juce::Rectangle<int>());
            smoothingLabel.setBounds(juce::Rectangle<int>());
            smoothingBox.setBounds(juce::Rectangle<int>());
        }
    }

    {
        auto crossoverArea = crossoverGroupBounds.reduced(10, 10);
        crossoverArea.removeFromTop(18);
        const int knobWidth = (crossoverArea.getWidth() - Theme::controlGap) / 2;

        auto hpArea = crossoverArea.removeFromLeft(knobWidth);
        crossoverArea.removeFromLeft(Theme::controlGap);
        auto lpArea = crossoverArea;

        granularHPLabel.setBounds(hpArea.removeFromTop(labelH));
        granularHPSlider.setBounds(hpArea.removeFromTop(knobSliderH));

        subLPLabel.setBounds(lpArea.removeFromTop(labelH));
        subLPSlider.setBounds(lpArea.removeFromTop(knobSliderH));
    }
}

void SubPanel::updateModeVisibility()
{
    const bool isAuto = (modeBox.getSelectedId() == 1);
    snapModeBox.setVisible(isAuto);
    snapModeLabel.setVisible(isAuto);
    smoothingBox.setVisible(isAuto);
    smoothingLabel.setVisible(isAuto);
    noteBox.setVisible(!isAuto);
    noteLabel.setVisible(!isAuto);
    octaveOffsetBox.setVisible(!isAuto);
    octaveOffsetLabel.setVisible(!isAuto);
}

void SubPanel::updateContentAlpha()
{
    const auto alpha = enableToggle.getToggleState() ? 1.0f : 0.4f;
    for (auto* component : { static_cast<juce::Component*>(&levelSlider),
                             static_cast<juce::Component*>(&levelLabel),
                             static_cast<juce::Component*>(&waveformBox),
                             static_cast<juce::Component*>(&waveformLabel),
                             static_cast<juce::Component*>(&modeBox),
                             static_cast<juce::Component*>(&modeLabel),
                             static_cast<juce::Component*>(&snapModeBox),
                             static_cast<juce::Component*>(&snapModeLabel),
                             static_cast<juce::Component*>(&smoothingBox),
                             static_cast<juce::Component*>(&smoothingLabel),
                             static_cast<juce::Component*>(&noteBox),
                             static_cast<juce::Component*>(&noteLabel),
                             static_cast<juce::Component*>(&octaveOffsetBox),
                             static_cast<juce::Component*>(&octaveOffsetLabel),
                             static_cast<juce::Component*>(&granularHPSlider),
                             static_cast<juce::Component*>(&granularHPLabel),
                             static_cast<juce::Component*>(&subLPSlider),
                             static_cast<juce::Component*>(&subLPLabel),
                             static_cast<juce::Component*>(&pitchNoteLabel) })
    {
        component->setAlpha(alpha);
    }
}

void SubPanel::parameterChanged()
{
    if (onParameterChanged)
        onParameterChanged();
}

void SubPanel::setDetectedPitch(const PitchInfo& pitch)
{
    pitchNoteLabel.setText(pitch.getNoteName(), juce::dontSendNotification);
    pitchCents = pitch.centsOffset;
    pitchConfidence = pitch.confidence;
    repaint();
}

bool SubPanel::getEnabled() const { return enableToggle.getToggleState(); }
float SubPanel::getLevel() const { return static_cast<float>(levelSlider.getValue()); }

SubWaveform SubPanel::getWaveform() const
{
    return waveformBox.getSelectedId() == 2 ? SubWaveform::Triangle : SubWaveform::Sine;
}

SubTuningMode SubPanel::getTuningMode() const
{
    return modeBox.getSelectedId() == 2 ? SubTuningMode::Manual : SubTuningMode::Auto;
}

PitchSnapMode SubPanel::getPitchSnapMode() const
{
    return snapModeBox.getSelectedId() == 2 ? PitchSnapMode::Loose : PitchSnapMode::Strict;
}

SmoothingSpeed SubPanel::getSmoothingSpeed() const
{
    switch (smoothingBox.getSelectedId())
    {
        case 1: return SmoothingSpeed::Slow;
        case 3: return SmoothingSpeed::Fast;
        default: return SmoothingSpeed::Medium;
    }
}

int SubPanel::getOctaveOffset() const
{
    switch (octaveOffsetBox.getSelectedId())
    {
        case 1: return -2;
        case 3: return 0;
        default: return -1;
    }
}

int SubPanel::getManualMidiNote() const
{
    const auto selectedId = noteBox.getSelectedId();
    if (selectedId < 1)
        return 36;
    return 11 + selectedId;
}

float SubPanel::getGranularHPFreq() const { return static_cast<float>(granularHPSlider.getValue()); }
float SubPanel::getSubLPFreq() const { return static_cast<float>(subLPSlider.getValue()); }

} // namespace grainhex
