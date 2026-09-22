#include "fierce_deity.hpp"

#include "combat_meter.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "stamina.hpp"

#include "d/actor/d_a_alink.h"
#include "d/d_cc_d.h"
#include "d/d_cc_uty.h"
#include "d/d_com_inf_game.h"
#include "d/d_item.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_object.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_leaf.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/save.h"

#include <algorithm>
#include <chrono>
#include <cstdint>

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::execute, FiercePlayerExecuteHook);
DEFINE_HOOK(&daAlink_c::checkMagicArmorWearAbility, FierceMagicArmorAbilityHook);
DEFINE_HOOK(&at_power_check, FierceAttackPowerHook);
DEFINE_HOOK(&cc_at_check, FierceDamageCheckHook);
DEFINE_HOOK(&dMeter2Draw_c::draw, FierceMeterDrawHook);
DEFINE_HOOK(&fpcLf_Delete, FiercePlayerDeleteHook);

constexpr float kMeterGainPerAttack = 5.0f;
constexpr float kMeterDrainPerSecond = 5.0f;

using Clock = std::chrono::steady_clock;

enum class ModelSwapState : u8 {
    None,
    Activating,
    RestoreRequested,
    Restoring,
};

struct RuntimeState {
    daAlink_c* link = nullptr;
    fpc_ProcID linkId = fpcM_ERROR_PROCESS_ID_e;
    float meter = 0.0f;
    bool active = false;
    bool spinChargeArmed = false;
    bool equipmentOverridden = false;
    bool modelSwapped = false;
    bool modelReloadFrame = false;
    ModelSwapState modelSwapState = ModelSwapState::None;
    u8 originalClothes = dItemNo_NONE_e;
    Clock::time_point lastDrainTime{};
};

RuntimeState s_state;
SaveObserverHandle s_saveObserver = 0;

bool is_sword_attack(const dCcU_AtInfo* attack) {
    return attack != nullptr && attack->mpCollider != nullptr &&
           attack->mHitType == HIT_TYPE_LINK_NORMAL_ATTACK &&
           attack->mpCollider->ChkAtType(AT_TYPE_NORMAL_SWORD | AT_TYPE_MASTER_SWORD);
}

bool menu_or_pause_active() {
    return dMeter2Info_getWindowStatus() != 0 || dMeter2Info_getPauseStatus() != 0 ||
           dComIfGp_isPauseFlag() || dComIfGp_event_runCheck() ||
           dMeter2Info_isShopTalkFlag() || dMsgObject_isTalkNowCheck();
}

void restore_equipment_selection() {
    if (!s_state.equipmentOverridden) {
        return;
    }
    dComIfGs_setSelectEquipClothes(s_state.originalClothes);
    dComIfGp_setSelectEquipClothes(s_state.originalClothes);
    s_state.equipmentOverridden = false;
}

void deactivate(daAlink_c* link, bool clearMeter) {
    const bool restoreModel = s_state.active ||
                              s_state.modelSwapState != ModelSwapState::None ||
                              s_state.modelSwapped;
    restore_equipment_selection();
    if (restoreModel && link != nullptr && !link->checkWolf() &&
        !link->checkSceneChangeAreaStart())
    {
        if (s_state.modelSwapState == ModelSwapState::Activating) {
            s_state.modelSwapState = ModelSwapState::RestoreRequested;
        } else if (s_state.modelSwapState == ModelSwapState::None) {
            link->setClothesChange(0);
            s_state.modelSwapState = ModelSwapState::Restoring;
        }
    } else if (restoreModel) {
        s_state.modelSwapState = ModelSwapState::None;
        s_state.modelSwapped = false;
    }
    s_state.active = false;
    s_state.spinChargeArmed = false;
    s_state.lastDrainTime = {};
    if (clearMeter) {
        s_state.meter = 0.0f;
    }
}

void reset_for_link(daAlink_c* link) {
    // The previous player may belong to another save. Never restore its clothes
    // into the current save; equipment restoration belongs to its deletion hook.
    s_state = {};
    s_state.link = link;
    s_state.linkId = link != nullptr ? fopAcM_GetID(link) : fpcM_ERROR_PROCESS_ID_e;
}

bool same_link(daAlink_c* link) {
    return link != nullptr && s_state.link == link && s_state.linkId == fopAcM_GetID(link);
}

void on_save_started(ModContext*, uint32_t, void*) {
    // SaveService runs after the new slot has been installed, including reloading
    // the same slot. Drop every transient flag without touching save equipment.
    reset_for_link(nullptr);
}

HookAction before_player_delete(ModContext*, void* args, void*, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (process == nullptr || fpcM_GetName(process) != fpcNm_ALINK_e) {
        return HOOK_CONTINUE;
    }
    auto* link = static_cast<daAlink_c*>(process);
    if (same_link(link)) {
        // Restore while the departing player's save is still current. Do not
        // start a model reload on an actor that is about to be destroyed.
        restore_equipment_selection();
        reset_for_link(nullptr);
    }
    return HOOK_CONTINUE;
}

