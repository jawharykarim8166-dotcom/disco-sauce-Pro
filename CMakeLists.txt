cmake_minimum_required(VERSION 3.22)
project(DiscoSaucePro VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DS_BUILD_PLUGIN "Build the VST3 / Standalone plugin (needs JUCE, downloaded automatically)" ON)
option(DS_BUILD_TESTS  "Build the DSP unit tests" ON)

# ---------------------------------------------------------------- DSP unit tests (pure C++, no JUCE)
if(DS_BUILD_TESTS)
  enable_testing()
  foreach(t param_tests fx_tests engine_tests tune_tests)
    add_executable(${t} Tests/${t}.cpp)
    if(MSVC)
      target_compile_options(${t} PRIVATE /O2 /EHsc)
    else()
      target_compile_options(${t} PRIVATE -O2)
    endif()
    add_test(NAME ${t} COMMAND ${t})
    set_tests_properties(${t} PROPERTIES TIMEOUT 3000)
  endforeach()
endif()

# ---------------------------------------------------------------- plugin
if(DS_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)

  juce_add_plugin(DiscoSaucePro
    COMPANY_NAME "flowmotionlb"
    PLUGIN_MANUFACTURER_CODE Flmo
    PLUGIN_CODE Dsp2
    FORMATS VST3 Standalone
    PRODUCT_NAME "Disco Sauce 2 Pro"
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

  target_sources(DiscoSaucePro PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)

  # the volcano Disco Sauce artwork lives at Assets/disco_sauce_volcano_4k.jpg and it becomes the header image
  set(DS_ART 0)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/disco_sauce_volcano_4k.jpg")
    set(DS_ART 1)
    juce_add_binary_data(DSAssets SOURCES Assets/disco_sauce_volcano_4k.jpg)
    target_link_libraries(DiscoSaucePro PRIVATE DSAssets)
  endif()

  target_compile_definitions(DiscoSaucePro PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 DS_HAS_ARTWORK=${DS_ART})

  target_link_libraries(DiscoSaucePro
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
endif()
