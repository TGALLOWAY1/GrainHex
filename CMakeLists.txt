cmake_minimum_required(VERSION 3.22)
project(GrainHex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# GrainHex standalone app
juce_add_gui_app(GrainHex
    PRODUCT_NAME "GrainHex"
    VERSION "0.1.0"
    COMPANY_NAME "GrainHex"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
)

target_sources(GrainHex PRIVATE
    # App
    Source/App/Main.cpp
    Source/App/Application.cpp
    Source/App/MainWindow.cpp

    # Audio
    Source/Audio/AudioEngine.cpp
    Source/Audio/ParameterSmoother.cpp

    # Source Input
    Source/SourceInput/SourceSampleManager.cpp
    Source/SourceInput/AudioFileLoader.cpp
    Source/SourceInput/RootNoteDetector.cpp

    # Granular
    Source/Granular/WindowFunctions.cpp
    Source/Granular/Interpolator.cpp
    Source/Granular/GrainVoice.cpp
    Source/Granular/GrainScheduler.cpp
    Source/Granular/GranularEngine.cpp

    # UI
    Source/UI/MainEditor.cpp
    Source/UI/WaveformView.cpp
)

target_include_directories(GrainHex PRIVATE
    Source
)

target_compile_definitions(GrainHex PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:GrainHex,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:GrainHex,JUCE_VERSION>"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(GrainHex PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Tests (optional, build with -DGRAINHEX_BUILD_TESTS=ON)
option(GRAINHEX_BUILD_TESTS "Build unit tests" OFF)
if(GRAINHEX_BUILD_TESTS)
    enable_testing()
    add_executable(GrainHexTests
        Source/Tests/RootNoteDetectorTests.cpp
        Source/SourceInput/RootNoteDetector.cpp
    )
    target_include_directories(GrainHexTests PRIVATE Source)
    target_link_libraries(GrainHexTests PRIVATE
        juce::juce_audio_basics
        juce::juce_core
    )
    add_test(NAME GrainHexTests COMMAND GrainHexTests)
endif()
