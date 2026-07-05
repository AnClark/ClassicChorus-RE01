#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_NAME    "Classic Chorus RE-01"
#define DISTRHO_PLUGIN_URI     "https://github.com/AnClark/ClassicChorus-RE01"
#define DISTRHO_PLUGIN_BRAND   "AnClark Liu"
#define DISTRHO_PLUGIN_CLAP_ID "studio.anclark.classic.chorus.re01"
#define PLUGIN_NAME_COMMON     "Classic Chorus"    // For plugin name labels / tooltips

#define DISTRHO_PLUGIN_NUM_INPUTS   2
#define DISTRHO_PLUGIN_NUM_OUTPUTS  2
#define DISTRHO_PLUGIN_IS_RT_SAFE   1
#define DISTRHO_PLUGIN_WANT_TIMEPOS 0
#define DISTRHO_PLUGIN_IS_SYNTH        0
#define DISTRHO_PLUGIN_WANT_STATE      1

#define DISTRHO_PLUGIN_HAS_UI          1
#define DISTRHO_UI_USE_CUSTOM           1
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH  "DearImGui.hpp"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE   DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_DEFAULT_WIDTH         706 + 120 - 8  // Base width + right panel width
#define DISTRHO_UI_DEFAULT_HEIGHT        120

/* VST2 unique ID – must be a bare 4-character token (no quotes, no commas) */
#define DISTRHO_PLUGIN_UNIQUE_ID       CCr1

// Global definitions for Classic Flanger plugin (Not DPF-related, only used in our own codebase)
#define CLASSIC_FLANGER_APPDATA_DIR_NAME "ClassicChorusRE01" // Subdirectory in user appdata folder for storing presets, etc.
#define CLASSIC_FLANGER_PRESET_FILE_NAME "presets.json" // Filename for storing user presets on disk

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
