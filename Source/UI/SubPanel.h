#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Audio/AudioTypes.h"
#include "Sub/SubOscillator.h"
#include <functional>

namespace grainhex {

/**
 * UI panel for sub tuner controls and pitch display.
 * Controls: enable, level, waveform, mode (auto/manual), smoothing,
 *           note selector, octave offset, HP/LP frequencies.
 * Pitch display: detected note label, cents bar.
 */
class SubPanel : public juce::Component
{
public:
    SubPanel();
    ~SubPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Parameter accessors
    bool getEnabled() const;
    float getLevel() const;
    SubWaveform getWaveform() const;
    SubTuningMode getTuningMode() const;
    PitchSnapMode getPitchSnapMode() const;
    SmoothingSpeed getSmoothingSpeed() const;
    int getOctaveOffset() const;
    int getManualMidiNote() const;
    float getGranularHPFreq() const;
    float getSubLPFreq() const;

    // Update pitch display from audio thread snapshot
    void setDetectedPitch(const PitchInfo& pitch);

    // Callback when any parameter changes
    std::function<void()> onParameterChanged;

private:
    void parameterChanged();
    void updateModeVisibility();

    // Controls
    juce::ToggleButton enableToggle { "Sub" };

    juce::Slider levelSlider;
    juce::Label levelLabel { {}, "Level" };

    juce::ComboBox waveformBox;
    juce::Label waveformLabel { {}, "Wave" };

    juce::ComboBox modeBox;
    juce::Label modeLabel { {}, "Mode" };

    juce::ComboBox snapModeBox;
    juce::Label snapModeLabel { {}, "Snap" };

    juce::ComboBox smoothingBox;
    juce::Label smoothingLabel { {}, "Smooth" };

    juce::ComboBox noteBox;
    juce::Label noteLabel { {}, "Note" };

    juce::ComboBox octaveOffsetBox;
    juce::Label octaveOffsetLabel { {}, "Octave" };

    juce::Slider granularHPSlider;
    juce::Label granularHPLabel { {}, "Gran HP" };

    juce::Slider subLPSlider;
    juce::Label subLPLabel { {}, "Sub LP" };

    // Pitch display
    juce::Label pitchNoteLabel;
    float pitchCents = 0.0f;
    float pitchConfidence = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubPanel)
};

} // namespace grainhex
