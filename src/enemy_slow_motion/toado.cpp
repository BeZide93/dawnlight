#include "boss.hpp"
#include "d/actor/d_a_e_ot.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    auto& a = *static_cast<daE_OT_c*>(actor);
    return a.mpMorf && a.mDemoMode == 0 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daE_OT_c*>(step.actor);
    step.animations = {a.mpMorf};
    own_boss_controller(step, a.mpEggAnm);
    step.chaseFloats = {&a.speedF, &a.mScale, &a.mAnmSpeed};
    step.chaseAngles = {&a.shape_angle.x, &a.shape_angle.y, &a.current.angle.y};
    hold_boss_timers(tick, a.mTimer1, a.mTimer2);
    // Attached eggs are placed from the parent's live joints. Only posMoveF's
    // subsequent velocity integration is scaled; the attachment stays exact.
}
}
const EnemySlowProfile& toado_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_OT_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
