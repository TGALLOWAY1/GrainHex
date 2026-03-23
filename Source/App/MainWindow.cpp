#include "App/MainWindow.h"
#include "UI/MainEditor.h"

namespace grainhex {

MainWindow::MainWindow(AudioEngine& engine, SourceSampleManager& sampleManager)
    : juce::DocumentWindow("GrainHex",
                            juce::Colour(0xff0d0d1a),
                            juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new MainEditor(engine, sampleManager), true);
    setResizable(true, true);
    setResizeLimits(900, 600, 2400, 1400);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace grainhex
