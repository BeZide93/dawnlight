#include "boss.hpp"
#include "d/actor/d_a_e_pz.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daE_PZ_c*>(actor);
    return a.arg0 != 10 && (a.arg0 >= 20 || (a.mpModelMorf && a.mActionMode != 1 && a.mActionMode != 5)) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daE_PZ_c*>(step.actor);
    step.animations = {a.mpModelMorf, a.mpBallModelMorf};
    own_boss_controller(step, a.mpPortalBtk);
    own_boss_controller(step, a.mpBallBrk);
    own_boss_controller(step, a.mpPzBtk);
    own_boss_controller(step, a.mpPzBrk);
    step.chaseFloats = {&a.current.pos.y, &a.field_0x7c4, &a.field_0x7c8, &a.mPzScale.x, &a.mPzScale.y, &a.mPzScale.z, &a.speedF};
    step.chaseAngles = {&a.current.angle.y};
    step.chasePositions = {&a.current.pos};
    hold_boss_timers(tick, a.field_0x7d0, a.field_0x7d1, a.field_0x7d2, a.field_0x7d3);
    for (auto* controller : a.mpPortalBrk) own_boss_controller(step, controller);
}
}
const EnemySlowProfile& phantom_zant_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_PZ_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
