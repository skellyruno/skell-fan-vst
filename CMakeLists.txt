cmake_minimum_required(VERSION 3.22)
project(SkellFan VERSION 1.8.0 LANGUAGES C CXX)

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

# SkellCore is built as a separate, pinned CMake target, not copied into this repo.
# Immutable revision prevents unrelated SkellCore updates changing the plugin.
FetchContent_Declare(SkellCore
    GIT_REPOSITORY https://github.com/skellyruno/skellcore-vst-template.git
    GIT_TAG 05191f3cfade5a57c1eb97576cce6a348705222c
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR SkellCore)
FetchContent_MakeAvailable(SkellCore)

# Preserve the old JUCE/VST3 identity so existing BreezeMod project instances
# and 11 automation IDs can continue to resolve after renaming the product.
juce_add_plugin(SkellFan
    COMPANY_NAME "Skell Audio"
    BUNDLE_ID "audio.skell.breezemod"
    PLUGIN_MANUFACTURER_CODE SkAu
    PLUGIN_CODE Bz01
    PRODUCT_NAME "SkellFan"
    DESCRIPTION "Fan-inspired four-mode stereo modulation"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3 Standalone)

juce_generate_juce_header(SkellFan)
juce_add_binary_data(SkellFanAssets SOURCES
    Assets/fan_body.png Assets/blades.png Assets/blades_blur.png
    Assets/grille.png Assets/hub_cap.png Assets/knob_chrome.png
    Assets/knob_blue.png Assets/key_strip.png Assets/small_key.png
    Assets/slider_thumb.png)

target_sources(SkellFan PRIVATE
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

target_compile_features(SkellFan PRIVATE cxx_std_17)
target_compile_definitions(SkellFan PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SkellFan PRIVATE
    SkellFanAssets SkellCore::DSP SkellCore::UI juce::juce_audio_utils juce::juce_audio_processors
    juce::juce_gui_extra juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

option(SKELLFAN_BUILD_TESTS "Build JUCE DSP smoke tests" ON)
if(SKELLFAN_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(SkellFanDspTests PRODUCT_NAME "SkellFan DSP Tests")
    juce_generate_juce_header(SkellFanDspTests)
    target_sources(SkellFanDspTests PRIVATE tests/DSPTests.cpp)
    target_compile_features(SkellFanDspTests PRIVATE cxx_std_17)
    target_compile_definitions(SkellFanDspTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(SkellFanDspTests PRIVATE juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
    add_test(NAME SkellFanDspTests COMMAND SkellFanDspTests)
    # Offscreen JUCE editor snapshots and live processor-mode/preset tests.
    juce_add_console_app(SkellFanUiTests PRODUCT_NAME "SkellFan UI Tests")
    juce_generate_juce_header(SkellFanUiTests)
    target_sources(SkellFanUiTests PRIVATE tests/UITests.cpp)
    target_compile_features(SkellFanUiTests PRIVATE cxx_std_17)
    target_compile_definitions(SkellFanUiTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    # The test's generated JuceHeader.h includes BinaryData.h. Expose the
    # actual JUCE asset target directly; SkellFan links it privately.
    target_link_libraries(SkellFanUiTests PRIVATE SkellFan SkellFanAssets SkellCore::DSP SkellCore::UI
        juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
        juce::juce_recommended_config_flags)
    add_test(NAME SkellFanUiTests COMMAND SkellFanUiTests)

    add_executable(SkellFanCoreTests)
    target_sources(SkellFanCoreTests PRIVATE tests/SkellCoreIntegrationTests.cpp)
    target_link_libraries(SkellFanCoreTests PRIVATE SkellCore::DSP)
    target_compile_features(SkellFanCoreTests PRIVATE cxx_std_17)
    add_test(NAME SkellFanCoreTests COMMAND SkellFanCoreTests)

    add_executable(SkellFanPortableTests tests/PortableTests.cpp)
    target_compile_features(SkellFanPortableTests PRIVATE cxx_std_17)
    add_test(NAME SkellFanPortableTests COMMAND SkellFanPortableTests)
endif()
