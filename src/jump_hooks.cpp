#include "bullet_time.hpp"
#include "config.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::checkAutoJumpAction, CheckAutoJumpAction);
DEFINE_HOOK(&daAlink_c::procAutoJump, ProcAutoJump);
DEFINE_HOOK(&daAlink_c::commonProcInit, CommonProcInit);
DEFINE_HOOK(&daAlink_c::setBodyAngleXReadyAnime, SetBodyAngleXReadyAnime);

enum class JumpBinding {
    LockR,
};

constexpr u16 kSwordItem = 0x103;

const daAlink_c* s_manualJumpOwner = nullptr;
daAlink_c* s_slowSpeedOwner = nullptr;
float s_previousNormalSpeed = 0.0f;

JumpBinding active_jump_binding() {
    return JumpBinding::LockR;
}

bool jump_pressed(JumpBinding binding) {
    switch (binding) {
    case JumpBinding::LockR:
        return mDoCPd_c::getTrigLockR(PAD_1) != 0;
    }
    return false;
}

bool jump_held(JumpBinding binding) {
    switch (binding) {
    case JumpBinding::LockR:
        return mDoCPd_c::getHoldLockR(PAD_1) != 0;
    }
    return false;
}

bool switch_target_active(daAlink_c* link) {
    return dComIfGs_getOptAttentionType() == 1 &&
           link->mAttention != nullptr &&
           link->mAttention->LockonTruth() &&
           (link->mTargetedActor != nullptr ||
               link->mAttention->LockonTarget(0) != nullptr);
}

bool target_or_shield_context_active(daAlink_c* link) {
    if (link->mAttention != nullptr && link->checkAttentionLock()) {
        return true;
    }
    if (link->mTargetedActor != nullptr) {
        return true;
    }
    if (manual_shielding_enabled() &&
        (mDoCPd_c::getHoldLockL(PAD_1) != 0 || switch_target_active(link)))
    {
        return true;
    }
    return false;
}

bool chain_context_active(daAlink_c* link) {
    if (link->checkFmChainGrabAnime()) {
        return true;
    }

    const u8 previousChainSlot = link->field_0x2fa3;
    const bool available = link->searchFmChainPos() != 0;
    link->field_0x2fa3 = previousChainSlot;
    return available;
}

bool status_blocks_r_jump(u8 status) {
    switch (status) {
    case BUTTON_STATUS_ENTER:
    case BUTTON_STATUS_GRAB:
    case BUTTON_STATUS_PULL_DOWN:
    case BUTTON_STATUS_PUSH:
    case BUTTON_STATUS_RESIST:
    case BUTTON_STATUS_STRIKE:
    case BUTTON_STATUS_PULL:
    case BUTTON_STATUS_HOLD_ON:
    case BUTTON_STATUS_UNK_123:
    case BUTTON_STATUS_UNK_150:
    case BUTTON_STATUS_UNK_153:
        return true;
    default:
        return false;
    }
}

bool action_prompt_context_active() {
    return status_blocks_r_jump(dComIfGp_getDoStatus()) ||
           status_blocks_r_jump(dComIfGp_getDoStatusForce());
}

bool front_wall_context_active(daAlink_c* link) {
    link->setFrontWallType();
    return link->checkResetFlg0(daPy_py_c::RFLG0_UNK_8) != 0;
}

bool r_action_context_active(daAlink_c* link) {
    if (dComIfGp_getRStatus() != BUTTON_STATUS_NONE ||
        dComIfGp_getRStatusForce() != BUTTON_STATUS_NONE ||
        action_prompt_context_active() ||
        front_wall_context_active(link))
    {
        return true;
    }
    return target_or_shield_context_active(link) || chain_context_active(link);
}

bool jump_state_ready(daAlink_c* link) {
    if (link == nullptr || !r_jump_enabled() || !jump_pressed(active_jump_binding())) {
        return false;
    }

    switch (link->mProcID) {
    case daAlink_c::PROC_WAIT:
    case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE:
    case daAlink_c::PROC_ATN_ACTOR_WAIT:
    case daAlink_c::PROC_ATN_ACTOR_MOVE:
    case daAlink_c::PROC_WAIT_TURN:
    case daAlink_c::PROC_MOVE_TURN:
        break;
    default:
        return false;
    }

    return !link->checkWolf()
        && link->mGndPolyAtt1 != 0xFF
        && !link->checkFlyAtnWait()
        && !link->checkModeFlg(0x70C12)
        && link->mProcID != daAlink_c::PROC_DOOR_OPEN
        && link->mProcID != daAlink_c::PROC_WARP
        && !link->getSumouMode()
        && !link->checkPlayerDemoMode()
        && !link->checkEventRun()
        && !dComIfGp_event_runCheck()
        && !link->checkMagneBootsFly()
        && !link->checkMagneBootsOn()
        && !link->checkNotJumpSinkLimit()
        && !link->checkGrabAnime()
        && link->mGrabItemAcKeep.getActor() == nullptr
        && link->mLinkAcch.ChkGroundHit()
        && !r_action_context_active(link);
}

