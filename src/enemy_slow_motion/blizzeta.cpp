#include "boss.hpp"
#include "d/actor/d_a_b_yo.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daB_YO_c::calcFreeMove, FreeMoveHook);
DEFINE_HOOK(&daB_YO_c::executeAttackBody, BodyHook);
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daB_YO_c*>(actor);
    return a.mpYetaMorf && a.mIsInactive != 1 && (a.mAction == daB_YO_c::ACT_CHASE || (a.mAction >= daB_YO_c::ACT_JUMP && a.mAction <= daB_YO_c::ACT_ATTACK_BODY) || a.mAction == daB_YO_c::ACT_DAMAGE) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daB_YO_c*>(step.actor);
    step.animations = {a.mpYetaMorf, a.mpYetaRevertedMorf};
    own_boss_controller(step, a.mpBtkAnm);
    own_boss_controller(step, a.mpYetaBtkAnm);
    own_boss_controller(step, a.mpYetaBtpAnm);
    own_boss_controller(step, a.mpYetaBrkAnm);
    own_boss_controller(step, a.mpYetaWhiteBrkAnm);
    step.chaseFloats = {&a.current.pos.y, &a.field_0xf64, &a.mBlureRate, &a.mColBlend, &a.mHensinScale, &a.mIceCenterSpeed, &a.mIceRange, &a.mScale, &a.speed.y, &a.speedF, &a.unk_F58};
    step.chaseAngles = {&a.current.angle.y, &a.mAngleSpeed, &a.mIceAngleSpeed, &a.mPlayerAngle, &a.shape_angle.y};
    step.chasePositions = {&a.current.pos, &a.mIceCenterPos};
    own_enemy_values(step.chaseFloats, a.mRoomAlpha);
    hold_boss_timers(tick, a.mActionTimer, a.mActionTimer2, a.mDamageTimer, a.mFreezardTimer, a.mIFrameTimer, a.mIFrameIronTimer, a.mQuakeTimer, a.mAttentionTimer, a.mCamLockOnTimer);
    step.action = a.mAction;
    step.subaction = a.mMode;
    step.neck = a.mIceAngle;
}
void before_move(EnemySlowStep& step) {
    auto& a = *static_cast<daB_YO_c*>(step.actor);
    if (a.mAction == step.action && a.mMode == step.subaction)
        a.mIceAngle = slow_enemy_angle(step.neck, a.mIceAngle, step.scale);
}
HookAction before_spin(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<daB_YO_c*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_YO_e);
    if (!step) return HOOK_CONTINUE;
    step->facing = a->shape_angle.y;
    step->values[10] = a->mFreeMoveMode;
    step->values[11] = a->mMode;
    return HOOK_CONTINUE;
}
void after_free_spin(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<daB_YO_c*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_YO_e);
    // Mode zero uses the already scaled angle chase. Wall reflection is an
    // instantaneous collision response, not angular velocity.
    if (step && step->values[10] != 0 && !a->mAcch.ChkWallHit())
        a->shape_angle.y = slow_enemy_angle(step->facing, a->shape_angle.y, step->scale);
}
void after_body_spin(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<daB_YO_c*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_YO_e);
    if (step && (step->values[11] == 0 || step->values[11] == 1 || step->values[11] == 10))
        a->shape_angle.y = slow_enemy_angle(step->facing, a->shape_angle.y, step->scale);
}
ModResult install() {
    auto result = mods::hook::add_pre<FreeMoveHook>(svc_hook, before_spin);
    if (result == MOD_OK) result = mods::hook::add_post<FreeMoveHook>(svc_hook, after_free_spin);
    if (result == MOD_OK) result = mods::hook::add_pre<BodyHook>(svc_hook, before_spin);
    if (result == MOD_OK) result = mods::hook::add_post<BodyHook>(svc_hook, after_body_spin);
    return result;
}
}
const EnemySlowProfile& blizzeta_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_YO_e, eligible, prepare,
        before_move, nullptr, install, nullptr, nullptr, nullptr, true};
    return profile;
}
}
