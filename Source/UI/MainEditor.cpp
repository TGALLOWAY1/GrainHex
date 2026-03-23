#include "UI/MainEditor.h"

namespace grainhex {

MainEditor::MainEditor(AudioEngine& engine, SourceSampleManager& manager)
    : audioEngine(engine), sampleManager(manager)
{
    setLookAndFeel(&lookAndFeel);

    // === TOP SECTION: Waveform + Browser + Transport ===

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

    // Sample browser
    addAndMakeVisible(sampleBrowser);
    sampleBrowser.onLoadSample = [this](const FactorySample& sample) { loadFactorySample(sample); };
    sampleBrowser.onPreviewSample = [this](const FactorySample& sample) { loadFactorySample(sample); };

    // Transport buttons
    addAndMakeVisible(playButton);
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Theme::accentGreen));
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colour(Theme::bgDarkest));
    playButton.onClick = [this]
    {
        audioEngine.setSineTestEnabled(false);
        audioEngine.play();
        updateTransportButtons();
    };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this]
    {
        audioEngine.stop();
        updateTransportButtons();
    };

    addAndMakeVisible(loadButton);
    loadButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load Sample", juce::File{}, sampleManager.getSupportedFormatsWildcard());
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                    loadFile(file);
            });
    };

    addAndMakeVisible(loopButton);
    loopButton.setToggleState(true, juce::dontSendNotification);
    loopButton.onClick = [this] { audioEngine.setLoopEnabled(loopButton.getToggleState()); };

    addAndMakeVisible(granularToggle);
    granularToggle.setToggleState(false, juce::dontSendNotification);
    granularToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(Theme::accentGreen));
    granularToggle.onClick = [this]
    {
        bool enabled = granularToggle.getToggleState();
        audioEngine.setGranularEnabled(enabled);
        granularPanel.setVisible(enabled);
        effectsPanel.setVisible(enabled);
        modulationPanel.setVisible(enabled);
        resized();
    };

    // Info labels
    addAndMakeVisible(rootNoteLabel);
    rootNoteLabel.setJustificationType(juce::Justification::centredRight);
    rootNoteLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::accentCyan));
    rootNoteLabel.setFont(juce::Font(18.0f).boldened());

    addAndMakeVisible(sampleInfoLabel);
    sampleInfoLabel.setJustificationType(juce::Justification::centredLeft);
    sampleInfoLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textDim));

    addAndMakeVisible(midiActivityLabel);
    midiActivityLabel.setText("MIDI", juce::dontSendNotification);
    midiActivityLabel.setJustificationType(juce::Justification::centred);
    midiActivityLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textMuted));
    midiActivityLabel.setFont(juce::Font(11.0f));

    // === CENTER: Granular panel ===
    addAndMakeVisible(granularPanel);
    granularPanel.setVisible(false);
    granularPanel.onParameterChanged = [this] { pushGranularParams(); };

    // === BOTTOM-LEFT: Sub panel ===
    addAndMakeVisible(subPanel);
    subPanel.onParameterChanged = [this] { pushSubParams(); };

    // === RIGHT SIDEBAR: Effects + Modulation ===
    addAndMakeVisible(effectsPanel);
    effectsPanel.setVisible(false);
    effectsPanel.onParameterChanged = [this] { pushEffectsParams(); };

    addAndMakeVisible(modulationPanel);
    modulationPanel.setVisible(false);
    modulationPanel.onParameterChanged = [this] { pushModulationParams(); };

    // === BOTTOM-RIGHT: Resample panel ===
    addAndMakeVisible(resamplePanel);
    resamplePanel.onResample = [this] { handleResample(); };
    resamplePanel.onRevertTo = [this](int index) { handleRevertTo(index); };
    resamplePanel.onUndo = [this] { handleUndo(); };
    resamplePanel.onExport = [this](int index) { handleExportEntry(index); };
    resamplePanel.onExportCurrent = [this] { handleExportCurrent(); };
    resamplePanel.onClearHistory = [this]
    {
        audioEngine.getResampleEngine().clearHistory();
        resamplePanel.updateFromEngine(audioEngine.getResampleEngine());
    };

    // === FOOTER: Master level, Mono lock, Export, Status ===
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
    volumeLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textDim));
    volumeLabel.setFont(juce::Font(11.0f));

    addAndMakeVisible(monoLockToggle);
    monoLockToggle.setToggleState(false, juce::dontSendNotification);
    monoLockToggle.onClick = [this]
    {
        audioEngine.setMonoLock(monoLockToggle.getToggleState());
    };

    addAndMakeVisible(exportButton);
    exportButton.onClick = [this] { handleExportCurrent(); };

    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textDim));
    statusLabel.setFont(juce::Font(11.0f));

    updateStatusLabel("Drop a sample or pick one from the browser");
    updateTransportButtons();

    // Setup MIDI note callback
    audioEngine.getMidiManager().setNoteCallback(
        [this](int midiNote, float /*velocity*/, bool isNoteOn)
        {
            auto& ge = audioEngine.getGranularEngine();
            auto& mm = audioEngine.getMidiManager();
            auto& me = audioEngine.getModulationEngine();

            if (isNoteOn)
            {
                float pitchOffset = mm.getPitchOffsetFromRoot(midiNote);
                ge.setPitchSemitones(pitchOffset);
                me.getEnvelope().noteOn();
            }
            else
            {
                me.getEnvelope().noteOff();
            }
        });

    // PRD layout: wider for sidebar, shorter for horizontal layout
    setSize(1100, 780);

    // Start timer (30 fps)
    startTimerHz(30);

    // Preload a default sample for quick auditioning, but do not auto-enable playback.
    auto* defaultSample = sampleBrowser.getDefaultSample();
    if (defaultSample != nullptr)
    {
        juce::MessageManager::callAsync([this, defaultSample]
        {
            loadFactorySample(*defaultSample);
        });
    }
}

