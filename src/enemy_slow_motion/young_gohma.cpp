#include "boss.hpp"
#include "d/actor/d_a_e_kg.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    [[maybe_unused]] const auto& a = *static_cast<e_kg_class*>(actor);
    return (a.mpMorf) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<e_kg_class*>(step.actor);
    step.animations = {a.mpMorf};
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.field_0x664, &a.speedF};
    step.chaseAngles = {&a.current.angle.y, &a.field_0x69e, &a.shape_angle.x, &a.shape_angle.y};
    hold_boss_timer_array(tick, a.field_0x694);
    hold_boss_timers(tick, a.field_0x69c);
}
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_kg.cpp#action", void(e_kg_class*), ActionHook);
void after_action(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_kg_class*>(args, 0);
    if (auto* step = boss_step(a, fpcNm_E_KG_e))
        hold_boss_timers(step->timerTick, a->field_0xa54);
}
ModResult install() { return mods::hook::add_post<ActionHook>(svc_hook, after_action); }
void before_collision(EnemySlowStep& step) {
    slow_enemy_gravity(step, -5.0f, -std::numeric_limits<float>::infinity(), false);
}
}
const EnemySlowProfile& young_gohma_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_KG_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, before_collision, nullptr, true};
    return profile;
}
}
