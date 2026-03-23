#include "UI/EffectsPanel.h"

namespace grainhex {

EffectsPanel::EffectsPanel()
{
    // -- Distortion section --
    addAndMakeVisible(distortionToggle);
    distortionToggle.setToggleState(false, juce::dontSendNotification);
    distortionToggle.onClick = [this] { parameterChanged(); };

    addAndMakeVisible(distortionModeBox);
    distortionModeBox.addItem("Soft Clip", 1);
    distortionModeBox.addItem("Hard Clip", 2);
    distortionModeBox.addItem("Wavefold", 3);
    distortionModeBox.setSelectedId(1, juce::dontSendNotification);
    distortionModeBox.onChange = [this] { parameterChanged(); };
    addAndMakeVisible(distortionModeLabel);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, double min, double max, double def, double step)
    {
        addAndMakeVisible(s);
        s.setRange(min, max, step);
        s.setValue(def, juce::dontSendNotification);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        s.onValueChange = [this] { parameterChanged(); };
        addAndMakeVisible(l);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(11.0f));
        l.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    };

    setupSlider(driveSlider, driveLabel, 1.0, 20.0, 1.0, 0.1);
    setupSlider(distMixSlider, distMixLabel, 0.0, 1.0, 0.0, 0.01);

    // -- Filter section --
    addAndMakeVisible(filterToggle);
    filterToggle.setToggleState(false, juce::dontSendNotification);
    filterToggle.onClick = [this] { parameterChanged(); };

    addAndMakeVisible(filterModeBox);
    filterModeBox.addItem("Low Pass", 1);
    filterModeBox.addItem("High Pass", 2);
    filterModeBox.addItem("Band Pass", 3);
    filterModeBox.setSelectedId(1, juce::dontSendNotification);
    filterModeBox.onChange = [this] { parameterChanged(); };
    addAndMakeVisible(filterModeLabel);

    setupSlider(cutoffSlider, cutoffLabel, 20.0, 20000.0, 8000.0, 1.0);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);

    setupSlider(resonanceSlider, resonanceLabel, 0.0, 1.0, 0.0, 0.01);
    setupSlider(envAmountSlider, envAmountLabel, -96.0, 96.0, 48.0, 1.0);
}

void EffectsPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::bgPanel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius);

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);

    g.setColour(juce::Colour(Theme::accentOrange));
    g.setFont(juce::Font(Theme::fontSectionHead).boldened());
    g.drawText("EFFECTS", 10, 4, 100, 20, juce::Justification::centredLeft);

    // Separator between distortion and filter
    int midY = getHeight() / 2 + 8;
    g.setColour(juce::Colour(Theme::border));
    g.drawHorizontalLine(midY, 8.0f, static_cast<float>(getWidth() - 8));
}

void EffectsPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(22); // Title

    int halfHeight = area.getHeight() / 2 - 2;

    // Distortion section (top)
    auto distArea = area.removeFromTop(halfHeight).reduced(2);
    {
        auto topRow = distArea.removeFromTop(24);
        distortionToggle.setBounds(topRow.removeFromLeft(90));
        distortionModeLabel.setBounds(topRow.removeFromLeft(35));
        distortionModeBox.setBounds(topRow);

        distArea.removeFromTop(4);
        auto knobRow = distArea;
        int knobW = knobRow.getWidth() / 2;

        auto driveArea = knobRow.removeFromLeft(knobW);
        driveLabel.setBounds(driveArea.removeFromTop(16));
        driveSlider.setBounds(driveArea);

        auto mixArea = knobRow;
        distMixLabel.setBounds(mixArea.removeFromTop(16));
        distMixSlider.setBounds(mixArea);
    }

    area.removeFromTop(4);

    // Filter section (bottom)
    auto filtArea = area.reduced(2);
    {
        auto topRow = filtArea.removeFromTop(24);
        filterToggle.setBounds(topRow.removeFromLeft(60));
        filterModeLabel.setBounds(topRow.removeFromLeft(30));
        filterModeBox.setBounds(topRow);

        filtArea.removeFromTop(4);
        auto knobRow = filtArea;
        int knobW = knobRow.getWidth() / 3;

        auto cutArea = knobRow.removeFromLeft(knobW);
        cutoffLabel.setBounds(cutArea.removeFromTop(16));
        cutoffSlider.setBounds(cutArea);

        auto resArea = knobRow.removeFromLeft(knobW);
        resonanceLabel.setBounds(resArea.removeFromTop(16));
        resonanceSlider.setBounds(resArea);

        auto envArea = knobRow;
        envAmountLabel.setBounds(envArea.removeFromTop(16));
        envAmountSlider.setBounds(envArea);
    }
}

void EffectsPanel::parameterChanged()
{
    if (onParameterChanged)
        onParameterChanged();
}

bool EffectsPanel::getDistortionEnabled() const { return distortionToggle.getToggleState(); }

DistortionMode EffectsPanel::getDistortionMode() const
{
    switch (distortionModeBox.getSelectedId())
    {
        case 2:  return DistortionMode::HardClip;
        case 3:  return DistortionMode::Wavefold;
        default: return DistortionMode::SoftClip;
    }
}

float EffectsPanel::getDrive() const { return static_cast<float>(driveSlider.getValue()); }
float EffectsPanel::getDistortionMix() const { return static_cast<float>(distMixSlider.getValue()); }

bool EffectsPanel::getFilterEnabled() const { return filterToggle.getToggleState(); }

FilterMode EffectsPanel::getFilterMode() const
{
    switch (filterModeBox.getSelectedId())
    {
        case 2:  return FilterMode::HighPass;
        case 3:  return FilterMode::BandPass;
        default: return FilterMode::LowPass;
    }
}

float EffectsPanel::getCutoff() const { return static_cast<float>(cutoffSlider.getValue()); }
float EffectsPanel::getResonance() const { return static_cast<float>(resonanceSlider.getValue()); }
float EffectsPanel::getEnvelopeAmount() const { return static_cast<float>(envAmountSlider.getValue()); }

} // namespace grainhex
