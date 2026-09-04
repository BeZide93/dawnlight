#pragma once

#include "mods/api.h"
#include "mods/svc/config.h"

#include <string>

namespace dawnlight {

enum class AimMode : int {
    Vanilla = 0,
    ThirdPerson = 1,
    Cinema = 2,
};

enum class NewSaveMode : int {
    Vanilla = 0,
    IntroSkip = 1,
};

enum class CustomModel : int {
    OrdonLink = 0,
    HeroClothes,
    ZoraArmor,
    MagicArmor,
    WolfLink,
    SumoLink,
    Horse,
    Items,
    Animations,
    OrdonShield,
    WoodenShield,
    HylianShield,
    Count,
};

enum class HudLayout : int {
    GameCube = 0,
    XBox = 1,
    WiiU = 2,
    Dawnlight = 3,
    Custom = 4,
};

enum class HudElement : int {
    A = 0,
    B,
    X,
    Y,
    Z,
    ButtonBacking,
    DPad,
    Midna,
    Hearts,
    Rupees,
    Keys,
    Oil,
    Oxygen,
    Minimap,
    DPadItemsText,
    DPadMapText,
    Count,
};

enum class HudButton : int {
    A = 0,
    B,
    X,
    Y,
    Z,
    Count,
};

enum class HudSettingsIoResult {
    Ok,
    FileMissing,
    PathUnavailable,
    ReadFailed,
    WriteFailed,
    InvalidFormat,
    ConfigFailed,
};

ModResult register_config(ModError* error);
int health_scale_percent();
bool automatic_ngplus_health_scaling();
bool save_compatibility_enabled();
bool item_integrity_fixes_enabled();
NewSaveMode new_save_mode();
AimMode aim_mode();
bool aim_movement_enabled();
int cinema_zoom_percent();
bool bullet_time_enabled();
bool flurry_rush_enabled();
bool fierce_deity_enabled();
bool great_spin_projectile_enabled();
bool manual_shielding_enabled();
bool r_jump_enabled();
bool z_item_slot_enabled();
bool dawnlight_touch_ui_enabled();
bool check_for_updates_enabled();
bool bossrush_hardmode_hazards_enabled();
bool hide_shield_enabled();
int eye_movement_range_percent();
bool custom_model_enabled(CustomModel model);
HudLayout hud_layout();
bool hardcoded_hud_layout_enabled();
bool custom_hud_layout_enabled();
bool round_xy_buttons_enabled();
bool hud_custom_button_backing_visible();
bool hud_custom_health_bar_enabled();
bool hud_custom_dpad_hide_arrows();
bool hud_custom_dpad_hide_shadows();
bool hud_button_backing_visible();
int hud_custom_element_x(HudElement element);
int hud_custom_element_y(HudElement element);
int hud_custom_element_scale_percent(HudElement element);
int hud_custom_button_item_offset_x(HudButton button);
int hud_custom_button_item_offset_y(HudButton button);
int hud_custom_button_item_scale_percent(HudButton button);
int hud_custom_button_ammo_offset_x(HudButton button);
int hud_custom_button_ammo_offset_y(HudButton button);
int hud_custom_button_ammo_scale_percent(HudButton button);
int hud_custom_button_text_offset_x(HudButton button);
int hud_custom_button_text_offset_y(HudButton button);
int hud_custom_button_text_scale_percent(HudButton button);
int hud_custom_button_item_anchor(HudButton button);
int hud_custom_button_text_anchor(HudButton button);
bool hud_custom_dpad_follows_minimap();
int hud_custom_minimap_slide_direction();

ConfigVarHandle health_scale_config_var();
ConfigVarHandle automatic_health_scale_config_var();
ConfigVarHandle save_compatibility_config_var();
ConfigVarHandle item_integrity_config_var();
ConfigVarHandle new_save_mode_config_var();
ConfigVarHandle aim_mode_config_var();
ConfigVarHandle aim_movement_config_var();
ConfigVarHandle cinema_zoom_config_var();
ConfigVarHandle bullet_time_config_var();
ConfigVarHandle flurry_rush_config_var();
ConfigVarHandle fierce_deity_config_var();
ConfigVarHandle great_spin_projectile_config_var();
ConfigVarHandle manual_shielding_config_var();
ConfigVarHandle r_jump_config_var();
ConfigVarHandle z_item_slot_config_var();
ConfigVarHandle dawnlight_touch_ui_config_var();
ConfigVarHandle check_for_updates_config_var();
ConfigVarHandle bossrush_hardmode_hazards_config_var();
ConfigVarHandle hide_shield_config_var();
ConfigVarHandle eye_movement_range_config_var();
ConfigVarHandle custom_model_config_var(CustomModel model);
ConfigVarHandle hud_layout_config_var();
ConfigVarHandle round_xy_buttons_config_var();
ConfigVarHandle hud_custom_button_backing_visible_config_var();
ConfigVarHandle hud_custom_health_bar_config_var();
ConfigVarHandle hud_custom_dpad_hide_arrows_config_var();
ConfigVarHandle hud_custom_dpad_hide_shadows_config_var();
ConfigVarHandle hud_custom_element_x_config_var(HudElement element);
ConfigVarHandle hud_custom_element_y_config_var(HudElement element);
ConfigVarHandle hud_custom_element_scale_config_var(HudElement element);
ConfigVarHandle hud_custom_button_item_offset_x_config_var(HudButton button);
ConfigVarHandle hud_custom_button_item_offset_y_config_var(HudButton button);
ConfigVarHandle hud_custom_button_item_scale_config_var(HudButton button);
ConfigVarHandle hud_custom_button_ammo_offset_x_config_var(HudButton button);
ConfigVarHandle hud_custom_button_ammo_offset_y_config_var(HudButton button);
ConfigVarHandle hud_custom_button_ammo_scale_config_var(HudButton button);
ConfigVarHandle hud_custom_button_text_offset_x_config_var(HudButton button);
ConfigVarHandle hud_custom_button_text_offset_y_config_var(HudButton button);
ConfigVarHandle hud_custom_button_text_scale_config_var(HudButton button);
ConfigVarHandle hud_custom_button_item_anchor_config_var(HudButton button);
ConfigVarHandle hud_custom_button_text_anchor_config_var(HudButton button);
ConfigVarHandle hud_custom_dpad_follows_minimap_config_var();
ConfigVarHandle hud_custom_minimap_slide_direction_config_var();

HudSettingsIoResult export_custom_hud_settings(std::string& outPath);
HudSettingsIoResult import_custom_hud_settings(std::string& outPath);
HudSettingsIoResult copy_hud_preset_to_custom(HudLayout layout);
HudSettingsIoResult reset_custom_hud_settings();
const char* hud_settings_io_result_message(HudSettingsIoResult result);

}  // namespace dawnlight