MainEditor::~MainEditor()
{
    setLookAndFeel(nullptr);
}

void MainEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(Theme::bgDark));

    // Title bar
    g.setColour(juce::Colour(Theme::accentGreen));
    g.setFont(juce::Font(Theme::fontTitle).boldened());
    g.drawText("GrainHex", 12, 6, 160, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colour(Theme::textMuted));
    g.setFont(juce::Font(Theme::fontLabel));
    g.drawText("v1.0", 130, 14, 50, 18, juce::Justification::centredLeft);

    // Update playhead and grain positions
    waveformView.setPlayheadPosition(audioEngine.getPlayheadPosition());
    if (audioEngine.isGranularEnabled())
        waveformView.setActiveGrainPositions(audioEngine.getGranularEngine().getActiveGrainPositions());
    else
        waveformView.setActiveGrainPositions({});
}

void MainEditor::resized()
{
    auto area = getLocalBounds().reduced(8);

    // === TITLE BAR (40px) ===
    auto titleBar = area.removeFromTop(36);
    // Root note + MIDI indicator on right
    midiActivityLabel.setBounds(titleBar.removeFromRight(40));
    rootNoteLabel.setBounds(titleBar.removeFromRight(100));

    area.removeFromTop(4);

    // === FOOTER (36px) ===
    auto footer = area.removeFromBottom(32);
    volumeLabel.setBounds(footer.removeFromLeft(42));
    volumeSlider.setBounds(footer.removeFromLeft(120));
    footer.removeFromLeft(8);
    monoLockToggle.setBounds(footer.removeFromLeft(65));
    footer.removeFromLeft(8);
    exportButton.setBounds(footer.removeFromLeft(85));
    footer.removeFromLeft(12);
    statusLabel.setBounds(footer);

    area.removeFromBottom(4);

    // === SAMPLE INFO ROW ===
    auto infoRow = area.removeFromBottom(20);
    sampleInfoLabel.setBounds(infoRow);
    area.removeFromBottom(4);

    // === RIGHT SIDEBAR (effects + modulation, 280px) ===
    const int sidebarWidth = 280;
    bool sidebarVisible = effectsPanel.isVisible() || modulationPanel.isVisible();

    juce::Rectangle<int> sidebarArea;
    if (sidebarVisible)
    {
        sidebarArea = area.removeFromRight(sidebarWidth);
        area.removeFromRight(6);
    }

    // === TOP SECTION: Waveform (left) + Browser (right) ===
    auto topSection = area.removeFromTop(180);
    {
        auto browserArea = topSection.removeFromRight(200);
        topSection.removeFromRight(6);
        sampleBrowser.setBounds(browserArea);
        waveformView.setBounds(topSection);
    }
    area.removeFromTop(6);

    // === TRANSPORT ROW ===
    auto transportRow = area.removeFromTop(30);
    playButton.setBounds(transportRow.removeFromLeft(55));
    transportRow.removeFromLeft(4);
    stopButton.setBounds(transportRow.removeFromLeft(50));
    transportRow.removeFromLeft(4);
    loopButton.setBounds(transportRow.removeFromLeft(60));
    transportRow.removeFromLeft(4);
    granularToggle.setBounds(transportRow.removeFromLeft(85));
    transportRow.removeFromLeft(12);
    loadButton.setBounds(transportRow.removeFromLeft(55));

    area.removeFromTop(6);

    // === CENTER: Granular controls ===
    if (granularPanel.isVisible())
    {
        granularPanel.setBounds(area.removeFromTop(170));
        area.removeFromTop(6);
    }

    // === BOTTOM: Sub tuner (left) + Resample history (right) ===
    {
        auto bottomArea = area;
        int halfWidth = bottomArea.getWidth() / 2 - 3;

        auto subArea = bottomArea.removeFromLeft(halfWidth);
        bottomArea.removeFromLeft(6);
        auto resampleArea = bottomArea;

        subPanel.setBounds(subArea);
        resamplePanel.setBounds(resampleArea);
    }

    // === SIDEBAR layout ===
    if (sidebarVisible)
    {
        int availableHeight = sidebarArea.getHeight();
        int effectsHeight = effectsPanel.isVisible() ? (availableHeight / 2 - 3) : 0;

        if (effectsPanel.isVisible() && !modulationPanel.isVisible())
            effectsHeight = availableHeight;

        if (effectsPanel.isVisible())
        {
            effectsPanel.setBounds(sidebarArea.removeFromTop(effectsHeight));
            sidebarArea.removeFromTop(6);
        }
        if (modulationPanel.isVisible())
        {
            modulationPanel.setBounds(sidebarArea);
        }
    }
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
            break;
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

            audioEngine.stop();
            audioEngine.setSourceSample(buffer, meta.originalSampleRate);

            if (meta.rootNote.midiNote >= 0)
                audioEngine.getMidiManager().setSourceRootNote(meta.rootNote.midiNote);

            waveformView.setSource(buffer, meta.originalSampleRate);
            waveformView.setLoopRegion(0, meta.numSamples);

            rootNoteLabel.setText(meta.rootNote.getNoteName(), juce::dontSendNotification);

            juce::String info = meta.filename
                + " | " + juce::String(meta.lengthSeconds, 2) + "s"
                + " | " + juce::String(meta.numChannels) + "ch"
                + " | " + juce::String(static_cast<int>(meta.originalSampleRate)) + " Hz";
            sampleInfoLabel.setText(info, juce::dontSendNotification);
            updateTransportButtons();
        }

        updateStatusLabel(message);
    });
}

