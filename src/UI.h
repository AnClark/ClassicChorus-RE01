#ifndef CLASSIC_Chorus_UI_H_INCLUDED
#define CLASSIC_Chorus_UI_H_INCLUDED

#include <string>
#include <queue>
#include <mutex>

#include "DistrhoUI.hpp"
#include "FileBrowserDialog.hpp"  // DPF cross-platform file browser API

#include "Structures.hpp"
#include "PresetManager.h"

#include "imgui-knobs.h"

/**
 * @file UI.h
 * @brief ImGui-based user interface for the Classic Chorus plugin.
 *
 * Implements the plugin editor window using Dear ImGui, including parameter
 * knobs, section grouping, the Kjaerhus Audio recreation logo and an About
 * dialog. Parameter changes from the host are cached locally and edits made
 * by the user are propagated back to the DSP through DISTRHO's UI API.
 */

/**
 * @class ClassicChorusUI
 * @brief DISTRHO UI implementation for the Classic Chorus plugin.
 *
 * The interface renders a fixed-size chassis with parameter sections for
 * delay and modulation, draws the plugin branding and handles mouse cursor
 * synchronization between ImGui and the DPF window.
 */
class ClassicChorusUI : public DISTRHO::UI
{
public:
    /**
     * @brief Constructs the UI and loads embedded fonts.
     */
    ClassicChorusUI();

protected:
    // ── Host callbacks ─────────────────────────────────────────────────────

    /**
     * @brief Called by the host when a parameter value changes.
     * @param index Zero-based parameter index.
     * @param value New parameter value.
     */
    void parameterChanged(uint32_t index, float value) override;

    /**
     * @brief Called by the host when a state value changes.
     * @param key State key.
     * @param value New state value.
     */
    void stateChanged(const char* key, const char* value) override;

    /**
     * @brief Called each frame to render the ImGui interface.
     *
     * Draws the chassis, parameter knobs, branding and the optional About
     * window, then updates the platform mouse cursor.
     */
    void onImGuiDisplay() override;

private:
    /* Cached state */
    float fParams[NUM_PARAMS] { 0.0f };   ///< Cached parameter values received from the host. @see paramInfo in Structures.h
    int   fLastMouseCursor = -1;          ///< Last ImGui mouse cursor type applied to the window.
    bool  fAboutWindowOpened = false;     ///< True while the About window is visible.

    // ── Preset Manager UI state ────────────────────────────────────────────

    // Dialog mode enum (for modal popups inside the preset manager overlay)
    enum class PmDialogMode { None, SaveNew, Rename, ConfirmDelete, ConfirmUpdate };

    bool              fPresetManagerOpened  = false;
    PmDialogMode      fPmDialogMode         = PmDialogMode::None;
    char              fPmNameBuffer[128]    = {};   // text input for Save As / Rename dialogs

    // Buffered values for atomic state restoration from stateChanged() callbacks
    std::string fRestoredPresetType = "Factory";
    std::string fRestoredPresetName;
    bool        fRestoredModified   = false;

    // ── Rendering helpers ──────────────────────────────────────────────────

    /** @brief Loads the embedded font set into the ImGui font atlas. */
    void _loadFonts();

    /**
     * @brief Draws the chassis background panel with a drop shadow.
     * @param margin  Inner margin between the window edge and the panel.
     * @param rounding Corner radius of the rounded rectangle.
     */
    void _drawChassisBackground(float margin, float rounding);

    /**
     * @brief Draws the clickable Kjaerhus Audio recreation logo.
     * @param size Desired logo area size, in pixels.
     */
    void _drawKjearhusLogo(const ImVec2& size);

    /** @brief Draws the plugin name and the "RE-01" model badge. */
    void _drawPluginName();

    // ── Parameter knobs ────────────────────────────────────────────────────

    /**
     * @brief Adds an ImGui knob for the given parameter.
     * @param paramId       Parameter identifier.
     * @param label         Label displayed beneath the knob.
     * @param v_min         Minimum knob value.
     * @param v_max         Maximum knob value.
     * @param marks         Array of scale mark descriptors.
     * @param mark_count    Number of scale marks.
     * @param isLogarithmic Whether the knob uses logarithmic scaling.
     * @param use_pivot     Whether the knob uses a pivot point.
     * @param pivot_value   Pivot value when @p use_pivot is true.
     * @param format        printf-style value format string.
     */
    void _addKnob(Parameters paramId, const char* label, float v_min, float v_max, const ImGuiKnobs_Mod::KnobScaleMark *marks, uint32_t mark_count, bool isLogarithmic = false, bool use_pivot = false, float pivot_value = 0.0f, const char* format = "%.1f");

