cmake_minimum_required(VERSION 3.22)
project(BreezeMod VERSION 1.6.0 LANGUAGES C CXX)

# Retain the exact JUCE acquisition strategy of the supplied compiled StopDial template.
include(FetchContent)
if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${JUCE_PATH}" JUCE)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BreezeMod
    COMPANY_NAME "Skell Audio"
    BUNDLE_ID "audio.skell.breezemod"
    PLUGIN_MANUFACTURER_CODE SkAu
    PLUGIN_CODE Bz01
    PRODUCT_NAME "BreezeMod"
    DESCRIPTION "Fan-inspired four-mode stereo modulation"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3 Standalone)

juce_generate_juce_header(BreezeMod)
juce_add_binary_data(BreezeModAssets SOURCES
    Assets/fan_body.png Assets/blades.png Assets/blades_blur.png
    Assets/grille.png Assets/hub_cap.png Assets/knob_chrome.png
    Assets/knob_blue.png Assets/key_strip.png Assets/small_key.png
    Assets/slider_thumb.png)

target_sources(BreezeMod PRIVATE
    Source/PluginProcessor.h Source/PluginProcessor.cpp
    Source/PluginEditor.h Source/PluginEditor.cpp
    Source/dsp/Lfo.h Source/dsp/ModulationMath.h Source/dsp/ModulationSmoother.h Source/dsp/ModDelay.h
    Source/dsp/Saturator.h Source/dsp/RotarySim.h
    Source/ui/Theme.h Source/ui/LookAndFeel.h Source/ui/LookAndFeel.cpp
    Source/ui/FanView.h Source/ui/FanView.cpp
    Source/ui/ChromeKnob.h Source/ui/ChromeKnob.cpp
    Source/ui/DepthSlider.h Source/ui/DepthSlider.cpp
    Source/ui/KeyButton.h Source/ui/KeyButton.cpp
    Source/ui/TopBar.h Source/ui/TopBar.cpp
    Source/ui/PresetBar.h Source/ui/PresetBar.cpp
    Source/ui/LfoScope.h Source/ui/LfoScope.cpp
    Source/ui/StatusPanels.h Source/ui/StatusPanels.cpp)

target_compile_features(BreezeMod PRIVATE cxx_std_17)
target_compile_definitions(BreezeMod PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BreezeMod PRIVATE
    BreezeModAssets juce::juce_audio_utils juce::juce_audio_processors
    juce::juce_gui_extra juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

option(BREEZEMOD_BUILD_TESTS "Build JUCE DSP smoke tests" ON)
if(BREEZEMOD_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(BreezeModDspTests PRODUCT_NAME "BreezeMod DSP Tests")
    juce_generate_juce_header(BreezeModDspTests)
    target_sources(BreezeModDspTests PRIVATE tests/DSPTests.cpp)
    target_compile_features(BreezeModDspTests PRIVATE cxx_std_17)
    target_compile_definitions(BreezeModDspTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(BreezeModDspTests PRIVATE juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
    add_test(NAME BreezeModDspTests COMMAND BreezeModDspTests)
    # Offscreen JUCE editor snapshots and live processor-mode/preset tests.
    juce_add_console_app(BreezeModUiTests PRODUCT_NAME "BreezeMod UI Tests")
    juce_generate_juce_header(BreezeModUiTests)
    target_sources(BreezeModUiTests PRIVATE tests/UITests.cpp)
    target_compile_features(BreezeModUiTests PRIVATE cxx_std_17)
    target_compile_definitions(BreezeModUiTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    # The test's generated JuceHeader.h includes BinaryData.h. Expose the
    # actual JUCE asset target directly; BreezeMod links it privately.
    target_link_libraries(BreezeModUiTests PRIVATE BreezeMod BreezeModAssets
        juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
        juce::juce_recommended_config_flags)
    add_test(NAME BreezeModUiTests COMMAND BreezeModUiTests)

    add_executable(BreezeModPortableTests tests/PortableTests.cpp)
    target_compile_features(BreezeModPortableTests PRIVATE cxx_std_17)
    add_test(NAME BreezeModPortableTests COMMAND BreezeModPortableTests)
endif()
