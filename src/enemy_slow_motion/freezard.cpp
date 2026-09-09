#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_fb.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_FB_c::execute, ExecuteHook);
DEFINE_HOOK(&daE_FB_c::executeWait, WaitHook);
DEFINE_HOOK(&daE_FB_c::executeAttack, AttackHook);
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureSound", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, u32, s8), SoundHook);

EnemySlowStep* freezard_step() {
    auto* step = current_enemy_slow_step();
    return step != nullptr && step->actor != nullptr && step->profile->name == fpcNm_E_FB_e ? step : nullptr;
}

bool eligible(fopAc_ac_c* actor) {
    auto& enemy = *static_cast<daE_FB_c*>(actor);
    return enemy.mpMorf != nullptr || enemy.mType == 10 || enemy.mType == 11;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<daE_FB_c*>(step.actor);
    step.animations[0] = actor.mpMorf;
    step.chaseAngles = {&actor.shape_angle.y, &actor.field_0x696, &actor.mHeadAngle};
    if (!timerTick) {
        hold_enemy_timer(actor.field_0x69c);
        hold_enemy_timer(actor.field_0x68c);
    }
}

HookAction before_wait(ModContext*, void* args, void*, void*) {
    auto* step = freezard_step();
    auto* actor = mods::arg<daE_FB_c*>(args, 0);
    if (step != nullptr && step->actor == actor && !step->timerTick) hold_enemy_timer(actor->field_0x680);
    return HOOK_CONTINUE;
}

HookAction before_attack(ModContext*, void* args, void*, void*) {
    auto* step = freezard_step();
    auto* actor = mods::arg<daE_FB_c*>(args, 0);
    if (step != nullptr && step->actor == actor && !step->timerTick && actor->mMoveMode != 0) {
        step->values[0] = 1.0f;
        step->values[1] = actor->field_0x68f;
        // Breath emitters keep following the jaw each frame. Only the native
        // alternating child-hitbox counter waits for the next simulation tick.
        actor->field_0x68f = 0;
    }
    return HOOK_CONTINUE;
}

void after_attack(ModContext*, void* args, void*, void*) {
    auto* step = freezard_step();
    auto* actor = mods::arg<daE_FB_c*>(args, 0);
    if (step != nullptr && step->actor == actor && step->values[0] != 0.0f && actor->mActionMode == 1) {
        actor->field_0x68f = static_cast<u8>(step->values[1]);
    }
}

void before_move(EnemySlowStep& step) {
    // Breath hitbox actors steer by directly subtracting an angle every tick.
    auto& actor = *static_cast<daE_FB_c*>(step.actor);
    actor.current.angle.y = slow_enemy_angle(step.originalAngles.y, actor.current.angle.y, step.scale);
}

HookAction before_sound(ModContext*, void* args, void* retval, void*) {
    auto* step = freezard_step();
    if (step != nullptr && !step->freshAnimationFrame &&
        mods::arg<Z2CreatureEnemy*>(args, 0) == &static_cast<daE_FB_c*>(step->actor)->mCreatureSound &&
        mods::arg<JAISoundID>(args, 1) == Z2SE_EN_FL_BLIZZARD_END) {
        *static_cast<Z2SoundHandlePool**>(retval) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

ModResult install() {
    auto result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_pre<WaitHook>(svc_hook, before_wait);
    if (result == MOD_OK) result = mods::hook::add_pre<AttackHook>(svc_hook, before_attack);
    if (result == MOD_OK) result = mods::hook::add_post<AttackHook>(svc_hook, after_attack);
    if (result == MOD_OK) result = mods::hook::add_pre<SoundHook>(svc_hook, before_sound);
    return result;
}
}

const EnemySlowProfile& freezard_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_FB_e, eligible, prepare, before_move, nullptr, install, nullptr
    };
    return profile;
}
}
