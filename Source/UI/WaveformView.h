#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Audio/AudioTypes.h"
#include <memory>
#include <vector>

namespace grainhex {

/**
 * Renders a waveform overview with playhead, loop region markers,
 * and drag-to-select loop region.
 */
class WaveformView : public juce::Component,
                     public juce::Timer
{
public:
    // Callback when user selects a loop region (normalized 0..1)
    using LoopRegionCallback = std::function<void(float startNorm, float endNorm)>;

    WaveformView();
    ~WaveformView() override = default;

    // Set the source buffer to display (generates peak data)
    void setSource(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate);

    // Update playhead position (in samples)
    void setPlayheadPosition(int64_t samplePos);

    // Set loop region (in samples)
    void setLoopRegion(int64_t startSample, int64_t endSample);

    // Set active grain positions (normalized 0..1) for visualization
    void setActiveGrainPositions(const std::vector<float>& positions);

    // Callback
    void setLoopRegionCallback(LoopRegionCallback cb) { loopCallback = std::move(cb); }

    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Timer for playhead updates
    void timerCallback() override;

private:
    void generatePeakData();
    float xToNormalized(float x) const;
    float normalizedToX(float norm) const;

    struct PeakData
    {
        float minVal = 0.0f;
        float maxVal = 0.0f;
    };

    std::shared_ptr<juce::AudioBuffer<float>> sourceBuffer;
    double sourceSampleRate = 44100.0;
    int64_t totalSamples = 0;

    std::vector<PeakData> peaks;

    // Playhead
    int64_t playheadSample = 0;

    // Loop region
    int64_t loopStartSample = 0;
    int64_t loopEndSample = 0;

    // Drag state
    bool isDragging = false;
    float dragStartNorm = 0.0f;
    float dragEndNorm = 0.0f;

    // Active grain positions (normalized)
    std::vector<float> grainPositions;

    LoopRegionCallback loopCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformView)
};

} // namespace grainhex
