#include "boss.hpp"
#include "d/actor/d_a_e_pm.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daE_PM_c*>(actor);
    return a.mpMorf && a.mSecondEncounter && a.mDemoMode == 0 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daE_PM_c*>(step.actor);
    step.animations = {a.mpMorf, a.mpTrumpetMorf, a.mpGlowEffectMorf};
    own_boss_controller(step, a.mpEyeAnm);
    step.chaseFloats = {&a.field_0x5fc, &a.speedF};
    step.chaseAngles = {&a.current.angle.y, &a.field_0x6fa, &a.mHeadAngleX, &a.mHeadAngleZ, &a.mLampAngle.x, &a.mLampAngle.z};
    hold_boss_timer_array(tick, a.mTimer);
}
}
const EnemySlowProfile& skull_kid_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_PM_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
