#include "boss.hpp"
#include "d/actor/d_a_b_ds.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daB_DS_c*>(actor);
    return boss_conditional_timers_available() && (!a.mIsOpeningDemo && a.mBossPhase != 100 && ((a.arg0 == daB_DS_c::TYPE_BATTLE_1 && a.mpMorf && a.mAction != daB_DS_c::ACT_OPENING_DEMO) || (a.arg0 == daB_DS_c::TYPE_BATTLE_2 && a.mpMorf && a.mAction != daB_DS_c::ACT_B2_OPENING_DEMO && a.mAction != daB_DS_c::ACT_B2_DEAD) || a.arg0 == daB_DS_c::TYPE_BULLET_A || a.arg0 == daB_DS_c::TYPE_BULLET_B || a.arg0 == daB_DS_c::TYPE_BULLET_C)) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool) {
    auto& a = *static_cast<daB_DS_c*>(step.actor);
    step.slowDirectMove = true;
    step.animations = {a.mpMorf, a.mpSwordMorf, a.mpZantMorf};
    own_boss_controller(step, a.mpSwordBrkAnm);
    own_boss_controller(step, a.mpOpPatternBrkAnm);
    own_boss_controller(step, a.mpOpPatternBtkAnm);
    own_boss_controller(step, a.mpPatternBrkAnm);
    own_boss_controller(step, a.mpPatternBtkAnm);
    step.chaseFloats = {&a.current.pos.x, &a.current.pos.y, &a.current.pos.z, &a.field_0x790.x, &a.field_0x790.y, &a.field_0x790.z, &a.field_0x7f8, &a.field_0x804, &a.field_0x808, &a.field_0x80c, &a.mBreathTimerBase, &a.mChkHigh, &a.mColBlend, &a.mCrackAlpha, &a.mEyeColorAlpha, &a.mWallR, &a.mZantScale.x, &a.mZantScale.y, &a.mZantScale.z, &a.speedF};
    step.chaseAngles = {&a.current.angle.x, &a.current.angle.y, &a.mBh2AttackAngleF, &a.mHeadAngle.x, &a.mHeadAngle.y, &a.mHeadAngle.z, &a.shape_angle.x, &a.shape_angle.y, &a.shape_angle.z};
    step.chasePositions = {&a.current.pos};
    step.conditionalByteTimers = {&a.mDamageTimer};
    step.conditionalIntTimers = {&a.mBirthTrapTimerF, &a.mPedestalFallTimer, &a.mSandFallTimer, &a.mSwordTimer, &a.mHintTimer1, &a.mHintTimer2, &a.field_0x6a8, &a.mModeTimer, &a.mHitTimer, &a.mP2FallTimer, &a.mOutTimer};
    // Timers are decremented inside selected states, sometimes more than once.
    // Intercept only actual native calls: no speculative increments of dormant timers.
}
}
const EnemySlowProfile& stallord_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_DS_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
