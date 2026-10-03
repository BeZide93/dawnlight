#include "twilit_stamina.hpp"
#include "twilit_essentials/stamina.h"
#include "config.hpp"
#include "service_imports.hpp"
#include "dusk/config_var.hpp"

#include <cmath>
#include <algorithm>
#include <cstddef>
#include <string_view>

namespace dawnlight {
namespace {
bool s_enabled = false;
using GetConfigVarFn = dusk::config::ConfigVarBase* (*)(std::string_view);
GetConfigVarFn s_getConfigVar = nullptr;

bool read_state(TwilitEssentialsStaminaState& state) {
    const auto* service = svc_te_stamina;
    if (!service || service->header.major_version != 1 ||
        service->header.struct_size < offsetof(TwilitEssentialsStaminaService, restore) ||
        !service->get_state || !service->try_consume || !service->drain) {
        s_enabled = false;
        return false;
    }
    if (service->get_state(mod_ctx, &state) != MOD_OK) return false;
    s_enabled = state.enabled != 0;
    return std::isfinite(state.current) && std::isfinite(state.maximum) &&
           state.current >= 0 && state.maximum > 0;
}

void deny() {
    const auto* service = svc_te_stamina;
    if (service && service->header.struct_size >= offsetof(TwilitEssentialsStaminaService, get_source) &&
        service->deny) service->deny(mod_ctx);
}
} // namespace

bool twilit_stamina_active() {
    TwilitEssentialsStaminaState state = TWILIT_ESSENTIALS_STAMINA_STATE_INIT;
    read_state(state);
    // A transient provider error must not make paid abilities free or switch
    // them to a second pool. A successful disabled state or missing import does.
    return s_enabled;
}

bool twilit_stamina_gameplay() {
    TwilitEssentialsStaminaState state = TWILIT_ESSENTIALS_STAMINA_STATE_INIT;
    return read_state(state) && state.enabled && state.gameplay;
}

float twilit_stamina_cost(StaminaSetting setting) {
    float base;
    uint32_t source;
    switch (setting) {
    // TE's base costs are per 30 Hz Link step: 0.35 for Bullet Time, 0.90
    // for sprint. The service reports their live source toggles/multipliers.
    // Dawnlight's duplicate sliders must not affect these rates.
    case StaminaSetting::BulletTime: base = 0.35f * 30; source = TWILIT_ESSENTIALS_STAMINA_SOURCE_BULLET_TIME; break;
    case StaminaSetting::Sprint: base = 0.90f * 30; source = TWILIT_ESSENTIALS_STAMINA_SOURCE_SPRINT; break;
    case StaminaSetting::WolfSprint: base = 0.90f * 30; source = TWILIT_ESSENTIALS_STAMINA_SOURCE_WOLF_SPRINT; break;
    default: return stamina_setting(setting);
    }
    const auto* service = svc_te_stamina;
    // v1.0 providers expose the same first three functions, but no cost sources.
    if (!service || service->header.struct_size < sizeof(TwilitEssentialsStaminaService) ||
        !service->get_source) return base;
    TwilitEssentialsStaminaSourceInfo info = TWILIT_ESSENTIALS_STAMINA_SOURCE_INFO_INIT;
    if (service->get_source(mod_ctx, source, &info) != MOD_OK ||
        !std::isfinite(info.cost_multiplier) || info.cost_multiplier < 0) return -1.0f;
    const float cost = info.enabled ? base * info.cost_multiplier : 0.0f;
    return std::isfinite(cost) ? cost : -1.0f;
}

bool twilit_stamina_available(float amount) {
    TwilitEssentialsStaminaState state = TWILIT_ESSENTIALS_STAMINA_STATE_INIT;
    return std::isfinite(amount) && amount >= 0 && read_state(state) &&
        state.enabled && state.gameplay &&
        (amount == 0 || (!state.exhausted && state.current >= amount));
}

bool twilit_stamina_consume(float amount) {
    if (!twilit_stamina_available(amount)) {
        if (twilit_stamina_gameplay()) deny();
        return false;
    }
    const auto result = svc_te_stamina->try_consume(mod_ctx, amount);
    if (result == MOD_CONFLICT) deny();
    return result == MOD_OK;
}

bool twilit_stamina_drain(float amount) {
    if (!std::isfinite(amount) || amount < 0 || !twilit_stamina_gameplay()) return false;
    return svc_te_stamina->drain(mod_ctx, amount) == MOD_OK;
}

static bool twilit_feature_enabled(const char* key) {
    if (!twilit_stamina_active()) return false;
    // Resolve live feature switches without retaining provider-owned CVars.
    // Dusklight escapes dots to underscores and literal underscores to doubles.
    if (!s_getConfigVar) {
        void* address = nullptr;
        if (!svc_hook || !svc_hook->resolve ||
            svc_hook->resolve(mod_ctx, "dusk::config::GetConfigVar", &address, nullptr) != MOD_OK ||
            !address) return false;
        s_getConfigVar = reinterpret_cast<GetConfigVarFn>(address);
    }
    const auto* value = s_getConfigVar(key);
    return value && static_cast<const dusk::config::ConfigVar<bool>*>(value)->getValue();
}

bool twilit_sprint_enabled(bool wolf) {
    return twilit_feature_enabled(wolf ?
        "mod.com_dusklight_twilit__essentials.staminaWolfSprint" :
        "mod.com_dusklight_twilit__essentials.staminaSprint");
}

bool twilit_bullet_time_enabled() {
    return twilit_feature_enabled("mod.com_dusklight_twilit__essentials.bulletTimeEnabled");
}

float twilit_sprint_drain_multiplier(float speed, float baseSpeed) {
    if (!twilit_feature_enabled("mod.com_dusklight_twilit__essentials.staminaSprintDrainBySpeed") ||
        !std::isfinite(speed) || !std::isfinite(baseSpeed) || baseSpeed <= 0) return 1;
    return std::clamp(speed / (baseSpeed * 1.55f), 0.25f, 2.0f);
}

void shutdown_twilit_stamina() {
    s_enabled = false;
    s_getConfigVar = nullptr;
}
} // namespace dawnlight
