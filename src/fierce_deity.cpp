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
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"

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

constexpr float kMeterGainPerAttack = 5.0f;
constexpr float kMeterDrainPerSecond = 5.0f;

using Clock = std::chrono::steady_clock;

struct RuntimeState {
    daAlink_c* link = nullptr;
    u16 linkId = 0;
    float meter = 0.0f;
    bool active = false;
    bool spinChargeArmed = false;
    bool equipmentOverridden = false;
    bool modelSwapPending = false;
    bool modelSwapped = false;
    u8 originalClothes = dItemNo_NONE_e;
    Clock::time_point lastDrainTime{};
};

RuntimeState s_state;

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
    const bool restoreModel = s_state.active || s_state.modelSwapPending || s_state.modelSwapped;
    restore_equipment_selection();
    if (restoreModel && link != nullptr && !link->checkWolf() &&
        !link->checkSceneChangeAreaStart())
    {
        link->setClothesChange(0);
    }
    s_state.active = false;
    s_state.modelSwapPending = false;
    s_state.modelSwapped = false;
    s_state.spinChargeArmed = false;
    s_state.lastDrainTime = {};
    if (clearMeter) {
        s_state.meter = 0.0f;
    }
}

void reset_for_link(daAlink_c* link) {
    restore_equipment_selection();
    s_state.link = link;
    s_state.linkId = link != nullptr ? link->setID : 0;
    s_state.meter = 0.0f;
    s_state.active = false;
    s_state.spinChargeArmed = false;
    s_state.modelSwapPending = false;
    s_state.modelSwapped = false;
    s_state.lastDrainTime = {};
}

bool same_link(daAlink_c* link) {
    return link != nullptr && s_state.link == link && s_state.linkId == link->setID;
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
    s_state.modelSwapPending = true;
    s_state.modelSwapped = false;
    dComIfGs_setSelectEquipClothes(dItemNo_ARMOR_e);
    dComIfGp_setSelectEquipClothes(dItemNo_ARMOR_e);
    link->setClothesChange(0);
}

void finish_model_swap(daAlink_c* link) {
    if (!s_state.modelSwapPending || link == nullptr || link->mClothesChangeWaitTimer != 0) {
        return;
    }
    restore_equipment_selection();
    s_state.modelSwapPending = false;
    s_state.modelSwapped = true;
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
        s_state.modelSwapPending = false;
        s_state.modelSwapped = false;
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

HookAction before_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!same_link(link)) {
        reset_for_link(link);
    }
    if (link == nullptr) {
        return HOOK_CONTINUE;
    }

    if (!fierce_deity_enabled()) {
        deactivate(link, true);
        return HOOK_CONTINUE;
    }

    return HOOK_CONTINUE;
}

void after_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!same_link(link) || !fierce_deity_enabled()) {
        return;
    }
    finish_model_swap(link);
    update_spin_activation(link);
    update_drain(link);
}

HookAction before_magic_armor_ability(ModContext*, void*, void* retval, void*) {
    if (!s_state.active && !s_state.modelSwapPending) {
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
    if (!fierce_deity_enabled() || s_state.meter <= 0.0f || menu_or_pause_active())
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
    ModResult result = mods::hook::add_pre<FiercePlayerExecuteHook>(svc_hook, before_player_execute);
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
    daAlink_c* link = daAlink_getAlinkActorClass();
    deactivate(same_link(link) ? link : nullptr, true);
    s_state = {};
}

bool fierce_deity_active() {
    return s_state.active;
}

}  // namespace dawnlight
