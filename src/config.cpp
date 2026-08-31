#include "config.hpp"
#include "service_imports.hpp"
#include "update_service.hpp"

#include "mods/service.hpp"
#include "mods/svc/config.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>

namespace dawnlight {
namespace {

ConfigVarHandle s_healthScale = 0;
ConfigVarHandle s_automaticHealthScale = 0;
ConfigVarHandle s_saveCompatibility = 0;
ConfigVarHandle s_itemIntegrity = 0;
ConfigVarHandle s_newSaveMode = 0;
ConfigVarHandle s_aimMode = 0;
ConfigVarHandle s_aimMovement = 0;
ConfigVarHandle s_cinemaZoomPercent = 0;
ConfigVarHandle s_bulletTime = 0;
ConfigVarHandle s_manualShielding = 0;
ConfigVarHandle s_rJump = 0;
ConfigVarHandle s_zItemSlot = 0;
ConfigVarHandle s_dawnlightTouchUi = 0;
ConfigVarHandle s_checkForUpdates = 0;
ConfigVarHandle s_bossrushHardmodeHazards = 0;
ConfigVarHandle s_hudLayout = 0;
ConfigVarHandle s_legacyWiiUHud = 0;
ConfigVarHandle s_roundXYButtons = 0;
ConfigVarHandle s_hudButtonBackingVisible = 0;
ConfigVarHandle s_hudHealthBar = 0;
ConfigVarHandle s_aimDefaultsMigrated = 0;
ConfigVarHandle s_hudLayoutMigrated = 0;
ConfigVarHandle s_hudLayoutMigratedV2 = 0;

constexpr size_t kHudElementCount = static_cast<size_t>(HudElement::Count);
constexpr size_t kHudButtonCount = static_cast<size_t>(HudButton::Count);
constexpr size_t kCustomModelCount = static_cast<size_t>(CustomModel::Count);

constexpr std::array<const char*, kCustomModelCount> kCustomModelConfigNames = {{
    "model-ordon-link",
    "model-hero-clothes",
    "model-zora-armor",
    "model-magic-armor",
    "model-wolf-link",
    "model-sumo-link",
    "model-horse",
    "model-items",
}};

std::array<ConfigVarHandle, kHudElementCount> s_hudElementX = {};
std::array<ConfigVarHandle, kHudElementCount> s_hudElementY = {};
std::array<ConfigVarHandle, kHudElementCount> s_hudElementScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemOffsetX = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemOffsetY = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonAmmoOffsetX = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonAmmoOffsetY = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonAmmoScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextOffsetX = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextOffsetY = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemAnchor = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextAnchor = {};
ConfigVarHandle s_hudDpadFollowsMinimap = 0;
ConfigVarHandle s_hudMinimapSlideDirection = 0;
ConfigVarHandle s_hudDpadHideArrows = 0;
ConfigVarHandle s_hudDpadHideShadows = 0;
std::array<ConfigVarHandle, kCustomModelCount> s_customModels = {};

void on_z_item_slot_changed(ModContext* ctx, ConfigVarHandle, const ConfigVarValue*,
    const ConfigVarValue*, void*) {
    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.title_rml = "Z-Items";
    toast.body_rml = "Restart game after toggling Z-items";
    svc_ui->push_toast(ctx, &toast);
}

void on_dawnlight_touch_ui_changed(ModContext* ctx, ConfigVarHandle, const ConfigVarValue*,
    const ConfigVarValue*, void*) {
    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.title_rml = "Dawnlight Touch UI";
    toast.body_rml = "Restart game after toggling Dawnlight Touch UI";
    svc_ui->push_toast(ctx, &toast);
}

void on_custom_model_changed(ModContext* ctx, ConfigVarHandle, const ConfigVarValue*,
    const ConfigVarValue*, void*) {
    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.title_rml = "Models";
    toast.body_rml = "Restart game to apply model changes";
    svc_ui->push_toast(ctx, &toast);
}

void on_check_for_updates_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value,
    const ConfigVarValue*, void*) {
    if (value != nullptr) {
        g_configCheckForUpdatesEnabled = value->bool_value;
    }
}

struct HudElementDefaults {
    const char* name;
    int x;
    int y;
    int scale;
};

struct HudButtonDefaults {
    const char* name;
    int itemOffsetX;
    int itemOffsetY;
    int itemScale;
    int ammoOffsetX;
    int ammoOffsetY;
    int ammoScale;
    int textOffsetX;
    int textOffsetY;
    int textScale;
    int itemAnchor;
    int textAnchor;
};

using HudElementDefaultArray = std::array<HudElementDefaults, kHudElementCount>;
using HudButtonDefaultArray = std::array<HudButtonDefaults, kHudButtonCount>;

constexpr HudElementDefaultArray kGameCubeHudElementDefaults = {{
    {"a", 0, 0, 100},
    {"b", 0, 0, 100},
    {"x", 0, 0, 100},
    {"y", 0, 0, 100},
    {"z", 0, 0, 100},
    {"button-backing", 0, 0, 100},
    {"dpad", 0, 0, 100},
    {"midna", 0, 0, 100},
    {"hearts", 0, 0, 100},
    {"rupees", 0, 0, 100},
    {"keys", 0, 0, 100},
    {"oil", 0, 0, 100},
    {"oxygen", 0, 0, 100},
    {"minimap", 0, 0, 100},
    {"dpad-items-text", 0, 0, 100},
    {"dpad-map-text", 0, 0, 100},
}};

constexpr HudButtonDefaultArray kGameCubeHudButtonDefaults = {{
    {"a", 0, 0, 100, 0, 0, 100, 0, 0, 100, 1, 0},
    {"b", 0, 0, 100, 0, 0, 100, 0, 0, 100, 1, 0},
    {"x", 0, 0, 100, 0, 0, 100, 0, 0, 100, 1, 0},
    {"y", 0, 0, 100, 0, 0, 100, 0, 0, 100, 0, 0},
    {"z", 0, 0, 100, 0, 0, 100, 0, 0, 100, 1, 0},
}};

constexpr std::array<HudElementDefaults, kHudElementCount> kHudElementDefaults = {{
    {"a", -35, 25, 100},
    {"b", 20, -27, 149},
    {"x", -102, -1, 170},
    {"y", -22, 0, 170},
    {"z", 0, 0, 100},
    {"button-backing", -100, 0, 100},
    {"dpad", 0, -280, 100},
    {"midna", -6, 0, 100},
    {"hearts", 0, 0, 100},
    {"rupees", 0, 0, 100},
    {"keys", 0, 0, 100},
    {"oil", 0, 0, 100},
    {"oxygen", 0, 0, 100},
    {"minimap", 0, 50, 70},
    {"dpad-items-text", 0, 0, 100},
    {"dpad-map-text", 0, 0, 100},
}};

constexpr std::array<HudButtonDefaults, kHudButtonCount> kHudButtonDefaults = {{
    {"a", 0, 0, 100, 0, 0, 100, 200, 0, 100, 1, 1},
    {"b", 30, 0, 50, 0, 0, 100, 160, 0, 50, 2, 1},
    {"x", -15, -15, 50, 0, 0, 100, 100, -15, 50, 0, 1},
    {"y", 0, 0, 50, 0, 0, 100, 150, 0, 50, 2, 1},
    {"z", 0, 0, 100, 0, -15, 70, 0, 0, 100, 1, 0},
}};

constexpr HudElementDefaultArray kWiiUHudElementDefaults = {{
    {"a", -5, -3, 100},
    {"b", -11, 5, 150},
    {"x", -73, -35, 170},
    {"y", -52, 32, 170},
    {"z", 0, 0, 100},
    {"button-backing", -100, 0, 100},
    {"dpad", 0, -280, 100},
    {"midna", -6, 0, 100},
    {"hearts", 0, 0, 100},
    {"rupees", 0, 0, 100},
    {"keys", 0, 0, 100},
    {"oil", 0, 0, 100},
    {"oxygen", 0, 0, 100},
    {"minimap", 0, 50, 70},
    {"dpad-items-text", 0, 0, 100},
    {"dpad-map-text", 0, 0, 100},
}};

constexpr HudButtonDefaultArray kWiiUHudButtonDefaults = {{
    {"a", 0, 0, 100, 0, 0, 100, 200, 0, 100, 1, 1},
    {"b", 30, 0, 50, 0, 0, 100, 160, 0, 50, 2, 1},
    {"x", -10, -5, 50, 0, 0, 100, 30, -15, 50, 2, 0},
    {"y", 20, 6, 50, 0, 0, 100, 65, 0, 50, 0, 0},
    {"z", 0, 0, 100, 0, -15, 80, 0, 0, 100, 1, 0},
}};

constexpr HudElementDefaultArray kDawnlightHudElementDefaults = {{
    {"a", -135, 25, 100},
    {"b", -80, -27, 150},
    {"x", -202, -1, 170},
    {"y", -122, 0, 170},
    {"z", -100, 0, 100},
    {"button-backing", -100, 0, 100},
    {"dpad", 0, -15, 100},
    {"midna", 0, 0, 100},
    {"hearts", 100, 0, 100},
    {"rupees", 40, 0, 100},
    {"keys", 0, 0, 100},
    {"oil", 100, 0, 100},
    {"oxygen", 100, 0, 100},
    {"minimap", 730, -190, 70},
    {"dpad-items-text", 0, 0, 100},
    {"dpad-map-text", 0, 0, 100},
}};

constexpr HudButtonDefaultArray kDawnlightHudButtonDefaults = {{
    {"a", 0, 0, 100, 0, 0, 100, 200, 0, 100, 1, 1},
    {"b", 30, 0, 50, 0, 0, 100, 160, 0, 50, 2, 1},
    {"x", 0, 0, 50, 0, 0, 100, 100, -15, 50, 0, 1},
    {"y", 0, 0, 50, 0, 0, 100, 150, 0, 50, 2, 1},
    {"z", 0, 0, 100, 0, -15, 70, 0, 0, 100, 1, 0},
}};

constexpr std::array<const char*, kHudElementCount> kHudElementJsonNames = {{
    "A",
    "B",
    "X",
    "Y",
    "Z",
    "Button Backing",
    "D-Pad",
    "Midna",
    "Hearts",
    "Rupees",
    "Keys",
    "Oil",
    "Oxygen",
    "Minimap",
    "D-Pad Items Text",
    "D-Pad Map Text",
}};

constexpr std::array<const char*, kHudButtonCount> kHudButtonJsonNames = {{
    "A",
    "B",
    "X",
    "Y",
    "Z",
}};

constexpr const char* kHudSettingsFileName = "hud_layout_settings.json";
constexpr const char* kItemAnchorNames[] = {"Left", "Right", "Top", "Bottom"};
constexpr const char* kTextAnchorNames[] = {"Left", "Right"};
constexpr const char* kSlideDirectionNames[] = {"Left -> Right", "Right -> Left"};

size_t hud_element_index(HudElement element) {
    return std::clamp<size_t>(static_cast<size_t>(element), 0, kHudElementCount - 1);
}

size_t hud_button_index(HudButton button) {
    return std::clamp<size_t>(static_cast<size_t>(button), 0, kHudButtonCount - 1);
}

size_t custom_model_index(CustomModel model) {
    return std::clamp<size_t>(static_cast<size_t>(model), 0, kCustomModelCount - 1);
}

ModResult register_bool(const char* name, bool defaultValue, ConfigVarHandle& handle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_BOOL;
    desc.default_bool = defaultValue;
    return svc_config->register_var(mod_ctx, &desc, &handle);
}

ModResult register_int(const char* name, int64_t defaultValue, ConfigVarHandle& handle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_INT;
    desc.default_int = defaultValue;
    return svc_config->register_var(mod_ctx, &desc, &handle);
}

ModResult register_custom_int(
    const char* category, const char* name, const char* field, int defaultValue,
    ConfigVarHandle& handle) {
    char key[64];
    std::snprintf(key, sizeof(key), "hud-custom-%s-%s-%s", category, name, field);
    return register_int(key, defaultValue, handle);
}

bool get_bool(ConfigVarHandle handle, bool fallback) {
    bool value = fallback;
    if (handle != 0) {
        svc_config->get_bool(mod_ctx, handle, &value);
    }
    return value;
}

int get_int(ConfigVarHandle handle, int fallback, int min, int max) {
    int64_t value = fallback;
    if (handle != 0) {
        svc_config->get_int(mod_ctx, handle, &value);
    }
    return static_cast<int>(std::clamp<int64_t>(value, min, max));
}

bool set_int(ConfigVarHandle handle, int value) {
    return handle != 0 && svc_config->set_int(mod_ctx, handle, value) == MOD_OK;
}

bool set_bool(ConfigVarHandle handle, bool value) {
    return handle != 0 && svc_config->set_bool(mod_ctx, handle, value) == MOD_OK;
}

int scale_to_percent(double scale) {
    return static_cast<int>(std::clamp<long long>(std::llround(scale * 100.0), 1, 9999));
}

int position_to_int(double value) {
    return static_cast<int>(std::clamp<long long>(std::llround(value), -9999, 9999));
}

const char* item_anchor_name(int anchor) {
    return kItemAnchorNames[std::clamp(anchor, 0, 3)];
}

const char* text_anchor_name(int anchor) {
    return kTextAnchorNames[std::clamp(anchor, 0, 1)];
}

std::filesystem::path hud_settings_file_path() {
    const char* dataDir = nullptr;
    if (svc_host == nullptr || svc_host->data_dir(mod_ctx, &dataDir) != MOD_OK || dataDir == nullptr) {
        return {};
    }

    const std::filesystem::path dataPath(dataDir);
    const std::filesystem::path configRoot = dataPath.parent_path().parent_path();
    if (configRoot.empty()) {
        return {};
    }
    return configRoot / "mods" / kHudSettingsFileName;
}

bool set_custom_hud_from_defaults(const HudElementDefaultArray& elementDefaults,
    const HudButtonDefaultArray& buttonDefaults, bool dpadFollowsMinimap,
    int minimapSlideDirection, bool roundXYButtons, bool buttonBackingVisible, bool setLayout) {
    for (size_t i = 0; i < kHudElementCount; ++i) {
        const auto& defaults = elementDefaults[i];
        if (!set_int(s_hudElementX[i], defaults.x) || !set_int(s_hudElementY[i], defaults.y) ||
            !set_int(s_hudElementScale[i], defaults.scale))
        {
            return false;
        }
    }

    for (size_t i = 0; i < kHudButtonCount; ++i) {
        const auto& defaults = buttonDefaults[i];
        if (!set_int(s_hudButtonItemOffsetX[i], defaults.itemOffsetX) ||
            !set_int(s_hudButtonItemOffsetY[i], defaults.itemOffsetY) ||
            !set_int(s_hudButtonItemScale[i], defaults.itemScale) ||
            !set_int(s_hudButtonAmmoOffsetX[i], defaults.ammoOffsetX) ||
            !set_int(s_hudButtonAmmoOffsetY[i], defaults.ammoOffsetY) ||
            !set_int(s_hudButtonAmmoScale[i], defaults.ammoScale) ||
            !set_int(s_hudButtonTextOffsetX[i], defaults.textOffsetX) ||
            !set_int(s_hudButtonTextOffsetY[i], defaults.textOffsetY) ||
            !set_int(s_hudButtonTextScale[i], defaults.textScale) ||
            !set_int(s_hudButtonItemAnchor[i], defaults.itemAnchor) ||
            !set_int(s_hudButtonTextAnchor[i], defaults.textAnchor))
        {
            return false;
        }
    }

    if (!set_bool(s_hudDpadFollowsMinimap, dpadFollowsMinimap) ||
        !set_int(s_hudMinimapSlideDirection, minimapSlideDirection) ||
        !set_bool(s_hudDpadHideArrows, false) ||
        !set_bool(s_hudDpadHideShadows, false) ||
        !set_bool(s_roundXYButtons, roundXYButtons) ||
        !set_bool(s_hudButtonBackingVisible, buttonBackingVisible) ||
        !set_bool(s_hudHealthBar, false))
    {
        return false;
    }

    return !setLayout ||
           set_int(s_hudLayout, static_cast<int>(HudLayout::Custom));
}

bool set_custom_hud_to_xbox_defaults(bool setLayout) {
    return set_custom_hud_from_defaults(
        kHudElementDefaults, kHudButtonDefaults, false, 0, true, false, setLayout);
}

size_t find_key_value_start(const std::string& json, const char* key, size_t from = 0) {
    const std::string needle = std::string("\"") + key + "\"";
    const size_t keyPos = json.find(needle, from);
    if (keyPos == std::string::npos) {
        return std::string::npos;
    }
    const size_t colon = json.find(':', keyPos + needle.size());
    if (colon == std::string::npos) {
        return std::string::npos;
    }
    size_t value = colon + 1;
    while (value < json.size() && std::isspace(static_cast<unsigned char>(json[value]))) {
        ++value;
    }
    return value;
}

bool read_json_object(const std::string& json, const char* key, std::string& outObject) {
    const size_t value = find_key_value_start(json, key);
    if (value == std::string::npos || value >= json.size() || json[value] != '{') {
        return false;
    }

    bool inString = false;
    bool escaped = false;
    int depth = 0;
    const size_t bodyStart = value + 1;
    for (size_t i = value; i < json.size(); ++i) {
        const char c = json[i];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                outObject = json.substr(bodyStart, i - bodyStart);
                return true;
            }
        }
    }
    return false;
}

