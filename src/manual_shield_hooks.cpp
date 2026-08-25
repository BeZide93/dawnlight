#include "config.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::swordSwingTrigger, SwordSwingTriggerHook);
DEFINE_HOOK(&daAlink_c::setShieldGuard, SetShieldGuardHook);
DEFINE_HOOK(&daAlink_c::checkItemAction, CheckItemActionHook);
DEFINE_HOOK(&daAlink_c::procGuardAttackInit, GuardAttackInitHook);

thread_local daAlink_c* s_manualGuardAttackOwner = nullptr;

bool switch_target_active(daAlink_c* link) {
    return dComIfGs_getOptAttentionType() == 1 &&
           link != nullptr &&
           link->mAttention != nullptr &&
           link->mAttention->LockonTruth() &&
           (link->mTargetedActor != nullptr || link->mAttention->LockonTarget(0) != nullptr);
}

bool manual_shield_button(daAlink_c* link) {
    if (!manual_shielding_enabled() || link == nullptr || !mDoCPd_c::getHoldLockR(PAD_1)) {
        return false;
    }

    return mDoCPd_c::getHoldLockL(PAD_1) != 0 || switch_target_active(link);
}

bool manual_shield_attack_trigger(daAlink_c* link) {
    return manual_shield_button(link) && link->itemTriggerCheck(daAlink_c::BTN_B);
}

bool shield_action_base_context(daAlink_c* link) {
    return link != nullptr &&
           (dComIfGs_isEventBit(dSv_event_flag_c::F_0338) ||
               link->checkNoResetFlg3(daPy_py_c::FLG3_TRANING_SHIELD_ATTACK)) &&
           link->checkGuardActionChange() &&
           !link->checkUpperReadyThrowAnime() &&
           !link->checkModeFlg(0x70C52) &&
           daPy_py_c::checkShieldGet() &&
           !daAlink_c::checkNotBattleStage() &&
           (link->mLinkAcch.ChkGroundHit() || link->checkMagneBootsOn());
}

bool shield_action_context(daAlink_c* link) {
    return shield_action_base_context(link) &&
           dComIfGp_getRStatus() == BUTTON_STATUS_NONE;
}

HookAction before_sword_swing_trigger(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!manual_shield_button(link)) {
        return HOOK_CONTINUE;
    }

    *static_cast<BOOL*>(retval) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

void after_set_shield_guard(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!manual_shielding_enabled() || link == nullptr) {
        return;
    }

    if (manual_shield_button(link)) {
        link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
        if (shield_action_base_context(link)) {
            link->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);
        }
    } else if (!link->checkSmallUpperGuardAnime()) {
        link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    }
}

HookAction before_guard_attack_init(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!manual_shielding_enabled() || s_manualGuardAttackOwner == link) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_check_item_action(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!manual_shielding_enabled() || link == nullptr || retval == nullptr) {
        return HOOK_CONTINUE;
    }
    if (!manual_shield_button(link) || !shield_action_base_context(link)) {
        return HOOK_CONTINUE;
    }

    link->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);
    if (!manual_shield_attack_trigger(link)) {
        *static_cast<BOOL*>(retval) = FALSE;
        return HOOK_SKIP_ORIGINAL;
    }

    s_manualGuardAttackOwner = link;
    *static_cast<BOOL*>(retval) = link->procGuardAttackInit();
    s_manualGuardAttackOwner = nullptr;
    return HOOK_SKIP_ORIGINAL;
}

void after_check_item_action(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto* result = static_cast<BOOL*>(retval);
    if (!manual_shielding_enabled() || link == nullptr || result == nullptr || *result) {
        return;
    }
    if (!shield_action_context(link)) {
        return;
    }

    if (manual_shield_button(link)) {
        link->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);
    }
    if (!manual_shield_attack_trigger(link)) {
        return;
    }

    s_manualGuardAttackOwner = link;
    *result = link->procGuardAttackInit();
    s_manualGuardAttackOwner = nullptr;
}

}  // namespace

ModResult install_manual_shield_hooks(ModError* error) {
    ModResult result =
        mods::hook_add_pre<SwordSwingTriggerHook>(svc_hook, before_sword_swing_trigger);
    if (result == MOD_OK) {
        result = mods::hook_add_post<SetShieldGuardHook>(svc_hook, after_set_shield_guard);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<GuardAttackInitHook>(svc_hook, before_guard_attack_init);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CheckItemActionHook>(svc_hook, before_check_item_action);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<CheckItemActionHook>(svc_hook, after_check_item_action);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight manual shielding hooks");
    }
    return MOD_OK;
}

}  // namespace dawnlight
