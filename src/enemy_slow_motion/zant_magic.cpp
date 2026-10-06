#include "boss.hpp"
#include "d/actor/d_a_b_zant_magic.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    [[maybe_unused]] const auto& a = *static_cast<daB_ZANTM_c*>(actor);
    return (true) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daB_ZANTM_c*>(step.actor);
    step.chaseFloats = {&a.field_0x5e8};
    step.chaseAngles = {&a.current.angle.x, &a.current.angle.y};
    hold_boss_timers(tick, a.mAliveTimer);
}
}
const EnemySlowProfile& zant_magic_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_ZANTM_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
