#include "UI.h"
#include "config.h"
#include "HardwareButton.hpp"

static constexpr ImGuiKnobs_Mod::KnobScaleMark kRangeMarks[] = {
    // Scales: 1.25, 2.5, 5, 10, 20, 40, 80, 160, 320 ms
    // Values are actual millisecond values, not normalized to second. All scales should have labels
    { 1.25f, "1.25" },
    { 2.5f, "2.5" },
    { 5.0f, "5" },
    { 10.0f, "10" },
    { 20.0f, "20" },
    { 40.0f, "40" },
    { 80.0f, "80" },
    { 160.0f, "160" },
    { 320.0f, "320" }
};

static constexpr ImGuiKnobs_Mod::KnobScaleMark kFineMarks[] = {
    {   0.5f, "0.5x" },
    { 0.55f, nullptr },
    { 0.6f, nullptr },
    { 0.65f, nullptr },
    { 0.7f, nullptr },
    { 0.75f, "0.75x" },
    { 0.8f, nullptr },
    { 0.85f, nullptr },
    { 0.9f, nullptr },
    { 0.95f, nullptr },
    {  1.0f, "1x" }
};

static constexpr ImGuiKnobs_Mod::KnobScaleMark kRateMarks[] = {
    {   0.050000f, "0.1" },
    {   0.091028f, nullptr },
    {   0.165723f, nullptr },
    {   0.301709f, nullptr },
    {   0.549280f, nullptr },
    {   1.000000f, "1.0" },
    {   1.820564f, nullptr },
    {   3.314454f, nullptr },
    {   6.034176f, nullptr },
    {  10.985605f, "10.0" },
};

static const ImGuiKnobs_Mod::KnobScaleMark kDepthMarks[] = {
    {   0.0f / 100.0f, "0" },
    {   10.0f / 100.0f, nullptr },
    {   20.0f / 100.0f, nullptr },
    {   30.0f / 100.0f, nullptr },
    {   40.0f / 100.0f, nullptr },
    {   50.0f / 100.0f, "50" },
    {   60.0f / 100.0f, nullptr },
    {   70.0f / 100.0f, nullptr },
    {   80.0f / 100.0f, nullptr },
    {   90.0f / 100.0f, nullptr },
    {  100.0f / 100.0f, "100" },
};

static const ImGuiKnobs_Mod::KnobScaleMark kMixMarks[] = {
    {   0.0f / 100.0f, "DIR." },
    {   12.5f / 100.0f, nullptr },
    {   25.0f / 100.0f, nullptr },
    {   37.5f / 100.0f, nullptr },
    {   50.0f / 100.0f, "1:1" },
    {   62.5f / 100.0f, nullptr },
    {   75.0f / 100.0f, nullptr },
    {   87.5f / 100.0f, nullptr },
    {  100.0f / 100.0f, "EFF." },
};

static const ImGuiKnobs_Mod::KnobScaleMark kLevelMarks[] = {
    {  -10.0f, "-10" },
    {  -8.0f, nullptr },
    {  -6.0f, nullptr },
    {  -4.0f, nullptr },
    {  -2.0f, nullptr },
    {   0.0f, "0" },
    {   2.0f, nullptr },
    {   4.0f, nullptr },
    {   6.0f, nullptr },
    {   8.0f, nullptr },
    {   10.0f, "+10" },
};

ClassicChorusUI::ClassicChorusUI()
    : DISTRHO::UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
{
    // Fetch current scale factor
    this->fScaleFactor = getScaleFactor();

    std::memset(fParams, 0, sizeof(fParams));

    // Initialize preset manager and load persisted user presets from disk
    fPresetManager = new PresetManager(this);
    const bool presetsLoaded = fPresetManager->loadUserPresetsFromDisk();
    if (!presetsLoaded) {
        // NOTE: _showMessageBox() can be used here because it pushes the message into a queue and doesn't require an active ImGui context at this point.
        //       The message will be displayed as a popup when the UI is rendered.
        //       @see _showMessageBox() and _handleMessageBoxIdle() in UI.h/UI.cpp
        _showMessageBox("WARNING: could not load user presets from disk. Presets will not be saved.");
    }

    _loadFonts();
    fAboutWindowOpened = false;
}

void ClassicChorusUI::parameterChanged(uint32_t index, float value)
{
    DISTRHO_SAFE_ASSERT_RETURN(index < NUM_PARAMS, )
    fParams[index] = value;

    // Mark preset as modified when user tweaks a knob
    if (fPresetManager)
        fPresetManager->markModified();
}

