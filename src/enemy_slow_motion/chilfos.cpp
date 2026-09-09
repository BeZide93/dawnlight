#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_kk.h"
#include "d/d_com_inf_game.h"
#include "d/d_item.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_KK_c::execute, ExecuteHook);
DEFINE_HOOK(&daE_KK_c::executeAttack, AttackHook);
DEFINE_HOOK(&fopAcM_createChild, SpearHook);
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureSound", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, u32, s8), SoundHook);

EnemySlowStep* chilfos_step() {
    auto* step = current_enemy_slow_step();
    return step != nullptr && step->actor != nullptr && step->profile->name == fpcNm_E_KK_e ? step : nullptr;
}

bool eligible(fopAc_ac_c* actor) {
    auto& enemy = *static_cast<daE_KK_c*>(actor);
    return enemy.mpWeaponMorfSO != nullptr && (enemy.field_0x679 == 1 || enemy.mpMorfSO != nullptr) &&
        (enemy.field_0x679 != 2 || checkItemGet(dItemNo_IRONBALL_e, 1));
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<daE_KK_c*>(step.actor);
    step.animations = {actor.mpMorfSO, actor.mpWeaponMorfSO};
    step.action = actor.mActionMode;
    step.subaction = actor.mMoveMode;
    step.chaseFloats = {&actor.speedF};
    step.chaseAngles = {&actor.current.angle.x, &actor.current.angle.y,
                       &actor.shape_angle.y, &actor.shape_angle.z, &actor.field_0x758};
    if (!timerTick) {
        hold_enemy_timer(actor.mTimer);
        hold_enemy_timer(actor.field_0x672);
        hold_enemy_timer(actor.mDamageTimer);
    }
}

HookAction before_spear(ModContext*, void* args, void* retval, void*) {
    auto* step = chilfos_step();
    if (step != nullptr && !step->freshAnimationFrame &&
        mods::arg<s16>(args, 0) == fpcNm_E_KK_e &&
        mods::arg<fpc_ProcID>(args, 1) == fopAcM_GetID(step->actor) &&
        mods::arg<u32>(args, 2) == 0xFF0001) {
        // executeSpearThrow ignores the result. Do not create another spear on
        // each subframe of the native int(frame)==23 condition.
        *static_cast<fpc_ProcID*>(retval) = fpcM_ERROR_PROCESS_ID_e;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction before_sound(ModContext*, void* args, void* retval, void*) {
    auto* step = chilfos_step();
    if (step == nullptr || step->freshAnimationFrame) return HOOK_CONTINUE;
    auto& actor = *static_cast<daE_KK_c*>(step->actor);
    const JAISoundID id = mods::arg<JAISoundID>(args, 1);
    if (mods::arg<Z2CreatureEnemy*>(args, 0) == &actor.mCreatureSound &&
        step->action == actor.mActionMode && step->subaction == actor.mMoveMode &&
        (id == Z2SE_EN_KK_FOOTNOTE || id == Z2SE_EN_KK_THROW ||
         id == Z2SE_EN_KK_ATTACK01 || id == Z2SE_EN_KK_ATTACK03)) {
        *static_cast<Z2SoundHandlePool**>(retval) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction before_attack(ModContext*, void* args, void*, void*) {
    auto* step = chilfos_step();
    auto* actor = mods::arg<daE_KK_c*>(args, 0);
    if (step != nullptr && step->actor == actor && actor->mMoveMode == 3 &&
        static_cast<int>(actor->mpMorfSO->getFrame()) == 24 && !step->freshAnimationFrame) {
        step->values = {1.0f, actor->speedF, static_cast<float>(actor->field_0x67c),
                        static_cast<float>(actor->field_0x67e)};
    }
    return HOOK_CONTINUE;
}

void after_attack(ModContext*, void* args, void*, void*) {
    auto* step = chilfos_step();
    auto* actor = mods::arg<daE_KK_c*>(args, 0);
    if (step != nullptr && step->actor == actor && step->values[0] != 0.0f &&
        actor->mActionMode == 8 && actor->mMoveMode == 3 && actor->field_0x67c == step->values[2]) {
        // Preserve any new wall recoil; only remove a repeated frame-24 impulse.
        actor->speedF = step->values[1];
        actor->field_0x67e = static_cast<u8>(step->values[3]);
    }
}

ModResult install() {
    auto result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_pre<SpearHook>(svc_hook, before_spear);
    if (result == MOD_OK) result = mods::hook::add_pre<SoundHook>(svc_hook, before_sound);
    if (result == MOD_OK) result = mods::hook::add_pre<AttackHook>(svc_hook, before_attack);
    if (result == MOD_OK) result = mods::hook::add_post<AttackHook>(svc_hook, after_attack);
    return result;
}
}

const EnemySlowProfile& chilfos_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_KK_e, eligible, prepare, nullptr, nullptr, install, nullptr
    };
    return profile;
}
}