bool read_json_number(const std::string& json, const char* key, double& outValue) {
    const size_t value = find_key_value_start(json, key);
    if (value == std::string::npos) {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const char* start = json.c_str() + value;
    const double parsed = std::strtod(start, &end);
    if (end == start || errno == ERANGE) {
        return false;
    }
    outValue = parsed;
    return true;
}

bool read_json_bool(const std::string& json, const char* key, bool& outValue) {
    const size_t value = find_key_value_start(json, key);
    if (value == std::string::npos) {
        return false;
    }
    if (json.compare(value, 4, "true") == 0) {
        outValue = true;
        return true;
    }
    if (json.compare(value, 5, "false") == 0) {
        outValue = false;
        return true;
    }
    return false;
}

bool read_json_string(const std::string& json, const char* key, std::string& outValue) {
    const size_t value = find_key_value_start(json, key);
    if (value == std::string::npos || value >= json.size() || json[value] != '"') {
        return false;
    }

    std::string parsed;
    bool escaped = false;
    for (size_t i = value + 1; i < json.size(); ++i) {
        const char c = json[i];
        if (escaped) {
            parsed.push_back(c);
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            outValue = parsed;
            return true;
        } else {
            parsed.push_back(c);
        }
    }
    return false;
}

bool apply_json_number(
    const std::string& object, const char* key, ConfigVarHandle handle, bool isScale) {
    double value = 0.0;
    if (!read_json_number(object, key, value)) {
        return true;
    }
    return set_int(handle, isScale ? scale_to_percent(value) : position_to_int(value));
}

bool apply_anchor_string(const std::string& object, const char* key, ConfigVarHandle handle,
    const char* const* names, size_t count) {
    std::string value;
    if (!read_json_string(object, key, value)) {
        return true;
    }
    for (size_t i = 0; i < count; ++i) {
        if (value == names[i]) {
            return set_int(handle, static_cast<int>(i));
        }
    }
    return true;
}

void write_json_number(std::ostream& out, const char* key, double value, bool comma) {
    out << "            \"" << key << "\": " << value << (comma ? "," : "") << "\n";
}

void write_json_string(std::ostream& out, const char* key, const char* value, bool comma) {
    out << "            \"" << key << "\": \"" << value << "\"" << (comma ? "," : "") << "\n";
}

void write_json_bool(std::ostream& out, const char* key, bool value, bool comma) {
    out << "            \"" << key << "\": " << (value ? "true" : "false")
        << (comma ? "," : "") << "\n";
}

void write_button_layout_fields(std::ostream& out, HudButton button, bool hasItem, bool hasAmmo,
    bool hasText, bool hasTrailingElementField) {
    if (hasItem) {
        write_json_string(
            out, "itemAnchor", item_anchor_name(hud_custom_button_item_anchor(button)), true);
        write_json_number(out, "itemOffsetX", hud_custom_button_item_offset_x(button), true);
        write_json_number(out, "itemOffsetY", hud_custom_button_item_offset_y(button), true);
        write_json_number(
            out, "itemScale", hud_custom_button_item_scale_percent(button) / 100.0, true);
    }
    if (hasAmmo) {
        write_json_number(out, "ammoOffsetX", hud_custom_button_ammo_offset_x(button), true);
        write_json_number(out, "ammoOffsetY", hud_custom_button_ammo_offset_y(button), true);
        write_json_number(
            out, "ammoScale", hud_custom_button_ammo_scale_percent(button) / 100.0, true);
    }
    if (hasText) {
        write_json_string(
            out, "textAnchor", text_anchor_name(hud_custom_button_text_anchor(button)), true);
        write_json_number(out, "textOffsetX", hud_custom_button_text_offset_x(button), true);
        write_json_number(out, "textOffsetY", hud_custom_button_text_offset_y(button), true);
        write_json_number(out, "textScale", hud_custom_button_text_scale_percent(button) / 100.0,
            hasTrailingElementField);
    }
}

bool button_for_element(
    HudElement element, HudButton& outButton, bool& outHasItem, bool& outHasAmmo, bool& outHasText) {
    outHasItem = false;
    outHasAmmo = false;
    outHasText = false;

    switch (element) {
    case HudElement::A:
        outButton = HudButton::A;
        outHasText = true;
        return true;
    case HudElement::B:
        outButton = HudButton::B;
        outHasItem = true;
        outHasText = true;
        return true;
    case HudElement::X:
        outButton = HudButton::X;
        outHasItem = true;
        outHasAmmo = true;
        outHasText = true;
        return true;
    case HudElement::Y:
        outButton = HudButton::Y;
        outHasItem = true;
        outHasAmmo = true;
        outHasText = true;
        return true;
    case HudElement::Z:
        outButton = HudButton::Z;
        outHasItem = true;
        outHasAmmo = true;
        outHasText = true;
        return true;
    default:
        return false;
    }
}

bool apply_button_layout_json(const std::string& object, HudButton button, bool hasItem,
    bool hasAmmo, bool hasText) {
    const size_t index = hud_button_index(button);
    if (hasItem) {
        if (!apply_anchor_string(object, "itemAnchor", s_hudButtonItemAnchor[index],
                kItemAnchorNames, std::size(kItemAnchorNames)) ||
            !apply_json_number(object, "itemOffsetX", s_hudButtonItemOffsetX[index], false) ||
            !apply_json_number(object, "itemOffsetY", s_hudButtonItemOffsetY[index], false) ||
            !apply_json_number(object, "itemScale", s_hudButtonItemScale[index], true))
        {
            return false;
        }
    }
    if (hasAmmo &&
        (!apply_json_number(object, "ammoOffsetX", s_hudButtonAmmoOffsetX[index], false) ||
            !apply_json_number(object, "ammoOffsetY", s_hudButtonAmmoOffsetY[index], false) ||
            !apply_json_number(object, "ammoScale", s_hudButtonAmmoScale[index], true)))
    {
        return false;
    }
    if (hasText) {
        if (!apply_anchor_string(object, "textAnchor", s_hudButtonTextAnchor[index],
                kTextAnchorNames, std::size(kTextAnchorNames)) ||
            !apply_json_number(object, "textOffsetX", s_hudButtonTextOffsetX[index], false) ||
            !apply_json_number(object, "textOffsetY", s_hudButtonTextOffsetY[index], false) ||
            !apply_json_number(object, "textScale", s_hudButtonTextScale[index], true))
        {
            return false;
        }
    }
    return true;
}

bool apply_element_json(const std::string& elementsObject, HudElement element) {
    const size_t index = hud_element_index(element);
    std::string object;
    if (!read_json_object(elementsObject, kHudElementJsonNames[index], object)) {
        return true;
    }

    if (!apply_json_number(object, "x", s_hudElementX[index], false) ||
        !apply_json_number(object, "y", s_hudElementY[index], false) ||
        !apply_json_number(object, "scale", s_hudElementScale[index], true))
    {
        return false;
    }

    if (element == HudElement::Minimap) {
        bool follows = false;
        if (read_json_bool(object, "dpadFollowsMinimap", follows) &&
            !set_bool(s_hudDpadFollowsMinimap, follows))
        {
            return false;
        }

        std::string slideDirection;
        if (read_json_string(object, "slideDirection", slideDirection)) {
            const int value = slideDirection == kSlideDirectionNames[1] ? 1 : 0;
            if (!set_int(s_hudMinimapSlideDirection, value)) {
                return false;
            }
        }
    }

    if (element == HudElement::ButtonBacking) {
        bool visible = false;
        if (read_json_bool(object, "visible", visible) &&
            !set_bool(s_hudButtonBackingVisible, visible))
        {
            return false;
        }
    }

    if (element == HudElement::DPad) {
        bool hideArrows = false;
        if (read_json_bool(object, "hideArrows", hideArrows) &&
            !set_bool(s_hudDpadHideArrows, hideArrows))
        {
            return false;
        }

        bool hideShadows = false;
        if (read_json_bool(object, "hideShadows", hideShadows) &&
            !set_bool(s_hudDpadHideShadows, hideShadows))
        {
            return false;
        }
    }

    if (element == HudElement::Hearts) {
        bool healthBar = false;
        if (read_json_bool(object, "healthBar", healthBar) &&
            !set_bool(s_hudHealthBar, healthBar))
        {
            return false;
        }
    }

    HudButton button = HudButton::A;
    bool hasItem = false;
    bool hasAmmo = false;
    bool hasText = false;
    if (button_for_element(element, button, hasItem, hasAmmo, hasText) &&
        !apply_button_layout_json(object, button, hasItem, hasAmmo, hasText))
    {
        return false;
    }
    return true;
}

ModResult register_custom_hud_config() {
    for (size_t i = 0; i < kHudElementCount; ++i) {
        const auto& defaults = kHudElementDefaults[i];
        if (register_custom_int("element", defaults.name, "x", defaults.x, s_hudElementX[i]) !=
                MOD_OK ||
            register_custom_int("element", defaults.name, "y", defaults.y, s_hudElementY[i]) !=
                MOD_OK ||
            register_custom_int(
                "element", defaults.name, "scale", defaults.scale, s_hudElementScale[i]) !=
                MOD_OK)
        {
            return MOD_ERROR;
        }
    }

    for (size_t i = 0; i < kHudButtonCount; ++i) {
        const auto& defaults = kHudButtonDefaults[i];
        if (register_custom_int("button", defaults.name, "item-x", defaults.itemOffsetX,
                s_hudButtonItemOffsetX[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "item-y", defaults.itemOffsetY,
                s_hudButtonItemOffsetY[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "item-scale", defaults.itemScale,
                s_hudButtonItemScale[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "ammo-x", defaults.ammoOffsetX,
                s_hudButtonAmmoOffsetX[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "ammo-y", defaults.ammoOffsetY,
                s_hudButtonAmmoOffsetY[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "ammo-scale", defaults.ammoScale,
                s_hudButtonAmmoScale[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-x", defaults.textOffsetX,
                s_hudButtonTextOffsetX[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-y", defaults.textOffsetY,
                s_hudButtonTextOffsetY[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-scale", defaults.textScale,
                s_hudButtonTextScale[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "item-anchor", defaults.itemAnchor,
                s_hudButtonItemAnchor[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-anchor", defaults.textAnchor,
                s_hudButtonTextAnchor[i]) != MOD_OK)
        {
            return MOD_ERROR;
        }
    }

    if (register_bool("hud-custom-dpad-follows-minimap", false, s_hudDpadFollowsMinimap) !=
            MOD_OK ||
        register_int("hud-custom-minimap-slide-direction", 0, s_hudMinimapSlideDirection) !=
            MOD_OK ||
        register_bool("hud-custom-dpad-hide-arrows", false, s_hudDpadHideArrows) != MOD_OK ||
        register_bool("hud-custom-dpad-hide-shadows", false, s_hudDpadHideShadows) != MOD_OK)
    {
        return MOD_ERROR;
    }

    return MOD_OK;
}

}  // namespace

ModResult register_config(ModError* error) {
    if (register_int("hp-scale-percent", 100, s_healthScale) != MOD_OK ||
        register_bool("ngplus-auto-hp-scaling", true, s_automaticHealthScale) != MOD_OK ||
        register_bool("save-compatibility", true, s_saveCompatibility) != MOD_OK ||
        register_bool("item-integrity-fixes", true, s_itemIntegrity) != MOD_OK ||
        register_int("new-save-mode", 0, s_newSaveMode) != MOD_OK ||
        register_int("aim-mode", 2, s_aimMode) != MOD_OK ||
        register_bool("aim-movement", true, s_aimMovement) != MOD_OK ||
        register_int("cinema-zoom-percent", 100, s_cinemaZoomPercent) != MOD_OK ||
        register_bool("bullet-time", true, s_bulletTime) != MOD_OK ||
        register_bool("manual-shielding", true, s_manualShielding) != MOD_OK ||
        register_bool("r-jump", true, s_rJump) != MOD_OK ||
        register_bool("z-item-slot", true, s_zItemSlot) != MOD_OK ||
        register_bool("dawnlight-touch-ui", true, s_dawnlightTouchUi) != MOD_OK ||
        register_bool("check-for-updates", true, s_checkForUpdates) != MOD_OK ||
        register_bool("bossrush-hardmode-hazards", false, s_bossrushHardmodeHazards) != MOD_OK ||
        register_int("hud-layout", static_cast<int64_t>(HudLayout::GameCube), s_hudLayout) !=
            MOD_OK ||
        register_bool("wii-u-hud", false, s_legacyWiiUHud) != MOD_OK ||
        register_bool("round-xy-buttons", false, s_roundXYButtons) != MOD_OK ||
        register_bool("hud-custom-button-backing-visible", false, s_hudButtonBackingVisible) !=
            MOD_OK ||
        register_bool("hud-custom-health-bar", false, s_hudHealthBar) != MOD_OK ||
        register_bool("aim-defaults-v2", false, s_aimDefaultsMigrated) != MOD_OK ||
        register_bool("hud-layout-migrated-v1", false, s_hudLayoutMigrated) != MOD_OK ||
        register_bool("hud-layout-migrated-v2", false, s_hudLayoutMigratedV2) != MOD_OK)
    {
        return mods::set_error(error, MOD_ERROR, "failed to register Dawnlight config variables");
    }
    for (size_t i = 0; i < kCustomModelCount; ++i) {
        if (register_int(kCustomModelConfigNames[i], 0, s_customModels[i]) != MOD_OK) {
            return mods::set_error(
                error, MOD_ERROR, "failed to register Dawnlight model variables");
        }
    }
    if (register_custom_hud_config() != MOD_OK) {
        return mods::set_error(
            error, MOD_ERROR, "failed to register Dawnlight custom HUD variables");
    }

    if (!get_bool(s_aimDefaultsMigrated, false)) {
        if (svc_config->set_int(mod_ctx, s_aimMode, static_cast<int64_t>(AimMode::Cinema)) != MOD_OK ||
            svc_config->set_bool(mod_ctx, s_aimMovement, true) != MOD_OK ||
            svc_config->set_bool(mod_ctx, s_aimDefaultsMigrated, true) != MOD_OK)
        {
            return mods::set_error(
                error, MOD_ERROR, "failed to migrate Dawnlight aim defaults");
        }
    }

    if (!get_bool(s_hudLayoutMigratedV2, false)) {
        int64_t layoutValue = static_cast<int64_t>(HudLayout::GameCube);
        svc_config->get_int(mod_ctx, s_hudLayout, &layoutValue);
        if (layoutValue == 2 || layoutValue == 3) {
            if (svc_config->set_int(mod_ctx, s_hudLayout, layoutValue + 1) != MOD_OK) {
                return mods::set_error(
                    error, MOD_ERROR, "failed to migrate Dawnlight HUD layout presets");
            }
        }
        if (svc_config->set_bool(mod_ctx, s_hudLayoutMigratedV2, true) != MOD_OK) {
            return mods::set_error(
                error, MOD_ERROR, "failed to finish Dawnlight HUD layout preset migration");
        }
    }

    if (!get_bool(s_hudLayoutMigrated, false)) {
        if (get_bool(s_legacyWiiUHud, false) &&
            svc_config->set_int(mod_ctx, s_hudLayout,
                static_cast<int64_t>(HudLayout::Dawnlight)) != MOD_OK)
        {
            return mods::set_error(
                error, MOD_ERROR, "failed to migrate Dawnlight HUD layout");
        }
        if (svc_config->set_bool(mod_ctx, s_hudLayoutMigrated, true) != MOD_OK) {
            return mods::set_error(
                error, MOD_ERROR, "failed to finish Dawnlight HUD layout migration");
        }
    }

    g_configCheckForUpdatesEnabled = get_bool(s_checkForUpdates, true);
    ModResult subscribeResult = svc_config->subscribe(
        mod_ctx, s_checkForUpdates, on_check_for_updates_changed, nullptr, nullptr);
    if (subscribeResult != MOD_OK) {
        return mods::set_error(
            error, subscribeResult, "failed to subscribe to Dawnlight update-check changes");
    }

    subscribeResult =
        svc_config->subscribe(mod_ctx, s_zItemSlot, on_z_item_slot_changed, nullptr, nullptr);
    if (subscribeResult != MOD_OK) {
        return mods::set_error(
            error, subscribeResult, "failed to subscribe to Dawnlight Z-Items changes");
    }

    subscribeResult = svc_config->subscribe(
        mod_ctx, s_dawnlightTouchUi, on_dawnlight_touch_ui_changed, nullptr, nullptr);
    if (subscribeResult != MOD_OK) {
        return mods::set_error(error, subscribeResult,
            "failed to subscribe to Dawnlight Touch UI changes");
    }
    for (ConfigVarHandle handle : s_customModels) {
        subscribeResult =
            svc_config->subscribe(mod_ctx, handle, on_custom_model_changed, nullptr, nullptr);
        if (subscribeResult != MOD_OK) {
            return mods::set_error(error, subscribeResult,
                "failed to subscribe to Dawnlight model changes");
        }
    }
    return MOD_OK;
}

int health_scale_percent() {
    int64_t value = 100;
    if (s_healthScale != 0) {
        svc_config->get_int(mod_ctx, s_healthScale, &value);
    }
    return static_cast<int>(std::clamp<int64_t>(value, 1, 9999));
}

bool automatic_ngplus_health_scaling() {
    return get_bool(s_automaticHealthScale, true);
}

bool save_compatibility_enabled() {
    return get_bool(s_saveCompatibility, true);
}

bool item_integrity_fixes_enabled() {
    return get_bool(s_itemIntegrity, true);
}

NewSaveMode new_save_mode() {
    int64_t value = static_cast<int64_t>(NewSaveMode::Vanilla);
    if (s_newSaveMode != 0) {
        svc_config->get_int(mod_ctx, s_newSaveMode, &value);
    }
    return static_cast<NewSaveMode>(std::clamp<int64_t>(value, 0, 1));
}

AimMode aim_mode() {
    int64_t value = static_cast<int64_t>(AimMode::Cinema);
    if (s_aimMode != 0) {
        svc_config->get_int(mod_ctx, s_aimMode, &value);
    }
    return static_cast<AimMode>(std::clamp<int64_t>(value, 0, 2));
}

bool aim_movement_enabled() {
    return get_bool(s_aimMovement, true);
}

int cinema_zoom_percent() {
    return get_int(s_cinemaZoomPercent, 100, 25, 400);
}

bool bullet_time_enabled() {
    return get_bool(s_bulletTime, true);
}

bool manual_shielding_enabled() {
    return get_bool(s_manualShielding, true);
}

bool r_jump_enabled() {
    return get_bool(s_rJump, true);
}

bool z_item_slot_enabled() {
    return get_bool(s_zItemSlot, true);
}

bool dawnlight_touch_ui_enabled() {
    return get_bool(s_dawnlightTouchUi, true);
}

bool check_for_updates_enabled() {
    return get_bool(s_checkForUpdates, true);
}

bool bossrush_hardmode_hazards_enabled() {
    return get_bool(s_bossrushHardmodeHazards, false);
}

bool custom_model_enabled(CustomModel model) {
    return get_int(s_customModels[custom_model_index(model)], 0, 0, 1) == 1;
}

HudLayout hud_layout() {
    int64_t value = static_cast<int64_t>(HudLayout::GameCube);
    if (s_hudLayout != 0) {
        svc_config->get_int(mod_ctx, s_hudLayout, &value);
    }
    return static_cast<HudLayout>(std::clamp<int64_t>(value, 0, 4));
}

bool hardcoded_hud_layout_enabled() {
    return hud_layout() != HudLayout::GameCube;
}

bool custom_hud_layout_enabled() {
    return hud_layout() == HudLayout::Custom;
}

bool round_xy_buttons_enabled() {
    return get_bool(s_roundXYButtons, false);
}

bool hud_custom_button_backing_visible() {
    return get_bool(s_hudButtonBackingVisible, false);
}

bool hud_custom_health_bar_enabled() {
    return get_bool(s_hudHealthBar, false);
}

bool hud_custom_dpad_hide_arrows() {
    return get_bool(s_hudDpadHideArrows, false);
}

bool hud_custom_dpad_hide_shadows() {
    return get_bool(s_hudDpadHideShadows, false);
}

bool hud_button_backing_visible() {
    switch (hud_layout()) {
    case HudLayout::GameCube:
        return true;
    case HudLayout::Custom:
        return hud_custom_button_backing_visible();
    case HudLayout::XBox:
    case HudLayout::WiiU:
    case HudLayout::Dawnlight:
    default:
        return false;
    }
}

int hud_custom_element_x(HudElement element) {
    const size_t index = hud_element_index(element);
    return get_int(s_hudElementX[index], kHudElementDefaults[index].x, -9999, 9999);
}

int hud_custom_element_y(HudElement element) {
    const size_t index = hud_element_index(element);
    return get_int(s_hudElementY[index], kHudElementDefaults[index].y, -9999, 9999);
}

int hud_custom_element_scale_percent(HudElement element) {
    const size_t index = hud_element_index(element);
    return get_int(s_hudElementScale[index], kHudElementDefaults[index].scale, 1, 9999);
}

int hud_custom_button_item_offset_x(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonItemOffsetX[index], kHudButtonDefaults[index].itemOffsetX, -9999, 9999);
}

int hud_custom_button_item_offset_y(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonItemOffsetY[index], kHudButtonDefaults[index].itemOffsetY, -9999, 9999);
}

int hud_custom_button_item_scale_percent(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonItemScale[index], kHudButtonDefaults[index].itemScale, 1, 9999);
}

int hud_custom_button_ammo_offset_x(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonAmmoOffsetX[index], kHudButtonDefaults[index].ammoOffsetX, -9999, 9999);
}

int hud_custom_button_ammo_offset_y(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonAmmoOffsetY[index], kHudButtonDefaults[index].ammoOffsetY, -9999, 9999);
}

int hud_custom_button_ammo_scale_percent(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonAmmoScale[index], kHudButtonDefaults[index].ammoScale, 1, 9999);
}

int hud_custom_button_text_offset_x(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonTextOffsetX[index], kHudButtonDefaults[index].textOffsetX, -9999, 9999);
}

int hud_custom_button_text_offset_y(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonTextOffsetY[index], kHudButtonDefaults[index].textOffsetY, -9999, 9999);
}

int hud_custom_button_text_scale_percent(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonTextScale[index], kHudButtonDefaults[index].textScale, 1, 9999);
}

int hud_custom_button_item_anchor(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(s_hudButtonItemAnchor[index], kHudButtonDefaults[index].itemAnchor, 0, 3);
}

int hud_custom_button_text_anchor(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(s_hudButtonTextAnchor[index], kHudButtonDefaults[index].textAnchor, 0, 1);
}

bool hud_custom_dpad_follows_minimap() {
    return get_bool(s_hudDpadFollowsMinimap, false);
}

int hud_custom_minimap_slide_direction() {
    return get_int(s_hudMinimapSlideDirection, 0, 0, 1);
}

ConfigVarHandle health_scale_config_var() {
    return s_healthScale;
}

ConfigVarHandle automatic_health_scale_config_var() {
    return s_automaticHealthScale;
}

ConfigVarHandle save_compatibility_config_var() {
    return s_saveCompatibility;
}

ConfigVarHandle item_integrity_config_var() {
    return s_itemIntegrity;
}

ConfigVarHandle new_save_mode_config_var() {
    return s_newSaveMode;
}

ConfigVarHandle aim_mode_config_var() {
    return s_aimMode;
}

ConfigVarHandle aim_movement_config_var() {
    return s_aimMovement;
}

ConfigVarHandle cinema_zoom_config_var() {
    return s_cinemaZoomPercent;
}

ConfigVarHandle bullet_time_config_var() {
    return s_bulletTime;
}

ConfigVarHandle manual_shielding_config_var() {
    return s_manualShielding;
}

ConfigVarHandle r_jump_config_var() {
    return s_rJump;
}

ConfigVarHandle z_item_slot_config_var() {
    return s_zItemSlot;
}

ConfigVarHandle dawnlight_touch_ui_config_var() {
    return s_dawnlightTouchUi;
}

ConfigVarHandle check_for_updates_config_var() {
    return s_checkForUpdates;
}

ConfigVarHandle bossrush_hardmode_hazards_config_var() {
    return s_bossrushHardmodeHazards;
}

ConfigVarHandle custom_model_config_var(CustomModel model) {
    return s_customModels[custom_model_index(model)];
}

ConfigVarHandle hud_layout_config_var() {
    return s_hudLayout;
}

ConfigVarHandle round_xy_buttons_config_var() {
    return s_roundXYButtons;
}

ConfigVarHandle hud_custom_button_backing_visible_config_var() {
    return s_hudButtonBackingVisible;
}

ConfigVarHandle hud_custom_health_bar_config_var() {
    return s_hudHealthBar;
}

ConfigVarHandle hud_custom_dpad_hide_arrows_config_var() {
    return s_hudDpadHideArrows;
}

ConfigVarHandle hud_custom_dpad_hide_shadows_config_var() {
    return s_hudDpadHideShadows;
}

ConfigVarHandle hud_custom_element_x_config_var(HudElement element) {
    return s_hudElementX[hud_element_index(element)];
}

ConfigVarHandle hud_custom_element_y_config_var(HudElement element) {
    return s_hudElementY[hud_element_index(element)];
}

ConfigVarHandle hud_custom_element_scale_config_var(HudElement element) {
    return s_hudElementScale[hud_element_index(element)];
}

ConfigVarHandle hud_custom_button_item_offset_x_config_var(HudButton button) {
    return s_hudButtonItemOffsetX[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_item_offset_y_config_var(HudButton button) {
    return s_hudButtonItemOffsetY[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_item_scale_config_var(HudButton button) {
    return s_hudButtonItemScale[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_ammo_offset_x_config_var(HudButton button) {
    return s_hudButtonAmmoOffsetX[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_ammo_offset_y_config_var(HudButton button) {
    return s_hudButtonAmmoOffsetY[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_ammo_scale_config_var(HudButton button) {
    return s_hudButtonAmmoScale[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_offset_x_config_var(HudButton button) {
    return s_hudButtonTextOffsetX[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_offset_y_config_var(HudButton button) {
    return s_hudButtonTextOffsetY[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_scale_config_var(HudButton button) {
    return s_hudButtonTextScale[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_item_anchor_config_var(HudButton button) {
    return s_hudButtonItemAnchor[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_anchor_config_var(HudButton button) {
    return s_hudButtonTextAnchor[hud_button_index(button)];
}

ConfigVarHandle hud_custom_dpad_follows_minimap_config_var() {
    return s_hudDpadFollowsMinimap;
}

ConfigVarHandle hud_custom_minimap_slide_direction_config_var() {
    return s_hudMinimapSlideDirection;
}

HudSettingsIoResult export_custom_hud_settings(std::string& outPath) {
    const std::filesystem::path path = hud_settings_file_path();
    if (path.empty()) {
        return HudSettingsIoResult::PathUnavailable;
    }
    outPath = path.generic_string();

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        return HudSettingsIoResult::WriteFailed;
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return HudSettingsIoResult::WriteFailed;
    }
    out.imbue(std::locale::classic());
    out << std::setprecision(8);

    out << "{\n";
    out << "    \"background\": " << (hud_custom_button_backing_visible() ? "true" : "false")
        << ",\n";
    out << "    \"elements\": {\n";
    for (size_t i = 0; i < kHudElementCount; ++i) {
        const auto element = static_cast<HudElement>(i);
        out << "        \"" << kHudElementJsonNames[i] << "\": {\n";
        write_json_number(out, "scale", hud_custom_element_scale_percent(element) / 100.0, true);

        HudButton button = HudButton::A;
        bool hasItem = false;
        bool hasAmmo = false;
        bool hasText = false;
        if (button_for_element(element, button, hasItem, hasAmmo, hasText)) {
            write_button_layout_fields(out, button, hasItem, hasAmmo, hasText, true);
        }

        if (element == HudElement::Minimap) {
            write_json_bool(out, "dpadFollowsMinimap", hud_custom_dpad_follows_minimap(), true);
            write_json_string(out, "slideDirection",
                kSlideDirectionNames[std::clamp(hud_custom_minimap_slide_direction(), 0, 1)],
                true);
        }

        if (element == HudElement::ButtonBacking) {
            write_json_bool(out, "visible", hud_custom_button_backing_visible(), true);
        }

        if (element == HudElement::DPad) {
            write_json_bool(out, "hideArrows", hud_custom_dpad_hide_arrows(), true);
            write_json_bool(out, "hideShadows", hud_custom_dpad_hide_shadows(), true);
        }

        if (element == HudElement::Hearts) {
            write_json_bool(out, "healthBar", hud_custom_health_bar_enabled(), true);
        }

        write_json_number(out, "x", hud_custom_element_x(element), true);
        write_json_number(out, "y", hud_custom_element_y(element), false);
        out << "        }" << (i + 1 < kHudElementCount ? "," : "") << "\n";
    }
    out << "    },\n";
    out << "    \"roundXYButtons\": " << (round_xy_buttons_enabled() ? "true" : "false") << ",\n";
    out << "    \"version\": 13\n";
    out << "}\n";

    return out.good() ? HudSettingsIoResult::Ok : HudSettingsIoResult::WriteFailed;
}

HudSettingsIoResult import_custom_hud_settings(std::string& outPath) {
    const std::filesystem::path path = hud_settings_file_path();
    if (path.empty()) {
        return HudSettingsIoResult::PathUnavailable;
    }
    outPath = path.generic_string();

    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return HudSettingsIoResult::FileMissing;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return HudSettingsIoResult::ReadFailed;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string json = buffer.str();

    std::string elementsObject;
    if (!read_json_object(json, "elements", elementsObject)) {
        return HudSettingsIoResult::InvalidFormat;
    }

    if (!set_custom_hud_to_xbox_defaults(true)) {
        return HudSettingsIoResult::ConfigFailed;
    }

    bool roundXY = true;
    if (read_json_bool(json, "roundXYButtons", roundXY) && !set_bool(s_roundXYButtons, roundXY)) {
        return HudSettingsIoResult::ConfigFailed;
    }
    bool background = false;
    if (read_json_bool(json, "background", background) &&
        !set_bool(s_hudButtonBackingVisible, background))
    {
        return HudSettingsIoResult::ConfigFailed;
    }

    for (size_t i = 0; i < kHudElementCount; ++i) {
        if (!apply_element_json(elementsObject, static_cast<HudElement>(i))) {
            return HudSettingsIoResult::ConfigFailed;
        }
    }

    return HudSettingsIoResult::Ok;
}

HudSettingsIoResult copy_hud_preset_to_custom(HudLayout layout) {
    bool applied = false;
    switch (layout) {
    case HudLayout::GameCube:
        applied = set_custom_hud_from_defaults(
            kGameCubeHudElementDefaults, kGameCubeHudButtonDefaults, true, 0, false, true, true);
        break;
    case HudLayout::XBox:
        applied = set_custom_hud_to_xbox_defaults(true);
        break;
    case HudLayout::WiiU:
        applied = set_custom_hud_from_defaults(
            kWiiUHudElementDefaults, kWiiUHudButtonDefaults, false, 0, true, false, true);
        break;
    case HudLayout::Dawnlight:
        applied = set_custom_hud_from_defaults(
            kDawnlightHudElementDefaults, kDawnlightHudButtonDefaults, false, 1, true, false,
            true);
        break;
    case HudLayout::Custom:
    default:
        applied = false;
        break;
    }
    return applied ? HudSettingsIoResult::Ok : HudSettingsIoResult::ConfigFailed;
}

HudSettingsIoResult reset_custom_hud_settings() {
    return copy_hud_preset_to_custom(HudLayout::XBox);
}

const char* hud_settings_io_result_message(HudSettingsIoResult result) {
    switch (result) {
    case HudSettingsIoResult::Ok:
        return "OK";
    case HudSettingsIoResult::FileMissing:
        return "Place hud_layout_settings.json in the mods folder, then press IMPORT HUD again.";
    case HudSettingsIoResult::PathUnavailable:
        return "The mods folder path is currently unavailable.";
    case HudSettingsIoResult::ReadFailed:
        return "Unable to read hud_layout_settings.json.";
    case HudSettingsIoResult::WriteFailed:
        return "Unable to write hud_layout_settings.json.";
    case HudSettingsIoResult::InvalidFormat:
        return "hud_layout_settings.json is not a valid Dawnlight HUD layout file.";
    case HudSettingsIoResult::ConfigFailed:
        return "Unable to apply the HUD layout settings.";
    default:
        return "Unknown HUD layout settings error.";
    }
}

}  // namespace dawnlight
