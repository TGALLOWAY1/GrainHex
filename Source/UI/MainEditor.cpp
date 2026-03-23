#include "UI/MainEditor.h"

namespace grainhex {

MainEditor::MainEditor(AudioEngine& engine, SourceSampleManager& manager)
    : audioEngine(engine), sampleManager(manager)
{
    // Waveform
    addAndMakeVisible(waveformView);
    waveformView.setLoopRegionCallback([this](float startNorm, float endNorm)
    {
        auto meta = sampleManager.getMetadata();
        if (meta.numSamples > 0)
        {
            int64_t start = static_cast<int64_t>(startNorm * static_cast<float>(meta.numSamples));
            int64_t end = static_cast<int64_t>(endNorm * static_cast<float>(meta.numSamples));
            audioEngine.setLoopRegion(start, end);
            waveformView.setLoopRegion(start, end);
        }
    });

    // Play button
    addAndMakeVisible(playButton);
    playButton.onClick = [this]
    {
        audioEngine.setSineTestEnabled(false);
        audioEngine.play();
    };

    // Stop button
    addAndMakeVisible(stopButton);
    stopButton.onClick = [this] { audioEngine.stop(); };

    // Sine test button
    addAndMakeVisible(sineTestButton);
    sineTestButton.onClick = [this]
    {
        bool enabled = !audioEngine.isSineTestEnabled();
        audioEngine.setSineTestEnabled(enabled);
        sineTestButton.setButtonText(enabled ? "Sine: ON" : "Sine Test");
    };

    // Load button
    addAndMakeVisible(loadButton);
    loadButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load Sample",
            juce::File{},
            sampleManager.getSupportedFormatsWildcard());

        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                    loadFile(file);
            });
    };

    // Loop toggle
    addAndMakeVisible(loopButton);
    loopButton.setToggleState(true, juce::dontSendNotification);
    loopButton.onClick = [this]
    {
        audioEngine.setLoopEnabled(loopButton.getToggleState());
    };

    // Granular toggle
    addAndMakeVisible(granularToggle);
    granularToggle.setToggleState(false, juce::dontSendNotification);
    granularToggle.onClick = [this]
    {
        bool enabled = granularToggle.getToggleState();
        audioEngine.setGranularEnabled(enabled);
        granularPanel.setVisible(enabled);
        resized();
    };

    // Granular panel
    addAndMakeVisible(granularPanel);
    granularPanel.setVisible(false);
    granularPanel.onParameterChanged = [this] { pushGranularParams(); };

    // Sub panel
    addAndMakeVisible(subPanel);
    subPanel.onParameterChanged = [this] { pushSubParams(); };

    // Volume
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8, juce::dontSendNotification);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeSlider.onValueChange = [this]
    {
        audioEngine.setMasterVolume(static_cast<float>(volumeSlider.getValue()));
    };
    addAndMakeVisible(volumeLabel);

    // Status labels
    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(rootNoteLabel);
    rootNoteLabel.setJustificationType(juce::Justification::centredRight);
    rootNoteLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ccff));
    rootNoteLabel.setFont(juce::Font(18.0f));

    addAndMakeVisible(sampleInfoLabel);
    sampleInfoLabel.setJustificationType(juce::Justification::centredLeft);
    sampleInfoLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    updateStatusLabel("Drop a WAV, AIFF, or FLAC file to begin");

    setSize(900, 850);

    // Start timer for pitch display updates (30 fps)
    startTimerHz(30);
}

void MainEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));

    // Title
    g.setColour(juce::Colour(0xff16c784));
    g.setFont(juce::Font(24.0f).boldened());
    g.drawText("GrainHex", 10, 8, 200, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("v0.3 — Phase 3", 160, 14, 120, 20, juce::Justification::centredLeft);

    // Update playhead and grain positions
    waveformView.setPlayheadPosition(audioEngine.getPlayheadPosition());
    if (audioEngine.isGranularEnabled())
        waveformView.setActiveGrainPositions(audioEngine.getGranularEngine().getActiveGrainPositions());
    else
        waveformView.setActiveGrainPositions({});
}

void MainEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Top bar: title area
    auto topBar = area.removeFromTop(40);

    // Root note display (top right)
    rootNoteLabel.setBounds(topBar.removeFromRight(120));

    area.removeFromTop(5);

    // Waveform
    waveformView.setBounds(area.removeFromTop(200));

    area.removeFromTop(8);

    // Transport controls row
    auto controlRow = area.removeFromTop(36);
    playButton.setBounds(controlRow.removeFromLeft(70));
    controlRow.removeFromLeft(5);
    stopButton.setBounds(controlRow.removeFromLeft(70));
    controlRow.removeFromLeft(5);
    loopButton.setBounds(controlRow.removeFromLeft(70));
    controlRow.removeFromLeft(5);
    granularToggle.setBounds(controlRow.removeFromLeft(90));
    controlRow.removeFromLeft(10);
    sineTestButton.setBounds(controlRow.removeFromLeft(90));
    controlRow.removeFromLeft(10);
    loadButton.setBounds(controlRow.removeFromLeft(100));

    controlRow.removeFromLeft(20);
    volumeLabel.setBounds(controlRow.removeFromLeft(30));
    volumeSlider.setBounds(controlRow.removeFromLeft(120));

    area.removeFromTop(8);

    // Granular panel (visible when enabled)
    if (granularPanel.isVisible())
    {
        granularPanel.setBounds(area.removeFromTop(170));
        area.removeFromTop(8);
    }

    // Sub panel (always visible)
    subPanel.setBounds(area.removeFromTop(150));
    area.removeFromTop(8);

    // Sample info row
    auto infoRow = area.removeFromTop(24);
    sampleInfoLabel.setBounds(infoRow);

    area.removeFromTop(4);

    // Status bar
    auto statusRow = area.removeFromTop(24);
    statusLabel.setBounds(statusRow);
}

bool MainEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
        if (sampleManager.isFormatSupported(juce::File(f)))
            return true;
    return false;
}

void MainEditor::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    for (auto& f : files)
    {
        juce::File file(f);
        if (sampleManager.isFormatSupported(file))
        {
            loadFile(file);
            break; // Only load first valid file
        }
    }
}

void MainEditor::loadFile(const juce::File& file)
{
    updateStatusLabel("Loading: " + file.getFileName() + "...");

    sampleManager.loadFileAsync(file, [this](bool success, const juce::String& message)
    {
        if (success)
        {
            auto buffer = sampleManager.getCurrentBuffer();
            auto meta = sampleManager.getMetadata();

            // Publish to audio engine
            audioEngine.setSourceSample(buffer, meta.originalSampleRate);

            // Update waveform
            waveformView.setSource(buffer, meta.originalSampleRate);
            waveformView.setLoopRegion(0, meta.numSamples);

            // Update info labels
            rootNoteLabel.setText(meta.rootNote.getNoteName(), juce::dontSendNotification);

            juce::String info = meta.filename
                + " | " + juce::String(meta.lengthSeconds, 2) + "s"
                + " | " + juce::String(meta.numChannels) + "ch"
                + " | " + juce::String(static_cast<int>(meta.originalSampleRate)) + " Hz";
            sampleInfoLabel.setText(info, juce::dontSendNotification);
        }

        updateStatusLabel(message);
    });
}

void MainEditor::updateStatusLabel(const juce::String& text)
{
    statusLabel.setText(text, juce::dontSendNotification);
}

void MainEditor::pushGranularParams()
{
    auto& ge = audioEngine.getGranularEngine();
    ge.setGrainSize(granularPanel.getGrainSize());
    ge.setGrainCount(granularPanel.getGrainCount());
    ge.setPosition(granularPanel.getPosition());
    ge.setSpray(granularPanel.getSpray());
    ge.setPitchSemitones(granularPanel.getPitchSemitones());
    ge.setPitchQuantize(granularPanel.getPitchQuantize());
    ge.setWindowShape(granularPanel.getWindowShape());
    ge.setDirection(granularPanel.getDirection());
    ge.setSpread(granularPanel.getSpread());
}

void MainEditor::pushSubParams()
{
    auto& se = audioEngine.getSubEngine();
    se.setEnabled(subPanel.getEnabled());
    se.setLevel(subPanel.getLevel());
    se.setWaveform(subPanel.getWaveform());
    se.setTuningMode(subPanel.getTuningMode());
    se.setPitchSnapMode(subPanel.getPitchSnapMode());
    se.setSmoothingSpeed(subPanel.getSmoothingSpeed());
    se.setOctaveOffset(subPanel.getOctaveOffset());
    se.setManualNote(subPanel.getManualMidiNote());
    se.setGranularHPFreq(subPanel.getGranularHPFreq());
    se.setSubLPFreq(subPanel.getSubLPFreq());
}

void MainEditor::timerCallback()
{
    // Update pitch display from audio thread snapshot
    auto pitch = audioEngine.getDetectedPitch();
    subPanel.setDetectedPitch(pitch);
}

} // namespace grainhex
