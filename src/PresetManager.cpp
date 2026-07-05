#include "PresetManager.h"
#include "UI.h"

#include "../3rdparty/json.hpp"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>

#if defined(_WIN32)
#  include <windows.h>
#  include <shlobj.h>
#elif defined(__APPLE__)
#  include <unistd.h>
#  include <pwd.h>
#else
// Linux / other POSIX
#  include <unistd.h>
#  include <pwd.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

// ── Factory presets ────────────────────────────────────────────────────────
// Preset fields (struct order): Name, Range, Fine, Rate, Depth, Mix, Level, Spread
static const Preset kFactoryPresets[] = {
    {"Fat on Rhodes",       40.0f,   0.76f,   0.166f,  0.21f,  0.5f,   4.6f,   1.0f},
    {"Clean Guitar Chorus", 10.0f,   0.75f,   0.437f,  0.32f,  0.5f,   4.6f,   1.0f},
    {"Lead Guitar Chorus",  20.0f,   0.75f,   0.263f,  0.25f,  0.5f,   4.6f,   1.0f},
    {"Hard Guitar Flanger", 1.25f,   0.765f,  2.4f,    0.54f,  0.5f,   4.6f,   1.0f},
    {"String Pad Chorus",   20.0f,   1.0f,    0.145f,  0.81f,  0.5f,   4.6f,   1.0f},
    {"String Pad Flanger",  1.25f,   0.5f,    0.1f,    1.0f,   0.5f,   4.6f,   1.0f},
    {"Solid El-Bass",       2.5f,    0.675f,  1.15f,   0.36f,  0.5f,   4.6f,   1.0f},
    {"Lead Synth",          5.0f,    0.935f,  0.437f,  0.98f,  0.5f,   4.6f,   1.0f},
    {"Vocal Overdub",       1.25f,   0.5f,    3.63f,   0.81f,  1.0f,   4.6f,   1.0f},
    {"Vocal Spread",        80.0f,   0.76f,   0.1f,    0.13f,  0.208f, 4.6f,   1.0f},
    {"Rock Singer",         80.0f,   1.0f,    0.275f,  0.16f,  0.216f, 4.6f,   1.0f},
    {"Fat Voice",           20.0f,   0.755f,  0.363f,  0.52f,  0.505f, 4.6f,   1.0f},
    {"Flanging Snare",      1.25f,   0.765f,  0.525f,  1.0f,   0.505f, 4.6f,   1.0f},
    {"Stereo to Cymbal",    20.0f,   0.78f,   0.457f,  0.32f,  0.505f, 4.6f,   1.0f},
    {"Unstable Record Player", 10.0f, 1.0f,   0.479f,  1.0f,   1.0f,   4.6f,   0.0f},
    {"I Come in Peace",        10.0f, 0.5f,   10.0f,   1.0f,   0.808f, 4.6f,   1.0f},
};
static constexpr int kFactoryPresetsCount = (int)(sizeof(kFactoryPresets) / sizeof(kFactoryPresets[0]));

// ── Default preset ─────────────────────────────────────────────────────────
// This is used as a fallback when loading an imported preset from a file that doesn't contain all the expected fields.
// Since the Preset struct has default member initializers, we can just use the field values from the default-constructed Preset object.
static const Preset kDefaultPreset;

// ── Constructor ────────────────────────────────────────────────────────────
PresetManager::PresetManager(ClassicChorusUI* ui)
    : fUI(ui)
{
}

// ── Factory preset accessors ───────────────────────────────────────────────
int PresetManager::factoryPresetCount() const
{
    return kFactoryPresetsCount;
}

const Preset& PresetManager::factoryPreset(int index) const
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < kFactoryPresetsCount, kFactoryPresets[0])
    return kFactoryPresets[index];
}

// ── User preset accessors ──────────────────────────────────────────────────
int PresetManager::userPresetCount() const
{
    return (int)fUserPresets.size();
}

const Preset& PresetManager::userPreset(int index) const
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < (int)fUserPresets.size(), fImportedPreset)
    return fUserPresets[index];
}