void set_manual_jump_direction(daAlink_c* link) {
    if (link->checkInputOnR()) {
        link->shape_angle.y = link->mMoveAngle;
    }
    link->current.angle.y = link->shape_angle.y;
}

void apply_manual_jump_movement(daAlink_c* link) {
    if (!link->checkInputOnR()) {
        link->speedF = 0.0f;
        link->mNormalSpeed = 0.0f;
    }

    link->mLinkAcch.ClrGroundHit();
    link->setJumpMode();
}

bool start_ground_jump(daAlink_c* link) {
    if (!jump_state_ready(link)) {
        return false;
    }

    set_manual_jump_direction(link);

    if (link->mEquipItem == kSwordItem &&
        (mDoCPd_c::getHoldB(PAD_1) || mDoCPd_c::getTrigB(PAD_1)))
    {
        if (link->procCutJumpInit(FALSE)) {
            apply_manual_jump_movement(link);
            s_manualJumpOwner = nullptr;
            clear_manual_jump(link);
            return true;
        }
        return false;
    }

    if (link->procAutoJumpInit(1)) {
        apply_manual_jump_movement(link);
        s_manualJumpOwner = link;
        mark_manual_jump_started(link);
        return true;
    }
    return false;
}

bool start_air_jump_attack(daAlink_c* link) {
    if (!r_jump_enabled() || s_manualJumpOwner != link ||
        !jump_held(active_jump_binding()) ||
        !mDoCPd_c::getTrigB(PAD_1) ||
        link->mEquipItem != kSwordItem)
    {
        return false;
    }

    set_manual_jump_direction(link);
    if (!link->procCutJumpInit(TRUE)) {
        return false;
    }
    apply_manual_jump_movement(link);
    s_manualJumpOwner = nullptr;
    clear_manual_jump(link);
    return true;
}

HookAction before_check_auto_jump(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!start_ground_jump(link)) {
        return HOOK_CONTINUE;
    }

    *static_cast<BOOL*>(retval) = TRUE;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_proc_auto_jump(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_slowSpeedOwner = nullptr;
    if (!start_air_jump_attack(link)) {
        update_bullet_time_before_jump(link);
        if (bullet_time_active_for(link)) {
            s_slowSpeedOwner = link;
            s_previousNormalSpeed = link->mNormalSpeed;
        }
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

void after_proc_auto_jump(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (s_slowSpeedOwner == link) {
        slow_bullet_time_jump_speed_change(link, s_previousNormalSpeed);
    }
    s_slowSpeedOwner = nullptr;
    update_bullet_time_after_jump(link);
}

HookAction before_common_proc_init(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const auto nextProc = mods::arg<daAlink_c::daAlink_PROC>(args, 1);
    if (s_manualJumpOwner == link && nextProc != daAlink_c::PROC_AUTO_JUMP) {
        s_manualJumpOwner = nullptr;
        clear_manual_jump(link);
    }
    return HOOK_CONTINUE;
}

HookAction before_set_body_angle_x_ready_anime(ModContext*, void* args, void*, void*) {
    return bullet_time_active_for(mods::arg<daAlink_c*>(args, 0)) ? HOOK_SKIP_ORIGINAL :
                                                                   HOOK_CONTINUE;
}

}  // namespace

ModResult install_jump_hooks(ModError* error) {
    ModResult result =
        mods::hook_add_pre<CheckAutoJumpAction>(svc_hook, before_check_auto_jump);
    if (result == MOD_OK) {
        result = mods::hook_add_pre<ProcAutoJump>(svc_hook, before_proc_auto_jump);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CommonProcInit>(svc_hook, before_common_proc_init);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<ProcAutoJump>(svc_hook, after_proc_auto_jump);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetBodyAngleXReadyAnime>(
            svc_hook, before_set_body_angle_x_ready_anime);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight R jump hooks");
    }
    return MOD_OK;
}

}  // namespace dawnlight
