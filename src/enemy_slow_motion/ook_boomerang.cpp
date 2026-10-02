#include "boss.hpp"
#include "c/c_damagereaction.h"
#include "d/actor/d_a_e_mk.h"
#include "d/actor/d_a_e_mk_bo.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mk_bo.cpp#action", void(e_mk_bo_class*), ActionHook);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mk_bo.cpp#e_mk_bo_shot", s8(e_mk_bo_class*), FlightHook);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mk_bo.cpp#e_mk_bo_r04", s8(e_mk_bo_class*), OrbitHook);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mk_bo.cpp#e_mk_bo_hasira", void(e_mk_bo_class*), PillarHook);

e_mk_bo_class& boomerang(fopAc_ac_c* actor) {
    static_assert(offsetof(e_mk_bo_class, enemy) == 0);
    return *reinterpret_cast<e_mk_bo_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boomerang(actor);
    if (!a.model || a.field_0x9b4 || a.field_0x600 || a.action < 0 || a.action > 1 ||
        cDmrNowMidnaTalk() || dComIfGp_event_runCheck()) return false;
    auto* parent = fopAcM_SearchByID(actor->parentActorID);
    if (!parent || fopAcM_GetName(parent) != fpcNm_E_MK_e) return false;
    return reinterpret_cast<e_mk_class*>(parent)->demoMode == e_mk_class::DEMO_MODE_NONE;
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boomerang(step.actor);
    step.action = a.action;
    step.values[0] = a.counter;
    // The only combat consumer is the modulo-eight flight sound. Held display
    // frames must neither repeat that event nor advance its simulation phase.
    a.counter = static_cast<s16>(enemy_periodic_counter_input(static_cast<u16>(a.counter), tick));
    hold_boss_timer_array(tick, a.timers);
    hold_boss_timers(tick, a.field_0x5f8);
    step.chaseAngles = {&a.enemy.current.angle.x, &a.enemy.current.angle.y,
        &a.field_0x5ec, &a.field_0x5ee, &a.field_0x5fa};
    step.chaseFloats = {&a.field_0x5f0, &a.enemy.current.pos.x, &a.enemy.current.pos.z};
}
HookAction before_action(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_bo_class*>(args, 0);
    if (auto* step = boss_step(a, fpcNm_E_MK_BO_e)) {
        // Boss Hard Mode pre-advances the projectile by 20 units before native
        // execute. Scale that bonus before homing/catch distance tests, then
        // scale native flight separately. This preserves 40 -> 60 (150%).
        a->enemy.current.pos = step->originalPosition +
            (a->enemy.current.pos - step->originalPosition) * step->scale;
        step->position = a->enemy.current.pos;
    }
    return HOOK_CONTINUE;
}
void after_flight(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_bo_class*>(args, 0);
    if (auto* step = boss_step(a, fpcNm_E_MK_BO_e)) {
        // Both flight functions return before action's CrrPos/hit_check and
        // ccAtSph.SetC, including the returning branch which skips CrrPos.
        a->enemy.current.pos = step->position +
            (a->enemy.current.pos - step->position) * step->scale;
        // Keep speed intact: striking Ook can replace it with an upward impulse.
    }
}
HookAction before_orbit(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_bo_class*>(args, 0);
    if (auto* step = boss_step(a, fpcNm_E_MK_BO_e); step && a->mode == 1 && a->timers[0])
        a->enemy.current.angle.y -= static_cast<s16>(std::lround(a->field_0x5ee * (1.0f - step->scale)));
    return HOOK_CONTINUE;
}
HookAction before_pillar(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_bo_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_E_MK_BO_e);
    if (!step) return HOOK_CONTINUE;
    const auto* parent = reinterpret_cast<e_mk_class*>(fopAcM_SearchByID(a->enemy.parentActorID));
    if (!parent || parent->demoMode != e_mk_class::DEMO_MODE_NONE || !parent->hasira)
        return HOOK_CONTINUE;
    step->values[1] = a->enemy.speed.y;
    step->values[2] = a->field_0x5fc;
    step->values[3] = 1;
    step->subaction = a->mode;
    // Let the native clamp/bounce test see the actual slowed displacement.
    // Restore physical velocity below, accounting for gravity and restitution.
    a->enemy.speed.y *= step->scale;
    return HOOK_CONTINUE;
}
void after_pillar(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_bo_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_E_MK_BO_e);
    if (!step || !step->values[3]) return;
    float velocity = step->values[1] - 5.0f * step->scale;
    if (a->mode != step->subaction) {
        velocity = step->subaction <= 3 ? velocity * -0.4f : 0.0f;
        if (step->subaction > 3 && !step->timerTick) a->mode = step->subaction;
    }
    a->enemy.speed.y = velocity;
    a->field_0x5fc = step->values[2] + (a->field_0x5fc - step->values[2]) * step->scale;
    step->values[3] = 0;
}
void after_action(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_bo_class*>(args, 0);
    if (auto* step = boss_step(a, fpcNm_E_MK_BO_e); step && step->action == 0)
        a->enemy.shape_angle.y = slow_enemy_angle(step->originalShapeAngles.y, a->enemy.shape_angle.y, step->scale);
}
void after_execute(EnemySlowStep& step) {
    if (!step.timerTick) boomerang(step.actor).counter = static_cast<s16>(step.values[0]);
}
ModResult install() {
    auto result = mods::hook::add_pre<ActionHook>(svc_hook, before_action);
    if (result == MOD_OK) result = mods::hook::add_post<ActionHook>(svc_hook, after_action);
    if (result == MOD_OK) result = mods::hook::add_post<FlightHook>(svc_hook, after_flight);
    if (result == MOD_OK) result = mods::hook::add_pre<OrbitHook>(svc_hook, before_orbit);
    if (result == MOD_OK) result = mods::hook::add_post<OrbitHook>(svc_hook, after_flight);
    if (result == MOD_OK) result = mods::hook::add_pre<PillarHook>(svc_hook, before_pillar);
    if (result == MOD_OK) result = mods::hook::add_post<PillarHook>(svc_hook, after_pillar);
    return result;
}
}
const EnemySlowProfile& ook_boomerang_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_MK_BO_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, nullptr, after_execute, true};
    return profile;
}
}
