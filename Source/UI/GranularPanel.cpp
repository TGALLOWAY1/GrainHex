#include "UI/GranularPanel.h"

namespace grainhex {

static void styleRotarySlider(juce::Slider& slider, double min, double max, double defaultVal, double interval = 0.0)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    slider.setRange(min, max, interval);
    slider.setValue(defaultVal, juce::dontSendNotification);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(Theme::accentGreen));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(Theme::border));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(Theme::textNormal));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

static void styleLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(Theme::textNormal));
    label.setFont(juce::Font(Theme::fontLabel));
}

static void styleComboBox(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(Theme::bgControl));
    box.setColour(juce::ComboBox::textColourId, juce::Colour(Theme::textNormal));
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(Theme::border));
}

GranularPanel::GranularPanel()
{
    // Grain Size: 1-500ms, default 80ms
    styleRotarySlider(grainSizeSlider, 1.0, 500.0, 80.0, 1.0);
    grainSizeSlider.setTextValueSuffix(" ms");
    grainSizeSlider.setSkewFactorFromMidPoint(80.0);
    addAndMakeVisible(grainSizeSlider);
    grainSizeSlider.onValueChange = [this] { parameterChanged(); };

    // Grain Count: 1-128, default 8
    styleRotarySlider(grainCountSlider, 1.0, 128.0, 8.0, 1.0);
    addAndMakeVisible(grainCountSlider);
    grainCountSlider.onValueChange = [this] { parameterChanged(); };

    // Position: 0-1, default 0.5
    styleRotarySlider(positionSlider, 0.0, 1.0, 0.5, 0.001);
    addAndMakeVisible(positionSlider);
    positionSlider.onValueChange = [this] { parameterChanged(); };

    // Spray: 0-1, default 0
    styleRotarySlider(spraySlider, 0.0, 1.0, 0.0, 0.001);
    addAndMakeVisible(spraySlider);
    spraySlider.onValueChange = [this] { parameterChanged(); };

    // Pitch: -24 to +24 semitones, default 0
    styleRotarySlider(pitchSlider, -24.0, 24.0, 0.0, 0.01);
    pitchSlider.setTextValueSuffix(" st");
    addAndMakeVisible(pitchSlider);
    pitchSlider.onValueChange = [this] { parameterChanged(); };

    // Spread: 0-1, default 0
    styleRotarySlider(spreadSlider, 0.0, 1.0, 0.0, 0.001);
    addAndMakeVisible(spreadSlider);
    spreadSlider.onValueChange = [this] { parameterChanged(); };

    // Pitch Quantize combo
    pitchQuantizeBox.addItem("Off", 1);
    pitchQuantizeBox.addItem("Chromatic", 2);
    pitchQuantizeBox.addItem("Major", 3);
    pitchQuantizeBox.addItem("Minor", 4);
    pitchQuantizeBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(pitchQuantizeBox);
    addAndMakeVisible(pitchQuantizeBox);
    pitchQuantizeBox.onChange = [this] { parameterChanged(); };

    // Window Shape combo
    windowShapeBox.addItem("Hann", 1);
    windowShapeBox.addItem("Triangle", 2);
    windowShapeBox.addItem("Trapezoid", 3);
    windowShapeBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(windowShapeBox);
    addAndMakeVisible(windowShapeBox);
    windowShapeBox.onChange = [this] { parameterChanged(); };

    // Direction combo
    directionBox.addItem("Forward", 1);
    directionBox.addItem("Reverse", 2);
    directionBox.addItem("Random", 3);
    directionBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox(directionBox);
    addAndMakeVisible(directionBox);
    directionBox.onChange = [this] { parameterChanged(); };

    // Labels
    for (auto* label : { &grainSizeLabel, &grainCountLabel, &positionLabel,
                          &sprayLabel, &pitchLabel, &spreadLabel,
                          &pitchQuantizeLabel, &windowShapeLabel, &directionLabel })
    {
        styleLabel(*label);
        addAndMakeVisible(label);
    }
}

void GranularPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::bgPanel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius);

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);

    g.setColour(juce::Colour(Theme::accentGreen));
    g.setFont(juce::Font(Theme::fontSectionHead).boldened());
    g.drawText("GRANULAR", 10, 4, 120, 20, juce::Justification::centredLeft);
}

void GranularPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(22); // Header space

    const int knobW = 75;
    const int knobH = 75;
    const int labelH = 16;
    const int comboH = 24;
    const int comboW = 90;
    const int gap = 4;

    // Row 1: Size, Count, Position, Spray, Pitch, Spread (6 knobs)
    auto row1 = area.removeFromTop(knobH + labelH + gap);
    int numKnobs = 6;
    int totalKnobWidth = numKnobs * knobW;
    int spacing = (row1.getWidth() - totalKnobWidth) / (numKnobs - 1);
    spacing = std::max(spacing, 2);

    struct KnobPair { juce::Slider* slider; juce::Label* label; };
    KnobPair knobs[] = {
        { &grainSizeSlider, &grainSizeLabel },
        { &grainCountSlider, &grainCountLabel },
        { &positionSlider, &positionLabel },
        { &spraySlider, &sprayLabel },
        { &pitchSlider, &pitchLabel },
        { &spreadSlider, &spreadLabel }
    };

    int xPos = row1.getX();
    for (auto& kp : knobs)
    {
        kp.label->setBounds(xPos, row1.getY(), knobW, labelH);
        kp.slider->setBounds(xPos, row1.getY() + labelH, knobW, knobH);
        xPos += knobW + spacing;
    }

    area.removeFromTop(gap);

    // Row 2: Combo boxes (Quantize, Window, Direction)
    auto row2 = area.removeFromTop(comboH + labelH + gap);

    struct ComboPair { juce::ComboBox* box; juce::Label* label; };
    ComboPair combos[] = {
        { &pitchQuantizeBox, &pitchQuantizeLabel },
        { &windowShapeBox, &windowShapeLabel },
        { &directionBox, &directionLabel }
    };

    int comboSpacing = (row2.getWidth() - 3 * comboW) / 4;
    comboSpacing = std::max(comboSpacing, 8);
    int cx = row2.getX() + comboSpacing;

    for (auto& cp : combos)
    {
        cp.label->setBounds(cx, row2.getY(), comboW, labelH);
        cp.box->setBounds(cx, row2.getY() + labelH, comboW, comboH);
        cx += comboW + comboSpacing;
    }
}

void GranularPanel::parameterChanged()
{
    if (onParameterChanged)
        onParameterChanged();
}

float GranularPanel::getGrainSize() const { return static_cast<float>(grainSizeSlider.getValue()); }
int GranularPanel::getGrainCount() const { return static_cast<int>(grainCountSlider.getValue()); }
float GranularPanel::getPosition() const { return static_cast<float>(positionSlider.getValue()); }
float GranularPanel::getSpray() const { return static_cast<float>(spraySlider.getValue()); }
float GranularPanel::getPitchSemitones() const { return static_cast<float>(pitchSlider.getValue()); }
float GranularPanel::getSpread() const { return static_cast<float>(spreadSlider.getValue()); }

PitchQuantizeScale GranularPanel::getPitchQuantize() const
{
    switch (pitchQuantizeBox.getSelectedId())
    {
        case 2: return PitchQuantizeScale::Chromatic;
        case 3: return PitchQuantizeScale::Major;
        case 4: return PitchQuantizeScale::Minor;
        default: return PitchQuantizeScale::Off;
    }
}

WindowShape GranularPanel::getWindowShape() const
{
    switch (windowShapeBox.getSelectedId())
    {
        case 2: return WindowShape::Triangle;
        case 3: return WindowShape::Trapezoid;
        default: return WindowShape::Hann;
    }
}

DirectionMode GranularPanel::getDirection() const
{
    switch (directionBox.getSelectedId())
    {
        case 2: return DirectionMode::Reverse;
        case 3: return DirectionMode::Random;
        default: return DirectionMode::Forward;
    }
}

} // namespace grainhex
