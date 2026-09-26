#include "touch_buttons.hpp"
#include "config.hpp"
#include "enemy_spawner.hpp"
#include "service_imports.hpp"
#include "update_service.hpp"

#include "mods/service.hpp"
#include "mods/svc/ui.h"

#include <map>
#include <array>
#include <string>

namespace dawnlight {
namespace {

UiWindowHandle s_settingsWindow = 0;
UiMenuTabHandle s_menuTab = 0;

constexpr const char* kAimModeOptions[] = {
    "Vanilla",
    "3rd Person",
    "Cinema",
};

constexpr const char* kGlideItemOptions[] = {"Cucco", "Glider"};
constexpr const char* kBulletTimeOptions[] = {"Off", "Always", "BOTW"};

constexpr const char* kNewSaveModeOptions[] = {
    "Vanilla",
    "Intro Skip",
};

constexpr const char* kModelOptions[] = {
    "Vanilla",
    "Custom",
};

constexpr const char* kHudLayoutOptions[] = {
    "GameCube",
    "X-Box",
    "Wii-U",
    "Dawnlight",
    "Custom",
};

constexpr const char* kHudItemAnchorOptions[] = {
    "Left",
    "Right",
    "Top",
    "Bottom",
};

constexpr const char* kHudTextAnchorOptions[] = {
    "Left",
    "Right",
};

constexpr const char* kMinimapSlideOptions[] = {
    "Left -> Right",
    "Right -> Left",
};

ModResult add_section(ModContext* ctx, UiElementHandle pane, const char* title) {
    return svc_ui->pane_add_section(ctx, pane, title);
}

ModResult add_text(ModContext* ctx, UiElementHandle pane, const char* text) {
    return svc_ui->pane_add_text(ctx, pane, text, nullptr);
}

ModResult add_button(ModContext* ctx, UiElementHandle pane, const char* label,
    UiPressedFn onPressed) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_BUTTON;
    desc.label = label;
    desc.on_pressed = onPressed;
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

void push_toast(const char* title, const char* body, const char* type = nullptr) {
    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.type = type;
    toast.title_rml = title;
    toast.body_rml = body;
    svc_ui->push_toast(mod_ctx, &toast);
}

// Stable callback data survives tab rebuilds. Managed controls display the
// effective value, while writes in Off mode still use ConfigService persistence.
struct ModeControlBinding {
    ConfigVarHandle var;
    UiControlKind kind;
    UiPredicateFn disabled;
};
std::map<ConfigVarHandle, ModeControlBinding> s_modeControls;

bool mode_control_disabled(ModContext* ctx, void* data) {
    const auto& binding = *static_cast<ModeControlBinding*>(data);
    int64_t value = 0;
    return mode_config_override(binding.var, value) ||
        (binding.disabled && binding.disabled(ctx, nullptr));
}

void mode_control_get(ModContext* ctx, void* data, UiControlValue* out) {
    const auto& binding = *static_cast<ModeControlBinding*>(data);
    int64_t value = 0;
    if (mode_config_override(binding.var, value)) {
        out->bool_value = value != 0;
        out->int_value = value;
    } else if (binding.kind == UI_CONTROL_TOGGLE) {
        svc_config->get_bool(ctx, binding.var, &out->bool_value);
    } else {
        svc_config->get_int(ctx, binding.var, &out->int_value);
    }
}

void mode_control_set(ModContext* ctx, void* data, const UiControlValue* value) {
    if (mode_control_disabled(ctx, data)) return;
    const auto& binding = *static_cast<ModeControlBinding*>(data);
    if (binding.kind == UI_CONTROL_TOGGLE) {
        svc_config->set_bool(ctx, binding.var, value->bool_value);
    } else {
        svc_config->set_int(ctx, binding.var, value->int_value);
    }
}

void bind_mode_control(UiControlDesc& desc) {
    if (mode_setting_for_config(desc.config_var) == ModeSetting::None) return;
    auto& binding = s_modeControls[desc.config_var];
    binding = {desc.config_var, desc.kind, desc.is_disabled};
    desc.binding = UI_BINDING_CALLBACKS;
    desc.user_data = &binding;
    desc.get = mode_control_get;
    desc.set = mode_control_set;
    desc.is_disabled = mode_control_disabled;
}

ModResult add_toggle(ModContext* ctx, UiElementHandle pane, const char* label,
    ConfigVarHandle var, const char* help = nullptr, UiPredicateFn isDisabled = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_TOGGLE;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    desc.is_disabled = isDisabled;
    bind_mode_control(desc);
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

ModResult add_number(ModContext* ctx, UiElementHandle pane, const char* label,
    ConfigVarHandle var, int min, int max, int step, const char* suffix,
    const char* help = nullptr, UiPredicateFn isDisabled = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_NUMBER;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    desc.min = min;
    desc.max = max;
    desc.step = step;
    desc.suffix = suffix;
    desc.is_disabled = isDisabled;
    bind_mode_control(desc);
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

ModResult add_select(ModContext* ctx, UiElementHandle pane, const char* label,
    ConfigVarHandle var, const char* const* options, size_t optionCount,
    const char* help = nullptr, UiPredicateFn isDisabled = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_SELECT;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    desc.options = options;
    desc.option_count = optionCount;
    desc.is_disabled = isDisabled;
    bind_mode_control(desc);
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

bool auto_jump_setting_disabled(ModContext*, void*) {
    // Normalize persisted state before displaying an unavailable child toggle.
    (void)disable_auto_jump_enabled();
    return !r_jump_enabled();
}

bool wolf_speed_disabled(ModContext*, void*) {
    return !wolf_sprint_enabled();
}

bool sprint_speed_disabled(ModContext*, void*) {
    return !sprint_enabled();
}

bool custom_hud_controls_disabled(ModContext*, void*) {
    return !custom_hud_layout_enabled();
}

void export_hud_settings(ModContext*, void*) {
    std::string path;
    const HudSettingsIoResult result = export_custom_hud_settings(path);
    if (result == HudSettingsIoResult::Ok) {
        push_toast("HUD Exported", "Exported hud_layout_settings.json to Dawnlight's data folder.");
    } else {
        push_toast("HUD Export Failed", hud_settings_io_result_message(result), "warning");
    }
}

void import_hud_settings(ModContext*, void*) {
    std::string path;
    const HudSettingsIoResult result = import_custom_hud_settings(path);
    if (result == HudSettingsIoResult::Ok) {
        push_toast("HUD Imported", "Imported hud_layout_settings.json from Dawnlight's data folder.");
    } else {
        push_toast("HUD Import Failed", hud_settings_io_result_message(result), "warning");
    }
}

void copy_hud_preset_settings(HudLayout layout, const char* successBody) {
    const HudSettingsIoResult result = copy_hud_preset_to_custom(layout);
    if (result == HudSettingsIoResult::Ok) {
        push_toast("HUD Copied", successBody);
    } else {
        push_toast("HUD Copy Failed", hud_settings_io_result_message(result), "warning");
    }
}

void copy_gamecube_hud_settings(ModContext*, void*) {
    copy_hud_preset_settings(HudLayout::GameCube, "GameCube copied to Custom HUD.");
}

void copy_xbox_hud_settings(ModContext*, void*) {
    copy_hud_preset_settings(HudLayout::XBox, "X-Box copied to Custom HUD.");
}

void copy_wiiu_hud_settings(ModContext*, void*) {
    copy_hud_preset_settings(HudLayout::WiiU, "Wii-U copied to Custom HUD.");
}

void copy_dawnlight_hud_settings(ModContext*, void*) {
    copy_hud_preset_settings(HudLayout::Dawnlight, "Dawnlight copied to Custom HUD.");
}

void spawn_selected_enemy(ModContext*, void*) {
    const ModResult result = spawn_enemy_for_testing(enemy_spawner_profile());
    if (result == MOD_UNAVAILABLE) {
        if (enemy_spawner_blocked_in_bossrush_hub()) {
            push_toast("Enemy Spawn Unavailable",
                "The Enemy Spawner is disabled in the Boss Rush hub to prevent model-memory crashes.",
                "warning");
        } else {
            push_toast("Enemy Spawn Failed", "Link must be active in a gameplay scene.", "warning");
        }
    } else if (result != MOD_OK) {
        push_toast("Enemy Spawn Failed", "The selected enemy could not be created.", "warning");
    }
}

ModResult add_custom_transform_controls(
    ModContext* ctx, UiElementHandle pane, const char* section, HudElement element) {
    if (add_section(ctx, pane, section) != MOD_OK) return MOD_ERROR;
    if (add_number(ctx, pane, "X Position", hud_custom_element_x_config_var(element), -9999,
            9999, 1, " px", nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, pane, "Y Position", hud_custom_element_y_config_var(element), -9999,
            9999, 1, " px", nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, pane, "Scale", hud_custom_element_scale_config_var(element), 1, 9999,
            1, "%", nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult add_custom_button_backing_controls(ModContext* ctx, UiElementHandle pane) {
    if (add_section(ctx, pane, "Custom Button Backing") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, pane, "Button Backing",
            hud_custom_button_backing_visible_config_var(),
            "Shows the decorative backing texture behind the HUD buttons.",
            custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, pane, "X Position",
            hud_custom_element_x_config_var(HudElement::ButtonBacking), -9999, 9999, 1, " px",
            nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, pane, "Y Position",
            hud_custom_element_y_config_var(HudElement::ButtonBacking), -9999, 9999, 1, " px",
            nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, pane, "Scale",
            hud_custom_element_scale_config_var(HudElement::ButtonBacking), 1, 9999, 1, "%",
            nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult add_custom_button_controls(ModContext* ctx, UiElementHandle pane, const char* section,
    HudButton button, bool hasItem, bool hasAmmo, bool hasText) {
    if (add_section(ctx, pane, section) != MOD_OK) return MOD_ERROR;

    if (hasItem) {
        if (add_select(ctx, pane, "Item Anchor",
                hud_custom_button_item_anchor_config_var(button), kHudItemAnchorOptions,
                std::size(kHudItemAnchorOptions), nullptr, custom_hud_controls_disabled) !=
            MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Item Offset X",
                hud_custom_button_item_offset_x_config_var(button), -9999, 9999, 1, " px",
                nullptr, custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Item Offset Y",
                hud_custom_button_item_offset_y_config_var(button), -9999, 9999, 1, " px",
                nullptr, custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Item Scale",
                hud_custom_button_item_scale_config_var(button), 1, 9999, 1, "%", nullptr,
                custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
    }

    if (hasAmmo) {
        if (add_number(ctx, pane, "Ammo Offset X",
                hud_custom_button_ammo_offset_x_config_var(button), -9999, 9999, 1, " px",
                nullptr, custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Ammo Offset Y",
                hud_custom_button_ammo_offset_y_config_var(button), -9999, 9999, 1, " px",
                nullptr, custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Ammo Scale",
                hud_custom_button_ammo_scale_config_var(button), 1, 9999, 1, "%", nullptr,
                custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
    }

    if (hasText) {
        if (add_select(ctx, pane, "Text Anchor",
                hud_custom_button_text_anchor_config_var(button), kHudTextAnchorOptions,
                std::size(kHudTextAnchorOptions), nullptr, custom_hud_controls_disabled) !=
            MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Text Offset X",
                hud_custom_button_text_offset_x_config_var(button), -9999, 9999, 1, " px",
                nullptr, custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Text Offset Y",
                hud_custom_button_text_offset_y_config_var(button), -9999, 9999, 1, " px",
                nullptr, custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
        if (add_number(ctx, pane, "Text Scale",
                hud_custom_button_text_scale_config_var(button), 1, 9999, 1, "%", nullptr,
                custom_hud_controls_disabled) != MOD_OK)
        {
            return MOD_ERROR;
        }
    }

    return MOD_OK;
}

ModResult build_general_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "General") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Dawnlight Mode", dawnlight_mode_config_var(),
            "Enables the intended Dawnlight settings and story progression. Controlled settings "
            "are locked while On. Off restores your saved personal settings, including after a restart.")
        != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Progression System", progression_system_config_var(),
            "Sprint is available from the start. Give Talo the Wooden Sword to unlock the Glider, "
            "free Ordona for Revali's Gale, and free Faron for Fierce Deity. Gale gains one charge "
            "per three full heart containers. Controlled settings are locked while On.")
        != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Notifications", notifications_config_var(),
            "Show Progression System unlock and Gale capacity notifications. Off by default. "
            "Does not affect ability unlocks, item-acquisition scenes or other Dawnlight messages.")
        != MOD_OK) return MOD_ERROR;
    if (add_section(ctx, left, "New Saves") != MOD_OK) return MOD_ERROR;
    if (add_select(ctx, left, "New Save Mode", new_save_mode_config_var(),
            kNewSaveModeOptions, std::size(kNewSaveModeOptions),
            "Changes how newly created empty save slots are initialized. Vanilla keeps upstream "
            "behavior, and Intro Skip starts after the Faron intro setup.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }

    return MOD_OK;
}

ModResult build_aiming_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Aiming") != MOD_OK) return MOD_ERROR;
    if (add_select(ctx, left, "Aim Mode", aim_mode_config_var(), kAimModeOptions,
            std::size(kAimModeOptions),
            "Vanilla keeps the original aiming flow. 3rd Person keeps Link visible while aiming. "
            "Cinema uses Dawnlight's close over-the-shoulder camera.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Aim Movement", aim_movement_config_var(),
            "Allows movement while aiming supported items. In Vanilla aim this keeps movement on "
            "the left stick and aiming on the C-stick/touch aim.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Cinema Zoom", cinema_zoom_config_var(), 25, 400, 5, "%",
            "Adjusts Cinema aim zoom. 100% is the current 1.0 zoom.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "3rd Person Reticle Y", third_person_reticle_offset_y_config_var(),
            -40, 40, 1, " deg",
            "Moves the 3rd Person aim reticle vertically. Positive values move it up; negative "
            "values move it down.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Bullet Time", bullet_time_config_var(), kBulletTimeOptions,
            std::size(kBulletTimeOptions),
            "Off disables Bullet Time. Always keeps the original airborne Bow aiming behavior. "
            "BOTW requires twice the original jump height above the ground to activate, independent "
            "of Jump Height and Gale Height. Uses 20% stamina per second. Press A to cancel.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult build_controls_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Controls") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Manual Shielding", manual_shielding_config_var(),
            "Moves shielding to Target + ZR and Shield Attack to Target + ZR + B. With Switch "
            "lock-on, ZR alone still shields while a target remains locked.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "R Jump", r_jump_config_var(),
            "Uses R as a fallback jump button for human and wolf Link when no R interaction or targeting action is active. "
            "As human Link, press R+B during the jump to start a jump attack.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Disable Auto Jump", disable_auto_jump_config_var(),
            "Stops human and wolf Link from automatically jumping when running off a ledge. "
            "Use the jump button to jump; walking off ledges keeps your forward momentum. Falling and ledge grabbing still work. "
            "Turns off when R Jump is disabled.", auto_jump_setting_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Jump Height", jump_height_config_var(), 100, 500, 10, "%",
            "Height of a manual jump. 100% is the original height; maximum 500%.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Glide", glide_config_var(),
            "Press ZR again in midair to deploy the selected glide item, including during ordinary falls. "
            "It is put away when you land.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Glide Item", glide_item_config_var(), kGlideItemOptions,
            std::size(kGlideItemOptions),
            "Choose a Cucco or Dawnlight's cloth glider. Both use the same glide movement.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Revali's Gale", revalis_gale_config_var(),
            "Press ZR to jump immediately. Keep holding through landing to stop and crouch, then release for a "
            "wind jump with Gale Height added to Jump Height, preserving your previous running speed and direction.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Gale Height", gale_height_config_var(), 100, 1000, 10, "%",
            "Additional height for Revali's Gale, relative to the original jump height. "
            "Default 500%; Jump Height 200% plus Gale Height 500% gives 700% total height.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Gale Counter", gale_counter_visible_config_var(),
            "Shows remaining Gale charges. Hiding the counter does not remove the resource cost.") != MOD_OK ||
        add_number(ctx, left, "Gale Charges", gale_counter_capacity_config_var(), 1, 12, 1, "",
            "Maximum Gale charges. Default 3. A successful Gale launch consumes one charge.") != MOD_OK ||
        add_number(ctx, left, "Gale Recovery Time", gale_recovery_config_var(), 1, 3600, 1, " sec",
            "Seconds to restore one charge, one at a time. Default 120. Recovery continues through "
            "cutscenes and scene changes; another use does not restart the timer.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Stamina Bar", stamina_config_var(),
            "Enables shared stamina costs and the stamina meter. When disabled, Dawnlight moves "
            "do not consume stamina.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Sprint", sprint_config_var(),
            "Hold the Roll button while running to sprint at the configured speed. "
            "Uses 5% stamina per second.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Wolf Sprint", wolf_sprint_config_var(),
            "Hold the Dash button (B in the Dawnlight layout) while moving as wolf Link to keep dash speed. "
            "Uses the assigned Dash action for controller and touch input.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Wolf Speed", wolf_speed_config_var(), 100, 300, 5, "%",
            "Wolf sprint speed: 100% is native dash speed. Keeps native slow-area limits. "
            "Earned momentum carries into wolf jumps and falls with Disable Auto Jump.",
            wolf_speed_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Sprint Speed", sprint_speed_config_var(), 100, 300, 5, "%",
            "Sprint speed relative to normal running. 150% keeps the original sprint speed. "
            "Animation uses half the actual speed bonus (150% movement = 125% playback). "
            "Manual jump distance follows your sprint momentum.",
            sprint_speed_disabled)
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Z Item Slot", z_item_slot_config_var(),
            "Adds an item slot on Z and moves Midna off the Z button. "
            "Restart the app after changing this setting.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Dawnlight Touch UI", dawnlight_touch_ui_config_var(),
            "Shows the third item on the touch Z button and moves Midna to the Skip button "
            "outside cutscenes. Keeps the L touch button available on the map. "
            "This works independently from Dawnlight's Z Item Slot for "
            "compatibility with other third-item mods. Restart the app after changing this "
            "setting.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "Touch Buttons", open_touch_buttons) != MOD_OK) return MOD_ERROR;
    return MOD_OK;
}

ModResult build_hud_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "HUD") != MOD_OK) return MOD_ERROR;
    if (add_select(ctx, left, "HUD Layout", hud_layout_config_var(), kHudLayoutOptions,
            std::size(kHudLayoutOptions),
            "GameCube keeps the original HUD. X-Box, Wii-U and Dawnlight apply fixed HUD layout "
            "presets. Custom exposes the same layout fields as editable settings.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Round X/Y Buttons", round_xy_buttons_config_var(),
            "Draws X and Y with Dawnlight's round HUD button style without the extra X/Y shine texture layers.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "EXPORT HUD", export_hud_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "IMPORT HUD", import_hud_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "COPY GAMECUBE TO CUSTOM", copy_gamecube_hud_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "COPY X-BOX TO CUSTOM", copy_xbox_hud_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "COPY WII-U TO CUSTOM", copy_wiiu_hud_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "COPY DAWNLIGHT TO CUSTOM", copy_dawnlight_hud_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_section(ctx, left, "Custom Minimap") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "D-Pad Follows Minimap",
            hud_custom_dpad_follows_minimap_config_var(),
            "When disabled, the D-Pad no longer rides along with the minimap slide animation.",
            custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Minimap Slide Direction",
            hud_custom_minimap_slide_direction_config_var(), kMinimapSlideOptions,
            std::size(kMinimapSlideOptions), nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "X Position", hud_custom_element_x_config_var(HudElement::Minimap),
            -9999, 9999, 1, " px", nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Y Position", hud_custom_element_y_config_var(HudElement::Minimap),
            -9999, 9999, 1, " px", nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Scale", hud_custom_element_scale_config_var(HudElement::Minimap),
            1, 9999, 1, "%", nullptr, custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_custom_transform_controls(ctx, left, "Custom A", HudElement::A) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_button_controls(ctx, left, "Custom A Text", HudButton::A, false, false, true) !=
        MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom B", HudElement::B) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_button_controls(ctx, left, "Custom B Content", HudButton::B, true, false, true) !=
        MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom X", HudElement::X) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_button_controls(ctx, left, "Custom X Content", HudButton::X, true, true, true) !=
        MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Y", HudElement::Y) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_button_controls(ctx, left, "Custom Y Content", HudButton::Y, true, true, true) !=
        MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Z", HudElement::Z) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_button_controls(ctx, left, "Custom Z Content", HudButton::Z, true, true, true) !=
        MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_button_backing_controls(ctx, left) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom D-Pad", HudElement::DPad) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "HIDE ARROWS", hud_custom_dpad_hide_arrows_config_var(),
            "Hides the four small direction arrows around the D-Pad.",
            custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "HIDE SHADOWS", hud_custom_dpad_hide_shadows_config_var(),
            "Hides the four background shadow panes behind the D-Pad.",
            custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(
            ctx, left, "D-Pad Items Text", HudElement::DPadItemsText) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(
            ctx, left, "D-Pad Map Text", HudElement::DPadMapText) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Midna", HudElement::Midna) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Hearts", HudElement::Hearts) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Health Bar", hud_custom_health_bar_config_var(),
            "Places the second row of hearts beside the first row.",
            custom_hud_controls_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Rupees", HudElement::Rupees) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Keys", HudElement::Keys) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Oil", HudElement::Oil) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Oxygen", HudElement::Oxygen) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(
            ctx, left, "Custom Stamina Bar", HudElement::StaminaBar) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(
            ctx, left, "Custom Fierce Deity Bar", HudElement::FierceDeityBar) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_custom_transform_controls(ctx, left, "Custom Gale Counter", HudElement::GaleCounter) != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult build_gameplay_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Combat") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Dual Wield", dual_wield_config_var(),
            "Replace the shield with an Ordon Sword carried at the left hip. Alternate "
            "hands during ordinary sword attacks, cross both blades to guard, and push "
            "them forward for Shield Attack. Turning this off restores normal equipment.")
        != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Arrow Modes", arrow_modes_config_var(),
            "Press ZR while aiming the Bow to cycle Normal, Fire (2 arrows, +50% damage), "
            "and Triple Shot (3 arrows). Fire arrows ignite lantern-compatible objects. "
            "Disabling returns to normal arrows; arrows already fired keep their effects.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Flurry Rush", flurry_rush_config_var(),
            "Perfectly evade a locked enemy attack with a side jump or backflip to slow the "
            "dodge and enemies for three seconds. A sword attack closes to melee range and "
            "restores Link's speed. Link cannot be hit while the effect is active. Releasing "
            "the lock-on ends the effect early. Uses 25% stamina.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Fierce Deity", fierce_deity_config_var(),
            "Builds power with damaging sword attacks. Use a charged Spin Attack at full power "
            "to transform, deal double sword damage, and consume the meter over time.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Great Spin Projectile", great_spin_projectile_config_var(),
            "Launches the Great Spin trail forward as a damaging sword projectile at full "
            "health. Uses 40% stamina.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(ctx, left, "Enemy Spawner") != MOD_OK) return MOD_ERROR;
    if (add_select(ctx, left, "Enemy", enemy_spawner_profile_config_var(),
            kEnemySpawnerProfileLabels.data(), kEnemySpawnerProfileLabels.size(),
            "Select an enemy with a Dawnlight slow-motion profile.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_button(ctx, left, "SPAWN", spawn_selected_enemy) != MOD_OK) return MOD_ERROR;

    return MOD_OK;
}

ModResult build_hard_mode_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Hard Mode") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Enemy Hard Mode", enemy_hard_mode_config_var(),
            "Makes supported normal enemies attack sooner and track Link more quickly. Does not "
            "change enemy health or damage.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "No Normal-Hit Invulnerability",
            remove_normal_hit_invulnerability_config_var(),
            "Removes Link's invulnerability after normal hits. Knockdowns keep their full "
            "invulnerability window.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Boss Hard Mode", bossrush_hardmode_hazards_config_var(),
            "Enables additional mechanics for supported bosses, including faster attacks, "
            "reinforcements, and the Ganondorf Boss Rush arena hazards.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(ctx, left, "Enemy Scaling") != MOD_OK) return MOD_ERROR;
    if (add_number(ctx, left, "HP Scaling", health_scale_config_var(), 1, 9999, 10, "%",
            "Scales enemy health when enemies spawn.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult build_models_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Models") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Hide Shield", hide_shield_config_var(),
            "Hides the Ordon, Wooden, and Hylian shield models while keeping shielding functional.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_number(ctx, left, "Eye Movement Range", eye_movement_range_config_var(), 0, 200,
            1, "%",
            "Scales both animated and procedural eye texture movement. 100% is vanilla; 0% "
            "removes all eye UV movement.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Ordon Link", custom_model_config_var(CustomModel::OrdonLink),
            kModelOptions, std::size(kModelOptions),
            "Custom loads mods/BMDL.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Hero's Clothes",
            custom_model_config_var(CustomModel::HeroClothes), kModelOptions,
            std::size(kModelOptions),
            "Custom loads mods/Kmdl.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Zora Armor", custom_model_config_var(CustomModel::ZoraArmor),
            kModelOptions, std::size(kModelOptions),
            "Custom loads mods/Zmdl.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Magic Armor", custom_model_config_var(CustomModel::MagicArmor),
            kModelOptions, std::size(kModelOptions),
            "Custom loads mods/Mmdl.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Wolf Link", custom_model_config_var(CustomModel::WolfLink),
            kModelOptions, std::size(kModelOptions),
            "Custom loads mods/Wmdl.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Sumo Link", custom_model_config_var(CustomModel::SumoLink),
            kModelOptions, std::size(kModelOptions),
            "Custom loads mods/alSumou.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Horse", custom_model_config_var(CustomModel::Horse),
            kModelOptions, std::size(kModelOptions),
            "Custom loads mods/Horse.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Items", custom_model_config_var(CustomModel::Items),
            kModelOptions, std::size(kModelOptions),
            "Custom loads mods/Alink.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Animations",
            custom_model_config_var(CustomModel::Animations), kModelOptions,
            std::size(kModelOptions),
            "Custom loads mods/AlAnm.arc after restarting. Missing files keep the vanilla animations.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Ordon Shield",
            custom_model_config_var(CustomModel::OrdonShield), kModelOptions,
            std::size(kModelOptions),
            "Custom loads mods/CWShd.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Wooden Shield",
            custom_model_config_var(CustomModel::WoodenShield), kModelOptions,
            std::size(kModelOptions),
            "Custom loads mods/SWShd.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_select(ctx, left, "Hylian Shield",
            custom_model_config_var(CustomModel::HylianShield), kModelOptions,
            std::size(kModelOptions),
            "Custom loads mods/HyShd.arc after restarting. Missing files keep the vanilla model.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

void settings_closed(ModContext*, UiWindowHandle, void*) {
    s_settingsWindow = 0;
}

void open_settings(ModContext* ctx, void*) {
    if (s_settingsWindow != 0) {
        return;
    }

    std::array<UiTabDesc, 7> tabs{};
    for (auto& tab : tabs) {
        tab = UI_TAB_DESC_INIT;
    }
    tabs[0].title = "General";
    tabs[0].build = build_general_tab;
    tabs[1].title = "Aiming";
    tabs[1].build = build_aiming_tab;
    tabs[2].title = "Controls";
    tabs[2].build = build_controls_tab;
    tabs[3].title = "HUD";
    tabs[3].build = build_hud_tab;
    tabs[4].title = "Gameplay";
    tabs[4].build = build_gameplay_tab;
    tabs[5].title = "Hard Mode";
    tabs[5].build = build_hard_mode_tab;
    tabs[6].title = "Models";
    tabs[6].build = build_models_tab;

    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs.data();
    desc.tab_count = tabs.size();
    desc.on_closed = settings_closed;
    svc_ui->window_push(ctx, &desc, &s_settingsWindow);
}

ModResult build_mod_panel(ModContext* ctx, UiElementHandle panel, void*, ModError*) {
    if (add_section(ctx, panel, "Dawnlight Settings") != MOD_OK) return MOD_ERROR;
    if constexpr (kDawnlightUpdateCheckerAvailable) {
        if (add_toggle(ctx, panel, "CHECK FOR UPDATES", check_for_updates_config_var(),
                "Checks BeZide93/dawnlight releases for a newer Dawnlight mod version.")
            != MOD_OK)
        {
            return MOD_ERROR;
        }
    }
    if (add_button(ctx, panel, "Open Dawnlight Settings", open_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Aim Movement, Aim Modes, and Bullet Time") != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Flurry Rush, Fierce Deity, and Great Spin Projectile") != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Shared Stamina and Lazy Tweaks compatibility") != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Manual Shielding, R Jump, and Sprint") != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Z Item Slot and Dawnlight Touch UI") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "Intro Skip new-save mode") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "Boss Rush hub, portals, resume, and hardmode") != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "HUD Layout Editor") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "Custom models, animations, and shields") != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Enemy HP and NG+ scaling") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "Save compatibility and item integrity fixes") != MOD_OK) return MOD_ERROR;
    return MOD_OK;
}

}  // namespace

ModResult register_ui(ModError* error) {
    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build_mod_panel;
    ModResult result = svc_ui->register_mods_panel(mod_ctx, &panel);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight mod panel");
    }

    UiMenuTabDesc tab = UI_MENU_TAB_DESC_INIT;
    tab.label = "Dawnlight";
    tab.on_selected = open_settings;
    result = svc_ui->register_menu_tab(mod_ctx, &tab, &s_menuTab);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight menu tab");
    }
    return MOD_OK;
}

}  // namespace dawnlight
