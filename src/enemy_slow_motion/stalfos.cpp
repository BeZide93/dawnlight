#include "integration.hpp"
#include "d/actor/d_a_e_sf.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
constexpr s16 kSitWait = 33;

e_sf_class& stalfos(fopAc_ac_c* actor) {
    static_assert(offsetof(e_sf_class, actor) == 0);
    return *reinterpret_cast<e_sf_class*>(actor);
}

bool eligible(fopAc_ac_c* actor) {
    const auto& a = stalfos(actor);
    return a.mpModelMorf != nullptr && a.mDemoMode == 0 &&
        a.mAction != kSitWait && !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = stalfos(step.actor);
    step.animations = {a.mpModelMorf};
    step.directCollision = &a.mBgc;
    step.sound = &a.mSound;
    step.frameSounds = {Z2SE_EN_SF_SWING_SWORD_S, Z2SE_EN_SF_SWING_SWORD_L};
    step.chaseFloats = {&a.actor.speedF, &a.field_0x6c4};
    step.chaseAngles = {&a.actor.current.angle.y, &a.actor.shape_angle.x,
        &a.actor.shape_angle.y, &a.actor.shape_angle.z, &a.mColor,
        &a.mHeadAngleY, &a.mHeadAngleZ, &a.mHeadBobAngleY, &a.mHeadAngleX,
        &a.field_0x6ea.x, &a.field_0x6ea.z};
    step.values[0] = a.mFrameCounter;
    step.values[1] = a.field_0x6bc;
    if (!tick) {
        for (auto& timer : a.mTimers) hold_enemy_timer(timer);
        hold_enemy_timer(a.mInvulnerabilityTimer);
        hold_enemy_timer(a.mUnkTimer1);
        hold_enemy_timer(a.mUnkTimer2);
        hold_enemy_timer(a.field_0x6bc);
        hold_enemy_timer(a.field_0x6f6);
        a.mFrameCounter = static_cast<s16>(enemy_periodic_counter_input(a.mFrameCounter, tick));
    }
}

void before_angle(EnemySlowStep& step, s16* value) {
    auto& a = stalfos(step.actor);
    if (value == &a.actor.shape_angle.z) {
        // action() integrates position before gravity, then resets gravity to -5.
        // Capture the actual branch's acceleration before that reset.
        step.gravity = a.field_0x6c0 != 0.0f ? -4.0f : a.actor.gravity;
    } else if (value == &a.field_0x6ea.x && step.values[1] > 0.0f) {
        // The optional head shake is derived from a countdown, not a BCK frame.
        const float time = std::max(0.0f, a.field_0x6bc - step.timerFraction);
        const s16 phase = static_cast<s16>(static_cast<int>(time * (BREG_S(5) + 12000)));
        a.mHeadBobAngleY = static_cast<s16>((BREG_F(18) + 200.0f) * time *
            cM_ssin(phase));
    }
}

void before_collision(EnemySlowStep& step) {
    slow_enemy_gravity(step, step.gravity, -120.0f, false);
}

void after_execute(EnemySlowStep& step) {
    if (!step.timerTick) stalfos(step.actor).mFrameCounter = static_cast<s16>(step.values[0]);
}
}

const EnemySlowProfile& stalfos_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_SF_e, eligible, prepare, nullptr,
        nullptr, nullptr, nullptr, before_collision, after_execute, true, nullptr, before_angle};
    return profile;
}
}
