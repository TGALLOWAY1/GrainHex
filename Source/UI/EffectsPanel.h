#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "FX/DistortionProcessor.h"
#include "FX/FilterProcessor.h"
#include "UI/GrainHexLookAndFeel.h"
#include <functional>

namespace grainhex {

/**
 * UI panel for distortion and filter controls.
 * Distortion: enable, mode, drive, mix.
 * Filter: enable, mode, cutoff, resonance, envelope amount.
 */
class EffectsPanel : public juce::Component
{
public:
    EffectsPanel();
    ~EffectsPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    // Distortion accessors
    bool getDistortionEnabled() const;
    DistortionMode getDistortionMode() const;
    float getDrive() const;
    float getDistortionMix() const;

    // Filter accessors
    bool getFilterEnabled() const;
    FilterMode getFilterMode() const;
    float getCutoff() const;
    float getResonance() const;
    float getEnvelopeAmount() const;

    // Callback when any parameter changes
    std::function<void()> onParameterChanged;

private:
    void parameterChanged();
    void updateSectionVisibility();

    // Distortion controls
    juce::ToggleButton distortionToggle { "Distortion" };
    juce::ComboBox distortionModeBox;
    juce::Label distortionModeLabel { {}, "Mode" };

    juce::Slider driveSlider;
    juce::Label driveLabel { {}, "Drive" };

    juce::Slider distMixSlider;
    juce::Label distMixLabel { {}, "Mix" };

    // Filter controls
    juce::ToggleButton filterToggle { "Filter" };
    juce::ComboBox filterModeBox;
    juce::Label filterModeLabel { {}, "Type" };

    juce::Slider cutoffSlider;
    juce::Label cutoffLabel { {}, "Cutoff" };

    juce::Slider resonanceSlider;
    juce::Label resonanceLabel { {}, "Reso" };

    juce::Slider envAmountSlider;
    juce::Label envAmountLabel { {}, "Env Amt" };

    bool distortionExpanded = true;
    bool filterExpanded = true;
    juce::Rectangle<int> distortionHeaderBounds;
    juce::Rectangle<int> distortionBodyBounds;
    juce::Rectangle<int> filterHeaderBounds;
    juce::Rectangle<int> filterBodyBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};

} // namespace grainhex
