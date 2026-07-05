#include "UI.h"

#include "imgui-knobs.h"
#include "CenteredSeparatorText.hpp"
#include "LEDIndicator.hpp"
#include "HardwareButton.hpp"

#include "../fonts/CormorantFont.hpp"
#include "../fonts/LiberationSans-Regular.hpp"
#include "../fonts/FontAwesome5.hpp"
#include "../fonts/IconFontAwesome5.h"
#include "src/Resources.hpp"

ImGuiKnobs_Mod::KnobScaleMarkStyle kScaleMarkStyle = {
    .outer_radius = 1.20f,
    .tick_length  = 0.50f,    
    .font_size    = 12.5f,
};

void ClassicChorusUI::_loadFonts()
{
    // Font sizes:
    // - Section title text:                   14px
    // - Chassis Regular text:                 12.5px (e.g. Knob labels)
    // - Larger text size:                     20px (e.g. Knob scale marks, down-sampled to 12.5px for better rendering quality)
    // - ImGui UI text size:                   14.5px (e.g. tooltip text, menu text)

    ImGuiIO& io(ImGui::GetIO());

    ImFontConfig fc;
    fc.FontDataOwnedByAtlas = false;
    fc.OversampleH = 1;
    fc.OversampleV = 1;
    fc.PixelSnapH = true;

    io.Fonts->Clear();

    // ↓ Font #0: Chassis regular text (e.g. Knob labels)
    //            Only load basic Latin glyphs for the chassis font to reduce atlas size, since most chassis text is simple alphanumeric characters.
    static constexpr ImWchar kChassisRanges[] = { ' ', '~', 178, 178 + 1, 0 }; // Basic Latin range. 178 = '²'
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)LiberationSansTTF_Compressed_compressed_data, LiberationSansTTF_Compressed_compressed_size, 12.5f * getScaleFactor(), &fc, kChassisRanges);

    // ↓ Font #1: Larger text size for section titles (e.g. "OUTPUT")
    //            Only load uppercase glyphs for the title font to reduce atlas size, since section titles are always uppercase.
    //            NOTE: Classic Chorus has section titles with space (e.g. "DELAY TIMES").
    static constexpr ImWchar kTitleRanges[] = { 'A', 'Z', ' ', ' ' + 1, 0 };
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)LiberationSansTTF_Compressed_compressed_data, LiberationSansTTF_Compressed_compressed_size, 14.0f * getScaleFactor(), &fc, kTitleRanges);

    // ↓ Font #2: Even larger text size for scale marks, which will be down-sampled to 12.5px by the Knob widget for better visual quality.
    //            For convenience, this font is also used to drawing the "Kjaerhus Audio" logo.
    //            Only load alphanumeric glyphs for the scale-mark font to reduce atlas size.
    static constexpr ImWchar kScaleMarkRanges[] = { 'A', 'Z', 'a', 'z', '0', '9', ' ', ' ' + 1, '+', ':', u'°', u'°' + 1, 8734, 8734 + 1, 198, 198 + 1,  0 };    // 8734 = infinity symbol, 198 = 'Æ' in Liberation Sans
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)LiberationSansTTF_Compressed_compressed_data, LiberationSansTTF_Compressed_compressed_size, 20.0f * getScaleFactor(), &fc, kScaleMarkRanges);

    // ↓ Font #3: Semi-BoldItalic Cormorant font for drawing "Classic Reverb" logo text
    //            Only load essential charset.
    static constexpr ImWchar kPluginNameRanges[] = { 'A', 'Z', 'a', 'z', '0', '9', ' ', ' ' + 1, 0 };
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)CormorantSemiBoldItalicTTF_compressed_data, CormorantSemiBoldItalicTTF_compressed_size, 20.0f * getScaleFactor(), &fc, kPluginNameRanges);

    // ↓ Font #4: Dejavu Sans for ImGui menu and tooltip text (not used in the chassis board, so we can load a full charset)
    io.Fonts->AddFontFromMemoryTTF((void*)dpf_resources::dejavusans_ttf, dpf_resources::dejavusans_ttf_size, 14.5f * getScaleFactor(), &fc);

    // ↓ Font #4 (merged): Font Awesome icons merged into the Dejavu Sans font above.
    //            MergeMode = true causes glyphs to be merged into the previously added font (Font #4 Dejavu Sans)
    //            rather than creating a new font entry. After this call there is still only Font #4 in the atlas,
    //            and icons can be used anywhere Dejavu Sans is active without switching fonts.
    static constexpr ImWchar kFontAwesomeRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    fc.MergeMode = true;
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)FontAwesomeTTF_compressed_data, FontAwesomeTTF_compressed_size, 14.5f * getScaleFactor(), &fc, kFontAwesomeRanges);
    fc.MergeMode = false;

    io.Fonts->Build();
    io.FontDefault = io.Fonts->Fonts[4];

    // Specify a larger font for the scale marks to improve rendering quality.
    // The Knob widget will down-sample it to the specified font size (12.5px) to achieve better visual quality.
    kScaleMarkStyle.custom_font = io.Fonts->Fonts[2];
}

