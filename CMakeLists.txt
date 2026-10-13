cmake_minimum_required(VERSION 3.22)

project(MidnightEdge VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

if (MSVC)
    # Static runtime so the .vst3 runs on machines without the VC++ redistributable
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(MIDNIGHT_EDGE_BUILD_TESTS "Build the offline render test" OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MidnightEdge
    COMPANY_NAME                "Midnight Audio"
    PRODUCT_NAME                "Midnight Edge"
    BUNDLE_ID                   com.midnightaudio.midnightedge
    PLUGIN_MANUFACTURER_CODE    Mdnt
    PLUGIN_CODE                 MdEg
    FORMATS                     VST3 Standalone
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Instrument Drum
    COPY_PLUGIN_AFTER_BUILD     FALSE)

set(MIDNIGHT_EDGE_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(MidnightEdge PRIVATE ${MIDNIGHT_EDGE_SOURCES})

target_compile_definitions(MidnightEdge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MidnightEdge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (MIDNIGHT_EDGE_BUILD_TESTS)
    juce_add_console_app(MidnightEdgeRenderTest PRODUCT_NAME "MidnightEdgeRenderTest")
    target_sources(MidnightEdgeRenderTest PRIVATE tests/RenderTest.cpp ${MIDNIGHT_EDGE_SOURCES})
    target_compile_definitions(MidnightEdgeRenderTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Build_Standalone=0
        JucePlugin_Name="Midnight Edge" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(MidnightEdgeRenderTest PRIVATE
        juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()

# Midnight Grind (hybrid semi-modular synth) lives in its own folder
add_subdirectory(Grind)
