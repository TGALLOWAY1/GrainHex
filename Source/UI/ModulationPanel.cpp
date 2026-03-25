#include "UI/ModulationPanel.h"

namespace grainhex {

namespace {

void styleLabel(juce::Label& label, juce::Justification justification)
{
    label.setJustificationType(justification);
    label.setFont(juce::Font(Theme::fontLabel));
    label.setColour(juce::Label::textColourId, juce::Colour(Theme::textNormal));
}

void styleCombo(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(Theme::bgControl));
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(Theme::border));
    box.setColour(juce::ComboBox::textColourId, juce::Colour(Theme::textNormal));
}

void styleRotarySlider(juce::Slider& slider, double min, double max,
                       double def, double step, juce::Colour accent)
{
    slider.setRange(min, max, step);
    slider.setValue(def, juce::dontSendNotification);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 16);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
}

void styleHiddenSlider(juce::Slider& slider, double min, double max, double def, double step)
{
    slider.setRange(min, max, step);
    slider.setValue(def, juce::dontSendNotification);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setVisible(false);
}

} // namespace

ModulationPanel::LFOShapePreview::LFOShapePreview()
{
    startTimerHz(30);
    lastTickMs = juce::Time::getMillisecondCounterHiRes();
}

void ModulationPanel::LFOShapePreview::setState(LFOShape newShape, float newRateHz, bool isEnabled)
{
    shape = newShape;
    rateHz = juce::jmax(0.01f, newRateHz);
    enabled = isEnabled;
    repaint();
}

float ModulationPanel::LFOShapePreview::evaluateShape(float phaseNorm) const
{
    switch (shape)
    {
        case LFOShape::Triangle:
            return phaseNorm < 0.5f ? (4.0f * phaseNorm - 1.0f) : (3.0f - 4.0f * phaseNorm);
        case LFOShape::Square:
            return phaseNorm < 0.5f ? 1.0f : -1.0f;
        case LFOShape::SampleAndHold:
        {
            const auto index = juce::jlimit(0, static_cast<int>(sampleHoldValues.size()) - 1,
                                            static_cast<int>(phaseNorm * static_cast<float>(sampleHoldValues.size())));
            return sampleHoldValues[static_cast<size_t>(index)];
        }
        case LFOShape::Sine:
        default:
            return std::sin(phaseNorm * juce::MathConstants<float>::twoPi);
    }
}

void ModulationPanel::LFOShapePreview::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(Theme::bgDarkest));
    g.fillRoundedRectangle(bounds, Theme::cornerRadius);
    g.setColour(juce::Colour(Theme::borderSubtle));
    g.drawRoundedRectangle(bounds.reduced(0.5f), Theme::cornerRadius, 1.0f);

    auto plotBounds = bounds.reduced(8.0f, 8.0f);
    const auto midY = plotBounds.getCentreY();
    const auto amplitude = plotBounds.getHeight() * 0.36f;
    const auto colour = juce::Colour(Theme::accentRed).withAlpha(enabled ? 0.95f : 0.45f);

    g.setColour(juce::Colour(Theme::textMuted).withAlpha(0.22f));
    g.drawLine(plotBounds.getX(), midY, plotBounds.getRight(), midY, 1.0f);

    juce::Path outline;
    juce::Path fill;
    constexpr int points = 80;

    for (int index = 0; index < points; ++index)
    {
        const auto norm = static_cast<float>(index) / static_cast<float>(points - 1);
        const auto x = plotBounds.getX() + plotBounds.getWidth() * norm;
        const auto y = midY - evaluateShape(norm) * amplitude;

        if (index == 0)
        {
            outline.startNewSubPath(x, y);
            fill.startNewSubPath(x, midY);
            fill.lineTo(x, y);
        }
        else
        {
            outline.lineTo(x, y);
            fill.lineTo(x, y);
        }
    }

    fill.lineTo(plotBounds.getRight(), midY);
    fill.closeSubPath();

    g.setColour(colour.withAlpha(enabled ? 0.18f : 0.08f));
    g.fillPath(fill);

    g.setColour(colour);
    g.strokePath(outline, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const auto playX = plotBounds.getX() + static_cast<float>(phase) * plotBounds.getWidth();
    g.setColour(juce::Colour(Theme::textBright).withAlpha(enabled ? 0.8f : 0.25f));
    g.drawLine(playX, plotBounds.getY(), playX, plotBounds.getBottom(), 1.5f);
}

