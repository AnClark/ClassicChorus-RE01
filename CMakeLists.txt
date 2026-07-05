cmake_minimum_required (VERSION 3.20)

project (ClassicChorus-RE01
    VERSION 1.0.0
    LANGUAGES C CXX
)

add_subdirectory (dpf)

configure_file (src/config.h.in ${PROJECT_BINARY_DIR}/config.h @ONLY)

option (CONFIG_BUILD_JACK_STANDALONE "Build JACK standalone format (for debugging)")
if (CONFIG_BUILD_JACK_STANDALONE)
    set (TARGET_JACK jack)
endif ()

include_directories (
    src
    widgets/dpf-imgui
    widgets/dpf-imgui/DearImGui
    widgets/imgui-ext
    widgets/imgui-knobs-mod
    ${PROJECT_BINARY_DIR}
)

dpf_add_plugin (${PROJECT_NAME}
    TARGETS vst2 vst3 lv2 clap ${TARGET_JACK}
    FILES_DSP
        src/ClassicChorusPlugin.cpp
        src/core/Chorus.cpp
    FILES_UI
        src/UI.cpp
        src/PresetManager.cpp
        src/ui/UIChassis.cpp
        src/ui/UISubroutines.cpp
        src/ui/PresetManagerWindow.cpp
        src/ui/FileBrowserStuff.cpp
        src/ui/MessageBoxStuff.cpp
        widgets/dpf-imgui/DearImGui.cpp
        widgets/imgui-knobs-mod/imgui-knobs.cpp
)
