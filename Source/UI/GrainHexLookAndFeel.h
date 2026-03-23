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
    static constexpr uint32_t bgDarkest    = 0xff0a0a16;
    static constexpr uint32_t bgDark       = 0xff0d0d1a;
    static constexpr uint32_t bgPanel      = 0xff12122a;
    static constexpr uint32_t bgControl    = 0xff1a1a2e;
    static constexpr uint32_t bgHover      = 0xff222244;

    // Border / separator
    static constexpr uint32_t border       = 0xff333355;
    static constexpr uint32_t borderActive = 0xff555588;

    // Accent colours
    static constexpr uint32_t accentGreen  = 0xff16c784;  // Primary — granular, resample
    static constexpr uint32_t accentPurple = 0xffcc66ff;  // Sub tuner
    static constexpr uint32_t accentRed    = 0xffff6b6b;  // Modulation
    static constexpr uint32_t accentCyan   = 0xff00ccff;  // Loop markers, info
    static constexpr uint32_t accentOrange = 0xffff9933;  // Effects / browser

    // Text
    static constexpr uint32_t textBright   = 0xffe0e0e8;
    static constexpr uint32_t textNormal   = 0xffb0b0c0;
    static constexpr uint32_t textDim      = 0xff707088;
    static constexpr uint32_t textMuted    = 0xff505068;

    // Typography sizes
    static constexpr float fontTitle       = 24.0f;
    static constexpr float fontSectionHead = 13.0f;
    static constexpr float fontLabel       = 11.0f;
    static constexpr float fontSmall       = 10.0f;
    static constexpr float fontValue       = 12.0f;

    // Geometry
    static constexpr float cornerRadius    = 6.0f;
    static constexpr float borderWidth     = 1.0f;
    static constexpr int   panelPadding    = 8;
    static constexpr int   controlGap      = 4;
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
        setColour(juce::TextButton::textColourOffId,         juce::Colour(Theme::textNormal));

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
        g.strokePath(bgArc, juce::PathStrokeType(3.0f));

        // Value arc
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                               rotaryStartAngle, angle, true);
        auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        g.setColour(fillColour);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f));

        // Thumb dot
        auto thumbRadius = 4.0f;
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
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(juce::Colour(Theme::border));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearHorizontal)
        {
            auto trackY = static_cast<float>(y + height / 2);
            auto trackHeight = 4.0f;

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
            g.fillEllipse(sliderPos - 5.0f, trackY - 5.0f, 10.0f, 10.0f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                             minSliderPos, maxSliderPos, style, slider);
        }
    }
};

} // namespace grainhex