// ── Preset selection ───────────────────────────────────────────────────────
void PresetManager::loadDefaultPreset()
{
    DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, )
    for (uint32_t i = 0; i < NUM_PARAMS; ++i)
        _triggerParamUpdate(i, paramInfo[i].defaultVal);
    syncPluginState(PresetType::Factory, -1, false);
}

void PresetManager::selectFactoryPreset(int index)
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < kFactoryPresetsCount, )
    _applyPreset(kFactoryPresets[index]);
    syncPluginState(PresetType::Factory, index, false);
}

void PresetManager::selectUserPreset(int index)
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < (int)fUserPresets.size(), )
    _applyPreset(fUserPresets[index]);
    syncPluginState(PresetType::User, index, false);
}

void PresetManager::selectImportedPreset()
{
    _applyPreset(fImportedPreset);
    syncPluginState(PresetType::Imported, -1, false);
}

// ── User preset CRUD ──────────────────────────────────────────────────────
bool PresetManager::nameExists(const std::string& name) const
{
    for (const auto& p : fUserPresets)
        if (p.name == name) return true;
    return false;
}

bool PresetManager::saveAsNew(const std::string& name)
{
    if (nameExists(name))
        return false;

    Preset p  = snapshotFromUI();
    p.name    = name;
    fUserPresets.push_back(p);

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(PresetType::User, (int)fUserPresets.size() - 1, false);
    return ok;
}

bool PresetManager::overwriteCurrent()
{
    if (fCurrentType != PresetType::User)
        return false;
    DISTRHO_SAFE_ASSERT_RETURN(fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size(), false)

    const std::string name = fUserPresets[fCurrentIndex].name;
    Preset p = snapshotFromUI();
    p.name   = name;
    fUserPresets[fCurrentIndex] = p;

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(false);
    return ok;
}

bool PresetManager::deleteCurrent()
{
    if (fCurrentType != PresetType::User)
        return false;
    DISTRHO_SAFE_ASSERT_RETURN(fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size(), false)

    fUserPresets.erase(fUserPresets.begin() + fCurrentIndex);
    PresetType newType;
    int        newIndex;
    if (fUserPresets.empty()) {
        newType  = PresetType::Factory;
        newIndex = 0;
    } else {
        newType  = PresetType::User;
        newIndex = std::min(fCurrentIndex, (int)fUserPresets.size() - 1);
    }

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(newType, newIndex, false);
    return ok;
}

bool PresetManager::renameCurrent(const std::string& newName)
{
    if (fCurrentType != PresetType::User)
        return false;
    DISTRHO_SAFE_ASSERT_RETURN(fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size(), false)

    // No-op: preset already has this name.
    if (fUserPresets[fCurrentIndex].name == newName)
        return true;
    // Reject: another preset already uses this name.
    if (nameExists(newName))
        return false;

    fUserPresets[fCurrentIndex].name = newName;

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(fModified);
    return ok;
}

// ── Import / Export ────────────────────────────────────────────────────────
bool PresetManager::hasImported() const
{
    return !fImportedPreset.name.empty();
}

const Preset* PresetManager::importedPreset() const
{
    return hasImported() ? &fImportedPreset : nullptr;
}

bool PresetManager::importFromFile(const std::string& filePath)
{
    try {
        std::ifstream f(filePath);
        if (!f.is_open()) return false;
        json j = json::parse(f);

        fImportedPreset.name        = j.value("name",         "Imported");
        fImportedPreset.range        = j.value("range",         kDefaultPreset.range);
        fImportedPreset.fine       = j.value("fine",        kDefaultPreset.fine);
        fImportedPreset.rate    = j.value("rate",     kDefaultPreset.rate);
        fImportedPreset.depth     = j.value("depth",     kDefaultPreset.depth);
        fImportedPreset.mix         = j.value("mix",          kDefaultPreset.mix);
        fImportedPreset.level       = j.value("level",        kDefaultPreset.level);
        fImportedPreset.spread      = j.value("spread",       kDefaultPreset.spread);
        selectImportedPreset();
        return true;
    } catch (...) {
        return false;
    }
}