void ModulationPanel::LFOShapePreview::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto deltaSeconds = (now - lastTickMs) / 1000.0;
    lastTickMs = now;

    if (enabled && isShowing())
    {
        phase += deltaSeconds * static_cast<double>(rateHz);
        phase = std::fmod(phase, 1.0);
        if (phase < 0.0)
            phase += 1.0;
    }

    repaint();
}

ModulationPanel::EnvelopeEditor::EnvelopeEditor(juce::Slider& attack, juce::Slider& decay,
                                                juce::Slider& sustain, juce::Slider& release)
    : attackSlider(attack), decaySlider(decay), sustainSlider(sustain), releaseSlider(release)
{
    setRepaintsOnMouseActivity(true);
}

float ModulationPanel::EnvelopeEditor::segmentWidthForTime(float seconds) const
{
    constexpr float minTime = 0.001f;
    const auto norm = juce::jlimit(0.0f, 1.0f, (seconds - minTime) / (maxEnvelopeTime - minTime));
    return minSegmentWidth + std::pow(norm, 0.35f) * extraSegmentWidth;
}

float ModulationPanel::EnvelopeEditor::timeForSegmentWidth(float width) const
{
    constexpr float minTime = 0.001f;
    const auto norm = juce::jlimit(0.0f, 1.0f, (width - minSegmentWidth) / extraSegmentWidth);
    return minTime + std::pow(norm, 1.0f / 0.35f) * (maxEnvelopeTime - minTime);
}

ModulationPanel::EnvelopeEditor::Layout ModulationPanel::EnvelopeEditor::getLayout() const
{
    auto plotBounds = getLocalBounds().toFloat().reduced(14.0f, 10.0f);
    plotBounds.removeFromTop(20.0f);
    plotBounds.removeFromBottom(14.0f);
    plotBounds = plotBounds.reduced(2.0f, 2.0f);

    auto attackWidth = segmentWidthForTime(static_cast<float>(attackSlider.getValue()));
    auto decayWidth = segmentWidthForTime(static_cast<float>(decaySlider.getValue()));
    auto releaseWidth = segmentWidthForTime(static_cast<float>(releaseSlider.getValue()));

    const auto usedWidth = attackWidth + decayWidth + releaseWidth;
    if (usedWidth > plotBounds.getWidth() && usedWidth > 0.0f)
    {
        const auto scale = plotBounds.getWidth() / usedWidth;
        attackWidth *= scale;
        decayWidth *= scale;
        releaseWidth *= scale;
    }

    const auto start = juce::Point<float>(plotBounds.getX(), plotBounds.getBottom());
    const auto peak = juce::Point<float>(start.x + attackWidth, plotBounds.getY());
    const auto sustainY = juce::jmap(static_cast<float>(sustainSlider.getValue()), 0.0f, 1.0f,
                                     plotBounds.getBottom(), plotBounds.getY());
    const auto sustain = juce::Point<float>(peak.x + decayWidth, sustainY);
    const auto end = juce::Point<float>(juce::jmin(plotBounds.getRight(), sustain.x + releaseWidth),
                                        plotBounds.getBottom());

    return { plotBounds, start, peak, sustain, end };
}

juce::String ModulationPanel::EnvelopeEditor::formatTime(float seconds) const
{
    if (seconds >= 1.0f)
        return juce::String(seconds, 1) + "s";

    return juce::String(static_cast<int>(std::round(seconds * 1000.0f))) + "ms";
}

ModulationPanel::EnvelopeEditor::DragNode ModulationPanel::EnvelopeEditor::hitTestNode(juce::Point<float> position) const
{
    const auto layout = getLayout();
    const auto attackDistance = position.getDistanceFrom(layout.peak);
    const auto sustainDistance = position.getDistanceFrom(layout.sustain);
    const auto releaseDistance = position.getDistanceFrom(layout.end);

    constexpr float hitRadius = 12.0f;
    if (attackDistance <= hitRadius)
        return DragNode::attackPeak;
    if (sustainDistance <= hitRadius)
        return DragNode::sustainPoint;
    if (releaseDistance <= hitRadius)
        return DragNode::releaseEnd;
    return DragNode::none;
}

