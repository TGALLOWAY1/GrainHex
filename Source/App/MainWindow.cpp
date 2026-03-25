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
    setResizable(false, false);
    setResizeLimits(1100, 780, 1100, 780);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace grainhex
