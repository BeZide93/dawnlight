#include "bow_modes.hpp"
#include "bow_volley.hpp"

#include "fierce_deity.hpp"
#include "service_imports.hpp"

#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JParticle/JPAEmitterManager.h"
#include "JSystem/JParticle/JPAResourceManager.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_arrow.h"
#include "d/d_cc_uty.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "d/d_particle_name.h"
#include "f_pc/f_pc_leaf.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_lib.h"
#include "m_Do/m_Do_mtx.h"
#include "mods/service.hpp"
#include "mods/svc/hook.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::execute, BowPlayerExecuteHook);
DEFINE_HOOK(&daAlink_c::checkUpperItemActionBow, BowActionHook);
DEFINE_HOOK(&daArrow_c::arrowShooting, BowLaunchHook);
DEFINE_HOOK(&daArrow_c::setArrowAt, BowColliderHook);
DEFINE_HOOK(&daArrow_c::setArrowWaterNextPos, BowWaterHook);
DEFINE_HOOK(&daArrow_c::execute, BowArrowExecuteHook);
DEFINE_HOOK(&fpcLf_Delete, BowArrowDeleteHook);
DEFINE_HOOK(&at_power_check, BowDamageHook);
DEFINE_HOOK(&cCcS::ChkNoHitAtTg, BowCollisionFilterHook);
DEFINE_HOOK(&cCcS::SetAtTgCommonHitInf, BowCollisionHitHook);
DEFINE_HOOK(&dMeter2Draw_c::draw, BowModeDrawHook);

enum class BowMode { Normal, Fire, Triple };
constexpr int kNoticeFrames = 60;
constexpr s16 kSpreadAngle = 0x900;
constexpr u16 kBulblinArrowFlame = dPa_RM(ID_ZI_S_RD_ARROWFIRE_A); // native 0x8113

// Native arrow attack power 2 maps to different damage values for different enemies.
// Keep that power unchanged and multiply the resolved damage in at_power_check instead.
const dCcD_SrcCps kLanternSource = {
    {
        {0, {{AT_TYPE_LANTERN_SWING, 0, 0x1b}, {0, 0}, 0}},
        {dCcD_SE_NONE, 0, 0, dCcD_MTRL_FIRE, {0}},
        {dCcD_SE_NONE, 0, 0, dCcD_MTRL_NONE, {0}},
        {0},
    },
    {{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 5.0f}},
};

struct ArrowState {
    daArrow_c* arrow = nullptr;
    BowMode mode = BowMode::Normal;
    bool launched = false;
    bool extinguished = false;
    u32 flame = 0;
    u16 flameEffect = 0;
    cXyz flameVelocity{};
    std::uint32_t volley = kNoBowVolley;
    dCcD_Cps lantern{};
    std::vector<std::pair<fpc_ProcID, cCcD_Obj*>> ignitionTargets;
};

// Heap-owned entries keep the collision proxy address stable as more arrows are fired.
std::unordered_map<fpc_ProcID, std::unique_ptr<ArrowState>> s_arrows;
BowMode s_mode = BowMode::Normal;
fpc_ProcID s_playerId = fpcM_ERROR_PROCESS_ID_e;
int s_noticeFrames = 0;
bool s_ammoWarning = false;
bool s_inputMasked = false;
interface_of_controller_pad s_savedPad{};
fpc_ProcID s_nockedArrow = fpcM_ERROR_PROCESS_ID_e;
BowMode s_releaseMode = BowMode::Normal;
daArrow_c* s_spreadSource = nullptr;
s16 s_spreadYaw = 0;

int arrow_cost(BowMode mode) {
    return mode == BowMode::Fire ? 2 : mode == BowMode::Triple ? 3 : 1;
}

bool bow_active(daAlink_c* link) {
    // Bomb arrows retain their native item-action switch; the Hawkeye still uses it to zoom.
    return link != nullptr && !link->checkWolf() && !fierce_deity_model_reload_active() &&
           (link->mEquipItem == dItemNo_BOW_e || link->mEquipItem == dItemNo_HAWK_ARROW_e) &&
           !link->checkEventRun() && !link->checkSceneChangeAreaStart() &&
           !link->checkDeadHP() && !dComIfGp_isPauseFlag() &&
           dMeter2Info_getWindowStatus() == 0 && dMeter2Info_getPauseStatus() == 0;
}