void MainEditor::loadFactorySample(const FactorySample& sample)
{
    audioEngine.stop();
    audioEngine.setSourceSample(sample.buffer, sample.sampleRate);

    if (sample.rootMidiNote >= 0)
        audioEngine.getMidiManager().setSourceRootNote(sample.rootMidiNote);

    waveformView.setSource(sample.buffer, sample.sampleRate);
    waveformView.setLoopRegion(0, sample.buffer->getNumSamples());

    // Derive note name from MIDI note
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int noteIdx = sample.rootMidiNote % 12;
    int octave = (sample.rootMidiNote / 12) - 1;
    juce::String noteName = juce::String(noteNames[noteIdx]) + juce::String(octave);
    rootNoteLabel.setText(noteName, juce::dontSendNotification);

    double lengthSec = static_cast<double>(sample.buffer->getNumSamples()) / sample.sampleRate;
    juce::String info = sample.name + " (" + sample.category + ")"
        + " | " + juce::String(lengthSec, 2) + "s"
        + " | " + juce::String(static_cast<int>(sample.sampleRate)) + " Hz";
    sampleInfoLabel.setText(info, juce::dontSendNotification);

    updateTransportButtons();
    updateStatusLabel("Loaded factory sample: " + sample.name);
}

void MainEditor::updateStatusLabel(const juce::String& text)
{
    statusLabel.setText(text, juce::dontSendNotification);
}

void MainEditor::updateTransportButtons()
{
    auto isPlaying = audioEngine.getTransportState() == TransportState::Playing;
    playButton.setEnabled(!isPlaying);
    stopButton.setEnabled(isPlaying);
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

void MainEditor::pushEffectsParams()
{
    auto& ec = audioEngine.getEffectsChain();

    ec.getDistortion().setEnabled(effectsPanel.getDistortionEnabled());
    ec.getDistortion().setMode(effectsPanel.getDistortionMode());
    ec.getDistortion().setDrive(effectsPanel.getDrive());
    ec.getDistortion().setMix(effectsPanel.getDistortionMix());

    ec.getFilter().setEnabled(effectsPanel.getFilterEnabled());
    ec.getFilter().setMode(effectsPanel.getFilterMode());
    ec.getFilter().setCutoff(effectsPanel.getCutoff());
    ec.getFilter().setResonance(effectsPanel.getResonance());
    ec.getFilter().setEnvelopeAmount(effectsPanel.getEnvelopeAmount());
}

void MainEditor::pushModulationParams()
{
    auto& me = audioEngine.getModulationEngine();

    me.getLFO().setEnabled(modulationPanel.getLFOEnabled());
    me.getLFO().setShape(modulationPanel.getLFOShape());
    me.getLFO().setRate(modulationPanel.getLFORate());
    me.getLFO().setDepth(modulationPanel.getLFODepth());

    me.getEnvelope().setEnabled(modulationPanel.getEnvelopeEnabled());
    me.getEnvelope().setAttack(modulationPanel.getAttack());
    me.getEnvelope().setDecay(modulationPanel.getDecay());
    me.getEnvelope().setSustain(modulationPanel.getSustain());
    me.getEnvelope().setRelease(modulationPanel.getRelease());
}

void MainEditor::timerCallback()
{
    updateTransportButtons();

    // Update pitch display from audio thread snapshot
    auto pitch = audioEngine.getDetectedPitch();
    subPanel.setDetectedPitch(pitch);

    // Update resample capture progress
    auto& re = audioEngine.getResampleEngine();
    if (re.isCapturing())
        resamplePanel.setCaptureProgress(re.getCaptureProgress());

    // Update MIDI activity indicator
    if (audioEngine.getMidiManager().consumeActivity())
        midiActivityLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::accentGreen));
    else
        midiActivityLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textMuted));
}

