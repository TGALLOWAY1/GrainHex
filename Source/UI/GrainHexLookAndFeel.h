#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace grainhex {

/**
 * Centralized colour and typography constants for the GrainHex dark theme.
 * Designed for studio use — high contrast on dark backgrounds.
 */
struct Theme
{
    // Background colours (darkest to lightest)
    static constexpr uint32_t bgDarkest    = 0xff08080f;
    static constexpr uint32_t bgDark       = 0xff0c0c14;
    static constexpr uint32_t bgPanel      = 0xff141428;
    static constexpr uint32_t bgPanelHover = 0xff1a1a34;
    static constexpr uint32_t bgControl    = 0xff1c1c32;
    static constexpr uint32_t bgHover      = 0xff252548;
    static constexpr uint32_t bgElevated   = 0xff1e1e3a;

    // Border / separator
    static constexpr uint32_t border       = 0xff2a2a44;
    static constexpr uint32_t borderActive = 0xff4a4a77;
    static constexpr uint32_t borderSubtle = 0xff1e1e36;

    // Accent colours
    static constexpr uint32_t accentGreen  = 0xff14b876;  // Primary — granular, resample
    static constexpr uint32_t accentPurple = 0xffb366e6;  // Sub tuner
    static constexpr uint32_t accentRed    = 0xffee6666;  // Modulation / warning
    static constexpr uint32_t accentCyan   = 0xff00bbee;  // Loop markers, info
    static constexpr uint32_t accentOrange = 0xffee8833;  // Effects / browser

    // Semantic button colours
    static constexpr uint32_t buttonPrimary     = 0xff14b876;
    static constexpr uint32_t buttonSecondary   = 0xff2a2a4a;
    static constexpr uint32_t buttonDestructive = 0xffee6666;
    static constexpr uint32_t buttonUtility     = 0xff1c1c32;

    // Text
    static constexpr uint32_t textBright   = 0xffe0e0e8;
    static constexpr uint32_t textNormal   = 0xffb0b0c0;
    static constexpr uint32_t textDim      = 0xff707088;
    static constexpr uint32_t textMuted    = 0xff505068;

    // Typography sizes
    static constexpr float fontTitle       = 22.0f;
    static constexpr float fontSectionHead = 12.0f;
    static constexpr float fontLabel       = 10.5f;
    static constexpr float fontSmall       = 9.5f;
    static constexpr float fontValue       = 11.0f;
    static constexpr float fontMetadata    = 9.0f;

    // Geometry
    static constexpr float cornerRadius    = 8.0f;
    static constexpr float borderWidth     = 0.5f;
    static constexpr int   panelPadding    = 10;
    static constexpr int   controlGap      = 6;
    static constexpr int   sectionGap      = 8;
    static constexpr int   knobSize        = 48;
    static constexpr int   knobSizeLarge   = 56;
};

/**
 * Custom LookAndFeel for GrainHex — dark theme with consistent styling
 * across all controls: sliders, buttons, combo boxes, toggle buttons.
 */
class GrainHexLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrainHexLookAndFeel()
    {
        // Set default colour scheme
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Theme::bgDark));

        // Sliders
        setColour(juce::Slider::rotarySliderFillColourId,    juce::Colour(Theme::accentGreen));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(Theme::border));
        setColour(juce::Slider::textBoxTextColourId,         juce::Colour(Theme::textNormal));
        setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        setColour(juce::Slider::thumbColourId,               juce::Colour(Theme::accentGreen));
        setColour(juce::Slider::trackColourId,               juce::Colour(Theme::border));
        setColour(juce::Slider::backgroundColourId,          juce::Colour(Theme::bgControl));

        // Labels
        setColour(juce::Label::textColourId,                 juce::Colour(Theme::textNormal));

        // Text buttons
        setColour(juce::TextButton::buttonColourId,          juce::Colour(Theme::bgControl));
        setColour(juce::TextButton::buttonOnColourId,        juce::Colour(Theme::accentGreen));
        setColour(juce::TextButton::textColourOnId,          juce::Colour(Theme::bgDarkest));
        setColour(juce::TextButton::textColourOffId,         juce::Colour(Theme::textBright));

        // Toggle buttons
        setColour(juce::ToggleButton::textColourId,          juce::Colour(Theme::textNormal));
        setColour(juce::ToggleButton::tickColourId,          juce::Colour(Theme::accentGreen));
        setColour(juce::ToggleButton::tickDisabledColourId,  juce::Colour(Theme::textMuted));

        // Combo boxes
        setColour(juce::ComboBox::backgroundColourId,        juce::Colour(Theme::bgControl));
        setColour(juce::ComboBox::textColourId,              juce::Colour(Theme::textNormal));
        setColour(juce::ComboBox::outlineColourId,           juce::Colour(Theme::border));
        setColour(juce::ComboBox::arrowColourId,             juce::Colour(Theme::textDim));

        // Popup menus
        setColour(juce::PopupMenu::backgroundColourId,       juce::Colour(Theme::bgPanel));
        setColour(juce::PopupMenu::textColourId,             juce::Colour(Theme::textNormal));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Theme::bgHover));
        setColour(juce::PopupMenu::highlightedTextColourId,  juce::Colour(Theme::textBright));

        // Scrollbar
        setColour(juce::ScrollBar::thumbColourId,            juce::Colour(Theme::border));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = static_cast<float>(juce::jmin(width, height)) / 2.0f - 4.0f;
        auto centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        auto centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Background arc
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(Theme::border));
        g.strokePath(bgArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // Value arc
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                               rotaryStartAngle, angle, true);
        auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (slider.isMouseOverOrDragging())
            fillColour = fillColour.brighter(0.15f);

        g.setColour(fillColour.withAlpha(0.18f));
        g.strokePath(valueArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour(fillColour);
        g.strokePath(valueArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Thumb dot
        auto thumbRadius = 5.0f;
        auto thumbX = centreX + (radius - 2.0f) * std::cos(angle - juce::MathConstants<float>::halfPi);
        auto thumbY = centreY + (radius - 2.0f) * std::sin(angle - juce::MathConstants<float>::halfPi);
        g.setColour(juce::Colour(Theme::textBright));
        g.fillEllipse(thumbX - thumbRadius, thumbY - thumbRadius,
                      thumbRadius * 2.0f, thumbRadius * 2.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto baseColour = backgroundColour;

        if (isButtonDown)
            baseColour = baseColour.brighter(0.2f);
        else if (isMouseOverButton)
            baseColour = baseColour.brighter(0.1f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, Theme::cornerRadius);

        g.setColour(juce::Colour(Theme::border));
        g.drawRoundedRectangle(bounds, Theme::cornerRadius, 1.0f);

        const auto primaryGreen = juce::Colour(Theme::buttonPrimary);
        const auto primaryOrange = juce::Colour(Theme::accentOrange);
        const auto destructive = juce::Colour(Theme::buttonDestructive);
        if (backgroundColour == primaryGreen || backgroundColour == primaryOrange || backgroundColour == destructive)
        {
            auto accentColour = backgroundColour.brighter(0.1f);
            g.setColour(accentColour);
            g.fillRoundedRectangle(bounds.removeFromBottom(2.0f), Theme::cornerRadius);
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearHorizontal)
        {
            auto trackY = static_cast<float>(y + height / 2);
            auto trackHeight = 5.0f;

            // Track background
            g.setColour(juce::Colour(Theme::border));
            g.fillRoundedRectangle(static_cast<float>(x), trackY - trackHeight / 2,
                                   static_cast<float>(width), trackHeight, 2.0f);

            // Filled portion
            g.setColour(slider.findColour(juce::Slider::thumbColourId));
            g.fillRoundedRectangle(static_cast<float>(x), trackY - trackHeight / 2,
                                   sliderPos - static_cast<float>(x), trackHeight, 2.0f);

            // Thumb
            g.setColour(juce::Colour(Theme::textBright));
            if (slider.isMouseOverOrDragging())
                g.setColour(juce::Colour(Theme::textBright).brighter(0.2f));
            g.fillEllipse(sliderPos - 6.0f, trackY - 6.0f, 12.0f, 12.0f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                             minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto indicatorSize = juce::jmin(16.0f, bounds.getHeight() - 4.0f);
        auto indicator = juce::Rectangle<float>(bounds.getX() + 2.0f,
                                                bounds.getCentreY() - indicatorSize * 0.5f,
                                                indicatorSize,
                                                indicatorSize);

        auto accent = button.findColour(juce::ToggleButton::tickColourId);
        auto outline = shouldDrawButtonAsHighlighted ? accent.brighter(0.15f) : juce::Colour(Theme::borderActive);
        auto fill = button.getToggleState() ? accent : juce::Colour(Theme::bgControl);

        if (shouldDrawButtonAsDown)
            fill = fill.brighter(0.1f);

        g.setColour(fill);
        g.fillEllipse(indicator);
        g.setColour(button.getToggleState() ? accent.brighter(0.1f) : outline);
        g.drawEllipse(indicator, 1.5f);

        if (button.getToggleState())
        {
            g.setColour(juce::Colour(Theme::textBright).withAlpha(0.9f));
            g.fillEllipse(indicator.reduced(indicatorSize * 0.28f));
        }

        auto textBounds = bounds.withX(indicator.getRight() + 8.0f);
        g.setColour(button.isEnabled() ? juce::Colour(Theme::textNormal)
                                       : juce::Colour(Theme::textMuted));
        g.setFont(juce::Font(Theme::fontValue, juce::Font::plain));
        g.drawFittedText(button.getButtonText(),
                         textBounds.toNearestInt(),
                         juce::Justification::centredLeft, 1);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        juce::ignoreUnused(buttonY, buttonH);

        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width) - 1.0f,
                                             static_cast<float>(height) - 1.0f);
        auto bg = juce::Colour(Theme::bgControl);
        if (box.isMouseOver(true))
            bg = juce::Colour(Theme::bgPanelHover);
        if (isButtonDown)
            bg = bg.brighter(0.08f);

        g.setColour(bg);
        g.fillRoundedRectangle(bounds, Theme::cornerRadius);

        g.setColour(box.isMouseOver(true) ? juce::Colour(Theme::borderActive) : juce::Colour(Theme::border));
        g.drawRoundedRectangle(bounds, Theme::cornerRadius, 1.0f);

        juce::Path arrow;
        const auto centreX = static_cast<float>(buttonX + buttonW / 2);
        const auto centreY = static_cast<float>(height) * 0.5f;
        arrow.startNewSubPath(centreX - 4.0f, centreY - 1.5f);
        arrow.lineTo(centreX, centreY + 3.0f);
        arrow.lineTo(centreX + 4.0f, centreY - 1.5f);
        g.setColour(juce::Colour(Theme::textDim));
        g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(10, 1, box.getWidth() - 28, box.getHeight() - 2);
        label.setFont(getComboBoxFont(box));
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(Theme::fontValue);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return juce::Font(juce::jmin(Theme::fontValue, static_cast<float>(buttonHeight) * 0.42f)).boldened();
    }
};

} // namespace grainhex