bool bow_aim_active(daAlink_c* link) {
    // Ready, reload, draw and shot animations all belong to active bow aiming.
    // Merely keeping the bow equipped after cancelling aim must leave ZR alone.
    return bow_active(link) && link->checkBowAnime();
}

u16 arrow_flame_effect() {
    auto* manager = dPa_control_c::mEmitterMng;
    auto* scene = manager != nullptr ? manager->getResourceManager(u8{1}) : nullptr;
    if (scene != nullptr && scene->checkUserIndexDuplication(kBulblinArrowFlame)) {
        return kBulblinArrowFlame;
    }
    // Some rooms omit Bulblin particles. Keep arrows visibly lit in those rooms
    // instead of requesting a missing resource and silently losing the flame.
    return ID_ZI_J_KANTERA_FIRE;
}

ArrowState* arrow_state(daArrow_c* arrow) {
    const auto it = s_arrows.find(fopAcM_GetID(arrow));
    return it != s_arrows.end() && it->second->arrow == arrow ? it->second.get() : nullptr;
}

void lantern_hit(fopAc_ac_c*, dCcD_GObjInf*, fopAc_ac_c*, dCcD_GObjInf*);

ArrowState& track_arrow(daArrow_c* arrow, BowMode mode) {
    auto& entry = s_arrows[fopAcM_GetID(arrow)];
    if (!entry) {
        entry = std::make_unique<ArrowState>();
        entry->arrow = arrow;
        entry->lantern.Set(kLanternSource);
        entry->lantern.SetStts(&arrow->field_0x64c);
        entry->lantern.SetAtHitCallback(lantern_hit);
    }
    entry->mode = mode;
    return *entry;
}

ArrowState* fire_state(cCcD_Obj* collider) {
    fopAc_ac_c* actor = collider != nullptr ? collider->GetAc() : nullptr;
    if (actor == nullptr || fopAcM_GetName(actor) != fpcNm_ARROW_e) {
        return nullptr;
    }
    auto* arrow = static_cast<daArrow_c*>(actor);
    ArrowState* state = arrow_state(arrow);
    return state != nullptr && state->mode == BowMode::Fire && !state->extinguished &&
                   collider == &arrow->field_0x688 ? state : nullptr;
}

void stop_flame(ArrowState& state) {
    if (state.flame != 0) {
        if (auto* emitter = dComIfGp_particle_getEmitter(state.flame)) {
            emitter->setParticleCallBackPtr(nullptr);
            emitter->setUserWork(0);
            emitter->stopCreateParticle();
            emitter->quitImmortalEmitter();
            emitter->becomeInvalidEmitter();
        }
        state.flame = 0;
    }
    state.flameEffect = 0;
}

void extinguish(ArrowState& state) {
    state.extinguished = true;
    state.arrow->field_0x688.SetAtMtrl(dCcD_MTRL_NONE);
    stop_flame(state);
}

void restore_ignition_targets(ArrowState& state) {
    // A torch reads its hit collider on the following update. Never leave it
    // pointing into mod-owned memory after an arrow is deleted or the mod unloads.
    for (const auto& [id, collider] : state.ignitionTargets) {
        if (fopAcM_SearchByID(id) != nullptr &&
            collider->GetTgHitObj() == &state.lantern)
        {
            collider->SetTgHit(&state.arrow->field_0x688);
        }
    }
}

void lantern_hit(fopAc_ac_c* actor, dCcD_GObjInf* hit,
                 fopAc_ac_c* target, dCcD_GObjInf* targetCollider)
{
    auto* arrow = static_cast<daArrow_c*>(actor);
    // Preserve the arrow's native impact/stop handling while the torch sees fire
    // from a lantern. Only the target's view of the attack uses the proxy.
    arrow->field_0x688.SetAtHit(targetCollider);
    arrow->field_0x688.SetAtHitApid(fopAcM_GetID(target));
    arrow->field_0x688.SetAtHitPos(*hit->GetAtHitPosP());
    if (hit->ChkAtShieldHit()) {
        arrow->field_0x688.OnAtShieldHit();
    }
    arrow->atHitCallBack(hit, target, targetCollider);
}

