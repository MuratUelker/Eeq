cmake_minimum_required(VERSION 3.22)
project(Eeq VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Eeq
    COMPANY_NAME "EeqAudio"
    PLUGIN_MANUFACTURER_CODE EeqA
    PLUGIN_CODE Eeq1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Eeq"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES "Fx" "EQ"
)

target_sources(Eeq PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/BiquadFilter.cpp
    Source/DSP/Equalizer.cpp
    Source/DSP/SpectrumAnalyzer.cpp
)

target_include_directories(Eeq PRIVATE Source)

target_compile_definitions(Eeq PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Eeq
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_core
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_audio_processors
)

# DSP Tests (standalone, no GUI)
add_executable(EeqTests
    Tests/DSPTest.cpp
)
target_link_libraries(EeqTests PRIVATE
    juce::juce_core
    juce::juce_dsp
)
target_include_directories(EeqTests PRIVATE Source)
target_sources(EeqTests PRIVATE
    Source/DSP/BiquadFilter.cpp
    Source/DSP/Equalizer.cpp
    Source/DSP/SpectrumAnalyzer.cpp
)
