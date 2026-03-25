#include "UI/WaveformView.h"

namespace grainhex {

WaveformView::WaveformView()
{
    startTimerHz(30);
}

void WaveformView::setSource(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate)
{
    sourceBuffer = std::move(buffer);
    sourceSampleRate = sampleRate;
    totalSamples = sourceBuffer ? sourceBuffer->getNumSamples() : 0;

    if (sourceBuffer != nullptr)
    {
        loopStartSample = 0;
        loopEndSample = totalSamples;
    }
    else
    {
        loopStartSample = 0;
        loopEndSample = 0;
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

void WaveformView::setActiveGrainPositions(const std::vector<float>& positions)
{
    grainPositions = positions;
}

void WaveformView::generatePeakData()
{
    peaks.clear();

    if (sourceBuffer == nullptr || totalSamples == 0 || getWidth() <= 0)
        return;

    const auto waveBounds = getWaveBounds();
    const auto numPixels = juce::jmax(1, static_cast<int>(waveBounds.getWidth()));
    peaks.resize(static_cast<size_t>(numPixels));

    const auto samplesPerPixel = static_cast<double>(totalSamples) / static_cast<double>(numPixels);
    const auto numChannels = juce::jmin(2, sourceBuffer->getNumChannels());

    for (int px = 0; px < numPixels; ++px)
    {
        PeakData peak;
        peak.numChannels = numChannels;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            peak.minVals[static_cast<size_t>(channel)] = 0.0f;
            peak.maxVals[static_cast<size_t>(channel)] = 0.0f;
        }

        const auto startIdx = static_cast<int64_t>(px * samplesPerPixel);
        const auto endIdx = std::min(static_cast<int64_t>((px + 1) * samplesPerPixel), totalSamples);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto* data = sourceBuffer->getReadPointer(channel);
            float minVal = 0.0f;
            float maxVal = 0.0f;

            for (int64_t sample = startIdx; sample < endIdx; ++sample)
            {
                minVal = std::min(minVal, data[sample]);
                maxVal = std::max(maxVal, data[sample]);
            }

            peak.minVals[static_cast<size_t>(channel)] = minVal;
            peak.maxVals[static_cast<size_t>(channel)] = maxVal;
        }

        peaks[static_cast<size_t>(px)] = peak;
    }
}

void WaveformView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto waveBounds = getWaveBounds();

    g.setColour(juce::Colour(Theme::bgDarkest));
    g.fillRoundedRectangle(bounds, Theme::cornerRadius);

    if (peaks.empty() || totalSamples == 0)
    {
        g.setColour(juce::Colour(Theme::textDim));
        g.setFont(14.0f);
        g.drawFittedText("Drop a WAV file here, import one, or pick a factory sample",
                         bounds.reduced(20.0f).toNearestInt(),
                         juce::Justification::centred, 2);

        g.setColour(juce::Colour(Theme::border));
        g.drawRoundedRectangle(bounds.reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);
        return;
    }

    const auto midY = waveBounds.getCentreY();
    const auto halfHeight = waveBounds.getHeight() * 0.42f;
    const auto loopStartNorm = totalSamples > 0 ? static_cast<float>(loopStartSample) / static_cast<float>(totalSamples) : 0.0f;
    const auto loopEndNorm = totalSamples > 0 ? static_cast<float>(loopEndSample) / static_cast<float>(totalSamples) : 1.0f;

    g.setColour(juce::Colour(Theme::borderSubtle));
    g.drawLine(waveBounds.getX(), midY, waveBounds.getRight(), midY, 1.0f);