HookAction before_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (fierce_deity_model_reload_active()) {
        return HOOK_CONTINUE;
    }
    if (s_playerId != fopAcM_GetID(link)) {
        s_playerId = fopAcM_GetID(link);
        s_mode = BowMode::Normal;
        s_noticeFrames = 0;
    }
    if (!bow_aim_active(link)) {
        s_noticeFrames = 0;
        return HOOK_CONTINUE;
    }
    if (s_noticeFrames > 0) {
        --s_noticeFrames;
    }
    auto& pad = mDoCPd_c::getCpadInfo(PAD_1);
    if (pad.mTrigLockR) {
        s_mode = s_mode == BowMode::Normal ? BowMode::Fire :
                 s_mode == BowMode::Fire ? BowMode::Triple : BowMode::Normal;
        s_noticeFrames = kNoticeFrames;
        s_ammoWarning = false;
    }

    // Consume ZR only while aiming. Jump, sprint and manual guard must not also
    // fire when ZR changes the bow mode. Holding the bow alone keeps normal input.
    s_savedPad = pad;
    s_inputMasked = true;
    pad.mHoldLockR = 0;
    pad.mTrigLockR = 0;
    pad.mTriggerRight = 0.0f;
    pad.mButtonFlags &= ~PAD_TRIGGER_R;
    pad.mPressedButtonFlags &= ~PAD_TRIGGER_R;
    return HOOK_CONTINUE;
}

void after_player_execute(ModContext*, void*, void*, void*) {
    if (s_inputMasked) {
        auto& pad = mDoCPd_c::getCpadInfo(PAD_1);
        pad.mHoldLockR = s_savedPad.mHoldLockR;
        pad.mTrigLockR = s_savedPad.mTrigLockR;
        pad.mTriggerRight = s_savedPad.mTriggerRight;
        pad.mButtonFlags = (pad.mButtonFlags & ~PAD_TRIGGER_R) |
                          (s_savedPad.mButtonFlags & PAD_TRIGGER_R);
        pad.mPressedButtonFlags = (pad.mPressedButtonFlags & ~PAD_TRIGGER_R) |
                                 (s_savedPad.mPressedButtonFlags & PAD_TRIGGER_R);
        s_inputMasked = false;
    }
}

HookAction before_bow_action(ModContext*, void* args, void*, void*) {
    s_nockedArrow = fpcM_ERROR_PROCESS_ID_e;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!bow_active(link) || !link->checkBowChargeWaitAnime() ||
        !link->checkReadyItem() || link->itemButton() || link->mItemVar0.field_0x3018 != 0)
    {
        return HOOK_CONTINUE;
    }
    auto* actor = link->mItemAcKeep.getActor();
    if (actor == nullptr || fopAcM_GetName(actor) != fpcNm_ARROW_e ||
        static_cast<daArrow_c*>(actor)->mArrowType != daArrow_c::ARROW_TYPE_NORMAL)
    {
        return HOOK_CONTINUE;
    }
    // The HUD applies pending ammunition deltas later. Include them to prevent
    // consecutive releases from spending the same last arrows twice.
    const int available = dComIfGs_getArrowNum() + dComIfGp_getItemArrowNumCount();
    if (available < arrow_cost(s_mode)) {
        link->deleteArrow(); // native empty-shot path: no projectile and no ammo deduction
        s_noticeFrames = kNoticeFrames;
        s_ammoWarning = true;
        return HOOK_CONTINUE;
    }
    s_nockedArrow = fopAcM_GetID(actor);
    s_releaseMode = s_mode;
    return HOOK_CONTINUE;
}

void after_bow_action(ModContext*, void*, void*, void*) {
    if (s_nockedArrow == fpcM_ERROR_PROCESS_ID_e) {
        return;
    }
    auto* actor = fopAcM_SearchByID(s_nockedArrow);
    s_nockedArrow = fpcM_ERROR_PROCESS_ID_e;
    if (actor == nullptr || fopAcM_GetName(actor) != fpcNm_ARROW_e ||
        (fopAcM_GetParam(actor) != 1 && fopAcM_GetParam(actor) != 2))
    {
        return;
    }
    auto* arrow = static_cast<daArrow_c*>(actor);
    if (s_releaseMode != BowMode::Fire) {
        // A mode change and release can happen before the nocked arrow's next update.
        // Retire its fire preview immediately, including when returning to Normal.
        if (auto* previous = arrow_state(arrow)) {
            stop_flame(*previous);
            previous->mode = s_releaseMode;
        }
    }
    if (s_releaseMode == BowMode::Normal) {
        return;
    }
    auto& state = track_arrow(static_cast<daArrow_c*>(actor), s_releaseMode);
    state.volley = s_releaseMode == BowMode::Triple ? fopAcM_GetID(actor) : kNoBowVolley;
    // Vanilla has already reserved the first arrow at the actual release.
    dComIfGp_setItemArrowNumCount(-(arrow_cost(s_releaseMode) - 1));
}

