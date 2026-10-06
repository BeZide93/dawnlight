#include "boss.hpp"
#include "d/actor/d_a_e_gm.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    [[maybe_unused]] const auto& a = *static_cast<daE_GM_c*>(actor);
    return (a.mpModelMorf) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daE_GM_c*>(step.actor);
    step.animations = {a.mpModelMorf};
    step.chaseFloats = {&a.field_0xa10.x, &a.field_0xa10.y, &a.field_0xa10.z, &a.field_0xa40, &a.field_0xa50, &a.mColor, &a.speedF};
    step.chaseAngles = {&a.current.angle.x, &a.current.angle.y, &a.current.angle.z, &a.field_0xa5c, &a.field_0xa60, &a.field_0xa64};
    hold_boss_timers(tick, a.field_0xa6b, a.field_0xa6c, a.field_0xa6d, a.field_0xa72);
    step.slowDirectMove = true;
}
void before_move(EnemySlowStep& step) {
    // Jump/fall states integrate gravity themselves and then call posMove;
    // ordinary movement calls posMoveF. Correct only the former path.
    if (step.directMoving)
        step.actor->speed.y -= step.actor->gravity * (1.0f - step.scale);
}
}
const EnemySlowProfile& baby_gohma_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_GM_e, eligible, prepare,
        before_move, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
