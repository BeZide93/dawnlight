#include "boss.hpp"
#include "d/actor/d_a_e_fm.h"

namespace dawnlight {
namespace {
e_fm_class& boss(fopAc_ac_c* actor) {
    return *static_cast<e_fm_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boss(actor);
    return a.mpFmModelMorf && a.mDemoCamMode == 0 && a.mAction < 11 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boss(step.actor);
    step.animations = {a.mpFmModelMorf, a.mpDemoFmModelMorf};
    own_boss_controller(step, a.mpCoreBtk);
    own_boss_controller(step, a.mpCoreBrk);
    own_boss_controller(step, a.mpAttackEfBrk);
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.speedF, &a.field_0x1834, &a.field_0x778, &a.field_0x798, &a.field_0x79c, &a.field_0x7b8, &a.field_0x7fc, &a.mChainColorR, &a.mCoreBrkFrame, &a.mKankyoBlend};
    step.chaseAngles = {&a.current.angle.y, &a.mBodyRotX, &a.mHeadRotZ, &a.shape_angle.x, &a.shape_angle.y, &a.shape_angle.z};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.mDamageInvulnerabilityTimer);
    for (auto* controller : a.mpFmBtk) own_boss_controller(step, controller);
    for (auto* controller : a.mpFmBrk) own_boss_controller(step, controller);
    for (auto* controller : a.mpAttackEfBtk) own_boss_controller(step, controller);
    for (int i = 0; i < 2; ++i) step.animations[2 + i] = a.mpAttackEfModelMorf[i];
}
}
const EnemySlowProfile& fyrus_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_FM_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
