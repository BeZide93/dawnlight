// Exercise the real config getters and UI callbacks against persisted fake cvars.
#include "../src/config.hpp"
#include "../src/progression.hpp"
#include "../src/ui.cpp"
#include <cassert>
#include <cmath>
#include <map>
#include <string>

ModContext* mod_ctx = nullptr;
const ConfigService* svc_config = nullptr;
const UiService* svc_ui = nullptr;
namespace dawnlight {
bool g_configCheckForUpdatesEnabled = false;
ProgressionState testProgress;
ProgressionState progression_state() { return testProgress; }
}
namespace {
struct Saved { ConfigVarType type; int64_t value; };
std::map<std::string, Saved> disk;
std::map<ConfigVarHandle, std::string> handles;
ConfigVarHandle nextHandle = 1;
ModResult register_var(ModContext*, const ConfigVarDesc* desc, ConfigVarHandle* out) {
    disk.try_emplace(desc->name, Saved{desc->type,
        desc->type == CONFIG_VAR_BOOL ? desc->default_bool : desc->default_int});
    *out = nextHandle++;
    handles[*out] = desc->name;
    return MOD_OK;
}
ModResult get_bool(ModContext*, ConfigVarHandle var, bool* value) {
    assert(disk.at(handles.at(var)).type == CONFIG_VAR_BOOL);
    *value = disk.at(handles.at(var)).value != 0;
    return MOD_OK;
}
ModResult get_int(ModContext*, ConfigVarHandle var, int64_t* value) {
    assert(disk.at(handles.at(var)).type == CONFIG_VAR_INT);
    *value = disk.at(handles.at(var)).value;
    return MOD_OK;
}
ModResult set_bool(ModContext*, ConfigVarHandle var, bool value) {
    disk.at(handles.at(var)).value = value;
    return MOD_OK;
}
ModResult set_int(ModContext*, ConfigVarHandle var, int64_t value) {
    disk.at(handles.at(var)).value = value;
    return MOD_OK;
}
ModResult subscribe(ModContext*, ConfigVarHandle, ConfigChangedFn, void*, ConfigSubscriptionHandle*) {
    return MOD_OK;
}
UiControlDesc control(ConfigVarHandle var, UiControlKind kind) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = kind;
    desc.config_var = var;
    dawnlight::bind_mode_control(desc);
    assert(desc.binding == UI_BINDING_CALLBACKS);
    return desc;
}
bool disabled(const UiControlDesc& desc) { return desc.is_disabled(mod_ctx, desc.user_data); }
UiControlValue shown(const UiControlDesc& desc) {
    UiControlValue value = UI_CONTROL_VALUE_INIT;
    desc.get(mod_ctx, desc.user_data, &value);
    return value;
}
}
int main() {
    using namespace dawnlight;
    ConfigService config{};
    config.register_var = register_var;
    config.get_bool = get_bool;
    config.get_int = get_int;
    config.set_bool = set_bool;
    config.set_int = set_int;
    config.subscribe = subscribe;
    svc_config = &config;
    assert(register_config(nullptr) == MOD_OK);
    assert(!dawnlight_mode_enabled() && !progression_system_enabled());
    set_int(nullptr, sprint_speed_config_var(), 185);
    set_int(nullptr, jump_height_config_var(), 240);
    set_int(nullptr, health_scale_config_var(), 175);
    set_int(nullptr, gale_recovery_config_var(), 91);
    set_int(nullptr, bullet_time_config_var(), 0);
    set_bool(nullptr, sprint_config_var(), false);
    set_bool(nullptr, glide_config_var(), true);
    set_bool(nullptr, fierce_deity_config_var(), true);

    auto sprint = control(sprint_config_var(), UI_CONTROL_TOGGLE);
    auto speed = control(sprint_speed_config_var(), UI_CONTROL_NUMBER);
    auto progression = control(progression_system_config_var(), UI_CONTROL_TOGGLE);
    assert(!disabled(speed) && shown(speed).int_value == 185);
    set_bool(nullptr, dawnlight_mode_config_var(), true);
    assert(progression_system_enabled() && sprint_enabled() && r_jump_enabled());
    assert(std::abs(sprint_speed_multiplier() - 1.5f) < 0.001f);
    assert(std::abs(jump_height_multiplier() - 1.1f) < 0.001f);
    assert(health_scale_percent() == 300 && gale_recovery_seconds() == 60);
    assert(bullet_time_mode() == BulletTimeMode::Botw);
    assert(flurry_rush_enabled() && enemy_hard_mode_enabled() && bossrush_hardmode_hazards_enabled());
    assert(manual_shielding_enabled() && arrow_modes_enabled() && great_spin_projectile_enabled());
    assert(remove_normal_hit_invulnerability_enabled());
    assert(!glide_enabled() && !revalis_gale_enabled() && !gale_counter_visible() && !fierce_deity_enabled());
    assert(glide_item() == GlideItem::Glider && gale_counter_capacity() == 1);
    assert(disabled(speed) && disabled(sprint) && disabled(progression));
    assert(shown(speed).int_value == 150 && shown(sprint).bool_value && shown(progression).bool_value);
    UiControlValue attempt = UI_CONTROL_VALUE_INIT;
    attempt.int_value = 300;
    speed.set(mod_ctx, speed.user_data, &attempt);
    progression.set(mod_ctx, progression.user_data, &attempt);
    assert(disk.at("sprint-speed-percent").value == 185); // No hidden settings overwritten.
    assert(progression_system_enabled());
    testProgress = {true, true, true, 6};
    assert(glide_enabled() && revalis_gale_enabled() && gale_counter_visible() && fierce_deity_enabled());
    assert(gale_counter_capacity() == 6);

    // Restart: register from the same disk values with fresh handles.
    assert(register_config(nullptr) == MOD_OK);
    assert(dawnlight_mode_enabled() && progression_system_enabled() && health_scale_percent() == 300);
    set_bool(nullptr, dawnlight_mode_config_var(), false);
    assert(!progression_system_enabled());
    assert(!sprint_enabled() && glide_enabled() && fierce_deity_enabled());
    assert(std::abs(sprint_speed_multiplier() - 1.85f) < 0.001f);
    assert(std::abs(jump_height_multiplier() - 2.4f) < 0.001f);
    assert(health_scale_percent() == 175 && gale_recovery_seconds() == 91);
    assert(bullet_time_mode() == BulletTimeMode::Off);
    speed = control(sprint_speed_config_var(), UI_CONTROL_NUMBER);
    attempt.int_value = 205;
    speed.set(mod_ctx, speed.user_data, &attempt);
    assert(disk.at("sprint-speed-percent").value == 205);

    set_bool(nullptr, progression_system_config_var(), true);
    set_bool(nullptr, dawnlight_mode_config_var(), true);
    set_bool(nullptr, dawnlight_mode_config_var(), false);
    assert(progression_system_enabled()); // Restore independent progression On.
    assert(sprint_enabled() && std::abs(sprint_speed_multiplier() - 2.05f) < 0.001f);
    testProgress = {}; // A new save immediately gates features again.
    assert(!glide_enabled() && !revalis_gale_enabled() && !fierce_deity_enabled());
    assert(gale_counter_capacity() == 1);
    assert(!disabled(speed)); // Progression leaves speed editable.
    auto glide = control(glide_config_var(), UI_CONTROL_TOGGLE);
    assert(disabled(glide) && !shown(glide).bool_value);
    set_bool(nullptr, progression_system_config_var(), false);
    assert(!disabled(glide) && shown(glide).bool_value);
}