void apply_spread_flight(daArrow_c* arrow) {
    if (s_spreadSource != nullptr && arrow != s_spreadSource) {
        auto* source = s_spreadSource;
        arrow->current = source->current;
        arrow->old = source->old;
        arrow->shape_angle = source->shape_angle;
        arrow->current.angle.y += s_spreadYaw;
        arrow->shape_angle.y += s_spreadYaw;
        arrow->mStartPos = source->mStartPos;
        arrow->mFlyMax = source->mFlyMax;
        arrow->field_0x99c = source->field_0x99c;
        arrow->mOutLengthRate = source->mOutLengthRate;
        arrow->speed = bow_spread_velocity(source->speed, s_spreadYaw);
        arrow->speedF = source->speedF;
    }
}

HookAction before_arrow_collider(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    apply_spread_flight(arrow);
    if (auto* state = arrow_state(arrow); state != nullptr && state->mode == BowMode::Fire) {
        if (arrow->field_0x945 != 0 || arrow->field_0x943 != 0) {
            extinguish(*state);
        }
        arrow->field_0x688.SetAtMtrl(state->extinguished ? dCcD_MTRL_NONE : dCcD_MTRL_FIRE);
    }
    return HOOK_CONTINUE;
}

void after_arrow_collider(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    if (auto* state = arrow_state(arrow); state != nullptr && state->mode == BowMode::Fire) {
        if (arrow->field_0x945 != 0 || arrow->field_0x943 != 0) {
            extinguish(*state);
        }
        state->lantern.SetAtVec(arrow->speed);
    }
}

void after_arrow_water(ModContext*, void* args, void* retval, void*) {
    auto* state = arrow_state(mods::arg<daArrow_c*>(args, 0));
    if (state != nullptr && state->mode == BowMode::Fire && *static_cast<int*>(retval) != 0) {
        extinguish(*state);
    }
}

void after_arrow_launch(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    auto* state = arrow_state(arrow);
    if (state == nullptr || state->launched) {
        return;
    }
    // This post-hook runs AFTER Dawnlight's camera correction. Otherwise it aims
    // the two side arrows back at the reticle and collapses the spread.
    apply_spread_flight(arrow);
    cXyz end = arrow->current.pos + arrow->speed * (arrow->mOutLengthRate + 1.0f);
    if (arrow->field_0x945 == 0) {
        arrow->setArrowWaterNextPos(&arrow->current.pos, &end);
    }
    arrow->field_0x56c.Set(&arrow->current.pos, &end, arrow);
    if (dComIfG_Bgsp().LineCross(&arrow->field_0x56c)) {
        end = arrow->field_0x56c.GetCross();
    }
    // Update the already-registered capsule in place, never register it twice.
    static_cast<cM3dGCps*>(&arrow->field_0x688)->Set(
        arrow->current.pos, end, arrow->field_0x688.GetR());
    arrow->field_0x688.CalcAtVec();
    state->launched = true;
    if (state->mode != BowMode::Triple || s_spreadSource != nullptr) {
        return;
    }
    s_spreadSource = arrow;
    for (const s16 yaw : {static_cast<s16>(-kSpreadAngle), kSpreadAngle}) {
        auto* extra = static_cast<daArrow_c*>(daArrow_c::makeArrow(arrow, 0));
        if (extra == nullptr) {
            dComIfGp_setItemArrowNumCount(1); // do not charge for a failed allocation
            continue;
        }
        track_arrow(extra, BowMode::Triple).volley = state->volley;
        s_spreadYaw = yaw;
        fopAcM_SetParam(extra, fopAcM_GetParam(arrow));
        // Use the native wait -> flight transition, including blur and arrow lifetime.
        // The active spread source prevents side arrows from spawning more arrows.
        extra->procWait();
        extra->setNormalMatrix();
    }
    s_spreadSource = nullptr;
}

