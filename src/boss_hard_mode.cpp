#include "boss_hard_mode.hpp"

#include "config.hpp"
#include "service_imports.hpp"

#include "SSystem/SComponent/c_math.h"
#include "m_Do/m_Do_ext.h"
#include "d/d_kankyo.h"
#include "d/actor/d_a_b_bq.h"
#include "d/actor/d_a_e_fm.h"
#include "d/actor/d_a_e_gob.h"
#include "d/actor/d_a_e_mk.h"
#include "d/actor/d_a_e_mk_bo.h"
#include "d/actor/d_a_e_vt.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace dawnlight {
namespace {

DEFINE_HOOK(&fpcMtd_Method, BossHardModeProcessHook);
DEFINE_HOOK_SYMBOL("dBgS_Acch::CrrPos", void(dBgS_Acch*, dBgS&), BossHardModeCollisionHook);
DEFINE_HOOK(&daE_VA_c::damage_check, DeathSwordDamageCheckHook);

constexpr s16 kDiababaActionWait = 1;
constexpr s16 kDiababaActionDamage = 3;
constexpr s16 kDangoroActionAttack = 3;
constexpr s16 kDangoroActionBall = 5;
constexpr s16 kFyrusActionNormal = 0;
constexpr s16 kFyrusActionFightRun = 1;
constexpr s16 kFyrusActionDamageRun = 4;
constexpr s16 kFyrusActionDown = 9;
constexpr float kOokBoomerangBaseSpeed = 40.0f;
constexpr float kDiababaToadpoliSideOffset = 1200.0f;
constexpr u32 kToadpoliNoPathParameters = 0x00FF0000;

struct BossState {
    fopAc_ac_c* actor = nullptr;
    fpc_ProcID id = fpcM_ERROR_PROCESS_ID_e;
    std::array<fpc_ProcID, 2> toadpoliIds{
        fpcM_ERROR_PROCESS_ID_e, fpcM_ERROR_PROCESS_ID_e};
    std::uint8_t deathSwordArrowHits = 0;
    bool waveFrameValid = false;
    float waveFrame = 0.0f;
};

struct ProcessFrame {
    fopAc_ac_c* actor = nullptr;
    cXyz position{};
    float speedF = 0.0f;
    s16 action = 0;
    s16 mode = 0;
};

struct DeathSwordDamageFrame {
    daE_VA_c* actor = nullptr;
    int action = 0;
    int mode = 0;
    u8 field1381 = 0;
    u8 glowBody = 0;
};

std::array<BossState, 16> s_bossStates{};
thread_local std::array<ProcessFrame, 32> s_processFrames{};
thread_local std::size_t s_processDepth = 0;
thread_local std::array<DeathSwordDamageFrame, 8> s_deathSwordDamageFrames{};
thread_local std::size_t s_deathSwordDamageDepth = 0;

BossState& state_for(fopAc_ac_c* actor) {
    const fpc_ProcID id = fopAcM_GetID(actor);
    for (auto& state : s_bossStates) {
        if (state.actor == actor && state.id == id) return state;
    }
    for (auto& state : s_bossStates) {
        if (state.actor == nullptr || fopAcM_SearchByID(state.id) != state.actor) {
            state = {};
            state.actor = actor;
            state.id = id;
            state.toadpoliIds = {
                fpcM_ERROR_PROCESS_ID_e, fpcM_ERROR_PROCESS_ID_e};
            return state;
        }
    }
    auto& state = s_bossStates[id % s_bossStates.size()];
    state = {};
    state.actor = actor;
    state.id = id;
    state.toadpoliIds = {
        fpcM_ERROR_PROCESS_ID_e, fpcM_ERROR_PROCESS_ID_e};
    return state;
}

bool is_supported_boss_actor(const s16 name) {
    switch (name) {
    case fpcNm_E_MK_BO_e:
    case fpcNm_B_BQ_e:
    case fpcNm_E_GOB_e:
    case fpcNm_E_FM_e:
    case fpcNm_E_VT_e:
        return true;
    default:
        return false;
    }
}

void advance_ook_boomerang(e_mk_bo_class& boomerang) {
    auto* actor = static_cast<fopAc_ac_c*>(&boomerang.enemy);
    auto* parent = fopAcM_SearchByID(actor->parentActorID);
    if (parent == nullptr || fopAcM_GetName(parent) != fpcNm_E_MK_e) return;

    auto* ook = reinterpret_cast<e_mk_class*>(parent);
    if (boomerang.action != 0 || ook->demoMode != e_mk_class::DEMO_MODE_NONE ||
        ook->action != e_mk_class::ACT_SHOOT || boomerang.mode < 0 || boomerang.mode > 3)
    {
        return;
    }

    if (boomerang.mode <= 1) {
        auto* player = dComIfGp_getPlayer(0);
        if (player == nullptr) return;
        cXyz target = player->current.pos;
        target.y += 100.0f;
        const cXyz delta = target - actor->current.pos;
        actor->current.angle.y = cM_atan2s(delta.x, delta.z);
        actor->current.angle.x = -cM_atan2s(
            delta.y, std::sqrt(delta.x * delta.x + delta.z * delta.z));
        boomerang.field_0x5e0 = target;
    }

    const float horizontal = cM_scos(actor->current.angle.x) * kOokBoomerangBaseSpeed;
    actor->current.pos.x += cM_ssin(actor->current.angle.y) * horizontal;
    actor->current.pos.y -= cM_ssin(actor->current.angle.x) * kOokBoomerangBaseSpeed;
    actor->current.pos.z += cM_scos(actor->current.angle.y) * horizontal;
}

void spawn_diababa_toadpoli(b_bq_class& diababa) {
    if (diababa.mAction != kDiababaActionWait || diababa.mDisableDraw) return;

    auto& state = state_for(&diababa);
    const float sinY = cM_ssin(diababa.shape_angle.y);
    const float cosY = cM_scos(diababa.shape_angle.y);
    auto* player = dComIfGp_getPlayer(0);

    for (std::size_t i = 0; i < state.toadpoliIds.size(); ++i) {
        if (state.toadpoliIds[i] != fpcM_ERROR_PROCESS_ID_e) continue;

        const float side = i == 0 ? -kDiababaToadpoliSideOffset
                                  : kDiababaToadpoliSideOffset;
        cXyz position = diababa.current.pos;
        position.x += side * cosY;
        position.z -= side * sinY;

        csXyz angle(0, diababa.shape_angle.y, 0);
        if (player != nullptr) {
            angle.y = cM_atan2s(
                player->current.pos.x - position.x,
                player->current.pos.z - position.z);
        }

        state.toadpoliIds[i] = fopAcM_createChild(
            fpcNm_E_TK_e, fopAcM_GetID(&diababa), kToadpoliNoPathParameters,
            &position, fopAcM_GetRoomNo(&diababa), &angle, nullptr, -1, nullptr);
    }
}

ProcessFrame snapshot_actor(fopAc_ac_c* actor) {
    ProcessFrame frame{};
    frame.actor = actor;
    frame.position = actor->current.pos;
    frame.speedF = actor->speedF;

    switch (fopAcM_GetName(actor)) {
    case fpcNm_B_BQ_e: {
        const auto& boss = *static_cast<b_bq_class*>(actor);
        frame.action = boss.mAction;
        frame.mode = boss.mMode;
        break;
    }
    case fpcNm_E_GOB_e: {
        const auto& boss = *static_cast<e_gob_class*>(actor);
        frame.action = boss.mAction;
        frame.mode = boss.mMode;
        break;
    }
    case fpcNm_E_FM_e: {
        const auto& boss = *static_cast<e_fm_class*>(actor);
        frame.action = boss.mAction;
        frame.mode = boss.mMode;
        auto& state = state_for(actor);
        if (boss.field_0x790 == 1 && boss.mpAttackEfModelMorf[1] != nullptr) {
            state.waveFrame = 0.0f;
            state.waveFrameValid = true;
        }
        break;
    }
    case fpcNm_E_VT_e: {
        const auto& boss = *static_cast<daE_VA_c*>(actor);
        frame.action = static_cast<s16>(boss.mAction);
        frame.mode = static_cast<s16>(boss.mMode);
        break;
    }
    default:
        break;
    }
    return frame;
}

void finish_diababa(const ProcessFrame& frame) {
    auto& boss = *static_cast<b_bq_class*>(frame.actor);
    spawn_diababa_toadpoli(boss);

    if (frame.action == kDiababaActionDamage && boss.mAction == kDiababaActionWait &&
        boss.mTimers[2] > 0)
    {
        // 80 frames in vanilla. Two thirds of the interval yields 1.5x frequency.
        boss.mTimers[2] = std::max<s16>(1, static_cast<s16>((boss.mTimers[2] * 2) / 3));
    }
}

void finish_dangoro(const ProcessFrame& frame) {
    auto& boss = *static_cast<e_gob_class*>(frame.actor);
    if (boss.mAction == kDangoroActionAttack &&
        (boss.mMode == 2 || boss.mMode == 12) && boss.mTimers[0] > 35)
    {
        boss.mTimers[0] = static_cast<s16>((boss.mTimers[0] + 1) / 2);
    }
    if (boss.mAction == kDangoroActionBall && boss.mMode == 2 && boss.mTimers[0] > 45) {
        boss.mTimers[0] = 45;
    }
}

void start_fyrus_wave(e_fm_class& boss, BossState& state) {
    boss.field_0x790 = 1;
    state.waveFrame = 0.0f;
    state.waveFrameValid = true;
}

void finish_fyrus(const ProcessFrame& frame) {
    auto& boss = *static_cast<e_fm_class*>(frame.actor);
    auto& state = state_for(frame.actor);

    if (frame.action == kFyrusActionDamageRun && frame.mode == 0 &&
        boss.mAction == kFyrusActionDamageRun && boss.mMode == 1 && boss.mTimers[0] > 0)
    {
        boss.mTimers[0] = std::max<s16>(1, static_cast<s16>((boss.mTimers[0] * 3) / 4));
    }

    const bool recoveredFromEyeHit =
        frame.action == kFyrusActionDamageRun && boss.mAction == kFyrusActionNormal;
    const bool stoodUp = frame.action == kFyrusActionDown && boss.mAction == kFyrusActionNormal;
    if (recoveredFromEyeHit || stoodUp) start_fyrus_wave(boss, state);

    if (boss.field_0x790 != 0 && boss.mpAttackEfModelMorf[1] != nullptr) {
        const float frameNow = boss.mpAttackEfModelMorf[1]->getFrame();
        if (state.waveFrameValid) {
            const float delta = frameNow - state.waveFrame;
            if (delta >= 0.0f && delta < 20.0f) {
                const float acceleratedFrame = frameNow + delta * 0.25f;
                // J3DFrameCtrl::setFrame is not exported by every PC platform
                // stub.  The field is public and this is exactly what the
                // console inline implementation does, so update it directly.
                boss.mpAttackEfModelMorf[1]->mFrameCtrl.mFrame = acceleratedFrame;
                state.waveFrame = acceleratedFrame;
            } else {
                state.waveFrame = frameNow;
            }
        } else {
            state.waveFrame = frameNow;
            state.waveFrameValid = true;
        }
    } else {
        state.waveFrameValid = false;
    }
}

void finish_actor(const ProcessFrame& frame) {
    if (frame.actor == nullptr) return;
    switch (fopAcM_GetName(frame.actor)) {
    case fpcNm_B_BQ_e:
        finish_diababa(frame);
        break;
    case fpcNm_E_GOB_e:
        finish_dangoro(frame);
        break;
    case fpcNm_E_FM_e:
        finish_fyrus(frame);
        break;
    default:
        break;
    }
}

float movement_scale_for(const ProcessFrame& frame) {
    if (frame.actor == nullptr) return 1.0f;
    switch (fopAcM_GetName(frame.actor)) {
    case fpcNm_E_GOB_e:
        return frame.action == kDangoroActionBall && frame.mode == 3 ? 1.3f : 1.0f;
    case fpcNm_E_FM_e:
        return frame.action == kFyrusActionNormal || frame.action == kFyrusActionFightRun ||
                frame.action == kFyrusActionDamageRun
            ? 1.5f : 1.0f;
    default:
        return 1.0f;
    }
}

HookAction before_process_method(ModContext*, void* args, void*, void*) {
    ProcessFrame frame{};
    void* process = mods::arg<void*>(args, 1);
    if (bossrush_hardmode_hazards_enabled() && process != nullptr &&
        fopAcM_IsActor(process))
    {
        auto* actor = static_cast<fopAc_ac_c*>(process);
        const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
        if (methods != nullptr && mods::arg<process_method_func>(args, 0) == methods->execute_method &&
            is_supported_boss_actor(fopAcM_GetName(actor)))
        {
            frame = snapshot_actor(actor);
            if (fopAcM_GetName(actor) == fpcNm_E_MK_BO_e) {
                advance_ook_boomerang(*reinterpret_cast<e_mk_bo_class*>(actor));
            }
        }
    }

    if (s_processDepth < s_processFrames.size()) s_processFrames[s_processDepth] = frame;
    ++s_processDepth;
    return HOOK_CONTINUE;
}

void after_process_method(ModContext*, void*, void*, void*) {
    if (s_processDepth == 0) return;
    --s_processDepth;
    if (s_processDepth < s_processFrames.size()) {
        finish_actor(s_processFrames[s_processDepth]);
        s_processFrames[s_processDepth] = {};
    }
}

HookAction before_collision(ModContext*, void* args, void*, void*) {
    if (!bossrush_hardmode_hazards_enabled() || s_processDepth == 0 ||
        s_processDepth > s_processFrames.size())
    {
        return HOOK_CONTINUE;
    }

    auto& frame = s_processFrames[s_processDepth - 1];
    if (frame.actor == nullptr) return HOOK_CONTINUE;

    auto* acch = mods::arg<dBgS_Acch*>(args, 0);
    if ((fopAcM_GetName(frame.actor) == fpcNm_E_GOB_e &&
         acch != &static_cast<e_gob_class*>(frame.actor)->mAcch) ||
        (fopAcM_GetName(frame.actor) == fpcNm_E_FM_e &&
         acch != &static_cast<e_fm_class*>(frame.actor)->mAcch))
    {
        return HOOK_CONTINUE;
    }

    const float scale = movement_scale_for(frame);
    if (scale > 1.0f) {
        const cXyz movement = frame.actor->current.pos - frame.position;
        frame.actor->current.pos.x += movement.x * (scale - 1.0f);
        frame.actor->current.pos.z += movement.z * (scale - 1.0f);
    }
    return HOOK_CONTINUE;
}

HookAction before_death_sword_damage(ModContext*, void* args, void*, void*) {
    DeathSwordDamageFrame frame{};
    auto* actor = mods::arg<daE_VA_c*>(args, 0);
    if (actor != nullptr) {
        frame.actor = actor;
        frame.action = actor->mAction;
        frame.mode = actor->mMode;
        frame.field1381 = actor->field_0x1381;
        frame.glowBody = actor->mGlowBody;
    }
    if (s_deathSwordDamageDepth < s_deathSwordDamageFrames.size()) {
        s_deathSwordDamageFrames[s_deathSwordDamageDepth] = frame;
    }
    ++s_deathSwordDamageDepth;
    return HOOK_CONTINUE;
}

void after_death_sword_damage(ModContext*, void* args, void*, void*) {
    if (s_deathSwordDamageDepth == 0) return;
    --s_deathSwordDamageDepth;
    if (s_deathSwordDamageDepth >= s_deathSwordDamageFrames.size()) return;

    const auto frame = s_deathSwordDamageFrames[s_deathSwordDamageDepth];
    s_deathSwordDamageFrames[s_deathSwordDamageDepth] = {};
    auto* actor = mods::arg<daE_VA_c*>(args, 0);
    if (!bossrush_hardmode_hazards_enabled() || actor == nullptr || actor != frame.actor ||
        frame.action != daE_VA_c::ACTION_OPACI_FLY_e ||
        actor->mAction != daE_VA_c::ACTION_OPACI_DAMAGE_e)
    {
        return;
    }

    const bool arrowHit = actor->mAtInfo.mpCollider != nullptr &&
        actor->mAtInfo.mpCollider->ChkAtType(AT_TYPE_ARROW);
    auto& state = state_for(actor);
    if (arrowHit && state.deathSwordArrowHits < 3) ++state.deathSwordArrowHits;

    if (state.deathSwordArrowHits < 3) {
        actor->mAction = frame.action;
        actor->mMode = frame.mode;
        actor->field_0x1381 = frame.field1381;
        actor->mGlowBody = frame.glowBody;
    } else {
        state.deathSwordArrowHits = 0;
    }
}

}  // namespace

ModResult install_boss_hard_mode_hooks(ModError* error) {
    ModResult result = mods::hook::add_pre<BossHardModeProcessHook>(
        svc_hook, before_process_method);
    if (result == MOD_OK) {
        result = mods::hook::add_post<BossHardModeProcessHook>(
            svc_hook, after_process_method);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<BossHardModeCollisionHook>(svc_hook, before_collision);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<DeathSwordDamageCheckHook>(
            svc_hook, before_death_sword_damage);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<DeathSwordDamageCheckHook>(
            svc_hook, after_death_sword_damage);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Boss Hard Mode hooks");
    }
    return MOD_OK;
}

}  // namespace dawnlight
