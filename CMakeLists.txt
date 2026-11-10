cmake_minimum_required (VERSION 3.20)

project (ClassicChorus-RE01
    VERSION 1.1.2
    DESCRIPTION "Classic Chorus RE-01 audio plugin"
    LANGUAGES C CXX
)

# Metadata for CPack
set (AUTHOR "AnClark Liu")
set (INSTALL_DIRECTORY ${PROJECT_NAME})
set (ICON_PATH "${PROJECT_SOURCE_DIR}/Screenshot.png")
set (PRODUCTBUILD_IDENTIFIER "com.anclark.classicchorus.re01")

# Explicitly specify C++ standard to deal with missing functions (e.g. std::clamp()) on macOS
set (CMAKE_CXX_STANDARD 20)

add_subdirectory (dpf)

configure_file (src/config.h.in ${PROJECT_BINARY_DIR}/config.h @ONLY)

# Packaging rules
include ("${PROJECT_SOURCE_DIR}/cmake/Packaging.cmake")

option (CONFIG_BUILD_JACK_STANDALONE "Build JACK standalone format (for debugging)")
if (CONFIG_BUILD_JACK_STANDALONE)
    set (TARGET_JACK jack)
endif ()

# GitHub CI's Ubuntu runner cannot directly run lv2-ttl-generator.exe, even though Wine and wine-binfmt is installed.
# And, LV2 is not so common on Windows (only REAPER, Ardour and Harrison Mixbus support LV2 on Windows).
# So disable LV2 on Windows by default.
option (CONFIG_BUILD_LV2_ON_WINDOWS "Build LV2 on Windows (including cross-compiling)." OFF)
if (WIN32 AND CONFIG_BUILD_LV2_ON_WINDOWS)
    message (STATUS "Win32 environment and LV2 build is enabled, will build LV2 plugin")
    set (TARGET_LV2 lv2)
elseif (WIN32)
    message (STATUS "Win32 environment and LV2 build is disabled. Bypass LV2")
else ()
    message (STATUS "Non-Win32 environment, will build LV2 plugin")
    set (TARGET_LV2 lv2)
endif ()

include_directories (
    src
    widgets/dpf-imgui
    widgets/dpf-imgui/DearImGui
    widgets/imgui-ext
    widgets/imgui-knobs-mod
    ${PROJECT_BINARY_DIR}
)

set (FONT_FILES
    fonts/LiberationSans-Regular.cpp
    fonts/CormorantFont.cpp
    fonts/FontAwesome5.cpp
)

dpf_add_plugin (${PROJECT_NAME}
    TARGETS vst2 vst3 ${TARGET_LV2} clap ${TARGET_JACK}
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
        ${FONT_FILES}
)
