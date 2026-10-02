#include "boss.hpp"
#include "d/actor/d_a_b_yo_ice.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    [[maybe_unused]] const auto& a = *static_cast<daB_YOI_c*>(actor);
    return (a.mpModel && a.mpBlizzeta) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daB_YOI_c*>(step.actor);
    step.chaseFloats = {&a.current.pos.y, &a.mCrackAlpha, &a.mScaleF, &a.mYoseSpeed, &a.speed.y};
    step.chaseAngles = {&a.mAngleSpeedY, &a.shape_angle.x, &a.shape_angle.z};
    step.chasePositions = {&a.current.pos};
    // Native execute returns early while deleting; other clocks do not run.
    if (a.mDeleteTimer) hold_boss_timers(tick, a.mDeleteTimer);
    else hold_boss_timers(tick, a.mTimer1, a.mTimer2, a.mIFrameTimer);
}
}
const EnemySlowProfile& blizzeta_ice_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_YOI_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
