# Midnight Grind: a hybrid semi-modular synth VST3 modelled on the Behringer GRIND.
# Builds on its own (cmake -S Grind) or as part of the repository's top-level project.
cmake_minimum_required(VERSION 3.22)

project(MidnightGrind VERSION 1.0.0 LANGUAGES C CXX)

if (NOT TARGET juce::juce_core)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    if (MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

option(MIDNIGHT_GRIND_BUILD_TESTS "Build the Midnight Grind offline render test" OFF)

#== Mutable Instruments Plaits / stmlib (MIT), vendored in ThirdParty/mutable
file(GLOB_RECURSE MUTABLE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/mutable/*.cc)
add_library(grind_mutable STATIC ${MUTABLE_SOURCES})
set_target_properties(grind_mutable PROPERTIES POSITION_INDEPENDENT_CODE ON)
# TEST selects stmlib's portable (non-ARM) code paths
target_compile_definitions(grind_mutable PUBLIC TEST)
target_include_directories(grind_mutable SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/mutable)
if (MSVC)
    target_compile_options(grind_mutable PRIVATE /W0)
else()
    target_compile_options(grind_mutable PRIVATE -w)
endif()

#== Plugin
juce_add_plugin(MidnightGrind
    COMPANY_NAME                "Midnight Audio"
    PRODUCT_NAME                "Midnight Grind"
    BUNDLE_ID                   com.midnightaudio.midnightgrind
    PLUGIN_MANUFACTURER_CODE    Mdnt
    PLUGIN_CODE                 MdGr
    FORMATS                     VST3 Standalone
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Instrument Synth
    COPY_PLUGIN_AFTER_BUILD     FALSE)

set(MIDNIGHT_GRIND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Performance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/dsp/Synth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/dsp/MacroOscillator.cpp)

target_sources(MidnightGrind PRIVATE ${MIDNIGHT_GRIND_SOURCES})

target_compile_definitions(MidnightGrind PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MidnightGrind
    PRIVATE
        grind_mutable
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

#== Offline render test
if (MIDNIGHT_GRIND_BUILD_TESTS)
    juce_add_console_app(MidnightGrindRenderTest PRODUCT_NAME "MidnightGrindRenderTest")
    target_sources(MidnightGrindRenderTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/GrindRenderTest.cpp ${MIDNIGHT_GRIND_SOURCES})
    target_compile_definitions(MidnightGrindRenderTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Build_Standalone=0
        JucePlugin_Name="Midnight Grind" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(MidnightGrindRenderTest PRIVATE
        grind_mutable juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