void ModulationPanel::EnvelopeEditor::setHoveredNode(DragNode node)
{
    if (hoveredNode == node)
        return;

    hoveredNode = node;
    auto cursorNode = activeNode != DragNode::none ? activeNode : hoveredNode;
    switch (cursorNode)
    {
        case DragNode::attackPeak:
        case DragNode::releaseEnd:
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            break;
        case DragNode::sustainPoint:
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            break;
        case DragNode::none:
        default:
            setMouseCursor(juce::MouseCursor::NormalCursor);
            break;
    }
    repaint();
}

void ModulationPanel::EnvelopeEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto layout = getLayout();
    const auto attackColour = juce::Colour(Theme::accentRed);
    const auto decayColour = juce::Colour(Theme::accentOrange);
    const auto releaseColour = juce::Colour(Theme::accentCyan);
    const auto labelBounds = getLocalBounds().reduced(12, 4);

    g.setColour(juce::Colour(Theme::bgDarkest));
    g.fillRoundedRectangle(bounds, Theme::cornerRadius);
    g.setColour(juce::Colour(Theme::borderSubtle));
    g.drawRoundedRectangle(bounds.reduced(0.5f), Theme::cornerRadius, 1.0f);

    g.setColour(juce::Colour(Theme::textMuted).withAlpha(0.25f));
    g.drawLine(layout.plotBounds.getX(), layout.plotBounds.getBottom(),
               layout.plotBounds.getRight(), layout.plotBounds.getBottom(), 1.0f);

    g.drawLine(layout.peak.x, layout.plotBounds.getY(), layout.peak.x, layout.plotBounds.getBottom(), 1.0f);
    g.drawLine(layout.sustain.x, layout.plotBounds.getY(), layout.sustain.x, layout.plotBounds.getBottom(), 1.0f);
    g.drawLine(layout.end.x, layout.plotBounds.getY(), layout.end.x, layout.plotBounds.getBottom(), 1.0f);

    juce::Path attackPath;
    attackPath.startNewSubPath(layout.start);
    attackPath.quadraticTo(layout.start.x + (layout.peak.x - layout.start.x) * 0.58f,
                           layout.start.y - layout.plotBounds.getHeight() * 0.18f,
                           layout.peak.x, layout.peak.y);

    juce::Path decayPath;
    decayPath.startNewSubPath(layout.peak);
    decayPath.quadraticTo(layout.peak.x + (layout.sustain.x - layout.peak.x) * 0.42f,
                          layout.sustain.y - layout.plotBounds.getHeight() * 0.24f,
                          layout.sustain.x, layout.sustain.y);

    juce::Path releasePath;
    releasePath.startNewSubPath(layout.sustain);
    releasePath.quadraticTo(layout.sustain.x + (layout.end.x - layout.sustain.x) * 0.35f,
                            layout.sustain.y + layout.plotBounds.getHeight() * 0.12f,
                            layout.end.x, layout.end.y);

    auto fillSegment = [&](const juce::Path& segment, juce::Colour colour)
    {
        juce::Path filled = segment;
        const auto endPoint = segment.getCurrentPosition();
        filled.lineTo(endPoint.x, layout.plotBounds.getBottom());
        filled.lineTo(segment.getBounds().getX(), layout.plotBounds.getBottom());
        filled.closeSubPath();
        g.setColour(colour.withAlpha(0.15f));
        g.fillPath(filled);
        g.setColour(colour);
        g.strokePath(segment, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    };

    fillSegment(attackPath, attackColour);
    fillSegment(decayPath, decayColour);
    g.setColour(decayColour.withAlpha(0.65f));
    const float dashes[] { 4.0f, 4.0f };
    g.drawDashedLine({ layout.sustain.x, layout.sustain.y, layout.plotBounds.getRight(), layout.sustain.y },
                     dashes, 2, 1.0f);
    fillSegment(releasePath, releaseColour);

    g.setFont(juce::Font(Theme::fontMetadata).boldened());
    g.setColour(attackColour);
    g.drawText("ATTACK " + formatTime(static_cast<float>(attackSlider.getValue())),
               juce::Rectangle<int>(juce::roundToInt(layout.start.x), labelBounds.getY(), 88, 14),
               juce::Justification::centredLeft, false);
    g.setColour(decayColour);
    g.drawText("DECAY " + formatTime(static_cast<float>(decaySlider.getValue())),
               juce::Rectangle<int>(juce::roundToInt(layout.peak.x) - 28, labelBounds.getY(), 104, 14),
               juce::Justification::centredLeft, false);
    g.setColour(releaseColour);
    g.drawText("RELEASE " + formatTime(static_cast<float>(releaseSlider.getValue())),
               juce::Rectangle<int>(juce::roundToInt(layout.sustain.x) - 18, labelBounds.getY(), 128, 14),
               juce::Justification::centredLeft, false);

    g.setColour(juce::Colour(Theme::textDim));
    g.setFont(juce::Font(Theme::fontMetadata));
    g.drawText("SUSTAIN " + juce::String(static_cast<int>(std::round(sustainSlider.getValue() * 100.0))) + "%",
               getLocalBounds().reduced(12, 8).removeFromBottom(14),
               juce::Justification::centredRight, false);

    auto drawNode = [&](juce::Point<float> point, juce::Colour outline, DragNode node, bool draggable)
    {
        const bool highlighted = node == activeNode || node == hoveredNode;
        const float radius = draggable ? (highlighted ? 8.0f : 6.0f) : 4.0f;
        g.setColour(juce::Colour(Theme::textBright));
        g.fillEllipse(point.x - radius, point.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(outline);
        g.drawEllipse(point.x - radius, point.y - radius, radius * 2.0f, radius * 2.0f, highlighted ? 2.0f : 1.4f);
    };

    drawNode(layout.start, juce::Colour(Theme::textMuted), DragNode::none, false);
    drawNode(layout.peak, attackColour, DragNode::attackPeak, true);
    drawNode(layout.sustain, decayColour, DragNode::sustainPoint, true);
    drawNode(layout.end, releaseColour, DragNode::releaseEnd, true);
}

void ModulationPanel::EnvelopeEditor::mouseMove(const juce::MouseEvent& event)
{
    setHoveredNode(hitTestNode(event.position));
}

void ModulationPanel::EnvelopeEditor::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (activeNode == DragNode::none)
        setHoveredNode(DragNode::none);
}