void after_arrow_execute(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    auto* state = arrow_state(arrow);
    const bool nocked = arrow->checkWait();
    if (nocked && arrow->mArrowType == daArrow_c::ARROW_TYPE_NORMAL) {
        auto* link = daAlink_getAlinkActorClass();
        const bool preview = bow_aim_active(link) && link->mItemAcKeep.getActor() == arrow;
        if (preview && s_mode == BowMode::Fire) {
            state = &track_arrow(arrow, BowMode::Fire);
        } else if (state != nullptr) {
            stop_flame(*state);
            state->mode = BowMode::Normal;
        }
    }
    if (state == nullptr || state->mode != BowMode::Fire || (!state->launched && !nocked)) {
        return;
    }
    if (arrow->field_0x945 != 0 || arrow->field_0x943 != 0 || arrow->field_0x93f != 0 ||
        (!nocked && fopAcM_GetParam(arrow) != 1 && fopAcM_GetParam(arrow) != 2))
    {
        stop_flame(*state);
        if (arrow->field_0x945 != 0 || arrow->field_0x943 != 0) {
            extinguish(*state);
        }
        return;
    }
    if (state->extinguished) {
        return;
    }
    cXyz tip;
    if (nocked) {
        const Vec localTip{0.0f, 0.0f, 90.0f};
        mDoMtx_multVec(arrow->mpModel->getBaseTRMtx(), &localTip, &tip);
    } else {
        tip = arrow->current.pos + arrow->speed * arrow->mOutLengthRate;
    }
    const u16 effect = arrow_flame_effect();
    if (state->flameEffect != effect) {
        stop_flame(*state);
        state->flameEffect = effect;
    }
    // Match e_arrow::fire_eff_set: original Bulblin resource, full native scale,
    // no environment tint, and the same 0.9x velocity particle-trace callback.
    const cXyz fallbackScale(0.65f, 0.65f, 0.65f);
    state->flame = dComIfGp_particle_set(
        state->flame, effect, &tip, nullptr, &arrow->shape_angle,
        effect == kBulblinArrowFlame ? nullptr : &fallbackScale,
        0xff, nullptr, -1, nullptr, nullptr, nullptr);
    if (auto* emitter = dComIfGp_particle_getEmitter(state->flame)) {
        state->flameVelocity = nocked ? cXyz::Zero : arrow->speed * 0.9f;
        const bool moving = state->flameVelocity.abs2() > 1.0f;
        emitter->setParticleCallBackPtr(moving ? dPa_control_c::getParticleTracePCB() : nullptr);
        emitter->setUserWork(moving ? reinterpret_cast<uintptr_t>(&state->flameVelocity) : 0);
    }
}

HookAction before_arrow_delete(ModContext*, void* args, void*, void*) {
    // Use the public lifecycle entry point: the file-qualified static arrow
    // deleter is absent from Android's symbol manifest. This runs before actor
    // destruction, including deletion during scene teardown, not just requests.
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (process == nullptr || fpcM_GetName(process) != fpcNm_ARROW_e) {
        return HOOK_CONTINUE;
    }
    auto* arrow = static_cast<daArrow_c*>(process);
    if (auto* state = arrow_state(arrow)) {
        stop_flame(*state);
        restore_ignition_targets(*state);
        s_arrows.erase(fopAcM_GetID(arrow));
    }
    return HOOK_CONTINUE;
}

void after_attack_power(ModContext*, void* args, void*, void*) {
    auto* info = mods::arg<dCcU_AtInfo*>(args, 0);
    if (fire_state(info->mpCollider) != nullptr) {
        // Round half-points up; preserve native immunity (zero remains zero).
        info->mAttackPower += (info->mAttackPower + 1) / 2;
    }
}

bool requires_lantern_type(fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return false;
    }
    switch (fopAcM_GetName(actor)) {
    case fpcNm_Obj_Lv1Cdl00_e:
    case fpcNm_Obj_Lv1Cdl01_e:
    case fpcNm_Obj_Lv2Candle_e:
    case fpcNm_Obj_FireWood_e:
    case fpcNm_Obj_FireWood2_e:
    case fpcNm_Obj_WdStick_e:
    case fpcNm_Tag_KtOnFire_e:
        return true;
    default:
        return false;
    }
}

