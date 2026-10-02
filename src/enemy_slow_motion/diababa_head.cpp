#include "boss.hpp"
#include "d/actor/d_a_b_bh.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    [[maybe_unused]] const auto& a = *static_cast<b_bh_class*>(actor);
    return (a.mpModelMorf) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<b_bh_class*>(step.actor);
    step.animations = {a.mpModelMorf};
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.field_0x690, &a.field_0x914, &a.field_0x918, &a.mBasePos.y, &a.mMouthMizuParticleSize, &a.speedF};
    step.chaseAngles = {&a.shape_angle.x, &a.shape_angle.y, &a.shape_angle.z};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.field_0x69e);
}
}
const EnemySlowProfile& diababa_head_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_BH_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