void MainEditor::handleResample()
{
    auto& re = audioEngine.getResampleEngine();
    if (re.isCapturing())
        return;

    if (!audioEngine.isGranularEnabled())
    {
        updateStatusLabel("Enable Granular mode before resampling");
        return;
    }

    if (re.oldestNeverExported())
        updateStatusLabel("Warning: oldest history entry was never exported and will be removed");

    float lengthSec = resamplePanel.getCaptureLengthSeconds();
    double sr = audioEngine.getDeviceSampleRate();

    re.startCapture(sr, static_cast<double>(lengthSec),
        [this](const ResampleHistoryEntry& entry)
        {
            reloadFromHistory(&entry);
            resamplePanel.updateFromEngine(audioEngine.getResampleEngine());
            resamplePanel.setCaptureProgress(0.0f);
            updateStatusLabel("Resample #" + juce::String(entry.id) + " captured — "
                             + juce::String(entry.getLengthSeconds(), 1) + "s"
                             + " | Root: " + entry.rootNote.getNoteName());
        });

    updateStatusLabel("Capturing resample...");
}

void MainEditor::handleRevertTo(int index)
{
    auto& re = audioEngine.getResampleEngine();
    auto* entry = re.revertTo(index);
    if (entry != nullptr)
    {
        reloadFromHistory(entry);
        resamplePanel.updateFromEngine(re);
        updateStatusLabel("Reverted to resample #" + juce::String(entry->id));
    }
}

void MainEditor::handleUndo()
{
    auto& re = audioEngine.getResampleEngine();
    auto* entry = re.undo();
    if (entry != nullptr)
    {
        reloadFromHistory(entry);
        resamplePanel.updateFromEngine(re);
        updateStatusLabel("Undo — reverted to resample #" + juce::String(entry->id));
    }
}

void MainEditor::handleExportEntry(int index)
{
    auto& re = audioEngine.getResampleEngine();
    auto* entry = re.getHistoryEntry(index);
    if (entry == nullptr || entry->buffer == nullptr)
        return;

    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Resample as WAV",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile("GrainHex_resample_" + juce::String(entry->id) + ".wav"),
        "*.wav");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, index, entry, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File())
                return;

            auto bitDepth = resamplePanel.getBitDepth();
            bool ok = WavExporter::exportToWav(*entry->buffer, entry->sampleRate, file, bitDepth);

            if (ok)
            {
                audioEngine.getResampleEngine().markExported(index);
                resamplePanel.updateFromEngine(audioEngine.getResampleEngine());
                updateStatusLabel("Exported: " + file.getFileName());
            }
            else
            {
                updateStatusLabel("Export failed: " + file.getFileName());
            }
        });
}

void MainEditor::handleExportCurrent()
{
    auto& re = audioEngine.getResampleEngine();
    int current = re.getCurrentIndex();
    if (current >= 0)
        handleExportEntry(current);
}

void MainEditor::reloadFromHistory(const ResampleHistoryEntry* entry)
{
    if (entry == nullptr || entry->buffer == nullptr)
        return;

    audioEngine.stop();
    audioEngine.setSourceSample(entry->buffer, entry->sampleRate);

    if (entry->rootNote.midiNote >= 0)
        audioEngine.getMidiManager().setSourceRootNote(entry->rootNote.midiNote);

    waveformView.setSource(entry->buffer, entry->sampleRate);
    waveformView.setLoopRegion(0, entry->buffer->getNumSamples());

    rootNoteLabel.setText(entry->rootNote.getNoteName(), juce::dontSendNotification);

    juce::String info = "Resample #" + juce::String(entry->id)
        + " | " + juce::String(entry->getLengthSeconds(), 2) + "s"
        + " | 2ch"
        + " | " + juce::String(static_cast<int>(entry->sampleRate)) + " Hz";
    sampleInfoLabel.setText(info, juce::dontSendNotification);
    updateTransportButtons();
}

} // namespace grainhex
