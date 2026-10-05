// Link against the production twilit_stamina.cpp object and the Dusklight SDK.
#include "twilit_stamina.hpp"
#include "twilit_essentials/stamina.h"
#include "mods/svc/hook.h"
#include <cassert>
#include <cmath>
#include <limits>

ModContext* mod_ctx = reinterpret_cast<ModContext*>(1);
const HookService* svc_hook = nullptr;
const TwilitEssentialsStaminaService* svc_te_stamina = nullptr;
namespace dawnlight {
int stamina_setting(StaminaSetting setting) {
    // Deliberately different from TE to detect accidental Dawnlight overrides.
    return setting == StaminaSetting::Glide ? 5 : 49;
}
}
namespace {
bool enabled = true, gameplay = true, exhausted = false, sourceEnabled = true;
float current = 100, multiplier = 2;
int payments = 0, denials = 0;
ModResult stateResult = MOD_OK, sourceResult = MOD_OK;
ModResult get_state(ModContext*, TwilitEssentialsStaminaState* out) {
    if (stateResult != MOD_OK) return stateResult;
    *out = TWILIT_ESSENTIALS_STAMINA_STATE_INIT;
    out->enabled = enabled; out->gameplay = gameplay; out->exhausted = exhausted;
    out->current = current; out->maximum = 100;
    return MOD_OK;
}
ModResult consume(ModContext*, float amount) {
    if (!enabled || !gameplay) return MOD_UNAVAILABLE;
    if (amount > 0 && (exhausted || current < amount)) return MOD_CONFLICT;
    ++payments; current -= amount; exhausted = current == 0;
    return MOD_OK;
}
ModResult drain(ModContext*, float amount) {
    if (amount > current) amount = current;
    return consume(nullptr, amount);
}
ModResult deny(ModContext*) { ++denials; return MOD_OK; }
ModResult get_source(ModContext*, uint32_t source, TwilitEssentialsStaminaSourceInfo* out) {
    assert(source == TWILIT_ESSENTIALS_STAMINA_SOURCE_SPRINT ||
           source == TWILIT_ESSENTIALS_STAMINA_SOURCE_WOLF_SPRINT);
    out->enabled = sourceEnabled; out->cost_multiplier = multiplier;
    return sourceResult;
}
void near(float a, float b) { assert(std::fabs(a - b) < 0.001f); }
}
int main() {
    using namespace dawnlight;
    assert(!twilit_stamina_active());
    const TwilitEssentialsStaminaService service{
        SERVICE_HEADER(TwilitEssentialsStaminaService, 1, 3),
        get_state, consume, drain, nullptr, deny, get_source};
    svc_te_stamina = &service;
    assert(twilit_stamina_active());
    near(twilit_stamina_cost(StaminaSetting::Sprint), 54);
    near(twilit_stamina_cost(StaminaSetting::WolfSprint), 54);
    near(twilit_stamina_cost(StaminaSetting::Glide), 5);
    sourceEnabled = false;
    near(twilit_stamina_cost(StaminaSetting::Sprint), 0);
    sourceEnabled = true;
    multiplier = std::numeric_limits<float>::quiet_NaN();
    assert(twilit_stamina_cost(StaminaSetting::Sprint) < 0);
    multiplier = 1;
    sourceResult = MOD_UNAVAILABLE;
    assert(twilit_stamina_cost(StaminaSetting::Sprint) < 0);
    sourceResult = MOD_OK;
    assert(twilit_stamina_consume(60)); near(current, 40);
    assert(!twilit_stamina_consume(50)); near(current, 40);
    assert(payments == 1 && denials == 1);
    assert(twilit_stamina_drain(100)); near(current, 0);
    assert(!twilit_stamina_available(.0001f));
    current = 20; assert(!twilit_stamina_available(1)); // Recovery lockout.
    assert(twilit_stamina_available(0));
    exhausted = false; gameplay = false;
    assert(!twilit_stamina_consume(0) && !twilit_stamina_drain(1));
    assert(twilit_stamina_active()); // Pause never selects the local pool.
    gameplay = true; stateResult = MOD_UNAVAILABLE;
    assert(twilit_stamina_active() && !twilit_stamina_consume(1));
    stateResult = MOD_OK; enabled = false;
    assert(!twilit_stamina_active());
    enabled = true;
    // ABI 1.0 really ends after drain: later members must not be read.
    struct LegacyService {
        ServiceHeader header;
        decltype(service.get_state) state;
        decltype(service.try_consume) consume;
        decltype(service.drain) drain;
    } legacy{{sizeof(LegacyService), 1, 0}, get_state, consume, drain};
    svc_te_stamina = reinterpret_cast<const TwilitEssentialsStaminaService*>(&legacy);
    assert(twilit_stamina_active()); near(twilit_stamina_cost(StaminaSetting::Sprint), 27);
    assert(twilit_stamina_consume(1));
    legacy.header.major_version = 2;
    assert(!twilit_stamina_active());
    svc_te_stamina = nullptr;
    assert(!twilit_stamina_active() && !twilit_stamina_consume(1));
    assert(!twilit_sprint_enabled());
    assert(twilit_owns_stamina_setting(StaminaSetting::Amount));
    assert(!twilit_owns_stamina_setting(StaminaSetting::Glide));
    shutdown_twilit_stamina();
}