void ModulationPanel::EnvelopeEditor::mouseDown(const juce::MouseEvent& event)
{
    activeNode = hitTestNode(event.position);
    switch (activeNode)
    {
        case DragNode::attackPeak:
        case DragNode::releaseEnd:
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            break;
        case DragNode::sustainPoint:
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            break;
        case DragNode::none:
        default:
            setMouseCursor(juce::MouseCursor::NormalCursor);
            break;
    }
    repaint();
}

void ModulationPanel::EnvelopeEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (activeNode == DragNode::none)
        return;

    const auto layout = getLayout();
    const auto availableWidth = layout.plotBounds.getWidth();
    const auto attackWidth = layout.peak.x - layout.start.x;
    const auto decayWidth = layout.sustain.x - layout.peak.x;

    switch (activeNode)
    {
        case DragNode::attackPeak:
        {
            const auto width = juce::jlimit(minSegmentWidth, availableWidth - minSegmentWidth * 2.0f,
                                            event.position.x - layout.start.x);
            attackSlider.setValue(timeForSegmentWidth(width), juce::sendNotificationSync);
            break;
        }

        case DragNode::sustainPoint:
        {
            const auto width = juce::jlimit(minSegmentWidth, availableWidth - attackWidth - minSegmentWidth,
                                            event.position.x - layout.peak.x);
            decaySlider.setValue(timeForSegmentWidth(width), juce::sendNotificationSync);

            const auto sustain = juce::jlimit(0.0f, 1.0f,
                                              (layout.plotBounds.getBottom() - event.position.y)
                                                  / juce::jmax(1.0f, layout.plotBounds.getHeight()));
            sustainSlider.setValue(sustain, juce::sendNotificationSync);
            break;
        }

        case DragNode::releaseEnd:
        {
            const auto width = juce::jlimit(minSegmentWidth, availableWidth - attackWidth - decayWidth,
                                            event.position.x - layout.sustain.x);
            releaseSlider.setValue(timeForSegmentWidth(width), juce::sendNotificationSync);
            break;
        }

        case DragNode::none:
        default:
            break;
    }
}