    /**
     * @brief Adds an ImGui knob using the parameter's native range from @ref paramInfo.
     *        This variant of _addKnob uses the predefined parameter ranges from kParamRanges,
     *        so you only need to specify the paramId and it will automatically use the correct min/max values.
     * @param paramId       Parameter identifier.
     * @param label         Label displayed beneath the knob.
     * @param marks         Array of scale mark descriptors.
     * @param mark_count    Number of scale marks.
     * @param isLogarithmic Whether the knob uses logarithmic scaling.
     * @param use_pivot     Whether the knob uses a pivot point.
     * @param pivot_value   Pivot value when @p use_pivot is true.
     * @param format        printf-style value format string.
     */
    inline void _addKnob(Parameters paramId, const char* label, const ImGuiKnobs_Mod::KnobScaleMark *marks, uint32_t mark_count, bool isLogarithmic = false, bool use_pivot = false, float pivot_value = 0.0f, const char* format = "%.1f")
    {
        _addKnob(paramId, label, paramInfo[paramId].minVal, paramInfo[paramId].maxVal, marks, mark_count, isLogarithmic, use_pivot, pivot_value, format);
    }

    void _addLEDIndicator(const char* label, bool isLit);

    void _addBinaryStateSwitch(Parameters paramId, const char* label, const char* state0Label, const char* state1Label, float LEDIndentWidth, float btnIndentWidth);

    // ── Layout helpers ─────────────────────────────────────────────────────

    /**
     * @brief Begins a titled parameter section.
     * @param title Section title displayed at the top.
     * @param width Section width, in pixels.
     * @return Always returns true.
     */
    bool _BeginSection(const char* title, float width);

    /** @brief Ends the current parameter section. */
    void _EndSection();

    // ── Input handling ───────────────────────────────────────────────────────

    /** @brief Synchronizes the ImGui mouse cursor with the DPF window cursor. */
    void _UpdateMouseCursor();

    // Preset Manager related procedures
    void _drawPresetManager(); // Draw the preset manager overlay window (implemented in PresetManagerUI.cpp)
    void _applyRestoredPresetState(); // Restore preset context from buffered stateChanged() values

    // ── Instances ──────────────────────────────────────────────────────────

    ScopedPointer<PresetManager> fPresetManager;
    friend class PresetManager;

    // ── File browser stuff (DPF cross-platform native file dialog) ─────────

    // Definitions & states
    enum class FileBrowserAction { None, Import, Export };
    DGL_NAMESPACE::FileBrowserHandle fFileBrowserHandle = nullptr;  // nullptr = no dialog open
    FileBrowserAction                fFileBrowserAction = FileBrowserAction::None;

    // Poll native file dialog each frame; process result when dialog closes
    void _handleFileBrowserIdle();  // Should be called from onImGuiDisplay() to handle file browser state and results

    // ── Message box stuff ──────────────────────────────────────────────────

    // Definitions & states
    std::queue<std::string> fMessageBoxQueue; // Queue of messages to be shown in message boxes
    bool                    fRequestMessagePopup = false; // Trigger flag to indicate that a message box popup should be displayed
    std::mutex              fMessageQueueMutex; // Mutex to protect access to the message box queue

    // Poll message queue and show message box when needed.
    void _handleMessageBoxIdle();   // Should be called from onImGuiDisplay().
    inline void _showMessageBox(const std::string& message)
    {
        // Apply a mutex to avoid possible conflict
        std::lock_guard<std::mutex> lock(fMessageQueueMutex);

        fMessageBoxQueue.push(message);
        fRequestMessagePopup = true;
    }

    // -------------------------------------------------------------------
    // Dear ImGui helper stuff

    float fScaleFactor { 1.0f };    // Local cache of current scale factor
                                    // (should be updated in constructor)

    // A macro-like function to convert absolute value to DPI-scaled value.
    // NOTICE: Dear ImGui itself does not support implicitly apply DPI scaling
    //         on absolute size / pos values. We have to convert by ourselves.
    inline float SCALE(float value)
    {
        return value * fScaleFactor;
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassicChorusUI)
};

#endif // CLASSIC_Chorus_UI_H_INCLUDED
