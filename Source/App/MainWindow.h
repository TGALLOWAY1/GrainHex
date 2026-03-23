#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Audio/AudioEngine.h"
#include "SourceInput/SourceSampleManager.h"

namespace grainhex {

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(AudioEngine& engine, SourceSampleManager& sampleManager);
    ~MainWindow() override = default;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace grainhex