HookAction before_collision(ModContext*, void* args, void*, void*) {
    auto*& attack = mods::arg_ref<cCcD_Obj*>(args, 1);
    auto* target = mods::arg<cCcD_Obj*>(args, 2);
    auto* state = fire_state(attack);
    if (state != nullptr && requires_lantern_type(target->GetAc())) {
        // These native actors require exact equality with AT_TYPE_LANTERN_SWING.
        // Substitute a persistent zero-damage proxy for this pair only. Enemies,
        // switches and webs keep the real arrow collider and its FIRE material.
        attack = &state->lantern;
    }
    return HOOK_CONTINUE;
}

std::uint32_t collider_volley(cCcD_Obj* collider) {
    auto* actor = collider != nullptr ? collider->GetAc() : nullptr;
    if (actor != nullptr && fopAcM_GetName(actor) == fpcNm_ARROW_e) {
        if (auto* state = arrow_state(static_cast<daArrow_c*>(actor))) {
            return state->volley;
        }
    }
    return kNoBowVolley;
}

HookAction before_collision_filter(ModContext* ctx, void* args, void* retval, void* data) {
    auto* attack = mods::arg<cCcD_Obj*>(args, 1);
    auto* target = mods::arg<cCcD_Obj*>(args, 2);
    if (same_bow_volley(collider_volley(attack), collider_volley(target))) {
        // Native arrows also accept arrow hits. The three overlapping launch
        // capsules otherwise hit each other and all disappear on their first update.
        *static_cast<bool*>(retval) = true;
        return HOOK_SKIP_ORIGINAL;
    }
    return before_collision(ctx, args, retval, data);
}

HookAction before_collision_hit(ModContext* ctx, void* args, void* retval, void* data) {
    auto* attack = mods::arg<cCcD_Obj*>(args, 1);
    auto* target = mods::arg<cCcD_Obj*>(args, 2);
    auto* state = fire_state(attack);
    if (state != nullptr && requires_lantern_type(target->GetAc())) {
        const auto entry = std::make_pair(fopAcM_GetID(target->GetAc()), target);
        if (std::find(state->ignitionTargets.begin(), state->ignitionTargets.end(), entry) ==
            state->ignitionTargets.end())
        {
            state->ignitionTargets.push_back(entry);
        }
    }
    return before_collision(ctx, args, retval, data);
}

void draw_arrow(J2DGrafContext* graf, float x, float y, float slant) {
    graf->line({x, y + 10.0f}, {x + slant, y - 10.0f});
    graf->line({x + slant - 4.0f, y - 5.0f}, {x + slant, y - 10.0f});
    graf->line({x + slant + 4.0f, y - 5.0f}, {x + slant, y - 10.0f});
    graf->line({x - 3.0f, y + 7.0f}, {x, y + 10.0f});
    graf->line({x + 3.0f, y + 7.0f}, {x, y + 10.0f});
}

