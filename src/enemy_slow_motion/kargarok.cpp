#include "integration.hpp"
#include "d/actor/d_a_e_kr.h"
#include "d/d_s_play.h"
#include "SSystem/SComponent/c_lib.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&mDoExt_McaMorfSO::play, PlayHook);
DEFINE_HOOK(&cLib_addCalc2, HoverHook);
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureVoice", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, s8), VoiceHook);
e_kr_class& kargarok(fopAc_ac_c* base) {
    static_assert(offsetof(e_kr_class, enemy) == 0);
    return *reinterpret_cast<e_kr_class*>(base);
}
bool eligible(fopAc_ac_c* base) {
    const auto& a = kargarok(base);
    // Escort/path/bomb-carrying and mounted encounters have outside owners.
    // Death/Ending Blow retain their native fallback, as do those variants.
    return a.mpMorf && !a.field_0x6e4 && !a.field_0x66b && base->health > 0 &&
        (a.mCurAction == 0 || a.mCurAction == 3 || a.mCurAction == 9);
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = kargarok(step.actor);
    step.action = a.mCurAction;
    step.subaction = a.field_0x672;
    step.animations = {a.mpMorf};
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&step.actor->speedF, &a.field_0x68c, &a.field_0x690,
        &a.field_0xea8, &a.field_0xeb8, &a.field_0xef8};
    step.chaseAngles = {&step.actor->current.angle.x, &step.actor->current.angle.y,
        &step.actor->current.angle.z, &a.field_0x6ea.x, &a.field_0x6f0.x, &a.field_0x6f0.z,
        &a.field_0xe7c, &a.field_0xe80, &a.field_0xeae, &a.field_0xeb0, &a.field_0xeb6,
        &a.field_0xe8e[9], &a.field_0xe8e[10]};
    step.values[0] = a.field_0x6d6;
    step.values[1] = a.field_0x6d8;
    step.values[2] = a.field_0x69c[0];
    step.values[3] = a.field_0xe82;
    step.values[4] = 0;
    step.values[5] = a.field_0x6a8;
    if (!tick) {
        a.field_0x6d6 = static_cast<s16>(enemy_periodic_counter_input(a.field_0x6d6, false));
        for (auto& timer : a.field_0x69c) hold_enemy_timer(timer);
        hold_enemy_timer(a.field_0x6aa);
        hold_enemy_timer(a.field_0x6c8);
        hold_enemy_timer(a.field_0xe82);
        hold_enemy_timer(a.field_0x6a8);
        if (a.mCurAction == 9) hold_enemy_timer(a.field_0xebe);
        // Suppress repeat modulo/equality events, without hiding zero-time
        // state transitions. Restore only if the native action kept this value.
        const int t = static_cast<int>(step.values[2]);
        if (t > 0 && ((t & 31) == 0 || t == 70 + XREG_S(0))) {
            ++a.field_0x69c[0];
            step.values[4] = 1;
        }
    }
}
HookAction before_hover(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (!step || step->profile->name != fpcNm_E_KR_e) return HOOK_CONTINUE;
    auto& a = kargarok(step->actor);
    if (a.mCurAction != 3 || a.field_0x672 != 4) return HOOK_CONTINUE;
    const auto* value = mods::arg<float*>(args, 0);
    const float phase = step->values[1] + (step->timerTick ? 1.0f : 0.0f) + step->timerFraction;
    auto& target = mods::arg_ref<float>(args, 1);
    if (value == &step->actor->current.pos.x)
        target = a.field_0x678.x + 200.0f * cM_ssin(static_cast<s16>(static_cast<int>(phase * (TREG_S(2) + 1000))));
    else if (value == &step->actor->current.pos.y)
        target = a.field_0x678.y + 100.0f * cM_ssin(static_cast<s16>(static_cast<int>(phase * (TREG_S(3) + 0x4b0))));
    else if (value == &step->actor->current.pos.z)
        target = a.field_0x678.z + 200.0f * cM_scos(static_cast<s16>(static_cast<int>(phase * (TREG_S(4) + 0x5dc))));
    return HOOK_CONTINUE;
}
HookAction before_play(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (!step || step->profile->name != fpcNm_E_KR_e) return HOOK_CONTINUE;
    auto& a = kargarok(step->actor);
    if (mods::arg<mDoExt_McaMorfSO*>(args, 0) != a.mpMorf) return HOOK_CONTINUE;
    if (step->action == 9 && step->subaction <= 2 && a.field_0x672 < 3 && !a.mAcch.ChkGroundHit()) {
        slow_enemy_gravity(*step, -3.0f, -30.0f, false);
        if (step->subaction <= 1) {
            step->actor->current.angle.y -= static_cast<s16>(std::lround(a.field_0x6d4 * (1.0f - step->scale)));
            if (step->actor->speed.y < 0) {
                const float decay = std::pow(0.96f, step->scale) / 0.96f;
                step->actor->speed.x *= decay;
                step->actor->speed.z *= decay;
            }
        }
    }
    // Native Kargarok builds model/colliders BEFORE CrrPos. Finish flight now,
    // then let the shared collision hook scale only the subsequent CC push.
    finish_enemy_translation(*step);
    step->originalPosition = step->actor->current.pos;
    step->directCollision = &a.mAcch;
    if (!step->timerTick) {
        // The lift timer only decrements on branches calling pos_move.
        if (step->values[5] > 0 && a.field_0x6a8 == step->values[5] + 1)
            a.field_0x6a8 = static_cast<s16>(step->values[5]);
        if (a.field_0x6d8 == static_cast<s16>(static_cast<u16>(static_cast<s16>(step->values[1])) + 1))
            a.field_0x6d8 = static_cast<s16>(step->values[1]);
        if (step->values[4] && a.field_0x69c[0] == step->values[2] + 1)
            a.field_0x69c[0] = static_cast<s16>(step->values[2]);
    }
    return HOOK_CONTINUE;
}
HookAction before_voice(ModContext*, void* args, void* result, void*) {
    auto* step = current_enemy_slow_step();
    if (!step || step->profile->name != fpcNm_E_KR_e) return HOOK_CONTINUE;
    auto& a = kargarok(step->actor);
    if (mods::arg<Z2CreatureEnemy*>(args, 0) != &a.mSound) return HOOK_CONTINUE;
    const auto id = mods::arg<JAISoundID>(args, 1);
    const bool repeatedFrame = !step->freshAnimationFrame && id == Z2SE_EN_KR_V_FURA;
    const bool repeatedMouth = !step->timerTick && step->values[3] == a.field_0xe84 &&
        a.field_0xe82 == a.field_0xe84 && id == a.field_0xe88;
    if (repeatedFrame || repeatedMouth) {
        *static_cast<Z2SoundHandlePool**>(result) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_execute(EnemySlowStep& step) {
    auto& a = kargarok(step.actor);
    if (!step.timerTick) a.field_0x6d6 = static_cast<s16>(step.values[0]);
    const float phase = a.field_0x6d6 + step.timerFraction;
    for (int i = 0; i <= 8; ++i)
        a.field_0xe8e[i] = a.field_0xea8 * cM_ssin(static_cast<s16>(static_cast<int>(phase * (7000 + KREG_S(2)) + i * (11000 + KREG_S(3)))));
    a.field_0xeac = a.field_0xeb8 * cM_ssin(static_cast<s16>(static_cast<int>(phase * (NREG_S(2) + 7000))));
}
ModResult install() {
    auto result = mods::hook::add_pre<PlayHook>(svc_hook, before_play);
    if (result == MOD_OK) result = mods::hook::add_pre<HoverHook>(svc_hook, before_hover);
    if (result == MOD_OK) result = mods::hook::add_pre<VoiceHook>(svc_hook, before_voice);
    return result;
}
}
const EnemySlowProfile& kargarok_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_KR_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, nullptr, after_execute, true};
    return profile;
}
}
