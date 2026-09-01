#include "config.hpp"
#include "service_imports.hpp"

#include "mods/service.hpp"
#include "mods/svc/ui.h"

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

ModResult add_toggle(ModContext* ctx, UiElementHandle pane, const char* label,
    ConfigVarHandle var, const char* help = nullptr, UiPredicateFn isDisabled = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_TOGGLE;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    desc.is_disabled = isDisabled;
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
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
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
    if (add_toggle(ctx, left, "Bullet Time", bullet_time_config_var(),
            "Slows gameplay while aiming the Bow during a manual R jump. Press A to cancel it.")
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
            "Uses R as a fallback jump button when no R interaction or targeting action is active. "
            "Press R+B during the jump to start a jump attack.")
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
            "outside cutscenes. This works independently from Dawnlight's Z Item Slot for "
            "compatibility with other third-item mods. Restart the app after changing this "
            "setting.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
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
            "Draws X and Y with Dawnlight's round HUD button style.")
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
    return MOD_OK;
}

ModResult build_gameplay_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "New Saves") != MOD_OK) return MOD_ERROR;
    if (add_select(ctx, left, "New Save Mode", new_save_mode_config_var(),
            kNewSaveModeOptions, std::size(kNewSaveModeOptions),
            "Changes how newly created empty save slots are initialized. Vanilla keeps upstream "
            "behavior, and Intro Skip starts after the Faron intro setup.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(ctx, left, "Boss Rush Hardmode") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Arena Hazards", bossrush_hardmode_hazards_config_var(),
            "Spawns three damaging projectiles every 10 seconds during active Boss Rush fights.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(ctx, left, "Enemy Scaling") != MOD_OK) return MOD_ERROR;
    if (add_number(ctx, left, "HP Scaling", health_scale_config_var(), 1, 9999, 10, "%",
            "Scales enemy health when enemies spawn. New Game Plus can raise the effective value "
            "above this setting when automatic scaling is enabled.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "NG+ Auto HP Scaling", automatic_health_scale_config_var(),
            "Applies Dawnlight's NG+ health floor based on the NG+ counter.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(ctx, left, "Compatibility") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Save Compatibility Repairs", save_compatibility_config_var(),
            "Repairs known Dawnlight save-state issues while loading or progressing saves.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Item Integrity Fixes", item_integrity_config_var(),
            "Keeps bottle contents and item combinations from turning into invalid items.")
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
    return MOD_OK;
}

ModResult build_deferred_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Waiting For Services") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, left,
            "New Game+ is not enabled in this upstream-main package yet because it needs a "
            "source-save selection flow.")
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

    std::array<UiTabDesc, 6> tabs{};
    for (auto& tab : tabs) {
        tab = UI_TAB_DESC_INIT;
    }
    tabs[0].title = "Aiming";
    tabs[0].build = build_aiming_tab;
    tabs[1].title = "Controls";
    tabs[1].build = build_controls_tab;
    tabs[2].title = "HUD";
    tabs[2].build = build_hud_tab;
    tabs[3].title = "Gameplay";
    tabs[3].build = build_gameplay_tab;
    tabs[4].title = "Models";
    tabs[4].build = build_models_tab;
    tabs[5].title = "Deferred";
    tabs[5].build = build_deferred_tab;

    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs.data();
    desc.tab_count = tabs.size();
    desc.on_closed = settings_closed;
    svc_ui->window_push(ctx, &desc, &s_settingsWindow);
}

ModResult build_mod_panel(ModContext* ctx, UiElementHandle panel, void*, ModError*) {
    if (add_section(ctx, panel, "Dawnlight Settings") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, panel, "CHECK FOR UPDATES", check_for_updates_config_var(),
            "Checks BeZide93/dawnlight releases for a newer Dawnlight mod version.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_button(ctx, panel, "Open Dawnlight Settings", open_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Aim Movement, Aim Modes, and Bullet Time") != MOD_OK) {
        return MOD_ERROR;
    }
    if (add_text(ctx, panel, "Manual Shielding and R Jump") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "Z item slot") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "Intro Skip new-save mode") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "Boss Rush prelaunch mode, hub, and portal prompts") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, panel, "HUD Layout Editor") != MOD_OK) return MOD_ERROR;
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
