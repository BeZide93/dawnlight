#include "boss.hpp"
#include "d/actor/d_a_b_gnd.h"

namespace dawnlight {
namespace {
b_gnd_class& boss(fopAc_ac_c* actor) {
    return *static_cast<b_gnd_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boss(actor);
    return a.mpModelMorf && a.mDemoCamMode == 0 && a.mNoDrawTimer == 0 && a.mActionMode != 6 && a.mActionMode != 19 && a.mActionMode != 22 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boss(step.actor);
    step.animations = {a.mpModelMorf, a.mpHorseMorf, a.mpZeldaModel};
    own_boss_controller(step, a.mpGndCoreBrk);
    own_boss_controller(step, a.mpGndEyeBtp);
    own_boss_controller(step, a.mpZeldaBtp);
    own_boss_controller(step, a.mpZeldaBtk);
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.speedF, &a.field_0x1e10, &a.field_0x1e4c, &a.field_0x1fd0, &a.field_0xeb0, &a.mKankyoBlend, &a.mPlaySpeed};
    step.chaseAngles = {&a.current.angle.y, &a.field_0x1e50.x, &a.field_0x1e50.y, &a.field_0x1fd6, &a.field_0xc68, &a.mGndBodyRotX, &a.mGndHeadRotZ, &a.mGndLegRotX, &a.mGndShoulderLRotY, &a.mHorseLegRot, &a.mSwordBlurAlpha, &a.shape_angle.x, &a.shape_angle.y, &a.shape_angle.z};
    hold_boss_timer_array(tick, a.field_0xc44);
    hold_boss_timers(tick, a.mDamageInvulnerabilityTimer, a.field_0xc5a, a.field_0xc72);
    for (auto* controller : a.mpGndEyeBtk) own_boss_controller(step, controller);
}
void before_collision(EnemySlowStep& step) {
    if (!eligible(step.actor)) { step.directCollision = nullptr; return; }
    slow_enemy_gravity(step, -5.0f, -100.0f, false);
}
}
const EnemySlowProfile& ganondorf_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_GND_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, before_collision, nullptr, true};
    return profile;
}
}
