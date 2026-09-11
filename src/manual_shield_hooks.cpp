#include "config.hpp"
#include "service_imports.hpp"

#include <array>

#include "global.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_button.h"
#include "d/d_pane_class.h"
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
DEFINE_HOOK(&dMeterButton_c::draw, MeterButtonDrawHook);

thread_local daAlink_c* s_manualGuardAttackOwner = nullptr;

struct AttackPromptTextureState {
    J2DPicture* picture = nullptr;
    ResTIMG const* texture = nullptr;
    JUtility::TColor black{};
    JUtility::TColor white{};
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;
};

struct AttackPromptLayerState {
    J2DPane* pane = nullptr;
    bool visible = false;
};

thread_local AttackPromptTextureState s_attackPromptTextureState;
thread_local std::array<AttackPromptLayerState, 8> s_attackPromptLayerStates{};
thread_local std::size_t s_attackPromptLayerStateCount = 0;

J2DPicture* picture_for(J2DScreen* screen, u64 tag) {
    J2DPane* pane = screen != nullptr ? screen->search(tag) : nullptr;
    return pane != nullptr && pane->getTypeID() == 18 ? static_cast<J2DPicture*>(pane) : nullptr;
}

ResTIMG const* picture_texture(J2DPicture* picture) {
    JUTTexture* texture = picture != nullptr ? picture->getTexture(0) : nullptr;
    return texture != nullptr ? texture->getTexInfo() : nullptr;
}

void restore_attack_prompt_texture() {
    auto& state = s_attackPromptTextureState;
    if (state.picture != nullptr && state.texture != nullptr) {
        state.picture->changeTexture(state.texture, 0);
        state.picture->resize(state.width, state.height);
        state.picture->move(state.x, state.y);
        state.picture->setTexCoord(state.picture->getTexture(0), BIND15, MIRROR0, false);
        state.picture->setBlackWhite(state.black, state.white);
    }
    state = {};

    for (std::size_t index = 0; index < s_attackPromptLayerStateCount; ++index) {
        auto& layer = s_attackPromptLayerStates[index];
        if (layer.pane != nullptr) {
            layer.visible ? layer.pane->show() : layer.pane->hide();
        }
        layer = {};
    }
    s_attackPromptLayerStateCount = 0;
}

void suppress_attack_prompt_layers(J2DPane* pane, J2DPicture* buttonBase) {
    if (pane == nullptr) {
        return;
    }

    if (pane != buttonBase && pane->getTypeID() == 18 &&
        s_attackPromptLayerStateCount < s_attackPromptLayerStates.size())
    {
        s_attackPromptLayerStates[s_attackPromptLayerStateCount++] = {
            pane,
            pane->isVisible(),
        };
        pane->hide();
    }

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        suppress_attack_prompt_layers(child, buttonBase);
    }
}

bool dawnlight_attack_prompt_active(dMeterButton_c* buttons) {
    const u8 status = dComIfGp_getAStatus();
    if (status != BUTTON_STATUS_UNK_129 &&
        (!manual_shielding_enabled() || status != BUTTON_STATUS_SHIELD_ATTACK))
    {
        return false;
    }

    return buttons != nullptr &&
           (buttons->field_0x4be[0] == dMeterButton_c::BUTTON_B_e ||
               buttons->field_0x4be[1] == dMeterButton_c::BUTTON_B_e);
}

void apply_twilight_hd_attack_prompt(dMeterButton_c* buttons) {
    restore_attack_prompt_texture();
    if (!dawnlight_attack_prompt_active(buttons) || buttons->mpButtonScreen == nullptr ||
        buttons->mpButtonB == nullptr)
    {
        return;
    }

    dMeter2_c* meter = dMeter2Info_getMeterClass();
    dMeter2Draw_c* draw = meter != nullptr ? meter->getMeterDrawPtr() : nullptr;
    J2DScreen* hudScreen = draw != nullptr ? draw->getMainScreenPtr() : nullptr;
    J2DPicture* contextA = picture_for(buttons->mpButtonScreen, MULTI_CHAR('a_btn1'));
    J2DPicture* contextB = picture_for(buttons->mpButtonScreen, MULTI_CHAR('b_btn'));
    J2DPicture* hudA = picture_for(hudScreen, MULTI_CHAR('a_btn'));
    J2DPicture* hudB = picture_for(hudScreen, MULTI_CHAR('b_btn'));
    ResTIMG const* contextATexture = picture_texture(contextA);
    ResTIMG const* hudATexture = picture_texture(hudA);
    ResTIMG const* hudBTexture = picture_texture(hudB);

    // Twilight HD applies the same selected A texture to both HUD layouts, but leaves the
    // contextual B pane untouched. Matching those live pointers detects it without a mod API.
    if (contextATexture == nullptr || contextATexture != hudATexture ||
        contextB == nullptr || hudBTexture == nullptr ||
        picture_texture(contextB) == hudBTexture)
    {
        return;
    }

    const auto bounds = contextB->getBounds();
    s_attackPromptTextureState = {
        contextB,
        picture_texture(contextB),
        contextB->getBlack(),
        contextB->getWhite(),
        bounds.i.x,
        bounds.i.y,
        bounds.getWidth(),
        bounds.getHeight(),
    };
    contextB->changeTexture(hudBTexture, 0);
    contextB->resize(bounds.getWidth(), bounds.getHeight());
    contextB->move(bounds.i.x, bounds.i.y);
    contextB->setTexCoord(contextB->getTexture(0), BIND15, MIRROR0, false);
    contextB->setBlackWhite(
        JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
    suppress_attack_prompt_layers(buttons->mpButtonB->getPanePtr(), contextB);
}

bool switch_target_active(daAlink_c* link) {
    return dComIfGs_getOptAttentionType() == 1 &&
           link != nullptr &&
           link->mAttention != nullptr &&
           link->mAttention->LockonTruth() &&
           (link->mTargetedActor != nullptr || link->mAttention->LockonTarget(0) != nullptr);
}

bool manual_shield_button(daAlink_c* link) {
    if (!manual_shielding_enabled() || link == nullptr || link->checkWolf() ||
        !mDoCPd_c::getHoldLockR(PAD_1)) {
        return false;
    }

    return mDoCPd_c::getHoldLockL(PAD_1) != 0 || switch_target_active(link);
}

bool manual_shield_attack_trigger(daAlink_c* link) {
    return manual_shield_button(link) && link->itemTriggerCheck(daAlink_c::BTN_B);
}

bool shield_action_base_context(daAlink_c* link) {
    return link != nullptr && !link->checkWolf() &&
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
    if (!manual_shielding_enabled() || link == nullptr || link->checkWolf()) {
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
    // Guard attack initializes human animations and shield collision data.
    if (link == nullptr || link->checkWolf()) {
        *static_cast<int*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
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

HookAction before_meter_button_draw(ModContext*, void* args, void*, void*) {
    apply_twilight_hd_attack_prompt(mods::arg<dMeterButton_c*>(args, 0));
    return HOOK_CONTINUE;
}

void after_meter_button_draw(ModContext*, void*, void*, void*) {
    restore_attack_prompt_texture();
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
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterButtonDrawHook>(svc_hook, before_meter_button_draw);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterButtonDrawHook>(svc_hook, after_meter_button_draw);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight manual shielding hooks");
    }
    return MOD_OK;
}

}  // namespace dawnlight
