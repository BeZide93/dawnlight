#include "bullet_time.hpp"
#include "config.hpp"
#include "gale_counter.hpp"
#include "glider_visual.hpp"
#include "glider_bmd.hpp"
#include "service_imports.hpp"
#include "stamina.hpp"

#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_ni.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <cmath>
#include "enemy_spawner.hpp"
#include "f_pc/f_pc_create_req.h"
#include "f_pc/f_pc_leaf.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "res/Object/AlAnm.h"

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::checkAutoJumpAction, CheckAutoJumpAction);
DEFINE_HOOK(&daAlink_c::procAutoJump, ProcAutoJump);
DEFINE_HOOK(&daAlink_c::procMove, ProcMoveSprint);
DEFINE_HOOK(&daAlink_c::getMainBckData, GetMainBckDataSprint);
DEFINE_HOOK(&daAlink_c::setDoubleAnime, SetDoubleAnimeSprint);
DEFINE_HOOK(&daAlink_c::execute, JumpAbilitiesExecute);
DEFINE_HOOK_SYMBOL("dusk::processGameCombos", void(), JumpGameCombos);
DEFINE_HOOK(&daAlink_c::draw, JumpAbilitiesDraw);
DEFINE_HOOK(&daAlink_c::setDrawHand, GliderDrawHand);
DEFINE_HOOK(&fpcLf_Delete, JumpAbilitiesDelete);
DEFINE_HOOK(&fpcLf_Draw, GlideCarrierDraw);
#if defined(__APPLE__)
DEFINE_HOOK(&fpcMtd_Method, GlideCarrierExecute);
#else
DEFINE_HOOK(&fpcMtd_Execute, GlideCarrierExecute);
#endif
// MSVC cannot const-initialize metadata from these virtual member pointers.
// Resolve the concrete implementations by name (receiver first in the ABI).
DEFINE_HOOK_SYMBOL("Z2SoundObjSimple::startSound",
    Z2SoundHandlePool*(Z2SoundObjSimple*, JAISoundID, u32, s8), GlideCarrierSound);
DEFINE_HOOK_SYMBOL("Z2SoundObjSimple::startLevelSound",
    Z2SoundHandlePool*(Z2SoundObjSimple*, JAISoundID, u32, s8), GlideCarrierLevelSound);
DEFINE_HOOK(&daAlink_c::commonProcInit, CommonProcInit);
DEFINE_HOOK(&daAlink_c::setBodyAngleXReadyAnime, SetBodyAngleXReadyAnime);

enum class JumpBinding {
    LockR,
};

constexpr u16 kSwordItem = 0x103;

const daAlink_c* s_manualJumpOwner = nullptr;
daAlink_c* s_slowSpeedOwner = nullptr;
float s_previousNormalSpeed = 0.0f;
daAlink_c* s_sprintOwner = nullptr;
bool s_galeInputCancelled = false;
bool s_galeChargeCancelledThisTick = false;

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

