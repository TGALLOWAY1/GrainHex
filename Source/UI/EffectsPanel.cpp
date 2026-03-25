#include "UI/EffectsPanel.h"

namespace grainhex {

namespace {

void styleLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centredLeft);
    label.setFont(juce::Font(Theme::fontLabel));
    label.setColour(juce::Label::textColourId, juce::Colour(Theme::textNormal));
}

void styleSlider(juce::Slider& slider, double min, double max, double def, double step)
{
    slider.setRange(min, max, step);
    slider.setValue(def, juce::dontSendNotification);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(Theme::accentOrange));
}

void styleCombo(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(Theme::bgControl));
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(Theme::border));
    box.setColour(juce::ComboBox::textColourId, juce::Colour(Theme::textNormal));
}

} // namespace

EffectsPanel::EffectsPanel()
{
    addAndMakeVisible(distortionToggle);
    distortionToggle.setToggleState(false, juce::dontSendNotification);
    distortionToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(Theme::accentOrange));
    distortionToggle.onClick = [this]
    {
        updateSectionVisibility();
        resized();
        repaint();
        parameterChanged();
    };

    addAndMakeVisible(distortionModeBox);
    distortionModeBox.addItem("Soft Clip", 1);
    distortionModeBox.addItem("Hard Clip", 2);
    distortionModeBox.addItem("Wavefold", 3);
    distortionModeBox.setSelectedId(1, juce::dontSendNotification);
    distortionModeBox.onChange = [this] { parameterChanged(); };
    styleCombo(distortionModeBox);

    addAndMakeVisible(filterModeBox);
    filterModeBox.addItem("Low Pass", 1);
    filterModeBox.addItem("High Pass", 2);
    filterModeBox.addItem("Band Pass", 3);
    filterModeBox.setSelectedId(1, juce::dontSendNotification);
    filterModeBox.onChange = [this] { parameterChanged(); };
    styleCombo(filterModeBox);

    addAndMakeVisible(filterToggle);
    filterToggle.setToggleState(false, juce::dontSendNotification);
    filterToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(Theme::accentOrange));
    filterToggle.onClick = [this]
    {
        updateSectionVisibility();
        resized();
        repaint();
        parameterChanged();
    };

    styleSlider(driveSlider, 1.0, 20.0, 1.0, 0.1);
    styleSlider(distMixSlider, 0.0, 1.0, 0.0, 0.01);
    styleSlider(cutoffSlider, 20.0, 20000.0, 8000.0, 1.0);
    styleSlider(resonanceSlider, 0.0, 1.0, 0.0, 0.01);
    styleSlider(envAmountSlider, -96.0, 96.0, 48.0, 1.0);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);

    for (auto* slider : { &driveSlider, &distMixSlider, &cutoffSlider, &resonanceSlider, &envAmountSlider })
    {
        addAndMakeVisible(*slider);
        slider->onValueChange = [this] { parameterChanged(); };
    }

    for (auto* label : { &distortionModeLabel, &driveLabel, &distMixLabel, &filterModeLabel,
                          &cutoffLabel, &resonanceLabel, &envAmountLabel })
    {
        styleLabel(*label);
        addAndMakeVisible(*label);
    }

    cutoffLabel.setText("Cutoff", juce::dontSendNotification);
    resonanceLabel.setText("Resonance", juce::dontSendNotification);
    envAmountLabel.setText("Env Amount", juce::dontSendNotification);

    updateSectionVisibility();
}

void EffectsPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::bgPanel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius);

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);

    g.setColour(juce::Colour(Theme::accentOrange));
    g.setFont(juce::Font(Theme::fontSectionHead).boldened());
    g.drawText("EFFECTS", 12, 6, 100, 18, juce::Justification::centredLeft);

    auto drawSection = [&](juce::Rectangle<int> headerBounds, juce::Rectangle<int> bodyBounds,
                           const juce::String& title, bool enabled, bool expanded)
    {
        g.setColour(enabled ? juce::Colour(Theme::bgPanelHover) : juce::Colour(Theme::bgControl));
        g.fillRoundedRectangle(headerBounds.toFloat(), Theme::cornerRadius);
        g.setColour(enabled ? juce::Colour(Theme::borderActive) : juce::Colour(Theme::border));
        g.drawRoundedRectangle(headerBounds.toFloat().reduced(0.5f), Theme::cornerRadius, 1.0f);

        juce::Path chevron;
        const auto cx = static_cast<float>(headerBounds.getRight() - 14);
        const auto cy = static_cast<float>(headerBounds.getCentreY());
        if (expanded && enabled)
        {
            chevron.startNewSubPath(cx - 4.0f, cy - 2.0f);
            chevron.lineTo(cx, cy + 2.5f);
            chevron.lineTo(cx + 4.0f, cy - 2.0f);
        }
        else
        {
            chevron.startNewSubPath(cx - 2.0f, cy - 4.0f);
            chevron.lineTo(cx + 2.5f, cy);
            chevron.lineTo(cx - 2.0f, cy + 4.0f);
        }

        g.setColour(juce::Colour(Theme::textBright));
        g.setFont(juce::Font(Theme::fontValue).boldened());
        g.drawText(title, headerBounds.withTrimmedLeft(36).withTrimmedRight(26),
                   juce::Justification::centredLeft, false);
        g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        if (enabled && expanded && !bodyBounds.isEmpty())
        {
            g.setColour(juce::Colour(Theme::bgElevated));
            g.fillRoundedRectangle(bodyBounds.toFloat(), Theme::cornerRadius);
            g.setColour(juce::Colour(Theme::borderSubtle));
            g.drawRoundedRectangle(bodyBounds.toFloat().reduced(0.5f), Theme::cornerRadius, 1.0f);
        }
    };

    drawSection(distortionHeaderBounds, distortionBodyBounds, "Distortion",
                distortionToggle.getToggleState(), distortionExpanded);
    drawSection(filterHeaderBounds, filterBodyBounds, "Filter",
                filterToggle.getToggleState(), filterExpanded);
}

void EffectsPanel::resized()
{
    auto area = getLocalBounds().reduced(Theme::panelPadding);
    area.removeFromTop(24);

    constexpr int headerHeight = 24;
    const int sectionGap = Theme::sectionGap;
    const bool showDistortionBody = distortionToggle.getToggleState() && distortionExpanded;
    const bool showFilterBody = filterToggle.getToggleState() && filterExpanded;
    const int availableBodyHeight = juce::jmax(0, area.getHeight() - headerHeight * 2 - sectionGap);
    const int distortionBodyHeight = showDistortionBody && showFilterBody ? availableBodyHeight / 2 : availableBodyHeight;
    const int filterBodyHeight = showFilterBody ? (showDistortionBody ? availableBodyHeight - distortionBodyHeight : availableBodyHeight) : 0;

    distortionHeaderBounds = area.removeFromTop(headerHeight);
    if (showDistortionBody)
    {
        area.removeFromTop(4);
        distortionBodyBounds = area.removeFromTop(distortionBodyHeight);
        area.removeFromTop(sectionGap);
    }
    else
    {
        distortionBodyBounds = {};
        area.removeFromTop(sectionGap);
    }

    filterHeaderBounds = area.removeFromTop(headerHeight);
    if (showFilterBody)
    {
        area.removeFromTop(4);
        filterBodyBounds = area.removeFromTop(filterBodyHeight);
    }
    else
    {
        filterBodyBounds = {};
    }

    distortionToggle.setBounds(distortionHeaderBounds.withTrimmedLeft(8).withWidth(28));
    filterToggle.setBounds(filterHeaderBounds.withTrimmedLeft(8).withWidth(28));

    auto clearHidden = [](juce::Component& component)
    {
        component.setBounds(juce::Rectangle<int>());
    };

    if (showDistortionBody)
    {
        auto body = distortionBodyBounds.reduced(10, 10);
        const int labelH = 14;
        const int comboH = 24;
        const int knobH = Theme::knobSize + 18;
        const int comboWidth = juce::jmax(80, body.getWidth() / 3);
        auto left = body.removeFromLeft(comboWidth);
        body.removeFromLeft(Theme::controlGap);
        auto driveArea = body.removeFromLeft((body.getWidth() - Theme::controlGap) / 2);
        body.removeFromLeft(Theme::controlGap);
        auto mixArea = body;

        distortionModeLabel.setBounds(left.removeFromTop(labelH));
        distortionModeBox.setBounds(left.removeFromTop(comboH));
        driveLabel.setBounds(driveArea.removeFromTop(labelH));
        driveSlider.setBounds(driveArea.removeFromTop(knobH));
        distMixLabel.setBounds(mixArea.removeFromTop(labelH));
        distMixSlider.setBounds(mixArea.removeFromTop(knobH));
    }
    else
    {
        clearHidden(distortionModeLabel);
        clearHidden(distortionModeBox);
        clearHidden(driveLabel);
        clearHidden(driveSlider);
        clearHidden(distMixLabel);
        clearHidden(distMixSlider);
    }

    if (showFilterBody)
    {
        auto body = filterBodyBounds.reduced(10, 10);
        const int labelH = 14;
        const int comboH = 24;
        const int largeKnobH = Theme::knobSizeLarge + 18;
        const int knobH = Theme::knobSize + 18;

        filterModeLabel.setBounds(body.removeFromTop(labelH));
        filterModeBox.setBounds(body.removeFromTop(comboH));
        body.removeFromTop(6);

        const int firstKnobWidth = juce::jmax(72, body.getWidth() / 3 + 10);
        auto cutoffArea = body.removeFromLeft(firstKnobWidth);
        body.removeFromLeft(Theme::controlGap);
        auto resonanceArea = body.removeFromLeft((body.getWidth() - Theme::controlGap) / 2);
        body.removeFromLeft(Theme::controlGap);
        auto envArea = body;

        cutoffLabel.setBounds(cutoffArea.removeFromTop(labelH));
        cutoffSlider.setBounds(cutoffArea.removeFromTop(largeKnobH));
        resonanceLabel.setBounds(resonanceArea.removeFromTop(labelH));
        resonanceSlider.setBounds(resonanceArea.removeFromTop(knobH));
        envAmountLabel.setBounds(envArea.removeFromTop(labelH));
        envAmountSlider.setBounds(envArea.removeFromTop(knobH));
    }
    else
    {
        clearHidden(filterModeLabel);
        clearHidden(filterModeBox);
        clearHidden(cutoffLabel);
        clearHidden(cutoffSlider);
        clearHidden(resonanceLabel);
        clearHidden(resonanceSlider);
        clearHidden(envAmountLabel);
        clearHidden(envAmountSlider);
    }
}

