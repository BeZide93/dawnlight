#include "profile.hpp"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_b_tn.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daB_TN_c::execute, ExecuteHook);
DEFINE_HOOK(&daB_TN_c::calcPartMove, PartMoveHook);
DEFINE_HOOK(&daB_TN_c::checkNormalAttackAble, AttackPermissionHook);
float s_attackCooldown = 0.0f;

void prepare(EnemySlowStep& step, bool timerTick) {
    auto* actor = static_cast<daB_TN_c*>(step.actor);
    step.action = actor->mActionMode1;
    step.subaction = actor->mActionMode2;
    step.neck = actor->mNeckAngle;
    step.waist = actor->mWaistAngle;
    step.animations = {actor->mpModelMorf1, actor->mpModelMorf2};
    if (!timerTick) {
        hold_enemy_timer(actor->mTimer1);
        hold_enemy_timer(actor->mInvincibilityTimer);
        hold_enemy_timer(actor->mTimer3);
        hold_enemy_timer(actor->mTimer4);
        hold_enemy_timer(actor->mTimer5);
        hold_enemy_timer(actor->mTimer6);
        hold_enemy_timer(actor->mTimer7);
        hold_enemy_timer(actor->mVibrationTimer);
        hold_enemy_timer(actor->mTimer9);
        hold_enemy_timer(actor->mTimer10);
        hold_enemy_timer(actor->mUpdateModelTimer);
        hold_enemy_timer(actor->mTimer12);
        hold_enemy_timer(actor->mTimer13);
        for (auto& timer : actor->field_0xa1c) {
            hold_enemy_timer(timer);
        }
    }
}

void before_move(EnemySlowStep& step) {
    auto* actor = static_cast<daB_TN_c*>(step.actor);
    slow_enemy_steering(step, actor->mActionMode1 == step.action && actor->mActionMode2 == step.subaction);
    actor->mNeckAngle = slow_enemy_angle(step.neck, actor->mNeckAngle, step.scale);
    actor->mWaistAngle = slow_enemy_angle(step.waist, actor->mWaistAngle, step.scale);
}

void observe_execute(fopAc_ac_c*, float scale) {
    s_attackCooldown = std::fmax(0.0f, s_attackCooldown - scale);
}

HookAction before_attack_permission(ModContext*, void* args, void* retval, void*) {
    auto* actor = mods::arg<daB_TN_c*>(args, 0);
    auto* step = current_enemy_slow_step();
    // Vanilla's shared m_attack_timer is private to the actor translation unit
    // and otherwise counts down at full speed. Track its accepted attacks in
    // slow time, leaving its ownership and player-state checks in place.
    if (step != nullptr && step->actor == actor && actor->mType == 1 &&
        s_attackCooldown > 0.000001f) {
        actor->mTimer3 = cM_rndF(60.0f) + 30.0f;
        *static_cast<bool*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void after_attack_permission(ModContext*, void* args, void* retval, void*) {
    auto* actor = mods::arg<daB_TN_c*>(args, 0);
    if (actor->mType == 1 && *static_cast<bool*>(retval)) {
        s_attackCooldown = 30.0f;
    }
}

HookAction before_part_move(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daB_TN_c*>(args, 0);
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor != actor) {
        return HOOK_CONTINUE;
    }

    // Same part state machines as vanilla, with fractional integration before
    // collision correction. Attached pieces still follow the live joint pose.
    for (int i = 0; i < 16; ++i) {
        if (actor->field_0xa1c[i] != 0) --actor->field_0xa1c[i];
        actor->mPositionsCopy[i] = actor->mPositions[i];
        const u16 previousState = actor->mStates[i];
        const cXyz previousVelocity = actor->field_0x8dc[i];
        const csXyz previousRotation = actor->field_0x99c[i];
        if (i == 12) actor->calcShieldMove();
        else if (i == 13) actor->calcSwordMoveA();
        else if (i == 15) actor->calcSwordMoveB();
        else actor->calcOtherPartMove(i);

        if (actor->mStates[i] == previousState &&
            (previousState == 3 || previousState == 4 || previousState == 5)) {
            actor->field_0x8dc[i] = previousVelocity +
                (actor->field_0x8dc[i] - previousVelocity) * step->scale;
            actor->field_0x99c[i].x = slow_enemy_angle(previousRotation.x, actor->field_0x99c[i].x, step->scale);
            actor->field_0x99c[i].y = slow_enemy_angle(previousRotation.y, actor->field_0x99c[i].y, step->scale);
            actor->field_0x99c[i].z = slow_enemy_angle(previousRotation.z, actor->field_0x99c[i].z, step->scale);
        }

        actor->mPositions[i] += *actor->mSttsArr[i].GetCCMoveP();
        if (actor->mStates[i] == 3 || actor->mStates[i] == 4 || actor->mStates[i] == 5) {
            actor->mPositions[i] += actor->field_0x8dc[i] * step->scale;
            actor->mAcchArr[i].CrrPos(dComIfG_Bgsp());
            if (actor->mChkCoHitOK && actor->mSphArr[i].ChkCoHit()) {
                cCcD_Obj* hit = actor->mSphArr[i].GetCoHitObj();
                if (hit != nullptr && dCc_GetAc(hit->GetAc()) == actor &&
                    hit->GetAtAtp() && actor->field_0xa1c[i] == 0) {
                    actor->mStates[i] = 6;
                    actor->field_0xa1c[i] = 30;
                }
                actor->mSphArr[i].ClrCoHit();
            }
        }
    }
    return HOOK_SKIP_ORIGINAL;
}


bool eligible(fopAc_ac_c* actor) {
    auto* darknut = static_cast<daB_TN_c*>(actor);
    // Scripted camera sequences contain exact integer timer events and scene
    // placement. Keep their vanilla scheduling instead of running them faster.
    if (darknut->mActionMode1 == daB_TN_c::ACT_ROOMDEMO ||
        darknut->mActionMode1 == daB_TN_c::ACT_OPENING ||
        darknut->mActionMode1 == daB_TN_c::ACT_CHANGEDEMO ||
        darknut->mActionMode1 == daB_TN_c::ACT_ENDING) {
        return false;
    }
    return darknut->mpModelMorf1 != nullptr && darknut->mpModelMorf2 != nullptr;
}


ModResult install() {
    ModResult result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_pre<PartMoveHook>(svc_hook, before_part_move);
    if (result == MOD_OK) result = mods::hook::add_pre<AttackPermissionHook>(svc_hook, before_attack_permission);
    if (result == MOD_OK) result = mods::hook::add_post<AttackPermissionHook>(svc_hook, after_attack_permission);
    return result;
}

void reset() { s_attackCooldown = 0.0f; }
}  // namespace

const EnemySlowProfile& darknut_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_TN_e, eligible, prepare, before_move,
                                          observe_execute, install, reset};
    return profile;
}
}  // namespace dawnlight
