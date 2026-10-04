#include "fierce_deity.hpp"

#include "combat_meter.hpp"
#include "fierce_deity_hud.hpp"
#include "kh2_hud_compat.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "stamina.hpp"
#include "touch_buttons.hpp"

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
#include "f_pc/f_pc_method.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_mtx.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/save.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>

namespace dawnlight {
namespace {

// Use the actor dispatcher: the member execute entry is not reached by every
// gameplay path/build. Match Link's innermost method table so nested dispatch
// does not tick the meter or advance a model reload more than once per frame.
// Apple builds inline fpcMtd_Execute; use the same fallback as bullet_time.cpp.
#if defined(__APPLE__)
DEFINE_HOOK(&fpcMtd_Method, FiercePlayerExecuteHook);
#else
DEFINE_HOOK(&fpcMtd_Execute, FiercePlayerExecuteHook);
#endif
DEFINE_HOOK(&daAlink_c::checkMagicArmorWearAbility, FierceMagicArmorAbilityHook);
DEFINE_HOOK(&at_power_check, FierceAttackPowerHook);
DEFINE_HOOK(&cc_at_check, FierceDamageCheckHook);
DEFINE_HOOK(&dMeter2Draw_c::draw, FierceMeterDrawHook);
DEFINE_HOOK(&fpcLf_Delete, FiercePlayerDeleteHook);
DEFINE_HOOK_SYMBOL("dusk::processGameCombos", void(), FierceGameCombosHook);
DEFINE_HOOK(&daAlink_c::midnaTalkTrigger, FierceMidnaTriggerHook);

constexpr float kMeterGainPerAttack = 5.0f;
constexpr float kMeterDrainPerSecond = 5.0f;

using Clock = std::chrono::steady_clock;

enum class ModelSwapState : u8 {
    None,
    Activating,
    Restoring,
};

struct RuntimeState {
    daAlink_c* link = nullptr;
    fpc_ProcID linkId = fpcM_ERROR_PROCESS_ID_e;
    float meter = 0.0f;
    bool active = false;
    FierceDeityVisual visual = FierceDeityVisual::MagicArmor;
    bool spinChargeArmed = false;
    bool activationInputConsumed = false;
    u32 consumedPartner = 0;
    u32 sticksHeld = 0;
    bool sticksConnected = false;
    u32 touchHeld = 0;
    u32 touchPressed = 0;
    bool refreshFootBaseline = false;
    bool equipmentOverridden = false;
    bool modelSwapped = false;
    FierceDeityTint displayedTint = FierceDeityTint::None;
    FierceDeityTint swapTint = FierceDeityTint::None;
    bool modelReloadFrame = false;
    ModelSwapState modelSwapState = ModelSwapState::None;
    u8 originalClothes = dItemNo_NONE_e;
    const char* failedArchive = nullptr;
    Clock::time_point lastDrainTime{};
};

RuntimeState s_state;
SaveObserverHandle s_saveObserver = 0;

// This reference outlives the player when a save/scene change interrupts I/O.
// Never destroy an archive while its DVD command is still running.
struct OutfitPreload {
    const char* archive = nullptr;
    int status = 1;
    bool cancelled = false;
};
OutfitPreload s_preload;

bool same_archive(const char* a, const char* b) {
    return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}

void release_preload() {
    if (s_preload.archive != nullptr) {
        dComIfG_deleteObjectResMain(s_preload.archive);
        s_preload = {};
    }
}

void poll_preload() {
    if (s_preload.archive == nullptr) return;
    if (s_preload.status > 0) {
        s_preload.status = dComIfG_syncObjectRes(s_preload.archive);
    }
    if (s_preload.cancelled && s_preload.status <= 0) release_preload();
}

void cancel_preload() {
    s_preload.cancelled = true;
    poll_preload();
}

const char* outfit_archive(u8 clothes) {
    if (clothes == dItemNo_WEAR_CASUAL_e) return "Bmdl";
    if (clothes == dItemNo_WEAR_ZORA_e) return "Zmdl";
    if (clothes == dItemNo_ARMOR_e) return "Mmdl";
    return "Kmdl";
}

bool prepare_outfit(const char* archive) {
    if (s_preload.archive != nullptr && !same_archive(s_preload.archive, archive)) {
        cancel_preload();
        if (s_preload.archive != nullptr) return false;
    }
    if (same_archive(s_state.failedArchive, archive)) return false;
    s_state.failedArchive = nullptr;
    if (s_preload.archive == nullptr) {
        // Use the resource manager's game heap, NOT Link's live archive heap:
        // loadModelDVD frees that heap before installing the replacement.
        if (!dComIfG_setObjectRes(archive, u8{0}, nullptr)) {
            s_state.failedArchive = archive;
            return false;
        }
        s_preload = {archive, 1, false};
    }
    s_preload.cancelled = false;
    poll_preload();
    if (s_preload.status < 0) {
        s_state.failedArchive = archive;
        release_preload();
        return false;
    }
    return s_preload.status == 0;
}

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

void deactivate(daAlink_c*, bool clearMeter) {
    // A pending preload has not touched Link. An already installed cosmetic
    // outfit is restored by service_model_swap at the next safe player update.
    const bool wasActive = s_state.active;
    s_state.active = false;
    s_state.spinChargeArmed = false;
    s_state.lastDrainTime = {};
    if (wasActive && s_state.modelSwapState == ModelSwapState::None) cancel_preload();
    if (clearMeter) s_state.meter = 0.0f;
}

void reset_for_link(daAlink_c* link) {
    clear_kh2_drive();
    // The previous player may belong to another save. Never restore its clothes
    // into the current save; equipment restoration belongs to its deletion hook.
    fierce_deity_transition_cancel(nullptr);
    cancel_preload();
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
        fierce_deity_transition_cancel(link);
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
           link->mClothesChangeWaitTimer == 0 &&
           !link->checkNoResetFlg2(daPy_py_c::FLG2_UNK_280000);
}

void activate(daAlink_c* link) {
    if (!can_transform(link)) return;
    s_state.active = true;
    s_state.meter = 100.0f;
    s_state.spinChargeArmed = false;
    s_state.lastDrainTime = Clock::now();
    s_state.visual = fierce_deity_visual();
    s_state.failedArchive = nullptr;
}

// Selection changes retarget the preload; the live model and save equipment
// remain untouched until the replacement is ready.
void update_visual_selection(daAlink_c*) {
    if (s_state.active) s_state.visual = fierce_deity_visual();
}

bool visual_uses_magic(FierceDeityVisual visual) {
    return visual == FierceDeityVisual::MagicArmor || visual == FierceDeityVisual::DarkMagic;
}

FierceDeityTint visual_tint(FierceDeityVisual visual) {
    switch (visual) {
    case FierceDeityVisual::White: return FierceDeityTint::White;
    case FierceDeityVisual::Gold: return FierceDeityTint::Gold;
    case FierceDeityVisual::Dark:
    case FierceDeityVisual::DarkMagic: return FierceDeityTint::Dark;
    default: return FierceDeityTint::None;
    }
}

bool service_model_swap(daAlink_c* link) {
    s_state.modelReloadFrame = false;
    if (link == nullptr) return false;

    if (s_state.modelSwapState == ModelSwapState::None) {
        if (fierce_deity_transition_busy()) return false;
        const bool useMagic = s_state.active && visual_uses_magic(s_state.visual);
        const auto useTint = s_state.active ? visual_tint(s_state.visual) : FierceDeityTint::None;
        if (!useMagic && !s_state.modelSwapped && s_state.displayedTint == FierceDeityTint::None && useTint == FierceDeityTint::None) {
            cancel_preload();
            return false;
        }
        // Leave native clothing changes, transformations and cutscenes alone.
        // Jumping/falling are safe: we never suspend the live player's update.
        if (!can_transform(link) || menu_or_pause_active()) {
            cancel_preload();
            return false;
        }
        const u8 selected = dComIfGs_getSelectEquipClothes();
        const u8 targetClothes = useMagic ? u8(dItemNo_ARMOR_e) : selected;
        const char* targetArchive = outfit_archive(targetClothes);
        if (same_archive(link->mArcName, targetArchive) && s_state.displayedTint == useTint) {
            cancel_preload();
            s_state.modelSwapped = useMagic;
            s_state.failedArchive = nullptr;
            return false;
        }
        if (!prepare_outfit(targetArchive)) return false;

        // All disk I/O is complete and our reference pins the replacement.
        // Only this synchronous native transaction sees the temporary outfit.
        fierce_deity_transition_prepare(link, s_state.displayedTint, useTint, s_state.active);
        if (!fierce_deity_transition_busy() && same_archive(link->mArcName, targetArchive)) {
            // A foreign model without native warp attributes (or allocation
            // failure) uses an immediate material change. Reloading its OWN
            // archive heap would invalidate the pinned same-archive resource.
            s_state.displayedTint = useTint;
            s_state.modelSwapped = useMagic;
            release_preload();
            return false;
        }
        s_state.swapTint = useTint;
        s_state.originalClothes = selected;
        s_state.equipmentOverridden = true;
        dComIfGs_setSelectEquipClothes(targetClothes);
        dComIfGp_setSelectEquipClothes(targetClothes);
        s_state.modelSwapState = useMagic ? ModelSwapState::Activating : ModelSwapState::Restoring;
        link->setClothesChange(0);
    }

    // Native timer: 4 -> 3 -> 2 (retire old heap) -> 0 (changeLink).
    // Preloading makes all three steps synchronous. Keep a bounded defensive
    // fallback if another mod changes the native loader's timing; no access to
    // model/animation pointers is allowed while that loader is incomplete.
    for (int step = 0; step < 3 && link->mClothesChangeWaitTimer != 0; ++step) {
        link->loadModelDVD();
    }
    if (link->mClothesChangeWaitTimer != 0) {
        s_state.modelReloadFrame = true;
        return true;
    }

    s_state.modelSwapped = s_state.modelSwapState == ModelSwapState::Activating;
    s_state.modelSwapState = ModelSwapState::None;
    restore_equipment_selection();
    s_state.displayedTint = s_state.swapTint;
    fierce_deity_transition_commit(link);
    s_state.refreshFootBaseline = true;
    release_preload(); // Link's native phase now owns its own resource reference.

    // Run the ORIGINAL execute in this SAME frame. It updates movement,
    // collision, base/joint matrices and attached equipment before any draw.
    return false;
}

void update_spin_activation(daAlink_c* link) {
    if (fierce_deity_activation() != FierceDeityActivation::SpinAttack ||
        menu_or_pause_active() || s_state.active || s_state.meter < 100.0f) {
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

// Run immediately after the host reads the pad, before native combos and Link.
// Match the native R+Y shortcut: hold R, then freshly press the partner button.
// R itself stays untouched so its initial manual jump can run normally.
HookAction before_fierce_game_combos(ModContext*, void*, void*, void*) {
    auto* link = daAlink_getAlinkActorClass();
    if (!same_link(link)) reset_for_link(link);
    s_state.activationInputConsumed = false;
    auto& pad = mDoCPd_c::getCpadInfo(PAD_1);
    // Touch Z can be removed/reassigned by the touch UI or HD HUD after pad
    // sampling. Its latched UI edge is still the same user button press.
    const u32 held = pad.mButtonFlags | s_state.touchHeld;
    const u32 pressed = pad.mPressedButtonFlags | s_state.touchPressed;
    s_state.touchPressed = 0; // discard blocked/menu presses too
    s_state.consumedPartner &= held;
    const bool directTouch = consume_dark_link_touch_press();
    // Stick clicks live in extButton, not mButtonFlags: those bit positions
    // in the GameCube interface encode stick directions. Sample even while
    // blocked or using another binding so held clicks cannot trigger later.
    constexpr u32 stickMask = PAD_BUTTON_LEFT_STICK | PAD_BUTTON_RIGHT_STICK;
    const auto& raw = JUTGamePad::mPadStatus[PAD_1];
    const bool connected = raw.err == PAD_ERR_NONE;
    const u32 sticks = connected ? raw.extButton & stickMask : 0;
    const u32 stickPressed = connected && s_state.sticksConnected ? sticks & ~s_state.sticksHeld : 0;
    s_state.sticksHeld = sticks;
    s_state.sticksConnected = connected;
    const auto binding = fierce_deity_activation();
    const u32 partner = binding == FierceDeityActivation::RA ? PAD_BUTTON_A : PAD_TRIGGER_Z;
    const bool rHeld = (held & PAD_TRIGGER_R) != 0 || pad.mHoldLockR != 0;
    if (!fierce_deity_enabled() || menu_or_pause_active() || !can_transform(link)) {
        s_state.consumedPartner = 0;
        return HOOK_CONTINUE;
    }
    const bool combo = (binding == FierceDeityActivation::RZ || binding == FierceDeityActivation::RA) &&
        rHeld && (pressed & partner) != 0;
    const u32 stick = binding == FierceDeityActivation::L3 ? PAD_BUTTON_LEFT_STICK :
        binding == FierceDeityActivation::R3 ? PAD_BUTTON_RIGHT_STICK : 0;
    if (directTouch || combo || (stickPressed & stick) != 0) {
        if (s_state.active) deactivate(link, false);
        else if (s_state.meter >= 100.0f) activate(link);
        if (combo) s_state.consumedPartner |= partner;
        s_state.activationInputConsumed = true;
    }

    // Leave R/jump untouched. Hold the consumed A/Z out until finger release,
    // so native held actions (including R+A Moon Jump) cannot leak on tick 2.
    pad.mButtonFlags &= ~s_state.consumedPartner;
    pad.mPressedButtonFlags &= ~s_state.consumedPartner;
    return HOOK_CONTINUE;
}

void after_fierce_midna_trigger(ModContext*, void*, void* retval, void*) {
    // HD HUD can remember touch-Midna separately from the pad bit. Suppress
    // that result only while this Z press belongs to the Fierce Deity shortcut.
    if (retval != nullptr && same_link(daAlink_getAlinkActorClass()) &&
        (s_state.consumedPartner & PAD_TRIGGER_Z) != 0) {
        *static_cast<BOOL*>(retval) = FALSE;
    }
}

void refresh_foot_baseline(daAlink_c* link) {
    if (!s_state.refreshFootBaseline || link->mpLinkModel == nullptr) return;
    // changeLink(1) clears OldFrameFlg. setFootSpeed then stores current.pos
    // (WORLD space) as its fallback. The next tick compares LOCAL foot joints
    // against those world coordinates and adds the huge delta to forward speed.
    // Rebase after the original execute has posed the replacement model.
    const u16 joints[] = {link->field_0x30bc, link->field_0x30be};
    for (int i = 0; i < 2; ++i) {
        if (joints[i] >= link->mpLinkModel->getModelData()->getJointNum()) return;
    }
    for (int i = 0; i < 2; ++i) {
        Mtx local;
        MTXConcat(link->mInvMtx, link->mpLinkModel->getAnmMtx(joints[i]), local);
        link->field_0x37b0[i].set(local[0][3], local[1][3], local[2][3]);
    }
    s_state.refreshFootBaseline = false;
}

void update_drain(daAlink_c* link) {
    if (!s_state.active) {
        return;
    }
    if (link->checkSceneChangeAreaStart()) {
        restore_equipment_selection();
        cancel_preload();
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

daAlink_c* dispatched_player(void* args) {
    auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr || mods::arg<void*>(args, 1) != link || link->sub_method == nullptr) {
        return nullptr;
    }
    const auto* methods = reinterpret_cast<const process_method_class*>(link->sub_method);
#if defined(__APPLE__)
    const bool executingPlayer = mods::arg<process_method_func>(args, 0) == methods->execute_method;
#else
    const bool executingPlayer = mods::arg<const process_method_class*>(args, 0) == methods;
#endif
    if (!executingPlayer) {
        return nullptr;
    }
    return link;
}

HookAction before_player_execute(ModContext*, void* args, void* retval, void*) {
    // Also drain cancelled I/O when the old player has already been deleted.
    if (s_preload.cancelled) poll_preload();
    auto* link = dispatched_player(args);
    if (link == nullptr) {
        return HOOK_CONTINUE;
    }
    if (!same_link(link)) {
        reset_for_link(link);
    }

    if (!fierce_deity_enabled()) {
        deactivate(link, true);
    }

    if (s_state.modelSwapState == ModelSwapState::None &&
        (link->mClothesChangeWaitTimer != 0 || !can_transform(link))) {
        fierce_deity_transition_cancel(link);
    }
    update_visual_selection(link);
    if (!service_model_swap(link)) {
        return HOOK_CONTINUE;
    }
    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

void after_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = dispatched_player(args);
    if (link == nullptr) {
        return;
    }
    if (!same_link(link)) return;
    if (!s_state.modelReloadFrame) refresh_foot_baseline(link);
    if (!s_state.modelReloadFrame && !menu_or_pause_active()) {
        fierce_deity_transition_tick(link);
    }
    if (!fierce_deity_enabled()) return;
    if (!s_state.modelReloadFrame) {
        update_spin_activation(link);
    }
    update_drain(link);
}

HookAction before_magic_armor_ability(ModContext*, void*, void* retval, void*) {
    if (!same_link(daAlink_getAlinkActorClass()) ||
        (!s_state.modelSwapped && s_state.modelSwapState == ModelSwapState::None &&
         (!s_state.active || !visual_uses_magic(s_state.visual)))) {
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
        menu_or_pause_active())
    {
        return;
    }
    draw_combat_meter(meter, s_state.meter, CombatMeterStyle::FierceDeity,
        stamina_meter_visible() ? 1 : 0);
}

void after_meter_draw(ModContext*, void* args, void*, void*) {
    if (update_kh2_drive()) return;
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
    result = mods::hook::add_pre<FierceGameCombosHook>(svc_hook, before_fierce_game_combos);
    if (result != MOD_OK) {
        return mods::set_error(error, result,
            "failed to install Dawnlight Dark Link input hook");
    }
    if ((result = add_post<FierceMidnaTriggerHook>(error, after_fierce_midna_trigger,
            "failed to install Dawnlight Dark Link touch Midna guard")) != MOD_OK) return result;
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
    return initialize_fierce_deity_visual(error);
}

void shutdown_fierce_deity() {
    clear_kh2_drive();
    if (s_saveObserver != 0 && svc_save != nullptr) {
        svc_save->unobserve_saves(mod_ctx, s_saveObserver);
    }
    s_saveObserver = 0;
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (same_link(link)) {
        deactivate(link, true);
    }
    // Hooks are about to be removed. Finish only the outstanding resource I/O
    // before dropping its reference; ordinary gameplay never waits here.
    while (s_preload.archive != nullptr && s_preload.status > 0) {
        poll_preload();
        std::this_thread::yield();
    }
    release_preload();
    fierce_deity_transition_cancel(link);
    if (same_link(link)) restore_equipment_selection();
    s_state = {};
}

DawnlightFierceDeityHudState fierce_deity_hud_state() {
    const bool hasPlayer = same_link(daAlink_getAlinkActorClass());
    const bool enabled = fierce_deity_enabled();
    return {sizeof(DawnlightFierceDeityHudState), enabled,
        enabled && hasPlayer && !menu_or_pause_active(),
        hasPlayer && s_state.active, hasPlayer ? std::clamp(s_state.meter, 0.0f, 100.0f) : 0.0f};
}

bool fierce_deity_active() {
    return s_state.active && same_link(daAlink_getAlinkActorClass());
}

bool fierce_deity_input_consumed() {
    return same_link(daAlink_getAlinkActorClass()) &&
        s_state.activationInputConsumed;
}

void fierce_deity_touch_button(uint32_t button, bool pressed) {
    auto* link = daAlink_getAlinkActorClass();
    if (!same_link(link)) reset_for_link(link);
    if (pressed) {
        s_state.touchPressed |= button & ~s_state.touchHeld;
        s_state.touchHeld |= button;
    } else {
        s_state.touchHeld &= ~button;
    }
}

bool fierce_deity_dark_visual_active() {
    auto* link = daAlink_getAlinkActorClass();
    return same_link(link) && s_state.displayedTint != FierceDeityTint::None &&
        s_state.modelSwapState == ModelSwapState::None && !s_state.modelReloadFrame &&
        !link->checkWolf() && link->mClothesChangeWaitTimer == 0;
}

FierceDeityTint fierce_deity_displayed_tint() {
    return fierce_deity_dark_visual_active() ? s_state.displayedTint : FierceDeityTint::None;
}

bool fierce_deity_model_reload_active() {
    return s_state.modelReloadFrame && same_link(daAlink_getAlinkActorClass());
}

}  // namespace dawnlight
