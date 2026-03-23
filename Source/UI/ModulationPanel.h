#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Modulation/LFO.h"
#include "Modulation/ADSREnvelope.h"
#include "UI/GrainHexLookAndFeel.h"
#include <functional>

namespace grainhex {

/**
 * UI panel for LFO and ADSR envelope controls.
 * LFO: enable, shape, rate, depth.
 * ADSR: enable, attack, decay, sustain, release.
 */
class ModulationPanel : public juce::Component
{
public:
    ModulationPanel();
    ~ModulationPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // LFO accessors
    bool getLFOEnabled() const;
    LFOShape getLFOShape() const;
    float getLFORate() const;
    float getLFODepth() const;

    // ADSR accessors
    bool getEnvelopeEnabled() const;
    float getAttack() const;
    float getDecay() const;
    float getSustain() const;
    float getRelease() const;

    // Callback
    std::function<void()> onParameterChanged;

private:
    void parameterChanged();

    // LFO controls
    juce::ToggleButton lfoToggle { "LFO" };

    juce::ComboBox lfoShapeBox;
    juce::Label lfoShapeLabel { {}, "Shape" };

    juce::Slider lfoRateSlider;
    juce::Label lfoRateLabel { {}, "Rate" };

    juce::Slider lfoDepthSlider;
    juce::Label lfoDepthLabel { {}, "Depth" };

    // ADSR controls
    juce::ToggleButton envToggle { "Envelope" };

    juce::Slider attackSlider;
    juce::Label attackLabel { {}, "A" };

    juce::Slider decaySlider;
    juce::Label decayLabel { {}, "D" };

    juce::Slider sustainSlider;
    juce::Label sustainLabel { {}, "S" };

    juce::Slider releaseSlider;
    juce::Label releaseLabel { {}, "R" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationPanel)
};

} // namespace grainhex
