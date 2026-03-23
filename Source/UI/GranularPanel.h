#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Granular/GrainScheduler.h"
#include "Granular/WindowFunctions.h"
#include "UI/GrainHexLookAndFeel.h"
#include <functional>

namespace grainhex {

/**
 * UI panel with 9 granular synthesis parameter controls.
 */
class GranularPanel : public juce::Component
{
public:
    GranularPanel();
    ~GranularPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Parameter accessors
    float getGrainSize() const;       // ms
    int getGrainCount() const;
    float getPosition() const;         // 0..1
    float getSpray() const;            // 0..1
    float getPitchSemitones() const;   // -24..24
    PitchQuantizeScale getPitchQuantize() const;
    WindowShape getWindowShape() const;
    DirectionMode getDirection() const;
    float getSpread() const;           // 0..1

    // Called when any parameter changes
    std::function<void()> onParameterChanged;

private:
    void parameterChanged();

    // Rotary knobs
    juce::Slider grainSizeSlider;
    juce::Slider grainCountSlider;
    juce::Slider positionSlider;
    juce::Slider spraySlider;
    juce::Slider pitchSlider;
    juce::Slider spreadSlider;

    // Labels
    juce::Label grainSizeLabel   { {}, "Size" };
    juce::Label grainCountLabel  { {}, "Count" };
    juce::Label positionLabel    { {}, "Position" };
    juce::Label sprayLabel       { {}, "Spray" };
    juce::Label pitchLabel       { {}, "Pitch" };
    juce::Label spreadLabel      { {}, "Spread" };

    // Combo boxes
    juce::ComboBox pitchQuantizeBox;
    juce::ComboBox windowShapeBox;
    juce::ComboBox directionBox;

    juce::Label pitchQuantizeLabel { {}, "Quantize" };
    juce::Label windowShapeLabel   { {}, "Window" };
    juce::Label directionLabel     { {}, "Direction" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularPanel)
};

} // namespace grainhex
