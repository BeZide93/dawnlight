#include "boss.hpp"
#include "d/actor/d_a_b_zant_mobile.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    [[maybe_unused]] const auto& a = *static_cast<daB_ZANTZ_c*>(actor);
    return (a.mpMorf) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daB_ZANTZ_c*>(step.actor);
    step.animations = {a.mpMorf};
    step.chaseFloats = {&a.current.pos.y, &a.field_0x660, &a.field_0x664};
    step.chaseAngles = {&a.shape_angle.y};
    hold_boss_timers(tick, a.field_0x668);
}
}
const EnemySlowProfile& zant_mobile_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_ZANTZ_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