    if (loopEndSample > loopStartSample)
    {
        const auto loopStartX = normalizedToX(loopStartNorm);
        const auto loopEndX = normalizedToX(loopEndNorm);
        auto loopArea = juce::Rectangle<float>(loopStartX, waveBounds.getY(),
                                               juce::jmax(2.0f, loopEndX - loopStartX),
                                               waveBounds.getHeight());

        g.setColour(juce::Colour(Theme::accentCyan).withAlpha(0.08f));
        g.fillRect(loopArea);

        juce::ColourGradient startShadow(juce::Colour(Theme::accentCyan).withAlpha(0.16f),
                                         loopArea.getX(), midY,
                                         juce::Colour(Theme::accentCyan).withAlpha(0.0f),
                                         loopArea.getX() + 12.0f, midY, false);
        g.setGradientFill(startShadow);
        g.fillRect(loopArea.removeFromLeft(14.0f));

        auto endShadowArea = juce::Rectangle<float>(loopEndX - 14.0f, waveBounds.getY(), 14.0f, waveBounds.getHeight());
        juce::ColourGradient endShadow(juce::Colour(Theme::accentCyan).withAlpha(0.0f),
                                       endShadowArea.getX(), midY,
                                       juce::Colour(Theme::accentCyan).withAlpha(0.16f),
                                       endShadowArea.getRight(), midY, false);
        g.setGradientFill(endShadow);
        g.fillRect(endShadowArea);

        const auto startHandle = getLoopStartHandleBounds();
        const auto endHandle = getLoopEndHandleBounds();
        auto drawHandle = [&](juce::Rectangle<float> handle, bool isHovered)
        {
            g.setColour(juce::Colour(Theme::accentCyan).withAlpha(isHovered ? 0.95f : 0.75f));
            g.fillRoundedRectangle(handle, 4.0f);
            g.drawLine(handle.getCentreX(), handle.getBottom(), handle.getCentreX(),
                       waveBounds.getBottom(), 2.0f);
        };

        drawHandle(startHandle, hoverLoopStart);
        drawHandle(endHandle, hoverLoopEnd);
    }

    auto drawChannel = [&](int channel, float alphaScale)
    {
        juce::Path fillPath;
        juce::Path topPath;
        juce::Path bottomPath;
        std::vector<juce::Point<float>> topPoints;
        std::vector<juce::Point<float>> bottomPoints;
        const auto peakCount = static_cast<int>(peaks.size());
        const auto xStep = peakCount > 1 ? waveBounds.getWidth() / static_cast<float>(peakCount - 1) : waveBounds.getWidth();

        for (int index = 0; index < peakCount; ++index)
        {
            const auto& peak = peaks[static_cast<size_t>(index)];
            if (peak.numChannels <= channel)
                continue;

            const auto x = waveBounds.getX() + static_cast<float>(index) * xStep;
            const auto topY = midY - peak.maxVals[static_cast<size_t>(channel)] * halfHeight;
            const auto bottomY = midY - peak.minVals[static_cast<size_t>(channel)] * halfHeight;
            topPoints.emplace_back(x, topY);
            bottomPoints.emplace_back(x, bottomY);
        }

        if (topPoints.empty() || bottomPoints.empty())
            return;

        fillPath.startNewSubPath(topPoints.front().x, midY);
        topPath.startNewSubPath(topPoints.front());
        for (const auto& point : topPoints)
        {
            fillPath.lineTo(point);
            topPath.lineTo(point);
        }

        bottomPath.startNewSubPath(bottomPoints.back());
        for (auto index = static_cast<int>(bottomPoints.size()) - 1; index >= 0; --index)
        {
            fillPath.lineTo(bottomPoints[static_cast<size_t>(index)]);
            bottomPath.lineTo(bottomPoints[static_cast<size_t>(index)]);
        }
        fillPath.closeSubPath();

        const auto accent = juce::Colour(Theme::accentGreen);
        juce::ColourGradient gradient(accent.withAlpha(0.78f * alphaScale), 0.0f, waveBounds.getY(),
                                      accent.withAlpha(0.30f * alphaScale), 0.0f, midY, false);
        gradient.addColour(1.0, accent.withAlpha(0.78f * alphaScale));
        g.setGradientFill(gradient);
        g.fillPath(fillPath);

        g.setColour(accent.withAlpha(0.92f * alphaScale));
        const auto outlineStroke = juce::PathStrokeType(1.15f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded);
        g.strokePath(topPath, outlineStroke);
        g.strokePath(bottomPath, outlineStroke);
    };

    drawChannel(0, 1.0f);
    drawChannel(1, 0.55f);

    if (!grainPositions.empty())
    {
        for (auto position : grainPositions)
        {
            const auto grainX = normalizedToX(position);
            juce::ColourGradient density(juce::Colour(Theme::accentGreen).withAlpha(0.0f),
                                         grainX - 18.0f, midY,
                                         juce::Colour(Theme::accentGreen).withAlpha(0.10f),
                                         grainX, midY, false);
            density.addColour(1.0, juce::Colour(Theme::accentGreen).withAlpha(0.0f));
            g.setGradientFill(density);
            g.fillRect(juce::Rectangle<float>(grainX - 18.0f, waveBounds.getY(), 36.0f, waveBounds.getHeight()));

            g.setColour(juce::Colour(Theme::accentGreen).withAlpha(0.18f));
            g.drawLine(grainX, waveBounds.getY(), grainX, waveBounds.getBottom(), 2.0f);
            g.setColour(juce::Colour(Theme::accentGreen).withAlpha(0.9f));
            g.drawLine(grainX, midY - 28.0f, grainX, midY + 28.0f, 1.5f);
        }
    }