void ClassicChorusUI::_addKnob(Parameters paramId, const char* label, float v_min, float v_max, const ImGuiKnobs_Mod::KnobScaleMark *marks, uint32_t mark_count, bool isLogarithmic, bool use_pivot, float pivot_value, const char* format)
{
    // This is a helper function to add a knob with given parameters.
    // It can be called from onImGuiDisplay() to reduce code duplication.
    //
    // @param const char* format:
    //        printf-style format string passed to ImGuiKnobs_Mod::Knob() and forwarded to
    //        ImGui::DragBehavior(). It affects not only the displayed value but also the
    //        internal quantization / logarithmic epsilon used while dragging, so a finer
    //        format (e.g. "%.2f") yields smaller adjustment steps for logarithmic knobs.

    constexpr float KNOB_SIZE = 50.0f;
    constexpr int DEFAULT_STEP = 10;

    constexpr auto IMGUIKNOBS_PI = 3.14159265358979323846f;
    constexpr float angle_min = IMGUIKNOBS_PI * (130.0f / 180.0f);   // Down-left 40° (starting point)
    constexpr float angle_max = IMGUIKNOBS_PI * (410.0f / 180.0f);   // Down-right 40° (+360°)

    ImGuiKnobFlags flags = ImGuiKnobFlags_TitleBottom;
    if (isLogarithmic) flags |= ImGuiKnobFlags_Logarithmic;
    if (use_pivot)     flags |= ImGuiKnobFlags_Pivot;

    // Knob color
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0x2f + 70, 0x4d + 70, 0x44 + 70, 0xff));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0x2f + 90, 0x4d + 90, 0x44 + 90, 0xff));

    // Now default font is Droid Sans, so we need to push the chassis font for the knob label and scale marks.
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    // NOTE: Pass the format string down to the knob widget. For logarithmic knobs this controls
    //       the rounding precision applied by ImGui::DragBehavior (e.g. "%.2f" rounds to 0.01).
    if (ImGuiKnobs_Mod::Knob(label, &fParams[paramId], v_min, v_max, 0.0f, format, ImGuiKnobVariant_Tick, KNOB_SIZE, flags,
        DEFAULT_STEP, angle_min, angle_max,
        marks, mark_count, &kScaleMarkStyle, pivot_value))
    {
        setParameterValue(paramId, fParams[paramId]);
        fPresetManager->markModified();
    }

    // NOTE: Putting ImGui::IsItemActivated() in ImGuiKnobs_Mod::Knob() will cause IsItemActivated() unavailable.
    if (ImGui::IsItemActivated())
    {
        editParameter(paramId, true);
        // TODO: Double-click to reset to default value
    }

    if (ImGui::IsItemDeactivated())
        editParameter(paramId, false);

    ImGui::PopFont();        
    ImGui::PopStyleColor(2);
}

void ClassicChorusUI::_addLEDIndicator(const char* label, bool isLit)
{
    ImGui::BeginGroup();

    // Now default font is Droid Sans, so we need to push the chassis font for the knob label and scale marks.
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    ImGui::AlignTextToFramePadding();
    ImGuiExt::LEDIndicator(label, isLit, ImVec4(1.f, 0.f, 0.f, 1.f), 4.0f);
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);    // Fix label vertical misalign
    ImGui::Text("%s", label);

    ImGui::PopFont();

    ImGui::EndGroup();
}

