#include "App/Application.h"
#include "App/MainWindow.h"

namespace grainhex {

void Application::initialise(const juce::String& /*commandLine*/)
{
    audioEngine.initialise();
    mainWindow = std::make_unique<MainWindow>(audioEngine, sampleManager);
}

void Application::shutdown()
{
    mainWindow.reset();
    audioEngine.shutdown();
}

void Application::systemRequestedQuit()
{
    quit();
}

} // namespace grainhex
