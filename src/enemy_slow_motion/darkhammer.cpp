#include "boss.hpp"
#include "d/actor/d_a_e_th.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    auto& a = *static_cast<e_th_class*>(actor);
    return a.mpModelMorf && a.mDemoCamMode == 0 && a.mAction < 22 &&
        !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<e_th_class*>(step.actor);
    step.animations = {a.mpModelMorf};
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.speedF, &a.mSpinAnmSpeed};
    step.chaseAngles = {&a.shape_angle.y, &a.mHeadRotY, &a.mHeadRotZ};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.field_0x6a4, a.field_0x6a6);
    // action's direct ground translation precedes CrrPos. The native spin
    // animation also drives the attached ball; never advance it independently.
}
}
const EnemySlowProfile& darkhammer_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_TH_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
