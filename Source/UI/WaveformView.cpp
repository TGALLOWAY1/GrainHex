#include "UI/WaveformView.h"

namespace grainhex {

WaveformView::WaveformView()
{
    startTimerHz(30); // 30fps playhead updates
}

void WaveformView::setSource(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate)
{
    sourceBuffer = buffer;
    sourceSampleRate = sampleRate;
    totalSamples = buffer ? buffer->getNumSamples() : 0;

    if (buffer)
    {
        loopStartSample = 0;
        loopEndSample = totalSamples;
    }

    generatePeakData();
    repaint();
}

void WaveformView::setPlayheadPosition(int64_t samplePos)
{
    playheadSample = samplePos;
}

void WaveformView::setLoopRegion(int64_t startSample, int64_t endSample)
{
    loopStartSample = startSample;
    loopEndSample = endSample;
    repaint();
}

void WaveformView::generatePeakData()
{
    peaks.clear();

    if (sourceBuffer == nullptr || totalSamples == 0 || getWidth() <= 0)
        return;

    int numPixels = getWidth();
    peaks.resize(static_cast<size_t>(numPixels));

    double samplesPerPixel = static_cast<double>(totalSamples) / static_cast<double>(numPixels);

    for (int px = 0; px < numPixels; ++px)
    {
        int64_t startIdx = static_cast<int64_t>(px * samplesPerPixel);
        int64_t endIdx = static_cast<int64_t>((px + 1) * samplesPerPixel);
        endIdx = std::min(endIdx, totalSamples);

        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (int ch = 0; ch < sourceBuffer->getNumChannels(); ++ch)
        {
            const float* data = sourceBuffer->getReadPointer(ch);
            for (int64_t i = startIdx; i < endIdx; ++i)
            {
                float s = data[i];
                minVal = std::min(minVal, s);
                maxVal = std::max(maxVal, s);
            }
        }

        peaks[static_cast<size_t>(px)] = { minVal, maxVal };
    }
}

void WaveformView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillRect(bounds);

    if (peaks.empty() || totalSamples == 0)
    {
        // Empty state
        g.setColour(juce::Colours::grey);
        g.setFont(16.0f);
        g.drawText("Drop a sample here (WAV / AIFF / FLAC)",
                    bounds, juce::Justification::centred);
        return;
    }

    float midY = bounds.getCentreY();
    float halfHeight = bounds.getHeight() * 0.45f;

    // Loop region highlight
    if (loopEndSample > loopStartSample)
    {
        float loopStartX = normalizedToX(static_cast<float>(loopStartSample) / static_cast<float>(totalSamples));
        float loopEndX = normalizedToX(static_cast<float>(loopEndSample) / static_cast<float>(totalSamples));

        g.setColour(juce::Colour(0x2000ccff));
        g.fillRect(loopStartX, bounds.getY(), loopEndX - loopStartX, bounds.getHeight());

        // Loop markers
        g.setColour(juce::Colour(0xff00ccff));
        g.fillRect(loopStartX, bounds.getY(), 2.0f, bounds.getHeight());
        g.fillRect(loopEndX - 2.0f, bounds.getY(), 2.0f, bounds.getHeight());
    }

    // Waveform
    g.setColour(juce::Colour(0xff16c784));
    int numPeaks = static_cast<int>(peaks.size());
    for (int i = 0; i < numPeaks; ++i)
    {
        float x = static_cast<float>(i);
        float top = midY - peaks[static_cast<size_t>(i)].maxVal * halfHeight;
        float bottom = midY - peaks[static_cast<size_t>(i)].minVal * halfHeight;
        g.drawVerticalLine(static_cast<int>(x), top, bottom);
    }

    // Playhead
    if (totalSamples > 0)
    {
        float norm = static_cast<float>(playheadSample) / static_cast<float>(totalSamples);
        float playX = normalizedToX(norm);
        g.setColour(juce::Colours::white);
        g.fillRect(playX, bounds.getY(), 2.0f, bounds.getHeight());
    }

    // Border
    g.setColour(juce::Colour(0xff333355));
    g.drawRect(bounds, 1.0f);
}

void WaveformView::resized()
{
    generatePeakData();
}

void WaveformView::mouseDown(const juce::MouseEvent& e)
{
    if (totalSamples == 0) return;

    isDragging = true;
    dragStartNorm = xToNormalized(static_cast<float>(e.x));
    dragEndNorm = dragStartNorm;
}

void WaveformView::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || totalSamples == 0) return;

    dragEndNorm = xToNormalized(static_cast<float>(e.x));
    dragEndNorm = juce::jlimit(0.0f, 1.0f, dragEndNorm);

    // Update visual loop region during drag
    float start = std::min(dragStartNorm, dragEndNorm);
    float end = std::max(dragStartNorm, dragEndNorm);

    loopStartSample = static_cast<int64_t>(start * static_cast<float>(totalSamples));
    loopEndSample = static_cast<int64_t>(end * static_cast<float>(totalSamples));

    repaint();
}

void WaveformView::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (!isDragging) return;
    isDragging = false;

    float start = std::min(dragStartNorm, dragEndNorm);
    float end = std::max(dragStartNorm, dragEndNorm);

    // Minimum selection size
    if (end - start < 0.005f)
    {
        // Too small — reset to full
        start = 0.0f;
        end = 1.0f;
        loopStartSample = 0;
        loopEndSample = totalSamples;
    }

    if (loopCallback)
        loopCallback(start, end);

    repaint();
}

void WaveformView::timerCallback()
{
    repaint(); // Refresh playhead
}

float WaveformView::xToNormalized(float x) const
{
    return x / static_cast<float>(getWidth());
}

float WaveformView::normalizedToX(float norm) const
{
    return norm * static_cast<float>(getWidth());
}

} // namespace grainhex
