#include "UI/SubPanel.h"

namespace grainhex {

static void styleRotarySlider(juce::Slider& slider, double min, double max, double defaultVal, double interval = 0.0)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    slider.setRange(min, max, interval);
    slider.setValue(defaultVal, juce::dontSendNotification);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffcc66ff)); // Purple for sub
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333355));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::lightgrey);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

static void styleLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    label.setFont(juce::Font(11.0f));
}

static void styleComboBox(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a2e));
    box.setColour(juce::ComboBox::textColourId, juce::Colours::lightgrey);
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff333355));
}

SubPanel::SubPanel()
{
    // Enable toggle
    addAndMakeVisible(enableToggle);
    enableToggle.setToggleState(false, juce::dontSendNotification);
    enableToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffcc66ff));
    enableToggle.onClick = [this] { parameterChanged(); };

    // Level: 0-1, default 0.7
    styleRotarySlider(levelSlider, 0.0, 1.0, 0.7, 0.01);
    addAndMakeVisible(levelSlider);
    levelSlider.onValueChange = [this] { parameterChanged(); };

    // Waveform
    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Triangle", 2);
    waveformBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(waveformBox);
    addAndMakeVisible(waveformBox);
    waveformBox.onChange = [this] { parameterChanged(); };

    // Mode (Auto/Manual)
    modeBox.addItem("Auto", 1);
    modeBox.addItem("Manual", 2);
    modeBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(modeBox);
    addAndMakeVisible(modeBox);
    modeBox.onChange = [this] { updateModeVisibility(); parameterChanged(); };

    // Pitch snap mode (Strict/Loose) — Auto mode only
    snapModeBox.addItem("Strict", 1);
    snapModeBox.addItem("Loose", 2);
    snapModeBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(snapModeBox);
    addAndMakeVisible(snapModeBox);
    snapModeBox.onChange = [this] { parameterChanged(); };

    // Smoothing speed — Auto mode only
    smoothingBox.addItem("Slow", 1);
    smoothingBox.addItem("Medium", 2);
    smoothingBox.addItem("Fast", 3);
    smoothingBox.setSelectedId(2, juce::dontSendNotification);
    styleComboBox(smoothingBox);
    addAndMakeVisible(smoothingBox);
    smoothingBox.onChange = [this] { parameterChanged(); };

    // Note selector (C0-B3) — Manual mode only
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int itemId = 1;
    for (int octave = 0; octave <= 3; ++octave)
    {
        for (int note = 0; note < 12; ++note)
        {
            juce::String name = juce::String(noteNames[note]) + juce::String(octave);
            noteBox.addItem(name, itemId++);
        }
    }
    noteBox.setSelectedId(25, juce::dontSendNotification); // C2 default (index 25 = MIDI 36)
    styleComboBox(noteBox);
    addAndMakeVisible(noteBox);
    noteBox.onChange = [this] { parameterChanged(); };

    // Octave offset
    octaveOffsetBox.addItem("-2", 1);
    octaveOffsetBox.addItem("-1", 2);
    octaveOffsetBox.addItem("0", 3);
    octaveOffsetBox.setSelectedId(2, juce::dontSendNotification); // Default -1
    styleComboBox(octaveOffsetBox);
    addAndMakeVisible(octaveOffsetBox);
    octaveOffsetBox.onChange = [this] { parameterChanged(); };

    // Granular HP filter frequency
    styleRotarySlider(granularHPSlider, 20.0, 300.0, 100.0, 1.0);
    granularHPSlider.setTextValueSuffix(" Hz");
    granularHPSlider.setSkewFactorFromMidPoint(100.0);
    addAndMakeVisible(granularHPSlider);
    granularHPSlider.onValueChange = [this] { parameterChanged(); };

    // Sub LP filter frequency
    styleRotarySlider(subLPSlider, 40.0, 400.0, 200.0, 1.0);
    subLPSlider.setTextValueSuffix(" Hz");
    subLPSlider.setSkewFactorFromMidPoint(200.0);
    addAndMakeVisible(subLPSlider);
    subLPSlider.onValueChange = [this] { parameterChanged(); };

    // Labels
    for (auto* label : { &levelLabel, &waveformLabel, &modeLabel, &snapModeLabel,
                          &smoothingLabel, &noteLabel, &octaveOffsetLabel,
                          &granularHPLabel, &subLPLabel })
    {
        styleLabel(*label);
        addAndMakeVisible(label);
    }

    // Pitch display
    addAndMakeVisible(pitchNoteLabel);
    pitchNoteLabel.setJustificationType(juce::Justification::centred);
    pitchNoteLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcc66ff));
    pitchNoteLabel.setFont(juce::Font(22.0f).boldened());
    pitchNoteLabel.setText("---", juce::dontSendNotification);

    // Initial mode visibility
    updateModeVisibility();
}

void SubPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff12122a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    g.setColour(juce::Colour(0xff333355));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

    g.setColour(juce::Colour(0xffcc66ff));
    g.setFont(juce::Font(13.0f).boldened());
    g.drawText("SUB TUNER", 10, 4, 120, 20, juce::Justification::centredLeft);

    // Draw cents bar next to pitch note label
    auto pitchArea = pitchNoteLabel.getBounds();
    int barX = pitchArea.getRight() + 8;
    int barY = pitchArea.getCentreY() - 15;
    int barW = 60;
    int barH = 30;

    // Background bar
    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillRect(barX, barY, barW, barH);
    g.setColour(juce::Colour(0xff333355));
    g.drawRect(barX, barY, barW, barH, 1);

    // Center line
    int centerX = barX + barW / 2;
    g.setColour(juce::Colours::grey);
    g.drawVerticalLine(centerX, static_cast<float>(barY), static_cast<float>(barY + barH));

    // Cents indicator (map -50..+50 cents to bar width)
    if (pitchConfidence > 0.3f)
    {
        float centsNorm = juce::jlimit(-50.0f, 50.0f, pitchCents) / 50.0f;
        int indicatorX = centerX + static_cast<int>(centsNorm * (barW / 2 - 2));
        g.setColour(juce::Colour(0xffcc66ff));
        g.fillRect(indicatorX - 1, barY + 2, 3, barH - 4);
    }

    // Confidence indicator
    g.setFont(10.0f);
    g.setColour(pitchConfidence > 0.5f ? juce::Colour(0xff16c784) : juce::Colours::grey);
    g.drawText(juce::String(static_cast<int>(pitchConfidence * 100.0f)) + "%",
               barX, barY + barH + 2, barW, 14, juce::Justification::centred);
}

void SubPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(22); // Header space

    const int knobW = 65;
    const int knobH = 65;
    const int labelH = 14;
    const int comboH = 22;
    const int comboW = 80;
    const int gap = 4;

    // Row 1: Enable toggle, Level knob, Pitch display, Gran HP, Sub LP knobs
    auto row1 = area.removeFromTop(knobH + labelH + gap);

    // Enable toggle
    enableToggle.setBounds(row1.removeFromLeft(60).withHeight(24).withY(row1.getY() + 10));
    row1.removeFromLeft(gap);

    // Level knob
    levelLabel.setBounds(row1.getX(), row1.getY(), knobW, labelH);
    levelSlider.setBounds(row1.getX(), row1.getY() + labelH, knobW, knobH);
    row1.removeFromLeft(knobW + gap);

    // Pitch display (note name + cents bar area)
    auto pitchDisplayArea = row1.removeFromLeft(140);
    pitchNoteLabel.setBounds(pitchDisplayArea.getX(), pitchDisplayArea.getY() + 5,
                             60, 40);
    // Cents bar is painted in paint()

    row1.removeFromLeft(gap);

    // Gran HP knob
    granularHPLabel.setBounds(row1.getX(), row1.getY(), knobW, labelH);
    granularHPSlider.setBounds(row1.getX(), row1.getY() + labelH, knobW, knobH);
    row1.removeFromLeft(knobW + gap);

    // Sub LP knob
    subLPLabel.setBounds(row1.getX(), row1.getY(), knobW, labelH);
    subLPSlider.setBounds(row1.getX(), row1.getY() + labelH, knobW, knobH);

    area.removeFromTop(gap);

    // Row 2: Combo boxes — Waveform, Mode, Snap/Note, Smoothing, Octave
    auto row2 = area.removeFromTop(comboH + labelH + gap);
    int cx = row2.getX();
    int comboGap = 8;

    struct ComboPair { juce::ComboBox* box; juce::Label* label; };
    ComboPair combos[] = {
        { &waveformBox, &waveformLabel },
        { &modeBox, &modeLabel },
        { &snapModeBox, &snapModeLabel },
        { &smoothingBox, &smoothingLabel },
        { &noteBox, &noteLabel },
        { &octaveOffsetBox, &octaveOffsetLabel }
    };

    for (auto& cp : combos)
    {
        cp.label->setBounds(cx, row2.getY(), comboW, labelH);
        cp.box->setBounds(cx, row2.getY() + labelH, comboW, comboH);
        cx += comboW + comboGap;
    }
}

void SubPanel::updateModeVisibility()
{
    bool isAuto = (modeBox.getSelectedId() == 1);

    // Auto-mode controls
    snapModeBox.setVisible(isAuto);
    snapModeLabel.setVisible(isAuto);
    smoothingBox.setVisible(isAuto);
    smoothingLabel.setVisible(isAuto);

    // Manual-mode controls
    noteBox.setVisible(!isAuto);
    noteLabel.setVisible(!isAuto);
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
    repaint(); // Repaint cents bar
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
    // noteBox items are 1-indexed, representing C0 (MIDI 12) through B3 (MIDI 59)
    int selectedId = noteBox.getSelectedId();
    if (selectedId < 1) return 36; // C2 default
    return 11 + selectedId; // Item 1 = C0 = MIDI 12
}

float SubPanel::getGranularHPFreq() const { return static_cast<float>(granularHPSlider.getValue()); }
float SubPanel::getSubLPFreq() const { return static_cast<float>(subLPSlider.getValue()); }

} // namespace grainhex