void ModulationPanel::EnvelopeEditor::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    activeNode = DragNode::none;
    setHoveredNode(hitTestNode(getMouseXYRelative().toFloat()));
}

ModulationPanel::ModulationPanel()
{
    addAndMakeVisible(lfoToggle);
    lfoToggle.setToggleState(false, juce::dontSendNotification);
    lfoToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(Theme::accentRed));
    lfoToggle.onClick = [this]
    {
        updateSectionVisibility();
        updatePreviewState();
        resized();
        repaint();
        parameterChanged();
    };

    addAndMakeVisible(lfoPreview);

    addAndMakeVisible(lfoShapeBox);
    lfoShapeBox.addItem("Sine", 1);
    lfoShapeBox.addItem("Triangle", 2);
    lfoShapeBox.addItem("Square", 3);
    lfoShapeBox.addItem("S&H", 4);
    lfoShapeBox.setSelectedId(1, juce::dontSendNotification);
    lfoShapeBox.onChange = [this]
    {
        updatePreviewState();
        parameterChanged();
    };
    styleCombo(lfoShapeBox);

    addAndMakeVisible(lfoSyncBox);
    lfoSyncBox.addItem("Free", 1);
    lfoSyncBox.addItem("1/1", 2);
    lfoSyncBox.addItem("1/2", 3);
    lfoSyncBox.addItem("1/4", 4);
    lfoSyncBox.addItem("1/8", 5);
    lfoSyncBox.addItem("1/16", 6);
    lfoSyncBox.addItem("1/4T", 7);
    lfoSyncBox.addItem("1/8T", 8);
    lfoSyncBox.addItem("1/4.", 9);
    lfoSyncBox.addItem("1/8.", 10);
    lfoSyncBox.setSelectedId(1, juce::dontSendNotification);
    lfoSyncBox.onChange = [this]
    {
        updatePreviewState();
        parameterChanged();
    };
    styleCombo(lfoSyncBox);

    styleRotarySlider(lfoRateSlider, 0.01, 30.0, 1.0, 0.01, juce::Colour(Theme::accentRed));
    lfoRateSlider.setSkewFactorFromMidPoint(3.0);
    lfoRateSlider.setTextValueSuffix(" Hz");
    lfoRateSlider.onValueChange = [this]
    {
        updatePreviewState();
        parameterChanged();
    };
    addAndMakeVisible(lfoRateSlider);

    styleRotarySlider(lfoDepthSlider, 0.0, 1.0, 0.5, 0.01, juce::Colour(Theme::accentRed));
    lfoDepthSlider.onValueChange = [this] { parameterChanged(); };
    addAndMakeVisible(lfoDepthSlider);

    for (auto* label : { &lfoShapeLabel, &lfoRateLabel, &lfoDepthLabel, &lfoSyncLabel })
    {
        addAndMakeVisible(*label);
    }
    styleLabel(lfoShapeLabel, juce::Justification::centredLeft);
    styleLabel(lfoSyncLabel, juce::Justification::centredLeft);
    styleLabel(lfoRateLabel, juce::Justification::centred);
    styleLabel(lfoDepthLabel, juce::Justification::centred);

    addAndMakeVisible(envToggle);
    envToggle.setToggleState(false, juce::dontSendNotification);
    envToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(Theme::accentRed));
    envToggle.onClick = [this]
    {
        updateSectionVisibility();
        resized();
        repaint();
        parameterChanged();
    };

    styleHiddenSlider(attackSlider, 0.001, 10.0, 0.01, 0.001);
    attackSlider.setSkewFactorFromMidPoint(0.5);
    attackSlider.onValueChange = [this]
    {
        envelopeEditor.repaint();
        parameterChanged();
    };
    addChildComponent(attackSlider);

    styleHiddenSlider(decaySlider, 0.001, 10.0, 0.1, 0.001);
    decaySlider.setSkewFactorFromMidPoint(0.5);
    decaySlider.onValueChange = [this]
    {
        envelopeEditor.repaint();
        parameterChanged();
    };
    addChildComponent(decaySlider);

    styleHiddenSlider(sustainSlider, 0.0, 1.0, 0.7, 0.01);
    sustainSlider.onValueChange = [this]
    {
        envelopeEditor.repaint();
        parameterChanged();
    };
    addChildComponent(sustainSlider);

    styleHiddenSlider(releaseSlider, 0.001, 10.0, 0.3, 0.001);
    releaseSlider.setSkewFactorFromMidPoint(0.5);
    releaseSlider.onValueChange = [this]
    {
        envelopeEditor.repaint();
        parameterChanged();
    };
    addChildComponent(releaseSlider);

    addAndMakeVisible(envelopeEditor);

    updateSectionVisibility();
    updatePreviewState();
}

void ModulationPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::bgPanel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius);

    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Theme::cornerRadius, Theme::borderWidth);

    g.setColour(juce::Colour(Theme::accentRed));
    g.setFont(juce::Font(Theme::fontSectionHead).boldened());
    g.drawText("MODULATION", 12, 6, 120, 18, juce::Justification::centredLeft);

    auto drawSection = [&](juce::Rectangle<int> headerBounds, juce::Rectangle<int> bodyBounds,
                           const juce::String& title, bool enabled, bool expanded)
    {
        g.setColour(enabled ? juce::Colour(Theme::bgPanelHover) : juce::Colour(Theme::bgControl));
        g.fillRoundedRectangle(headerBounds.toFloat(), Theme::cornerRadius);
        g.setColour(enabled ? juce::Colour(Theme::borderActive) : juce::Colour(Theme::border));
        g.drawRoundedRectangle(headerBounds.toFloat().reduced(0.5f), Theme::cornerRadius, 1.0f);

        juce::Path chevron;
        const auto cx = static_cast<float>(headerBounds.getRight() - 14);
        const auto cy = static_cast<float>(headerBounds.getCentreY());
        if (expanded && enabled)
        {
            chevron.startNewSubPath(cx - 4.0f, cy - 2.0f);
            chevron.lineTo(cx, cy + 2.5f);
            chevron.lineTo(cx + 4.0f, cy - 2.0f);
        }
        else
        {
            chevron.startNewSubPath(cx - 2.0f, cy - 4.0f);
            chevron.lineTo(cx + 2.5f, cy);
            chevron.lineTo(cx - 2.0f, cy + 4.0f);
        }

        g.setColour(juce::Colour(Theme::textBright));
        g.setFont(juce::Font(Theme::fontValue).boldened());
        g.drawText(title, headerBounds.withTrimmedLeft(36).withTrimmedRight(26),
                   juce::Justification::centredLeft, false);
        g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        if (enabled && expanded && !bodyBounds.isEmpty())
        {
            g.setColour(juce::Colour(Theme::bgElevated));
            g.fillRoundedRectangle(bodyBounds.toFloat(), Theme::cornerRadius);
            g.setColour(juce::Colour(Theme::borderSubtle));
            g.drawRoundedRectangle(bodyBounds.toFloat().reduced(0.5f), Theme::cornerRadius, 1.0f);
        }
    };

    drawSection(lfoHeaderBounds, lfoBodyBounds, "LFO", lfoToggle.getToggleState(), lfoExpanded);
    drawSection(envHeaderBounds, envBodyBounds, "Envelope", envToggle.getToggleState(), envExpanded);
}

