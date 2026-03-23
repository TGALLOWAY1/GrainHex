#include "UI/ModulationPanel.h"

namespace grainhex {

ModulationPanel::ModulationPanel()
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, double min, double max, double def, double step)
    {
        addAndMakeVisible(s);
        s.setRange(min, max, step);
        s.setValue(def, juce::dontSendNotification);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 16);
        s.onValueChange = [this] { parameterChanged(); };
        addAndMakeVisible(l);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(11.0f));
        l.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    };

    // -- LFO section --
    addAndMakeVisible(lfoToggle);
    lfoToggle.setToggleState(false, juce::dontSendNotification);
    lfoToggle.onClick = [this] { parameterChanged(); };

    addAndMakeVisible(lfoShapeBox);
    lfoShapeBox.addItem("Sine", 1);
    lfoShapeBox.addItem("Triangle", 2);
    lfoShapeBox.addItem("Square", 3);
    lfoShapeBox.addItem("S&H", 4);
    lfoShapeBox.setSelectedId(1, juce::dontSendNotification);
    lfoShapeBox.onChange = [this] { parameterChanged(); };
    addAndMakeVisible(lfoShapeLabel);

    setupSlider(lfoRateSlider, lfoRateLabel, 0.01, 30.0, 1.0, 0.01);
    lfoRateSlider.setSkewFactorFromMidPoint(3.0);

    setupSlider(lfoDepthSlider, lfoDepthLabel, 0.0, 1.0, 0.5, 0.01);

    // -- ADSR section --
    addAndMakeVisible(envToggle);
    envToggle.setToggleState(false, juce::dontSendNotification);
    envToggle.onClick = [this] { parameterChanged(); };

    setupSlider(attackSlider, attackLabel, 0.001, 10.0, 0.01, 0.001);
    attackSlider.setSkewFactorFromMidPoint(0.5);

    setupSlider(decaySlider, decayLabel, 0.001, 10.0, 0.1, 0.001);
    decaySlider.setSkewFactorFromMidPoint(0.5);

    setupSlider(sustainSlider, sustainLabel, 0.0, 1.0, 0.7, 0.01);

    setupSlider(releaseSlider, releaseLabel, 0.001, 10.0, 0.3, 0.001);
    releaseSlider.setSkewFactorFromMidPoint(0.5);
}

void ModulationPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::bgPanel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius);

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);

    g.setColour(juce::Colour(Theme::accentRed));
    g.setFont(juce::Font(Theme::fontSectionHead).boldened());
    g.drawText("MODULATION", 10, 4, 120, 20, juce::Justification::centredLeft);

    // Separator
    int midY = getHeight() / 2 + 8;
    g.setColour(juce::Colour(Theme::border));
    g.drawHorizontalLine(midY, 8.0f, static_cast<float>(getWidth() - 8));
}

void ModulationPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(22); // Title

    int halfHeight = area.getHeight() / 2 - 2;

    // LFO section (top)
    auto lfoArea = area.removeFromTop(halfHeight).reduced(2);
    {
        auto topRow = lfoArea.removeFromTop(24);
        lfoToggle.setBounds(topRow.removeFromLeft(50));
        lfoShapeLabel.setBounds(topRow.removeFromLeft(35));
        lfoShapeBox.setBounds(topRow);

        lfoArea.removeFromTop(4);
        auto knobRow = lfoArea;
        int knobW = knobRow.getWidth() / 2;

        auto rateArea = knobRow.removeFromLeft(knobW);
        lfoRateLabel.setBounds(rateArea.removeFromTop(16));
        lfoRateSlider.setBounds(rateArea);

        auto depthArea = knobRow;
        lfoDepthLabel.setBounds(depthArea.removeFromTop(16));
        lfoDepthSlider.setBounds(depthArea);
    }

    area.removeFromTop(4);

    // ADSR section (bottom)
    auto envArea = area.reduced(2);
    {
        auto topRow = envArea.removeFromTop(24);
        envToggle.setBounds(topRow.removeFromLeft(90));

        envArea.removeFromTop(4);
        auto knobRow = envArea;
        int knobW = knobRow.getWidth() / 4;

        auto aArea = knobRow.removeFromLeft(knobW);
        attackLabel.setBounds(aArea.removeFromTop(16));
        attackSlider.setBounds(aArea);

        auto dArea = knobRow.removeFromLeft(knobW);
        decayLabel.setBounds(dArea.removeFromTop(16));
        decaySlider.setBounds(dArea);

        auto sArea = knobRow.removeFromLeft(knobW);
        sustainLabel.setBounds(sArea.removeFromTop(16));
        sustainSlider.setBounds(sArea);

        auto rArea = knobRow;
        releaseLabel.setBounds(rArea.removeFromTop(16));
        releaseSlider.setBounds(rArea);
    }
}

void ModulationPanel::parameterChanged()
{
    if (onParameterChanged)
        onParameterChanged();
}

bool ModulationPanel::getLFOEnabled() const { return lfoToggle.getToggleState(); }

LFOShape ModulationPanel::getLFOShape() const
{
    switch (lfoShapeBox.getSelectedId())
    {
        case 2:  return LFOShape::Triangle;
        case 3:  return LFOShape::Square;
        case 4:  return LFOShape::SampleAndHold;
        default: return LFOShape::Sine;
    }
}

float ModulationPanel::getLFORate() const { return static_cast<float>(lfoRateSlider.getValue()); }
float ModulationPanel::getLFODepth() const { return static_cast<float>(lfoDepthSlider.getValue()); }

bool ModulationPanel::getEnvelopeEnabled() const { return envToggle.getToggleState(); }
float ModulationPanel::getAttack() const { return static_cast<float>(attackSlider.getValue()); }
float ModulationPanel::getDecay() const { return static_cast<float>(decaySlider.getValue()); }
float ModulationPanel::getSustain() const { return static_cast<float>(sustainSlider.getValue()); }
float ModulationPanel::getRelease() const { return static_cast<float>(releaseSlider.getValue()); }

} // namespace grainhex