bool sprint_requested(daAlink_c* link) {
    if (link == nullptr || !sprint_enabled() || !link->doButton()) {
        return false;
    }

    const bool canSprint = link->mProcID == daAlink_c::PROC_MOVE &&
           !link->doTrigger() && link->checkInputOnR() &&
           link->mLinkAcch.ChkGroundHit() && !link->checkWolf() &&
           !link->checkEventRun() && !dComIfGp_event_runCheck() &&
           !link->checkMagneBootsOn() && !link->getSumouMode() &&
           link->mGrabItemAcKeep.getActor() == nullptr;
    if (!canSprint) {
        return false;
    }
    return canSprint && stamina_available_for_sprint();
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

bool ground_jump_context_ready(daAlink_c* link) {
    if (link == nullptr) {
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

bool jump_state_ready(daAlink_c* link) {
    return !s_galeInputCancelled && (r_jump_enabled() || revalis_gale_enabled()) &&
        jump_pressed(active_jump_binding()) &&
        ground_jump_context_ready(link);
}

void apply_manual_jump_height(daAlink_c* link, float heightMultiplier) {
    // Height is proportional to launch velocity squared, not velocity itself.
    link->speed.y *= std::sqrt(heightMultiplier);
}

void set_manual_jump_direction(daAlink_c* link) {
    if (link->checkInputOnR()) {
        link->shape_angle.y = link->mMoveAngle;
    }
    link->current.angle.y = link->shape_angle.y;
}

float sprint_jump_speed_multiplier(daAlink_c* link) {
    if (!sprint_requested(link) || link->mpHIO == nullptr) {
        return 1.0f;
    }
    const f32 runSpeed = link->mpHIO->mMove.m.mMaxSpeed;
    if (runSpeed <= 0.0f) {
        return 1.0f;
    }
    // Carry only the sprint bonus already reached on the ground. Holding Roll
    // at a standstill must not grant a full-speed launch; native indoor limits
    // and acceleration still matter. Snapshot before jump init replaces speed.
    return std::clamp(link->mNormalSpeed / runSpeed, 1.0f, sprint_speed_multiplier());
}

void apply_sprint_jump_speed(daAlink_c* link, const float multiplier) {
    // The native initializer has already calculated vertical speed/gravity.
    // Scale horizontal motion once, then leave airborne steering and collision
    // to the existing jump procedure (including Bullet Time).
    link->mNormalSpeed *= multiplier;
    link->speedF *= multiplier;
    link->mMaxSpeed *= multiplier;
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

    const float sprintJumpMultiplier = sprint_jump_speed_multiplier(link);
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
        apply_sprint_jump_speed(link, sprintJumpMultiplier);
        apply_manual_jump_height(link, jump_height_multiplier());
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

#include "jump_abilities.inc"

HookAction before_check_auto_jump(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (handle_jump_abilities(link)) {
        *static_cast<BOOL*>(retval) = TRUE;
        return HOOK_SKIP_ORIGINAL;
    }
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

HookAction before_proc_move_sprint(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_sprintOwner = nullptr;
    if (!sprint_requested(link)) {
        return HOOK_CONTINUE;
    }

    link->mMaxSpeed *= sprint_speed_multiplier();
    s_sprintOwner = link;
    mark_sprint_stamina_active();
    return HOOK_CONTINUE;
}

void after_proc_move_sprint(ModContext*, void*, void*, void*) {
    // Animation substitutions belong only to this movement update. Do not
    // leave the owner active for later actions or the next actor/frame.
    s_sprintOwner = nullptr;
}

bool sprint_animation_active(const daAlink_c* link) {
    return link != nullptr && s_sprintOwner == link && link->mProcID == daAlink_c::PROC_MOVE;
}

HookAction before_set_double_anime_sprint(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!sprint_animation_active(link) || link->mMaxSpeed <= 0.0f) {
        return HOOK_CONTINUE;
    }

    // checkNextAction consumes the boosted limit for acceleration, then resets
    // mMaxSpeed to the native movement limit before building the walk/run blend.
    // Use its ground-adjusted speed ratio so acceleration, analog input, slopes
    // and indoor limits affect cadence instead of immediately playing at the cap.
    // Add half the earned speed bonus to playback: 150% movement -> 125%
    // cadence. Indoor slowdown is already in the measured ratio, so do not
    // apply a second room multiplier.
    const f32 speedBonus = std::max(0.0f, link->getMoveGroundAngleSpeedRate() - 1.0f);
    const f32 cadence = 1.0f + 0.5f * speedBonus;
    for (int layer = 0; layer < 2; ++layer) {
        const auto animation = mods::arg<daAlink_c::daAlink_ANM>(args, 4 + layer);
        if (animation == daAlink_c::ANM_RUN || animation == daAlink_c::ANM_RUN_B) {
            mods::arg_ref<f32>(args, 2 + layer) *= cadence;
        }
    }
    // Let the native blend synchronize upper/lower animation and footstep
    // timing. Adjust fresh arguments, never multiply persistent frame rates.
    return HOOK_CONTINUE;
}

HookAction before_get_main_bck_data_sprint(ModContext*, void* args, void*, void*) {
    const auto* link = mods::arg<const daAlink_c*>(args, 0);
    auto& animation = mods::arg_ref<daAlink_c::daAlink_ANM>(args, 1);
    if (sprint_animation_active(link) && animation == daAlink_c::ANM_RUN) {
        animation = daAlink_c::ANM_RUN_B;
    }
    return HOOK_CONTINUE;
}

HookAction before_common_proc_init(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const auto nextProc = mods::arg<daAlink_c::daAlink_PROC>(args, 1);
    jump_abilities_proc_change(link, nextProc);
    if (s_sprintOwner == link && nextProc != daAlink_c::PROC_MOVE) {
        s_sprintOwner = nullptr;
    }
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

bool gale_shortcut_priority_active(const daAlink_c* link) {
    return link != nullptr && link == s_jumpAbilities.owner &&
        s_galeChargeCancelledThisTick;
}

ModResult install_jump_hooks(ModError* error) {
    init_glider_visual();
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
        result = mods::hook_add_pre<ProcMoveSprint>(svc_hook, before_proc_move_sprint);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<ProcMoveSprint>(svc_hook, after_proc_move_sprint);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetDoubleAnimeSprint>(
            svc_hook, before_set_double_anime_sprint);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<GetMainBckDataSprint>(
            svc_hook, before_get_main_bck_data_sprint);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetBodyAngleXReadyAnime>(
            svc_hook, before_set_body_angle_x_ready_anime);
    }
    if (result == MOD_OK) result = mods::hook_add_pre<JumpGameCombos>(svc_hook, before_jump_game_combos);
    if (result == MOD_OK) result = mods::hook_add_pre<JumpAbilitiesExecute>(svc_hook, before_jump_abilities_execute);
    if (result == MOD_OK) result = mods::hook_add_post<JumpAbilitiesExecute>(svc_hook, after_jump_abilities_execute);
    if (result == MOD_OK) result = mods::hook_add_post<JumpAbilitiesDraw>(svc_hook, after_jump_abilities_draw);
    if (result == MOD_OK) result = mods::hook_add_post<GliderDrawHand>(svc_hook, after_glider_draw_hand);
    if (result == MOD_OK) result = mods::hook_add_pre<JumpAbilitiesDelete>(svc_hook, before_jump_abilities_delete);
    if (result == MOD_OK) result = mods::hook_add_post<GlideCarrierExecute>(svc_hook, after_glide_carrier_execute);
    if (result == MOD_OK) result = mods::hook_add_pre<GlideCarrierDraw>(svc_hook, before_glide_carrier_draw);
    if (result == MOD_OK) result = mods::hook_add_pre<GlideCarrierSound>(svc_hook, before_glide_carrier_sound);
    if (result == MOD_OK) result = mods::hook_add_pre<GlideCarrierLevelSound>(svc_hook, before_glide_carrier_sound);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight R jump hooks");
    }
    return MOD_OK;
}

void shutdown_jump_hooks() {
    reset_jump_abilities(daAlink_getAlinkActorClass());
    shutdown_glider_bmd();
}

}  // namespace dawnlight
