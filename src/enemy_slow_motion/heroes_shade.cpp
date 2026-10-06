#include "boss.hpp"
#include "../heroes_shade_encounter.hpp"
#include "d/actor/d_a_npc_kn.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daNpc_Kn_c::decTmr, TimerHook);
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daNpc_Kn_c*>(actor);
    return boss_conditional_timers_available() && a.mpModelMorf[0] &&
        heroes_shade_combat_slow_eligible(actor);
}
void prepare(EnemySlowStep& step, bool) {
    auto& a = *static_cast<daNpc_Kn_c*>(step.actor);
    step.slowDirectMove = true;
    step.animations = {a.mpModelMorf[0], a.mpModelMorf[1]};
    own_boss_controller(step, &a.mBckAnm);
    own_boss_controller(step, &a.mBtpAnm);
    own_boss_controller(step, &a.mBtkAnm);
    own_boss_controller(step, &a.mBrkAnm);
    own_boss_controller(step, &a.mBpkAnm);
    step.chaseFloats = {&a.speedF};
    step.chaseAngles = {&a.mCurAngle.y, &a.current.angle.y, &a.shape_angle.y, &a.field_0x1712};
    step.conditionalIntTimers = {&a.field_0xdec, &a.field_0xddc, &a.mBtpPauseTimer};
}
HookAction before_timer(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<daNpc_Kn_c*>(args, 0);
    auto* step = boss_step(a, fpcNm_NPC_KN_e);
    // decTmr runs AFTER action, so hold the value at the actual decrement.
    if (step) hold_boss_timers(step->timerTick, a->mTimer);
    return HOOK_CONTINUE;
}
ModResult install() { return mods::hook::add_pre<TimerHook>(svc_hook, before_timer); }
}
const EnemySlowProfile& heroes_shade_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_NPC_KN_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, nullptr, nullptr, true};
    return profile;
}
}
