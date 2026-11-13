cmake_minimum_required(VERSION 3.22)

# macOS: run on older Macs (Adam's 2015 Intel MacBook Pro on macOS 11) and on
# Apple Silicon, from one universal binary. Must be set before project().
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
endif()

# Windows: link the C++ runtime statically so users don't need a
# Visual C++ Redistributable installed.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(SCOPE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local JUCE checkout with -DJUCE_DIR=/path/to/JUCE, otherwise fetch it.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Brand placeholders: change these once the brand name is decided.
set(BRAND_NAME "YourBrand")
set(PRODUCT_NAME "Scope")

set(SCOPE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND SCOPE_FORMATS AU)
endif()

juce_add_plugin(Scope
    COMPANY_NAME "${BRAND_NAME}"
    PLUGIN_MANUFACTURER_CODE Ybrd
    PLUGIN_CODE Scp1
    PRODUCT_NAME "${PRODUCT_NAME}"
    FORMATS ${SCOPE_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(Scope PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/dsp/Analyzer.cpp
    Source/hosting/PluginChain.cpp
    Source/ui/Views.cpp
    Source/ui/ChainPanel.cpp)

target_include_directories(Scope PRIVATE Source)

target_compile_definitions(Scope PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0
    SCOPE_BRAND_NAME="${BRAND_NAME}"
    SCOPE_PRODUCT_NAME="${PRODUCT_NAME}")

target_link_libraries(Scope
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Developer tools: a tiny test FX plugin and a headless host that loads the
# built Scope.vst3, runs audio through its FX chain and renders screenshots.
option(SCOPE_BUILD_TOOLS "Build test plugin and snapshot host" OFF)
if(SCOPE_BUILD_TOOLS)
    juce_add_plugin(TestHalfGain
        COMPANY_NAME "${BRAND_NAME}"
        PLUGIN_MANUFACTURER_CODE Ybrd
        PLUGIN_CODE Thg1
        PRODUCT_NAME "TestHalfGain"
        FORMATS VST3
        IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE)
    target_sources(TestHalfGain PRIVATE tools/TestHalfGain.cpp)
    target_compile_definitions(TestHalfGain PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(TestHalfGain PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)

    juce_add_console_app(HostTest PRODUCT_NAME "HostTest")
    target_sources(HostTest PRIVATE tools/HostTest.cpp)
    target_compile_definitions(HostTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_LV2=0 JUCE_PLUGINHOST_LADSPA=0)
    target_link_libraries(HostTest PRIVATE juce::juce_audio_utils juce::juce_audio_processors
        PUBLIC juce::juce_recommended_config_flags)
    add_dependencies(HostTest Scope_VST3 TestHalfGain_VST3)

    # Screenshot renderer: compiles the plugin sources directly into an app.
    juce_add_console_app(Snapshots PRODUCT_NAME "Snapshots")
    target_sources(Snapshots PRIVATE tools/Snapshots.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/dsp/Analyzer.cpp
        Source/hosting/PluginChain.cpp Source/ui/Views.cpp Source/ui/ChainPanel.cpp)
    target_include_directories(Snapshots PRIVATE Source)
    target_compile_definitions(Snapshots PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_LV2=0 JUCE_PLUGINHOST_LADSPA=0
        JucePlugin_Name="${PRODUCT_NAME}"
        SCOPE_BRAND_NAME="${BRAND_NAME}" SCOPE_PRODUCT_NAME="${PRODUCT_NAME}")
    target_link_libraries(Snapshots PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()