void ModulationPanel::resized()
{
    auto area = getLocalBounds().reduced(Theme::panelPadding);
    area.removeFromTop(24);

    constexpr int headerHeight = 24;
    const bool showLfoBody = lfoToggle.getToggleState() && lfoExpanded;
    const bool showEnvBody = envToggle.getToggleState() && envExpanded;
    const int availableBodyHeight = juce::jmax(0, area.getHeight() - headerHeight * 2 - Theme::sectionGap);

    int lfoBodyHeight = 0;
    int envBodyHeight = 0;
    if (showLfoBody && showEnvBody)
    {
        lfoBodyHeight = juce::jlimit(114, 138, availableBodyHeight / 2);
        envBodyHeight = juce::jmax(96, availableBodyHeight - lfoBodyHeight);
    }
    else if (showLfoBody)
    {
        lfoBodyHeight = availableBodyHeight;
    }
    else if (showEnvBody)
    {
        envBodyHeight = availableBodyHeight;
    }

    lfoHeaderBounds = area.removeFromTop(headerHeight);
    if (showLfoBody)
    {
        area.removeFromTop(4);
        lfoBodyBounds = area.removeFromTop(lfoBodyHeight);
        area.removeFromTop(Theme::sectionGap);
    }
    else
    {
        lfoBodyBounds = {};
        area.removeFromTop(Theme::sectionGap);
    }

    envHeaderBounds = area.removeFromTop(headerHeight);
    if (showEnvBody)
    {
        area.removeFromTop(4);
        envBodyBounds = area.removeFromTop(envBodyHeight);
    }
    else
    {
        envBodyBounds = {};
    }

    lfoToggle.setBounds(lfoHeaderBounds.withTrimmedLeft(8).withWidth(28));
    envToggle.setBounds(envHeaderBounds.withTrimmedLeft(8).withWidth(28));

    auto clearHidden = [](juce::Component& component)
    {
        component.setBounds({});
    };

    if (showLfoBody)
    {
        auto body = lfoBodyBounds.reduced(10, 10);
        lfoPreview.setBounds(body.removeFromTop(62));
        body.removeFromTop(8);

        auto leftColumn = body.removeFromLeft(88);
        body.removeFromLeft(Theme::controlGap);
        auto rateArea = body.removeFromLeft((body.getWidth() - Theme::controlGap) / 2);
        body.removeFromLeft(Theme::controlGap);
        auto depthArea = body;

        constexpr int labelHeight = 14;
        constexpr int comboHeight = 24;
        const int knobHeight = Theme::knobSize + 18;

        lfoShapeLabel.setBounds(leftColumn.removeFromTop(labelHeight));
        lfoShapeBox.setBounds(leftColumn.removeFromTop(comboHeight));
        leftColumn.removeFromTop(6);
        lfoSyncLabel.setBounds(leftColumn.removeFromTop(labelHeight));
        lfoSyncBox.setBounds(leftColumn.removeFromTop(comboHeight));

        lfoRateLabel.setBounds(rateArea.removeFromTop(labelHeight));
        lfoRateSlider.setBounds(rateArea.removeFromTop(knobHeight));

        lfoDepthLabel.setBounds(depthArea.removeFromTop(labelHeight));
        lfoDepthSlider.setBounds(depthArea.removeFromTop(knobHeight));
    }
    else
    {
        clearHidden(lfoPreview);
        clearHidden(lfoShapeLabel);
        clearHidden(lfoShapeBox);
        clearHidden(lfoSyncLabel);
        clearHidden(lfoSyncBox);
        clearHidden(lfoRateLabel);
        clearHidden(lfoRateSlider);
        clearHidden(lfoDepthLabel);
        clearHidden(lfoDepthSlider);
    }

    if (showEnvBody)
    {
        envelopeEditor.setBounds(envBodyBounds.reduced(10, 10));
    }
    else
    {
        clearHidden(envelopeEditor);
    }
}

void ModulationPanel::mouseUp(const juce::MouseEvent& event)
{
    if (lfoHeaderBounds.contains(event.getPosition()) && !lfoToggle.getBounds().contains(event.getPosition()))
    {
        lfoExpanded = !lfoExpanded;
        updateSectionVisibility();
        resized();
        repaint();
        return;
    }

    if (envHeaderBounds.contains(event.getPosition()) && !envToggle.getBounds().contains(event.getPosition()))
    {
        envExpanded = !envExpanded;
        updateSectionVisibility();
        resized();
        repaint();
    }
}