    if (totalSamples > 0)
    {
        const auto playNorm = juce::jlimit(0.0f, 1.0f,
                                           static_cast<float>(playheadSample) / static_cast<float>(juce::jmax<int64_t>(1, totalSamples)));
        const auto playX = normalizedToX(playNorm);
        g.setColour(juce::Colour(Theme::textBright).withAlpha(0.18f));
        g.drawLine(playX, waveBounds.getY(), playX, waveBounds.getBottom(), 3.5f);
        g.setColour(juce::Colour(Theme::textBright));
        g.drawLine(playX, waveBounds.getY(), playX, waveBounds.getBottom(), 1.5f);

        juce::Path marker;
        marker.startNewSubPath(playX - 5.0f, waveBounds.getY() + 2.0f);
        marker.lineTo(playX + 5.0f, waveBounds.getY() + 2.0f);
        marker.lineTo(playX, waveBounds.getY() + 10.0f);
        marker.closeSubPath();
        g.fillPath(marker);
    }

    if (hoverNorm >= 0.0f)
    {
        const auto hoverX = normalizedToX(hoverNorm);
        g.setColour(juce::Colour(Theme::textBright).withAlpha(0.22f));
        g.drawLine(hoverX, waveBounds.getY(), hoverX, waveBounds.getBottom(), 1.0f);

        auto tooltipText = formatHoverText(hoverNorm);
        auto tooltipArea = juce::Rectangle<float>(lastMousePosition.x + 12.0f, lastMousePosition.y - 10.0f,
                                                  dragMode == DragMode::none ? 82.0f : 128.0f, 22.0f);
        tooltipArea = tooltipArea.withPosition(
            juce::jlimit(bounds.getX() + 6.0f, bounds.getRight() - tooltipArea.getWidth() - 6.0f, tooltipArea.getX()),
            juce::jlimit(bounds.getY() + 6.0f, bounds.getBottom() - tooltipArea.getHeight() - 6.0f, tooltipArea.getY()));

        g.setColour(juce::Colour(Theme::bgPanelHover));
        g.fillRoundedRectangle(tooltipArea, 6.0f);
        g.setColour(juce::Colour(Theme::borderActive));
        g.drawRoundedRectangle(tooltipArea, 6.0f, 1.0f);
        g.setColour(juce::Colour(Theme::textBright));
        g.setFont(juce::Font(Theme::fontMetadata));
        g.drawFittedText(tooltipText, tooltipArea.reduced(8.0f, 4.0f).toNearestInt(),
                         juce::Justification::centredLeft, 1);
    }

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(bounds.reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);
}

void WaveformView::resized()
{
    generatePeakData();
}

void WaveformView::mouseDown(const juce::MouseEvent& e)
{
    if (totalSamples == 0)
        return;

    lastMousePosition = e.position;
    hoverNorm = xToNormalized(e.position.x);

    if (getLoopStartHandleBounds().contains(e.position))
    {
        dragMode = DragMode::draggingLoopStart;
    }
    else if (getLoopEndHandleBounds().contains(e.position))
    {
        dragMode = DragMode::draggingLoopEnd;
    }
    else
    {
        dragMode = DragMode::selectingRegion;
        dragStartNorm = hoverNorm;
        dragEndNorm = hoverNorm;
    }
}

void WaveformView::mouseDrag(const juce::MouseEvent& e)
{
    if (totalSamples == 0 || dragMode == DragMode::none)
        return;

    constexpr float minSelection = 0.005f;

    lastMousePosition = e.position;
    hoverNorm = xToNormalized(e.position.x);

    switch (dragMode)
    {
        case DragMode::draggingLoopStart:
        {
            const auto endNorm = totalSamples > 0 ? static_cast<float>(loopEndSample) / static_cast<float>(totalSamples) : 1.0f;
            dragStartNorm = juce::jlimit(0.0f, endNorm - minSelection, hoverNorm);
            dragEndNorm = endNorm;
            loopStartSample = static_cast<int64_t>(dragStartNorm * static_cast<float>(totalSamples));
            break;
        }

        case DragMode::draggingLoopEnd:
        {
            const auto startNorm = totalSamples > 0 ? static_cast<float>(loopStartSample) / static_cast<float>(totalSamples) : 0.0f;
            dragStartNorm = startNorm;
            dragEndNorm = juce::jlimit(startNorm + minSelection, 1.0f, hoverNorm);
            loopEndSample = static_cast<int64_t>(dragEndNorm * static_cast<float>(totalSamples));
            break;
        }

        case DragMode::selectingRegion:
        {
            dragEndNorm = hoverNorm;
            const auto start = std::min(dragStartNorm, dragEndNorm);
            const auto end = std::max(dragStartNorm, dragEndNorm);
            loopStartSample = static_cast<int64_t>(start * static_cast<float>(totalSamples));
            loopEndSample = static_cast<int64_t>(end * static_cast<float>(totalSamples));
            break;
        }

        case DragMode::none:
            break;
    }

    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    repaint();
}