bool PresetManager::exportCurrentToFile(const std::string& filePath)
{
    const Preset* p = currentPreset();
    if (!p)
        return false;

    try {
        json j;
        j["name"]          = p->name;
        j["range"]         = p->range;
        j["fine"]          = p->fine;
        j["rate"]          = p->rate;
        j["depth"]         = p->depth;
        j["mix"]           = p->mix;
        j["level"]         = p->level;
        j["spread"]        = p->spread;
        std::ofstream f(filePath);
        if (!f.is_open()) return false;
        f << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

bool PresetManager::commitImported(const std::string& name)
{
    Preset p = fImportedPreset;
    p.name   = name;
    fUserPresets.push_back(p);

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(PresetType::User, (int)fUserPresets.size() - 1, false);
    return ok;
}

// ── Disk I/O ───────────────────────────────────────────────────────────────
bool PresetManager::loadUserPresetsFromDisk()
{
    const std::string path = _getUserPresetsFilePath();
    try {
        std::ifstream f(path);
        if (!f.is_open()) return true; // File not found yet — OK
        json j = json::parse(f);
        fUserPresets.clear();
        for (auto& entry : j["presets"]) {
            Preset p;
            p.name          = entry.value("name",           "");
            p.range         = entry.value("range",          kDefaultPreset.range);
            p.fine          = entry.value("fine",           kDefaultPreset.fine);
            p.rate          = entry.value("rate",           kDefaultPreset.rate);
            p.depth         = entry.value("depth",          kDefaultPreset.depth);
            p.mix           = entry.value("mix",            kDefaultPreset.mix);
            p.level         = entry.value("level",          kDefaultPreset.level);
            p.spread        = entry.value("spread",         kDefaultPreset.spread);
            if (!p.name.empty())
                fUserPresets.push_back(std::move(p));
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool PresetManager::saveUserPresetsToDisk()
{
    if (!_ensureDataDirExists())
        return false;

    const std::string path = _getUserPresetsFilePath();
    try {
        json arr = json::array();
        for (const auto& p : fUserPresets) {
            json entry;
            entry["name"]          = p.name;
            entry["range"]         = p.range;
            entry["fine"]          = p.fine;
            entry["rate"]          = p.rate;
            entry["depth"]         = p.depth;
            entry["mix"]           = p.mix;
            entry["level"]         = p.level;
            entry["spread"]        = p.spread;
            arr.push_back(entry);
        }
        json j;
        j["version"] = 1;
        j["presets"] = arr;
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

// ── State accessors ────────────────────────────────────────────────────────
const Preset* PresetManager::currentPreset() const
{
    switch (fCurrentType) {
    case PresetType::Factory:
        if (fCurrentIndex >= 0 && fCurrentIndex < kFactoryPresetsCount)
            return &kFactoryPresets[fCurrentIndex];
        break;
    case PresetType::User:
        if (fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size())
            return &fUserPresets[fCurrentIndex];
        break;
    case PresetType::Imported:
        return &fImportedPreset;
    }
    return nullptr;
}

void PresetManager::markModified()
{
    if (!fModified)
        syncPluginState(true);
}

void PresetManager::clearModified()
{
    if (fModified)
        syncPluginState(false);
}

void PresetManager::syncPluginState(PresetType type, int index, bool modified)
{
    fCurrentType  = type;
    fCurrentIndex = index;
    syncPluginState(modified);
}

void PresetManager::syncPluginState(bool modified)
{
    fModified = modified;
    DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, )

    const Preset* p      = currentPreset();
    const char*   name   = p ? p->name.c_str() : "";
    const char*   mod    = fModified ? "true" : "false";
    const char*   typeStr;

    switch (fCurrentType) {
    case PresetType::Factory:  typeStr = "Factory";  break;
    case PresetType::User:     typeStr = "User";     break;
    case PresetType::Imported: typeStr = "Imported"; break;
    default:                   typeStr = "Factory";  break;
    }

    fUI->setState(STATE_PRESET_NAME,     name);
    fUI->setState(STATE_PRESET_MODIFIED, mod);
    fUI->setState(STATE_PRESET_TYPE,     typeStr);
}

Preset PresetManager::snapshotFromUI() const
{
    Preset p;
    p.range       = fUI->fParams[pParamRange];
    p.fine        = fUI->fParams[pParamFine];
    p.rate        = fUI->fParams[pParamRate];
    p.depth       = fUI->fParams[pParamDepth];
    p.mix         = fUI->fParams[pParamMix];
    p.level       = fUI->fParams[pParamLevel];
    p.spread      = fUI->fParams[pParamSpread];
    return p;
}

void PresetManager::restoreFromState(const std::string& typeStr,
                                     const std::string& nameStr,
                                     bool modified)
{
    // Restore type
    if (typeStr == "User")          fCurrentType = PresetType::User;
    else if (typeStr == "Imported") fCurrentType = PresetType::Imported;
    else                            fCurrentType = PresetType::Factory;

    // Restore index by searching for the name in the appropriate list.
    // Factory fallback is -1 ("Default") — not index 0 — so that an empty
    // preset_name on first launch resolves to Default, not the first factory preset.
    fCurrentIndex = -1;
    if (fCurrentType == PresetType::Factory) {
        for (int i = 0; i < kFactoryPresetsCount; ++i) {
            if (kFactoryPresets[i].name == nameStr) { fCurrentIndex = i; break; }
        }
    } else if (fCurrentType == PresetType::User) {
        fCurrentIndex = -1;
        for (int i = 0; i < (int)fUserPresets.size(); ++i) {
            if (fUserPresets[i].name == nameStr) { fCurrentIndex = i; break; }
        }
        // If the user preset no longer exists (e.g. deleted after state was saved),
        // fall back to Factory / -1 (= Default) so the plugin is in a defined state.
        if (fCurrentIndex == -1) {
            fCurrentType  = PresetType::Factory;
            fModified     = false;
        }
    }
    // For Imported, index stays -1 (the name is in fImportedPreset which we can't restore)

    fModified = modified;
}

// ── Private: parameter application ────────────────────────────────────────
void PresetManager::_applyPreset(const Preset& preset)
{
    DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, )
    _triggerParamUpdate(pParamRange,       preset.range);
    _triggerParamUpdate(pParamFine,        preset.fine);
    _triggerParamUpdate(pParamRate,        preset.rate);
    _triggerParamUpdate(pParamDepth,       preset.depth);
    _triggerParamUpdate(pParamMix,         preset.mix);
    _triggerParamUpdate(pParamLevel,       preset.level);
    _triggerParamUpdate(pParamSpread,      preset.spread);
}

void PresetManager::_triggerParamUpdate(uint32_t index, float value)
{
    DISTRHO_SAFE_ASSERT_RETURN(index < NUM_PARAMS, )
    fUI->setParameterValue(index, value);
    fUI->parameterChanged(index, value);
}

// ── Private: platform-specific data directory ─────────────────────────────
std::string PresetManager::_getUserDataDir() const
{
#if defined(_WIN32)
    PWSTR wpath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &wpath))) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, nullptr, 0, nullptr, nullptr);
        std::string result(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wpath, -1, &result[0], len, nullptr, nullptr);
        CoTaskMemFree(wpath);
        return result + "\\" CLASSIC_FLANGER_APPDATA_DIR_NAME;
    }
    // Fallback
    const char* appdata = getenv("APPDATA");
    return std::string(appdata ? appdata : ".") + "\\" CLASSIC_FLANGER_APPDATA_DIR_NAME;
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : nullptr;
    }
    return std::string(home ? home : ".") + "/Library/Application Support/" CLASSIC_FLANGER_APPDATA_DIR_NAME;
#else
    // Linux / other POSIX
    const char* xdgData = getenv("XDG_DATA_HOME");
    if (xdgData && xdgData[0] != '\0')
        return std::string(xdgData) + "/" CLASSIC_FLANGER_APPDATA_DIR_NAME;
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : nullptr;
    }
    return std::string(home ? home : ".") + "/.local/share/" CLASSIC_FLANGER_APPDATA_DIR_NAME;
#endif
}

std::string PresetManager::_getUserPresetsFilePath() const
{
#if defined(_WIN32)
    return _getUserDataDir() + "\\" CLASSIC_FLANGER_PRESET_FILE_NAME;
#else
    return _getUserDataDir() + "/" CLASSIC_FLANGER_PRESET_FILE_NAME;
#endif
}

bool PresetManager::_ensureDataDirExists() const
{
    try {
        fs::create_directories(_getUserDataDir());
        return true;
    } catch (...) {
        return false;
    }
}
