#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Modulation/LFO.h"
#include "Modulation/ADSREnvelope.h"
#include "UI/GrainHexLookAndFeel.h"
#include <functional>

namespace grainhex {

/**
 * UI panel for LFO and ADSR envelope controls.
 * Redesigned as collapsible sections with visual editors.
 */
class ModulationPanel : public juce::Component
{
public:
    ModulationPanel();
    ~ModulationPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    // LFO accessors
    bool getLFOEnabled() const;
    LFOShape getLFOShape() const;
    float getLFORate() const;
    float getLFODepth() const;
    float getLFOSyncDivision() const;
    void setBPM(float newBpm);
    float getBPM() const { return bpm; }

    // ADSR accessors
    bool getEnvelopeEnabled() const;
    float getAttack() const;
    float getDecay() const;
    float getSustain() const;
    float getRelease() const;

    std::function<void()> onParameterChanged;

private:
    class LFOShapePreview : public juce::Component,
                            private juce::Timer
    {
    public:
        LFOShapePreview();

        void paint(juce::Graphics& g) override;
        void setState(LFOShape newShape, float newRateHz, bool isEnabled);

    private:
        float evaluateShape(float phaseNorm) const;
        void timerCallback() override;

        LFOShape shape = LFOShape::Sine;
        float rateHz = 1.0f;
        double phase = 0.0;
        double lastTickMs = 0.0;
        bool enabled = false;
        std::array<float, 8> sampleHoldValues { -0.82f, 0.31f, 0.74f, -0.18f,
                                                0.56f, -0.63f, 0.14f, 0.9f };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOShapePreview)
    };

    class EnvelopeEditor : public juce::Component
    {
    public:
        EnvelopeEditor(juce::Slider& attack, juce::Slider& decay,
                       juce::Slider& sustain, juce::Slider& release);

        void paint(juce::Graphics& g) override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

    private:
        enum class DragNode
        {
            none,
            attackPeak,
            sustainPoint,
            releaseEnd
        };

        struct Layout
        {
            juce::Rectangle<float> plotBounds;
            juce::Point<float> start;
            juce::Point<float> peak;
            juce::Point<float> sustain;
            juce::Point<float> end;
        };

        Layout getLayout() const;
        float segmentWidthForTime(float seconds) const;
        float timeForSegmentWidth(float width) const;
        DragNode hitTestNode(juce::Point<float> position) const;
        juce::String formatTime(float seconds) const;
        void setHoveredNode(DragNode node);

        juce::Slider& attackSlider;
        juce::Slider& decaySlider;
        juce::Slider& sustainSlider;
        juce::Slider& releaseSlider;

        DragNode hoveredNode = DragNode::none;
        DragNode activeNode = DragNode::none;

        static constexpr float minSegmentWidth = 24.0f;
        static constexpr float extraSegmentWidth = 48.0f;
        static constexpr float maxEnvelopeTime = 10.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeEditor)
    };

    void parameterChanged();
    void updateSectionVisibility();
    void updatePreviewState();
    static float syncIdToDivision(int selectedId);

    // LFO controls
    juce::ToggleButton lfoToggle { "LFO" };
    juce::ComboBox lfoShapeBox;
    juce::Label lfoShapeLabel { {}, "Shape" };
    juce::Slider lfoRateSlider;
    juce::Label lfoRateLabel { {}, "Rate" };
    juce::Slider lfoDepthSlider;
    juce::Label lfoDepthLabel { {}, "Depth" };
    juce::ComboBox lfoSyncBox;
    juce::Label lfoSyncLabel { {}, "Sync" };
    LFOShapePreview lfoPreview;

    // ADSR controls
    juce::ToggleButton envToggle { "Envelope" };
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    EnvelopeEditor envelopeEditor { attackSlider, decaySlider, sustainSlider, releaseSlider };

    bool lfoExpanded = true;
    bool envExpanded = true;
    float bpm = 120.0f;
    juce::Rectangle<int> lfoHeaderBounds;
    juce::Rectangle<int> lfoBodyBounds;
    juce::Rectangle<int> envHeaderBounds;
    juce::Rectangle<int> envBodyBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationPanel)
};

} // namespace grainhex