void EffectsPanel::mouseUp(const juce::MouseEvent& event)
{
    if (distortionHeaderBounds.contains(event.getPosition()) && !distortionToggle.getBounds().contains(event.getPosition()))
    {
        distortionExpanded = !distortionExpanded;
        updateSectionVisibility();
        resized();
        repaint();
        return;
    }

    if (filterHeaderBounds.contains(event.getPosition()) && !filterToggle.getBounds().contains(event.getPosition()))
    {
        filterExpanded = !filterExpanded;
        updateSectionVisibility();
        resized();
        repaint();
    }
}

void EffectsPanel::mouseMove(const juce::MouseEvent& event)
{
    if (distortionHeaderBounds.contains(event.getPosition()) || filterHeaderBounds.contains(event.getPosition()))
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void EffectsPanel::updateSectionVisibility()
{
    const bool showDistortionBody = distortionToggle.getToggleState() && distortionExpanded;
    const bool showFilterBody = filterToggle.getToggleState() && filterExpanded;

    for (auto* component : { static_cast<juce::Component*>(&distortionModeBox),
                             static_cast<juce::Component*>(&distortionModeLabel),
                             static_cast<juce::Component*>(&driveSlider),
                             static_cast<juce::Component*>(&driveLabel),
                             static_cast<juce::Component*>(&distMixSlider),
                             static_cast<juce::Component*>(&distMixLabel) })
    {
        component->setVisible(showDistortionBody);
    }

    for (auto* component : { static_cast<juce::Component*>(&filterModeBox),
                             static_cast<juce::Component*>(&filterModeLabel),
                             static_cast<juce::Component*>(&cutoffSlider),
                             static_cast<juce::Component*>(&cutoffLabel),
                             static_cast<juce::Component*>(&resonanceSlider),
                             static_cast<juce::Component*>(&resonanceLabel),
                             static_cast<juce::Component*>(&envAmountSlider),
                             static_cast<juce::Component*>(&envAmountLabel) })
    {
        component->setVisible(showFilterBody);
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