void ClassicChorusUI::stateChanged(const char* key, const char* value)
{
    // Buffer each restored value; rebuild state after all three arrive.
    if (std::strcmp(key, STATE_PRESET_TYPE) == 0)
        fRestoredPresetType = value;
    else if (std::strcmp(key, STATE_PRESET_NAME) == 0)
        fRestoredPresetName = value;
    else if (std::strcmp(key, STATE_PRESET_MODIFIED) == 0)
        fRestoredModified = (std::strcmp(value, "true") == 0);

    _applyRestoredPresetState();
}

void ClassicChorusUI::onImGuiDisplay()
{
    const float margin = SCALE(4.0f);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    static constexpr auto kWindowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Begin("Main Window", nullptr, kWindowFlags))
    {
        const float rounding = SCALE(10.0f);
        const ImVec2 winSize = ImGui::GetWindowSize();

        _drawChassisBackground(margin, rounding);

        ImGui::SetCursorPos(ImVec2(margin, margin));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);

        const ImVec2 childSize(winSize.x - 2.0f * margin, winSize.y - 2.0f * margin);
        if (ImGui::BeginChild("BackgroundPanel", childSize, false,
                              ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::Dummy(ImVec2(SCALE(2.0f), 0.0f));
            ImGui::SameLine();

            if (_BeginSection("DELAY TIME", SCALE(90.0f * 2)))
            {
                ImGui::Dummy(ImVec2(SCALE(16.0f), 0.0f));
                ImGui::SameLine();

                // NOTICE: Use "%.2f" format (acts as accuracy hint) for the logarithmic Delay knob so the 0.1..1.0 ms range has
                //         enough quantization steps to feel smooth while dragging.
                //         See _addKnob() for more details.
                _addKnob(pParamRange, " RANGE (ms)", kRangeMarks, IM_ARRAYSIZE(kRangeMarks), true, false, 0.0f, "%.2f");

                ImGui::SameLine(0.0f, SCALE(32.0f + 4.0f));
                _addKnob(pParamFine, " FINE", kFineMarks, IM_ARRAYSIZE(kFineMarks), false, false, 0.0f, "%.3f");

                _EndSection();
            }

            ImGui::SameLine(0.0f, SCALE(22.0f));

            if (_BeginSection("MODULATION", SCALE(80.0f * 3 - 4.0f)))
            {
                ImGui::Dummy(ImVec2(SCALE(2.0f), 0.0f));
                ImGui::SameLine();
                _addKnob(pParamRate, " RATE (Hz)", kRateMarks, IM_ARRAYSIZE(kRateMarks), true, false, 0.0f, "%.2f");

                ImGui::SameLine(0.0f, SCALE(30.0f));
                _addBinaryStateSwitch(pParamSpread, " SPREAD", "OFF", "ON", SCALE(2.0f), SCALE(1.0f));

                ImGui::SameLine(0.0f, SCALE(26.0f));
                _addKnob(pParamDepth, " DEPTH (%)", kDepthMarks, IM_ARRAYSIZE(kDepthMarks), false, false, 0.0f, "%.2f");

                _EndSection();
            }

            ImGui::SameLine(0.0f, SCALE(20.0f));

            if (_BeginSection("OUTPUT", SCALE(82.0f * 2 - 4.0f)))
            {
                ImGui::Dummy(ImVec2(SCALE(6.0f), 0.0f));
                ImGui::SameLine();
                _addKnob(pParamMix, "MIX", kMixMarks, IM_ARRAYSIZE(kMixMarks), false, false, 0.0f, "%.2f");

                ImGui::SameLine(0.0f, SCALE(36.0f));
                _addKnob(pParamLevel, " LEVEL (dB)", kLevelMarks, IM_ARRAYSIZE(kLevelMarks),
                        false,
                        false,
                        0.0f,
                        "%.3f"
                );

                _EndSection();
            }

            ImGui::SameLine(0.0f, SCALE(24.0f));

            {
                ImGui::BeginGroup();

                ImGui::Dummy(ImVec2(0, SCALE(2)));
                _drawKjearhusLogo(ImVec2(SCALE(108.0f), SCALE(44.0f)));

                // ── Preset Manager button ──────────────────────────────────
                {
                    ImGui::Dummy(ImVec2(0, SCALE(2)));

                    //ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 20.0f);

                    ImGui::BeginGroup();
                    ImGui::AlignTextToFramePadding();

                    ImGui::Dummy(ImVec2(SCALE(2), 0));
                    ImGui::SameLine(0.0f, 0.0f);

                    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

                    {
                        const Preset* curPreset = fPresetManager->currentPreset();
                        std::string   btnLabel;
                        if (curPreset) {
                            btnLabel = curPreset->name;
                            if (fPresetManager->isModified()) btnLabel += " *";
                        } else {
                            btnLabel = "Select Preset...";
                        }
                        btnLabel += "##Preset";

                        if (ImGuiExt::HardwareButton(btnLabel.c_str(),
                                           ImVec2(SCALE(128.0f), ImGui::GetFrameHeight()),
                                           ImVec4(0x2f / 255.0f, 0x4d / 255.0f, 0x44 / 255.0f, 1.0f)))
                        {
                            fPresetManagerOpened = !fPresetManagerOpened;
                        }
                    }

                    ImGui::PopFont();

                    ImGui::EndGroup();
                }

                ImGui::Dummy(ImVec2(0.0f, SCALE(0.5f)));

                _drawPluginName();

                ImGui::EndGroup();
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::End();
    }

    ImGui::PopStyleColor();

    // ── "About" window (fullscreen) ───────────────────────────────────────────────
    static constexpr auto about_window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (fAboutWindowOpened)
    {
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        if (ImGui::Begin("About Window", &fAboutWindowOpened, about_window_flags))
        {
            {
                ImGui::Columns(2, "AboutColumns", false);
                ImGui::SetColumnWidth(0, SCALE(400.0f - 5.0f));
                ImGui::SetColumnWidth(1, SCALE(420.0f - 15.0f));

                {
                    const String versionStr = String(DISTRHO_PLUGIN_NAME) + "  |  Version " +
                                        String(VERSION_MAJOR) + "." +
                                        String(VERSION_MINOR) + "." +
                                        String(VERSION_PATCH);

                    ImGui::SeparatorText(versionStr);
                    ImGui::Text("Reverse engineering of Kjaerhus Audio " PLUGIN_NAME_COMMON " (2003).");
                    ImGui::Text("Original algorithm by Kjaerhus Audio.");
                    ImGui::Text("Copyright (c) 2026 AnClark Liu <clarklaw4701@qq.com>");

                    ImGui::SeparatorText("License: GNU General Public License v3.0 or later");
                    ImGui::Dummy(ImVec2(0, SCALE(2)));
                    ImGui::TextWrapped(DISTRHO_PLUGIN_NAME " is free software: "
                                            "you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation,"
                                            "either version 3 of the License, or (at your option) any later version.");
                }

                ImGui::NextColumn();

                {
                    ImGui::SeparatorText("Disclaimer");
                    ImGui::TextWrapped("This is an unofficial, reverse-engineered clone of the discontinued Kjaerhus " PLUGIN_NAME_COMMON ", aiming at bringing "
                                            "this vintage and fantastic plugin to life again.");
                    ImGui::TextWrapped("This project is NOT related to official Kjaerhus Audio,\nAcoustica LLC. and their affiliates.");
                    ImGui::Dummy(ImVec2(0, SCALE(2)));
                    ImGui::TextWrapped("The Kjaerhus logo is used under fair use for identification purposes only, "
                                            "and is not intended to infringe any trademarks.");
                    ImGui::Dummy(ImVec2(0, SCALE(2)));
                    ImGui::TextWrapped("VST is a trademark of Steinberg GmbH.");
                }

                ImGui::Columns(1);
            }

            {
                ImGui::BeginGroup();

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, SCALE(5.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0x2f, 0x4d, 0x44, 0xff));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0x2f + 20, 0x4d + 20, 0x44 + 20, 0xff));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0x2f + 40, 0x4d + 40, 0x44 + 40, 0xff));

                // Fixed position OK button at bottom-right (screen coordinates)
                static const ImVec2 button_size = ImVec2(SCALE(60 - 5), SCALE(25));
                ImVec2 buttonPos = ImVec2(viewport->Pos.x + viewport->Size.x - button_size.x - SCALE(22.0f),
                                        viewport->Pos.y + viewport->Size.y - button_size.y - SCALE(10.0f));
                ImGui::SetCursorScreenPos(buttonPos);
                if (ImGui::Button("OK", button_size))
                {
                    fAboutWindowOpened = false;
                }

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(); // FrameRounding

                ImGui::EndGroup();
            }

            ImGui::End();
        }
    }

    // Update the OS mouse cursor based on the current ImGui mouse cursor state
    _UpdateMouseCursor();

    // Poll the native file browser dialog (Import/Export). Must be called every frame.
    _handleFileBrowserIdle();

    // Draw the preset manager overlay (renders nothing when fPresetManagerOpened == false)
    _drawPresetManager();

    // Handle message box display
    _handleMessageBoxIdle();
}

// -----------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new ClassicChorusUI();
}

END_NAMESPACE_DISTRHO
