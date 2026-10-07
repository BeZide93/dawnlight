#include "touch_buttons.hpp"
#include "touch_button_state.hpp"
#include "config.hpp"
#include "mods/svc/config.h"
#include "mods/svc/ui.h"
#include "mods/svc/log.h"
#include "mods/svc/hook.h"
#include "mods/service.hpp"

#include <cstdio>
#include <string>
#include <string_view>
#include <sstream>
#include <locale>
#include <cmath>
#include <vector>

#if defined(__ANDROID__)
#include "global.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include <SDL3/SDL_gamepad.h>
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_msg_object.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "dusk/config_var.hpp"
// Same pinned private touch ABI used by the existing Dawnlight touch hooks.
#define private public
#define protected public
#include "dusk/ui/touch_controls.hpp"
#include "dusk/ui/touch_controls_editor.hpp"
#undef protected
#undef private
#include "dusk/ui/touch_controls_common.hpp"
#endif

namespace dawnlight {
namespace {
struct ButtonConfig { ConfigVarHandle enabled = 0, x = 0, y = 0, size = 0, layout = 0; };
std::array<ButtonConfig, touch::Count> s_buttons{};
ConfigVarHandle s_quickAccessLayout = 0;
UiWindowHandle s_touchWindow = 0;
bool s_midnaTouchPending = false;
bool s_midnaTouchTriggered = false;
touch::ActionInput s_jumpTouchInput, s_darkLinkTouchInput;
#if defined(__ANDROID__)
bool s_touchRuntimeAvailable = false;
bool s_touchEditorAvailable = false;
#endif

bool button_enabled(size_t i) {
    bool value = false;
    svc_config->get_bool(mod_ctx, s_buttons[i].enabled, &value);
    return value;
}
int config_number(ConfigVarHandle var, int fallback) {
    int64_t value = fallback;
    svc_config->get_int(mod_ctx, var, &value);
    return static_cast<int>(std::clamp<int64_t>(value, -10000, 10000));
}
touch::Layout button_layout(size_t i) {
    const auto& c = s_buttons[i];
    const auto d = touch::Defaults[i];
    return touch::clamp({config_number(c.x, d.x), config_number(c.y, d.y), config_number(c.size, d.size)});
}
ModResult build_button_choices(ModContext* ctx, UiWindowHandle, UiElementHandle left,
    UiElementHandle, void*, ModError*) {
    if (svc_ui->pane_add_text(ctx, left,
        "Enable extra buttons for Dawnlight Touch UI on Android. Dusklight Touch Controls and "
        "Dawnlight Touch UI must both be enabled. Changes here apply immediately.", nullptr) != MOD_OK) return MOD_ERROR;
#if defined(__ANDROID__)
    if (!s_touchRuntimeAvailable && svc_ui->pane_add_text(ctx, left,
        "Touch Buttons are not active. Enable Dawnlight Touch UI and restart. If they remain unavailable, "
        "this app build does not support the required touch features.", nullptr) != MOD_OK) return MOD_ERROR;
#endif
    for (size_t i = 0; i < touch::Count; ++i) {
        UiControlDesc desc = UI_CONTROL_DESC_INIT;
        desc.kind = UI_CONTROL_TOGGLE;
        desc.label = touch::Names[i];
        desc.binding = UI_BINDING_CONFIG_VAR;
        desc.config_var = s_buttons[i].enabled;
        if (i == 0) desc.help_rml = "Virtual left bumper (LB/L1) for compatible mods, including Twilight HD HUD. The action is defined by the mod; no fixed Midna action is assigned.";
        if (i == touch::Midna) desc.help_rml = "Call Midna with a dedicated touch button. Shown when Midna is available; move and resize it in the Touch Layout Editor.";
        if (i == touch::L3 || i == touch::R3) desc.help_rml = "Virtual left/right stick click for the game and compatible mods.";
        if (i == touch::Jump) desc.help_rml = "Jump, hold to charge Revali's Gale, or hold in the air to glide. Independent of Jump Button; normal feature settings and stamina costs apply.";
        if (i == touch::DarkLink) desc.help_rml = "Toggle Dark Link directly, independent of its activation binding. Requires the feature to be unlocked/enabled and a full meter unless Free Transform is enabled.";
        if (svc_ui->pane_add_control(ctx, left, &desc, nullptr) != MOD_OK) return MOD_ERROR;
    }
    return MOD_OK;
}
void touch_window_closed(ModContext*, UiWindowHandle, void*) {
    s_touchWindow = 0;
}

#if defined(__ANDROID__)
#include "touch_buttons_native.inc"
#endif
}  // namespace

bool consume_midna_touch_press() {
#if defined(__ANDROID__)
    prune_extra_presses(s_touchOwner);
#endif
    const bool pressed = s_midnaTouchTriggered;
    s_midnaTouchTriggered = false;
    return pressed;
}

bool touch_jump_pressed() {
#if defined(__ANDROID__)
    prune_extra_presses(s_touchOwner);
    return s_jumpTouchInput.pressed;
#else
    return false;
#endif
}
bool touch_jump_held() {
#if defined(__ANDROID__)
    prune_extra_presses(s_touchOwner);
    return s_extraHooksReady && s_extraPresses.held(touch::Jump);
#else
    return false;
#endif
}
bool consume_dark_link_touch_press() {
#if defined(__ANDROID__)
    prune_extra_presses(s_touchOwner);
    return s_darkLinkTouchInput.consume();
#else
    return false;
#endif
}

ModResult register_touch_button_config(ModError* error) {
    for (size_t i = 0; i < touch::Count; ++i) {
        auto& c = s_buttons[i];
        ConfigVarHandle* handles[] = {&c.enabled, &c.x, &c.y, &c.size};
        const char* fields[] = {"enabled", "x", "y", "size"};
        const auto d = touch::Defaults[i];
        const int defaults[] = {0, d.x, d.y, d.size};
        for (size_t f = 0; f < 4; ++f) {
            const auto key = std::string("touch-button-") + touch::Keys[i] + "-" + fields[f];
            ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
            desc.name = key.c_str();
            desc.type = f == 0 ? CONFIG_VAR_BOOL : CONFIG_VAR_INT;
            desc.default_bool = false;
            desc.default_int = defaults[f];
            const auto result = register_owned_setting(&desc, handles[f]);
            if (result != MOD_OK) return mods::set_error(error, result, "failed to register touch button setting");
        }
        const auto key = std::string("touch-button-") + touch::Keys[i] + "-layout";
        ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
        desc.name = key.c_str();
        desc.type = CONFIG_VAR_STRING;
        desc.default_string = "";
        const auto result = register_owned_setting(&desc, &c.layout);
        if (result != MOD_OK) return mods::set_error(error, result, "failed to register touch button layout");
    }
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = "touch-button-twe-quick-access-layout";
    desc.type = CONFIG_VAR_STRING;
    desc.default_string = "";
    const auto result = register_owned_setting(&desc, &s_quickAccessLayout);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to register Quick Access touch layout");
    return MOD_OK;
}
void open_touch_buttons(ModContext* ctx, void*) {
    if (s_touchWindow != 0) return;
    UiTabDesc tabs[1] = {UI_TAB_DESC_INIT};
    tabs[0].title = "Touch Buttons";
    tabs[0].build = build_button_choices;
    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs;
    desc.tab_count = 1;
    desc.on_closed = touch_window_closed;
    svc_ui->window_push(ctx, &desc, &s_touchWindow);
}
ModResult install_touch_button_hooks(ModError* error) {
#if defined(__ANDROID__)
    return install_native_buttons(error);
#else
    (void)error;
    return MOD_OK;
#endif
}
void shutdown_touch_buttons() {
#if defined(__ANDROID__)
    shutdown_native_buttons();
#endif
    if (s_touchWindow != 0) svc_ui->window_close(mod_ctx, s_touchWindow);
    touch_window_closed(nullptr, 0, nullptr);
}
}  // namespace dawnlight