void ClassicChorusUI::_addBinaryStateSwitch(Parameters paramId, const char* label, const char* state0Label, const char* state1Label, float LEDIndentWidth, float btnIndentWidth)
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8.0f);   
    ImGui::BeginGroup();
    
    // WORKAROUND: SameLine() leaves window->DC.IsSameLine == true.
    //             BeginGroup() preserves/restores this flag but does *not* reset it,
    //             so the first ItemSize() inside the group still uses
    //             CursorPosPrevLine.y as the line origin instead of the cursor
    //             position we just adjusted with SetCursorPosY(). The Y offset is
    //             therefore only applied to the LED itself; the label drawn after
    //             the internal SameLine(), plus all following widgets, snap back to
    //             the original line.
    //             Resetting IsSameLine here forces the group to honor the cursor position
    //             which is set above.
    //     See also:
    //             - widgets/imgui-knobs-mod/imgui-knobs.cpp (CurrLineTextBaseOffset
    //             workaround)
    //             - Dear ImGui issue #4190
    ImGui::GetCurrentWindow()->DC.IsSameLine = false;
    
    // Allow user to manually specify indent to make the LEDs and buttons centralized by the bottom label
    // (or fit with their tastes)
    ImGui::Indent(LEDIndentWidth);

    _addLEDIndicator(state0Label, fParams[paramId] == 0.0f);
    _addLEDIndicator(state1Label, fParams[paramId] >= 0.5f);

    ImGui::Unindent(LEDIndentWidth);

    ImGui::Indent(btnIndentWidth);

    {
        ImGui::AlignTextToFramePadding();
        ImGui::BeginGroup();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::PushID(paramId);
        if (ImGuiExt::HardwareButton("SET##WaveformToggle", ImVec2(50, 16), ImVec4(0.4, 0.4, 0.4, 1.0)))
        {
            fParams[paramId] = fParams[paramId] >= 0.5f ? 0.0f : 1.0f;
            setParameterValue(paramId, fParams[paramId]);
            fPresetManager->markModified();
        }
        ImGui::PopFont();
        ImGui::PopID();

        ImGui::EndGroup();             
    }

    ImGui::Unindent(btnIndentWidth);

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("%s", label);
    ImGui::PopFont();

    ImGui::EndGroup();
}

bool ClassicChorusUI::_BeginSection(const char* title, float width)
{
    ImGui::BeginGroup();

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(255, 255, 255, 255));
    ImGuiExt::CenteredSeparatorText(title, width);
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    return true;
}

void ClassicChorusUI::_EndSection()
{
    ImGui::EndGroup();
}

void ClassicChorusUI::_UpdateMouseCursor()
{
    ImGuiMouseCursor mouseCursor = ImGui::GetIO().MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (fLastMouseCursor != mouseCursor)
    {
        fLastMouseCursor = mouseCursor;
        switch (mouseCursor)
        {
        case ImGuiMouseCursor_None:
        case ImGuiMouseCursor_Arrow:
            getWindow().setCursor(MouseCursor::kMouseCursorArrow);
            break;
        case ImGuiMouseCursor_TextInput:
            getWindow().setCursor(MouseCursor::kMouseCursorCaret);
            break;
        case ImGuiMouseCursor_ResizeAll:
            getWindow().setCursor(MouseCursor::kMouseCursorCrosshair);
            break;
        case ImGuiMouseCursor_ResizeNS:
            getWindow().setCursor(MouseCursor::kMouseCursorUpDown);
            break;
        case ImGuiMouseCursor_ResizeEW:
            getWindow().setCursor(MouseCursor::kMouseCursorLeftRight);
            break;
        case ImGuiMouseCursor_ResizeNESW:
            getWindow().setCursor(MouseCursor::kMouseCursorUpRightDownLeft);
            break;
        case ImGuiMouseCursor_ResizeNWSE:
            getWindow().setCursor(MouseCursor::kMouseCursorUpLeftDownRight);
            break;
        case ImGuiMouseCursor_Hand:
            getWindow().setCursor(MouseCursor::kMouseCursorHand);
            break;
        case ImGuiMouseCursor_NotAllowed:
            getWindow().setCursor(MouseCursor::kMouseCursorNotAllowed);
            break;
        default:
            getWindow().setCursor(MouseCursor::kMouseCursorArrow);
            break;
        }
    }
}
