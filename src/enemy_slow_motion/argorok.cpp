#include "boss.hpp"
#include "d/actor/d_a_b_dr.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daB_DR_c*>(actor);
    return boss_conditional_timers_available() && (a.field_0x7d5 == 0 && a.arg0 != 10 && a.arg0 != 0xff && a.arg0 != 0xfe && (a.arg0 >= 20 || (a.mpModelMorf && a.mActionMode < 11))) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool) {
    auto& a = *static_cast<daB_DR_c*>(step.actor);
    step.slowDirectMove = true;
    step.animations = {a.mpModelMorf};
    own_boss_controller(step, a.mpCoreBrk);
    step.chaseFloats = {&a.current.pos.x, &a.current.pos.y, &a.current.pos.z, &a.field_0x724, &a.field_0x72c, &a.field_0x738, &a.field_0x740, &a.field_0x744, &a.field_0x748, &a.speed.y, &a.speedF};
    step.chaseAngles = {&a.current.angle.x, &a.current.angle.y, &a.field_0x750, &a.field_0x752, &a.mHeadAngle.x, &a.mHeadAngle.y, &a.shape_angle.x, &a.shape_angle.y, &a.shape_angle.z};
    step.chasePositions = {&a.current.pos, &a.mBoot_c_trance};
    step.conditionalByteTimers = {&a.field_0x7d0};
    own_enemy_values(step.conditionalIntTimers, a.mTimer);
    // Timers are decremented inside selected states, sometimes more than once.
    // Intercept only actual native calls: no speculative increments of dormant timers.
}
}
const EnemySlowProfile& argorok_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_DR_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
