cmake_minimum_required(VERSION 3.22)
project(OctaveDivider VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

option(COPY_AFTER_BUILD "Copiar el VST3 a la carpeta del sistema al compilar" ON)

include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OctaveDivider
    COMPANY_NAME "TuNombre"
    PLUGIN_MANUFACTURER_CODE Tnom
    PLUGIN_CODE Octv
    FORMATS VST3 Standalone
    PRODUCT_NAME "Octave Divider"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

target_sources(OctaveDivider PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h Source/OctaveCore.h)
target_compile_definitions(OctaveDivider PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(OctaveDivider PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