void after_meter_draw(ModContext*, void*, void*, void*) {
    auto* link = daAlink_getAlinkActorClass();
    if (s_noticeFrames <= 0 || !bow_aim_active(link) || dComIfGd_getView() == nullptr) {
        return;
    }
    auto* graf = dComIfGp_getCurrentGrafPort();
    if (graf == nullptr) {
        return;
    }
    cXyz head = link->eyePos;
    head.y += 45.0f;
    Vec screen{}, camera{};
    mDoLib_pos2camera(&head, &camera);
    mDoLib_project(&head, &screen);
    const float left = mDoGph_gInf_c::getMinXF() + 32.0f;
    const float right = mDoGph_gInf_c::getMaxXF() - 32.0f;
    const float bottom = mDoGph_gInf_c::getHeightF() - 40.0f;
    // First-person aim may put Link's head behind the camera or outside the view.
    if (camera.z >= -1.0f || !std::isfinite(screen.x) || !std::isfinite(screen.y) ||
        screen.x < left || screen.x > right || screen.y < 36.0f || screen.y > bottom)
    {
        screen.x = (left + right) * 0.5f;
        screen.y = mDoGph_gInf_c::getHeightF() * 0.5f + 52.0f;
    }
    const auto tl = graf->mColorTL, tr = graf->mColorTR;
    const auto br = graf->mColorBR, bl = graf->mColorBL;
    const auto width = graf->mLineWidth;
    const auto previous = graf->mPrevPos;
    graf->setup2D();
    const u8 alpha = static_cast<u8>(std::min(s_noticeFrames, 12) * 255 / 12);
    graf->setColor(JUtility::TColor(15, 20, 28, alpha * 3 / 4));
    graf->fillBox({screen.x - 27, screen.y - 27, screen.x + 27, screen.y + 23});
    graf->setLineWidth(18);
    graf->setColor(s_ammoWarning ? JUtility::TColor(255, 80, 70, alpha) :
        s_mode == BowMode::Fire ? JUtility::TColor(255, 175, 65, alpha) :
        s_mode == BowMode::Triple ? JUtility::TColor(110, 220, 255, alpha) :
                                   JUtility::TColor(245, 245, 225, alpha));
    draw_arrow(graf, screen.x, screen.y, 0);
    if (s_mode == BowMode::Triple) {
        draw_arrow(graf, screen.x - 8, screen.y, -6);
        draw_arrow(graf, screen.x + 8, screen.y, 6);
    } else if (s_mode == BowMode::Fire) {
        // Small flame around the arrowhead, recognizable without relying on color.
        graf->line({screen.x - 7, screen.y - 9}, {screen.x - 6, screen.y - 18});
        graf->moveTo(screen.x - 6, screen.y - 18);
        graf->lineTo(screen.x - 2, screen.y - 14);
        graf->lineTo(screen.x + 2, screen.y - 23);
        graf->lineTo(screen.x + 7, screen.y - 14);
        graf->lineTo(screen.x + 6, screen.y - 9);
    }
    // One dot per arrow consumed; red indicates insufficient ammunition.
    for (int i = 0; i < arrow_cost(s_mode); ++i) {
        const float x = screen.x + (i - (arrow_cost(s_mode) - 1) * 0.5f) * 8.0f;
        graf->fillBox({x - 2, screen.y + 16, x + 2, screen.y + 20});
    }
    graf->setColor(tl, tr, br, bl);
    graf->setLineWidth(width);
    graf->mPrevPos = previous;
}

}  // namespace

ModResult initialize_bow_modes(ModError* error) {
    HookOptions afterAim = HOOK_OPTIONS_INIT;
    afterAim.priority = -100; // priorities run high to low; camera correction uses zero
    const ModResult results[] = {
        mods::hook::add_pre<BowPlayerExecuteHook>(svc_hook, before_player_execute),
        mods::hook::add_post<BowPlayerExecuteHook>(svc_hook, after_player_execute),
        mods::hook::add_pre<BowActionHook>(svc_hook, before_bow_action),
        mods::hook::add_post<BowActionHook>(svc_hook, after_bow_action),
        mods::hook::add_post<BowLaunchHook>(svc_hook, after_arrow_launch, &afterAim),
        mods::hook::add_pre<BowColliderHook>(svc_hook, before_arrow_collider),
        mods::hook::add_post<BowColliderHook>(svc_hook, after_arrow_collider),
        mods::hook::add_post<BowWaterHook>(svc_hook, after_arrow_water),
        mods::hook::add_post<BowArrowExecuteHook>(svc_hook, after_arrow_execute),
        mods::hook::add_pre<BowArrowDeleteHook>(svc_hook, before_arrow_delete),
        mods::hook::add_post<BowDamageHook>(svc_hook, after_attack_power),
        mods::hook::add_pre<BowCollisionFilterHook>(svc_hook, before_collision_filter),
        mods::hook::add_pre<BowCollisionHitHook>(svc_hook, before_collision_hit),
        mods::hook::add_post<BowModeDrawHook>(svc_hook, after_meter_draw),
    };
    for (const ModResult result : results) {
        if (result != MOD_OK) {
            return mods::set_error(error, result, "failed to install Dawnlight bow modes");
        }
    }
    return MOD_OK;
}

void shutdown_bow_modes() {
    after_player_execute(nullptr, nullptr, nullptr, nullptr);
    for (auto& [id, state] : s_arrows) {
        stop_flame(*state);
        if (fopAcM_SearchByID(id) == state->arrow) {
            restore_ignition_targets(*state);
            state->arrow->field_0x688.SetAtMtrl(dCcD_MTRL_NONE);
        }
    }
    s_arrows.clear();
    s_mode = BowMode::Normal;
    s_playerId = fpcM_ERROR_PROCESS_ID_e;
    s_noticeFrames = 0;
    s_nockedArrow = fpcM_ERROR_PROCESS_ID_e;
    s_spreadSource = nullptr;
}

}  // namespace dawnlight