bool can_transform(daAlink_c* link) {
    return link != nullptr && !link->checkWolf() &&
           !link->checkDeadHP() && !link->checkEventRun() && !link->checkSceneChangeAreaStart() &&
           !link->checkHorseRide() && !link->checkCanoeRide() && !link->checkBoardRide() &&
           !link->checkSpinnerRide() && link->getSumouMode() == 0 &&
           link->mClothesChangeWaitTimer == 0;
}

void activate(daAlink_c* link) {
    if (!can_transform(link)) {
        return;
    }
    s_state.active = true;
    s_state.meter = 100.0f;
    s_state.spinChargeArmed = false;
    s_state.lastDrainTime = Clock::now();
    s_state.originalClothes = dComIfGs_getSelectEquipClothes();
    s_state.equipmentOverridden = true;
    s_state.modelSwapped = false;
    s_state.modelSwapState = ModelSwapState::Activating;
    dComIfGs_setSelectEquipClothes(dItemNo_ARMOR_e);
    dComIfGp_setSelectEquipClothes(dItemNo_ARMOR_e);
    link->setClothesChange(0);
}

bool service_model_swap(daAlink_c* link) {
    s_state.modelReloadFrame = false;
    if (link == nullptr || s_state.modelSwapState == ModelSwapState::None) {
        return false;
    }

    s_state.modelReloadFrame = true;

    if (link->mClothesChangeWaitTimer != 0) {
        // loadModelDVD() normally runs at the start of execute(). Its timer-2 phase frees
        // Link's model heap, so the rest of execute() must not touch animations that frame.
        link->loadModelDVD();
    }

    if (link->mClothesChangeWaitTimer != 0) {
        return true;
    }

    if (s_state.modelSwapState == ModelSwapState::RestoreRequested) {
        link->setClothesChange(0);
        s_state.modelSwapState = ModelSwapState::Restoring;
    } else if (s_state.modelSwapState == ModelSwapState::Activating) {
        restore_equipment_selection();
        s_state.modelSwapState = ModelSwapState::None;
        s_state.modelSwapped = true;
    } else {
        s_state.modelSwapState = ModelSwapState::None;
        s_state.modelSwapped = false;
    }

    // Resume normal player execution on the next frame, after the completed model swap.
    return true;
}

void update_spin_activation(daAlink_c* link) {
    if (s_state.active || s_state.meter < 100.0f) {
        s_state.spinChargeArmed = false;
        return;
    }

    if (link->mProcID == daAlink_c::PROC_CUT_TURN_MOVE ||
        link->mProcID == daAlink_c::PROC_CUT_TURN_CHARGE)
    {
        s_state.spinChargeArmed = true;
        return;
    }

    if (s_state.spinChargeArmed && link->mProcID == daAlink_c::PROC_CUT_TURN &&
        link->getCutAtFlg())
    {
        activate(link);
        return;
    }

    if (link->mProcID != daAlink_c::PROC_CUT_TURN) {
        s_state.spinChargeArmed = false;
    }
}

void update_drain(daAlink_c* link) {
    if (!s_state.active) {
        return;
    }
    if (link->checkSceneChangeAreaStart()) {
        restore_equipment_selection();
        s_state.active = false;
        s_state.modelSwapped = false;
        s_state.modelReloadFrame = false;
        s_state.modelSwapState = ModelSwapState::None;
        s_state.spinChargeArmed = false;
        s_state.meter = 0.0f;
        s_state.lastDrainTime = {};
        return;
    }
    if (link->checkDeadHP() || link->checkWolf()) {
        deactivate(link, true);
        return;
    }

    const Clock::time_point now = Clock::now();
    if (menu_or_pause_active()) {
        s_state.lastDrainTime = now;
        return;
    }
    if (s_state.lastDrainTime.time_since_epoch().count() == 0) {
        s_state.lastDrainTime = now;
        return;
    }

    const float elapsed = std::clamp(
        std::chrono::duration<float>(now - s_state.lastDrainTime).count(), 0.0f, 0.25f);
    s_state.lastDrainTime = now;
    s_state.meter = std::max(0.0f, s_state.meter - elapsed * kMeterDrainPerSecond);
    if (s_state.meter <= 0.0f) {
        deactivate(link, true);
    }
}

HookAction before_player_execute(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!same_link(link)) {
        reset_for_link(link);
    }
    if (link == nullptr) {
        return HOOK_CONTINUE;
    }

    if (!fierce_deity_enabled()) {
        deactivate(link, true);
    }

    if (!service_model_swap(link)) {
        return HOOK_CONTINUE;
    }
    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

void after_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!same_link(link) || !fierce_deity_enabled()) {
        return;
    }
    if (!s_state.modelReloadFrame) {
        update_spin_activation(link);
    }
    update_drain(link);
}