void ModulationPanel::mouseMove(const juce::MouseEvent& event)
{
    if (lfoHeaderBounds.contains(event.getPosition()) || envHeaderBounds.contains(event.getPosition()))
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ModulationPanel::parameterChanged()
{
    if (onParameterChanged)
        onParameterChanged();
}

void ModulationPanel::updateSectionVisibility()
{
    const bool showLfoBody = lfoToggle.getToggleState() && lfoExpanded;
    const bool showEnvBody = envToggle.getToggleState() && envExpanded;

    for (auto* component : { static_cast<juce::Component*>(&lfoPreview),
                             static_cast<juce::Component*>(&lfoShapeLabel),
                             static_cast<juce::Component*>(&lfoShapeBox),
                             static_cast<juce::Component*>(&lfoSyncLabel),
                             static_cast<juce::Component*>(&lfoSyncBox),
                             static_cast<juce::Component*>(&lfoRateLabel),
                             static_cast<juce::Component*>(&lfoRateSlider),
                             static_cast<juce::Component*>(&lfoDepthLabel),
                             static_cast<juce::Component*>(&lfoDepthSlider) })
    {
        component->setVisible(showLfoBody);
    }

    envelopeEditor.setVisible(showEnvBody);
}

void ModulationPanel::updatePreviewState()
{
    const auto syncDivision = getLFOSyncDivision();
    const bool freeRunning = syncDivision < 0.0f;
    const auto effectiveRate = freeRunning ? getLFORate()
                                           : bpm / (60.0f * juce::jmax(0.125f, syncDivision));

    lfoRateSlider.setEnabled(freeRunning);
    lfoRateSlider.setAlpha(freeRunning ? 1.0f : 0.45f);
    lfoRateLabel.setAlpha(freeRunning ? 1.0f : 0.55f);
    lfoPreview.setState(getLFOShape(), effectiveRate, lfoToggle.getToggleState());
}

float ModulationPanel::syncIdToDivision(int selectedId)
{
    switch (selectedId)
    {
        case 2:  return 4.0f;        // 1/1
        case 3:  return 2.0f;        // 1/2
        case 4:  return 1.0f;        // 1/4
        case 5:  return 0.5f;        // 1/8
        case 6:  return 0.25f;       // 1/16
        case 7:  return 2.0f / 3.0f; // 1/4T
        case 8:  return 1.0f / 3.0f; // 1/8T
        case 9:  return 1.5f;        // 1/4.
        case 10: return 0.75f;       // 1/8.
        case 1:
        default: return -1.0f;
    }
}

bool ModulationPanel::getLFOEnabled() const { return lfoToggle.getToggleState(); }

LFOShape ModulationPanel::getLFOShape() const
{
    switch (lfoShapeBox.getSelectedId())
    {
        case 2:  return LFOShape::Triangle;
        case 3:  return LFOShape::Square;
        case 4:  return LFOShape::SampleAndHold;
        default: return LFOShape::Sine;
    }
}

float ModulationPanel::getLFORate() const { return static_cast<float>(lfoRateSlider.getValue()); }
float ModulationPanel::getLFODepth() const { return static_cast<float>(lfoDepthSlider.getValue()); }
float ModulationPanel::getLFOSyncDivision() const { return syncIdToDivision(lfoSyncBox.getSelectedId()); }

void ModulationPanel::setBPM(float newBpm)
{
    bpm = juce::jlimit(20.0f, 300.0f, newBpm);
    updatePreviewState();
}

bool ModulationPanel::getEnvelopeEnabled() const { return envToggle.getToggleState(); }
float ModulationPanel::getAttack() const { return static_cast<float>(attackSlider.getValue()); }
float ModulationPanel::getDecay() const { return static_cast<float>(decaySlider.getValue()); }
float ModulationPanel::getSustain() const { return static_cast<float>(sustainSlider.getValue()); }
float ModulationPanel::getRelease() const { return static_cast<float>(releaseSlider.getValue()); }

} // namespace grainhex
