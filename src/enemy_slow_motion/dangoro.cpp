#include "boss.hpp"
#include "d/actor/d_a_e_gob.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    auto& a = *static_cast<e_gob_class*>(actor);
    // 8 is the player-owned grab, 0/1 and 10/11 are demos/dialogue.
    return a.mpModelMorf && a.mDemoCamMode == 0 && a.mAction >= 2 &&
        a.mAction <= 9 && a.mAction != 8 &&
        // The slam assigns another actor's platform impulse at integer frame 5.
        // Keep the two actors on the original clock during that shared action.
        !(a.mAction == 3 && a.mMode == 13) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<e_gob_class*>(step.actor);
    step.animations = {a.mpModelMorf};
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.speedF, &a.field_0x680, &a.field_0x684, &a.field_0x688};
    step.chaseAngles = {&a.current.angle.y, &a.current.angle.z, &a.shape_angle.y,
        &a.shape_angle.z, &a.field_0x6a0, &a.field_0x6a2, &a.field_0x6b2,
        &a.field_0x6b4, &a.field_0x6b6, &a.mBodyRotY, &a.mBodyRotZ,
        &a.mHeadRotY, &a.mHeadRotZ};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.mDamageInvulnerabilityTimer, a.field_0x6aa);
}
void before_collision(EnemySlowStep& step) {
    auto& a = *static_cast<e_gob_class*>(step.actor);
    if (!eligible(&a)) { step.directCollision = nullptr; return; }
    slow_enemy_gravity(step, a.gravity, -std::numeric_limits<float>::infinity(), false);
    // The shared correction preserves the current/old Y shift used for rolling.
}
}
const EnemySlowProfile& dangoro_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_GOB_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, before_collision, nullptr, true};
    return profile;
}
}