void WaveformView::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    if (dragMode == DragMode::none)
        return;

    auto start = totalSamples > 0 ? static_cast<float>(loopStartSample) / static_cast<float>(totalSamples) : 0.0f;
    auto end = totalSamples > 0 ? static_cast<float>(loopEndSample) / static_cast<float>(totalSamples) : 1.0f;

    if (dragMode == DragMode::selectingRegion && end - start < 0.005f)
    {
        start = 0.0f;
        end = 1.0f;
        loopStartSample = 0;
        loopEndSample = totalSamples;
    }

    dragMode = DragMode::none;
    hoverLoopStart = getLoopStartHandleBounds().contains(lastMousePosition);
    hoverLoopEnd = getLoopEndHandleBounds().contains(lastMousePosition);
    setMouseCursor((hoverLoopStart || hoverLoopEnd) ? juce::MouseCursor::LeftRightResizeCursor
                                                    : juce::MouseCursor::NormalCursor);

    if (loopCallback)
        loopCallback(start, end);

    repaint();
}

void WaveformView::mouseMove(const juce::MouseEvent& e)
{
    if (totalSamples == 0)
        return;

    lastMousePosition = e.position;
    hoverNorm = xToNormalized(e.position.x);
    hoverLoopStart = getLoopStartHandleBounds().contains(e.position);
    hoverLoopEnd = getLoopEndHandleBounds().contains(e.position);

    setMouseCursor((hoverLoopStart || hoverLoopEnd) ? juce::MouseCursor::LeftRightResizeCursor
                                                    : juce::MouseCursor::NormalCursor);
    repaint();
}

void WaveformView::mouseExit(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    hoverNorm = -1.0f;
    hoverLoopStart = false;
    hoverLoopEnd = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void WaveformView::timerCallback()
{
    repaint();
}

float WaveformView::xToNormalized(float x) const
{
    const auto waveBounds = getWaveBounds();
    return juce::jlimit(0.0f, 1.0f, (x - waveBounds.getX()) / juce::jmax(1.0f, waveBounds.getWidth()));
}

float WaveformView::normalizedToX(float norm) const
{
    const auto waveBounds = getWaveBounds();
    return waveBounds.getX() + juce::jlimit(0.0f, 1.0f, norm) * waveBounds.getWidth();
}

juce::Rectangle<float> WaveformView::getWaveBounds() const
{
    return getLocalBounds().toFloat().reduced(8.0f, 10.0f);
}

juce::Rectangle<float> WaveformView::getLoopStartHandleBounds() const
{
    const auto waveBounds = getWaveBounds();
    const auto loopStartNorm = totalSamples > 0 ? static_cast<float>(loopStartSample) / static_cast<float>(totalSamples) : 0.0f;
    return { normalizedToX(loopStartNorm) - 6.0f, waveBounds.getY() + 2.0f, 12.0f, 12.0f };
}

juce::Rectangle<float> WaveformView::getLoopEndHandleBounds() const
{
    const auto waveBounds = getWaveBounds();
    const auto loopEndNorm = totalSamples > 0 ? static_cast<float>(loopEndSample) / static_cast<float>(totalSamples) : 1.0f;
    return { normalizedToX(loopEndNorm) - 6.0f, waveBounds.getY() + 2.0f, 12.0f, 12.0f };
}

juce::String WaveformView::formatHoverText(float norm) const
{
    const auto sampleIndex = static_cast<int64_t>(norm * static_cast<float>(juce::jmax<int64_t>(1, totalSamples)));
    const auto seconds = sourceSampleRate > 0.0 ? static_cast<double>(sampleIndex) / sourceSampleRate : 0.0;

    if (dragMode == DragMode::none)
        return juce::String(seconds, 2) + " s";

    return juce::String(seconds, 2) + " s | " + juce::String(sampleIndex) + " smp";
}

} // namespace grainhex
