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

#if defined(__ANDROID__)
#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_object.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "dusk/config_var.hpp"
// Same pinned private touch ABI used by the existing Dawnlight touch hooks.
#define private public
#define protected public
#include "dusk/ui/touch_controls.hpp"
#undef protected
#undef private
#include "dusk/ui/touch_controls_common.hpp"
#endif

namespace dawnlight {
namespace {
struct ButtonConfig { ConfigVarHandle enabled = 0, x = 0, y = 0, size = 0; };
std::array<ButtonConfig, touch::Count> s_buttons{};
UiWindowHandle s_touchWindow = 0;
size_t s_editButton = 0;
UiElementHandle s_preview = 0;
std::string s_previewRml;
float s_viewWidth = 800.f, s_viewHeight = 450.f;
#if defined(__ANDROID__)
bool s_touchRuntimeAvailable = false;
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
void reset_layout(ModContext* ctx, void*) {
    const auto& c = s_buttons[s_editButton];
    const auto d = touch::Defaults[s_editButton];
    svc_config->set_int(ctx, c.x, d.x);
    svc_config->set_int(ctx, c.y, d.y);
    svc_config->set_int(ctx, c.size, d.size);
}
ConfigVarHandle edit_var(size_t field) {
    const auto& c = s_buttons[s_editButton];
    return field == 1 ? c.x : field == 2 ? c.y : c.size;
}
std::array<size_t, 4> s_editFields = {0, 1, 2, 3};
void editor_get(ModContext* ctx, void* data, UiControlValue* out) {
    const auto field = *static_cast<size_t*>(data);
    if (field == 0) out->int_value = s_editButton;
    else svc_config->get_int(ctx, edit_var(field), &out->int_value);
}
void editor_set(ModContext* ctx, void* data, const UiControlValue* value) {
    const auto field = *static_cast<size_t*>(data);
    if (field == 0) s_editButton = static_cast<size_t>(std::clamp<int64_t>(value->int_value, 0, touch::Count - 1));
    else svc_config->set_int(ctx, edit_var(field), std::clamp<int64_t>(value->int_value,
        field == 3 ? 28 : 0, field == 3 ? 120 : 100));
}
ModResult update_preview(ModContext* ctx, void*, ModError*) {
    if (s_preview == 0) return MOD_OK;
    // Preview uses the last live touch viewport, scaled to fit the settings pane.
    const float width = 360.f;
    const float scale = width / std::max(s_viewWidth, 1.f);
    const float height = s_viewHeight * scale;
    char text[512];
    std::snprintf(text, sizeof(text),
        "<div style='position:relative;width:360dp;height:%.1fdp;background-color:#17212b;border:1dp #8294a4;'>", height);
    std::string rml = text;
    for (size_t i = 0; i < touch::Count; ++i) {
        if (!button_enabled(i) && i != s_editButton) continue;
        const auto r = touch::rect(button_layout(i), s_viewWidth, s_viewHeight);
        std::snprintf(text, sizeof(text),
            "<div style='position:absolute;left:%.1fdp;top:%.1fdp;width:%.1fdp;height:%.1fdp;"
            "display:flex;align-items:center;justify-content:center;border:1dp #ccd7df;border-radius:6dp;"
            "font-size:12dp;color:#ffffff;background-color:%s;'>%s</div>",
            r.x * scale, r.y * scale, r.size * scale, r.size * scale,
            i == s_editButton ? "#386d88" : "#424c56", touch::Labels[i]);
        rml += text;
    }
    rml += "</div>";
    if (rml != s_previewRml) {
        const auto result = svc_ui->elem_set_rml(ctx, s_preview, rml.c_str());
        if (result != MOD_OK) return result;
        s_previewRml = std::move(rml);
    }
    return MOD_OK;
}
ModResult build_button_choices(ModContext* ctx, UiWindowHandle, UiElementHandle left,
    UiElementHandle, void*, ModError*) {
    s_preview = 0;
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
        if (i == 0) desc.help_rml = "Independent left analog trigger, including Dawnlight's manual shield. The existing L button stays available.";
        if (svc_ui->pane_add_control(ctx, left, &desc, nullptr) != MOD_OK) return MOD_ERROR;
    }
    return MOD_OK;
}
ModResult build_button_editor(ModContext* ctx, UiWindowHandle, UiElementHandle left,
    UiElementHandle, void*, ModError* error) {
    s_preview = 0;
    s_previewRml.clear();
    if (svc_ui->pane_add_text(ctx, left,
        "Select a button, then adjust its position and size. X runs left to right; Y runs top to bottom. "
        "The highlighted button appears in this preview even when disabled. Layout changes are saved automatically.", nullptr) != MOD_OK) return MOD_ERROR;
    const char* labels[] = {"Button", "Horizontal Position", "Vertical Position", "Button Size"};
    for (size_t field = 0; field < 4; ++field) {
        UiControlDesc desc = UI_CONTROL_DESC_INIT;
        desc.kind = field == 0 ? UI_CONTROL_SELECT : UI_CONTROL_NUMBER;
        desc.label = labels[field];
        desc.binding = UI_BINDING_CALLBACKS;
        desc.get = editor_get;
        desc.set = editor_set;
        desc.user_data = &s_editFields[field];
        if (field == 0) {
            desc.options = touch::Names.data();
            desc.option_count = touch::Names.size();
        } else {
            desc.min = field == 3 ? 28 : 0;
            desc.max = field == 3 ? 120 : 100;
            desc.step = 1;
            desc.suffix = field == 3 ? "dp" : "%";
        }
        if (svc_ui->pane_add_control(ctx, left, &desc, nullptr) != MOD_OK) return MOD_ERROR;
    }
    UiControlDesc reset = UI_CONTROL_DESC_INIT;
    reset.kind = UI_CONTROL_BUTTON;
    reset.label = "Reset Selected Button Layout";
    reset.on_pressed = reset_layout;
    if (svc_ui->pane_add_control(ctx, left, &reset, nullptr) != MOD_OK) return MOD_ERROR;
    if (svc_ui->pane_add_rml(ctx, left, "", &s_preview) != MOD_OK) return MOD_ERROR;
    return update_preview(ctx, nullptr, error);
}
void touch_window_closed(ModContext*, UiWindowHandle, void*) {
    s_touchWindow = 0;
    s_preview = 0;
    s_previewRml.clear();
}

#if defined(__ANDROID__)
#include "touch_buttons_native.inc"
#endif
}  // namespace

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
            const auto result = svc_config->register_var(mod_ctx, &desc, handles[f]);
            if (result != MOD_OK) return mods::set_error(error, result, "failed to register touch button setting");
        }
    }
    return MOD_OK;
}
void open_touch_buttons(ModContext* ctx, void*) {
    if (s_touchWindow != 0) return;
    UiTabDesc tabs[2] = {UI_TAB_DESC_INIT, UI_TAB_DESC_INIT};
    tabs[0].title = "Touch Buttons";
    tabs[0].build = build_button_choices;
    tabs[1].title = "Layout Editor";
    tabs[1].build = build_button_editor;
    tabs[1].update = update_preview;
    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs;
    desc.tab_count = 2;
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
