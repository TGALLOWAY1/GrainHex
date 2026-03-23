#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Audio/AudioEngine.h"
#include "SourceInput/SourceSampleManager.h"

namespace grainhex {

class MainWindow;

class Application : public juce::JUCEApplication
{
public:
    Application() = default;

    const juce::String getApplicationName() override    { return "GrainHex"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise(const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;

    AudioEngine& getAudioEngine() { return audioEngine; }
    SourceSampleManager& getSampleManager() { return sampleManager; }

private:
    AudioEngine audioEngine;
    SourceSampleManager sampleManager;
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace grainhex