HookAction before_magic_armor_ability(ModContext*, void*, void* retval, void*) {
    if (!same_link(daAlink_getAlinkActorClass()) ||
        (!s_state.active && s_state.modelSwapState == ModelSwapState::None)) {
        return HOOK_CONTINUE;
    }
    *static_cast<BOOL*>(retval) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

void after_attack_power_check(ModContext*, void* args, void*, void*) {
    auto* attack = mods::arg<dCcU_AtInfo*>(args, 0);
    if (!s_state.active || !same_link(daAlink_getAlinkActorClass()) ||
        !is_sword_attack(attack) || attack->mpActor != s_state.link ||
        attack->mAttackPower == 0)
    {
        return;
    }

    attack->mAttackPower = static_cast<u16>(std::min<u32>(
        static_cast<u32>(attack->mAttackPower) * 2U, 0xFFFFU));
}

void after_damage_check(ModContext*, void* args, void*, void*) {
    auto* enemy = mods::arg<fopAc_ac_c*>(args, 0);
    auto* attack = mods::arg<dCcU_AtInfo*>(args, 1);
    if (!fierce_deity_enabled() || s_state.active || enemy == nullptr || attack == nullptr ||
        !same_link(daAlink_getAlinkActorClass()) ||
        !is_sword_attack(attack) || attack->mpActor != s_state.link ||
        fopAcM_GetGroup(enemy) != fopAc_ENEMY_e)
    {
        return;
    }

    s_state.meter = std::min(100.0f, s_state.meter + kMeterGainPerAttack);
}

void draw_fierce_meter(dMeter2Draw_c* meter) {
    if (!fierce_deity_enabled() || !same_link(daAlink_getAlinkActorClass()) ||
        s_state.meter <= 0.0f || menu_or_pause_active())
    {
        return;
    }
    draw_combat_meter(meter, s_state.meter, CombatMeterStyle::FierceDeity,
        stamina_meter_visible() ? 1 : 0);
}

void after_meter_draw(ModContext*, void* args, void*, void*) {
    draw_fierce_meter(mods::arg<dMeter2Draw_c*>(args, 0));
}

template <typename Hook>
ModResult add_post(ModError* error, void (*callback)(ModContext*, void*, void*, void*),
    const char* message) {
    const ModResult result = mods::hook::add_post<Hook>(svc_hook, callback);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result, message);
}

}  // namespace

ModResult initialize_fierce_deity(ModError* error) {
    ModResult result = svc_save->observe_saves(
        mod_ctx, on_save_started, on_save_started, nullptr, nullptr, &s_saveObserver);
    if (result != MOD_OK) {
        return mods::set_error(error, result,
            "failed to observe Dawnlight Fierce Deity save lifecycle");
    }
    result = mods::hook::add_pre<FiercePlayerDeleteHook>(svc_hook, before_player_delete);
    if (result != MOD_OK) {
        return mods::set_error(error, result,
            "failed to install Dawnlight Fierce Deity player deletion hook");
    }
    result = mods::hook::add_pre<FiercePlayerExecuteHook>(svc_hook, before_player_execute);
    if (result != MOD_OK) {
        return mods::set_error(error, result,
            "failed to install Dawnlight Fierce Deity player pre-hook");
    }
    if ((result = add_post<FiercePlayerExecuteHook>(error, after_player_execute,
             "failed to install Dawnlight Fierce Deity player hook")) != MOD_OK ||
        (result = add_post<FierceAttackPowerHook>(error, after_attack_power_check,
             "failed to install Dawnlight Fierce Deity damage hook")) != MOD_OK)
    {
        return result;
    }
    result = mods::hook::add_pre<FierceMagicArmorAbilityHook>(
        svc_hook, before_magic_armor_ability);
    if (result != MOD_OK) {
        return mods::set_error(error, result,
            "failed to install Dawnlight Fierce Deity armor suppression hook");
    }
    if ((result = add_post<FierceDamageCheckHook>(error, after_damage_check,
             "failed to install Dawnlight Fierce Deity hit hook")) != MOD_OK ||
        (result = add_post<FierceMeterDrawHook>(error, after_meter_draw,
             "failed to install Dawnlight Fierce Deity meter hook")) != MOD_OK)
    {
        return result;
    }
    return MOD_OK;
}

void shutdown_fierce_deity() {
    if (s_saveObserver != 0 && svc_save != nullptr) {
        svc_save->unobserve_saves(mod_ctx, s_saveObserver);
    }
    s_saveObserver = 0;
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (same_link(link)) {
        deactivate(link, true);
    }
    s_state = {};
}

bool fierce_deity_active() {
    return s_state.active && same_link(daAlink_getAlinkActorClass());
}

bool fierce_deity_model_reload_active() {
    return s_state.modelReloadFrame && same_link(daAlink_getAlinkActorClass());
}

}  // namespace dawnlight
