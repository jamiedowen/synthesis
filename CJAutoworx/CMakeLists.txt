# CJ Autoworx RevHead: an engine synthesizer. Built from the top-level
# CMakeLists.txt, which fetches JUCE once for both plugins.

option(CJ_AUTOWORX_BUILD_TESTS "Build the CJ Autoworx offline render test" OFF)

juce_add_binary_data(CJAutoworxData
    NAMESPACE CJBinary
    SOURCES Resources/jarrad.png)

juce_add_plugin(CJAutoworx
    COMPANY_NAME                "CJ Autoworx"
    PRODUCT_NAME                "CJ Autoworx RevHead"
    BUNDLE_ID                   com.cjautoworx.revhead
    PLUGIN_MANUFACTURER_CODE    CJAw
    PLUGIN_CODE                 RvHd
    FORMATS                     VST3 Standalone
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    ICON_BIG                    Resources/jarrad.png
    VST3_CATEGORIES             Instrument Synth
    COPY_PLUGIN_AFTER_BUILD     FALSE)

set(CJ_AUTOWORX_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp)

target_sources(CJAutoworx PRIVATE ${CJ_AUTOWORX_SOURCES})

target_compile_definitions(CJAutoworx PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(CJAutoworx
    PRIVATE
        CJAutoworxData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (CJ_AUTOWORX_BUILD_TESTS)
    juce_add_console_app(CJAutoworxRenderTest PRODUCT_NAME "CJAutoworxRenderTest")
    target_sources(CJAutoworxRenderTest PRIVATE tests/RenderTest.cpp ${CJ_AUTOWORX_SOURCES})
    target_compile_definitions(CJAutoworxRenderTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Build_Standalone=0
        JucePlugin_Name="CJ Autoworx RevHead" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(CJAutoworxRenderTest PRIVATE
        CJAutoworxData juce::juce_audio_utils juce::juce_audio_formats
        juce::juce_recommended_config_flags)
endif()
