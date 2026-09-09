#include "profile.hpp"
#include "d/actor/d_a_e_tk2.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&fopAcM_createChild, CreateChildHook);

bool eligible(fopAc_ac_c* base) {
    return static_cast<e_tk2_class*>(base)->mpMorf != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<e_tk2_class*>(step.actor);
    step.animations = {actor.mpMorf};
    step.directCollision = &actor.mAcch;
    step.chaseFloats = {&actor.mAnimSpeed};
    step.chaseAngles = {&actor.shape_angle.y};
    step.action = actor.mAction;
    step.subaction = actor.mMode;
    if (!timerTick) {
        for (auto& timer : actor.mActionTimer) hold_enemy_timer(timer);
        hold_enemy_timer(actor.mInvincibilityTimer);
    }
}

bool repeated_attack(const EnemySlowStep& step) {
    const auto& actor = *static_cast<e_tk2_class*>(step.actor);
    return !step.freshAnimationFrame && step.action == 2 && step.subaction == 1 &&
        actor.mAction == step.action && actor.mMode == step.subaction;
}

HookAction before_create_child(ModContext*, void* args, void* retval, void*) {
    auto* step = current_enemy_slow_step();
    if (step != nullptr && step->actor != nullptr && step->profile != nullptr &&
        step->profile->name == fpcNm_E_TK2_e && repeated_attack(*step) &&
        mods::arg<s16>(args, 0) == fpcNm_E_TK_BALL_e &&
        mods::arg<fpc_ProcID>(args, 1) == fopAcM_GetID(step->actor)) {
        // Keep the existing suspended ball ID instead of replacing it with an error.
        *static_cast<fpc_ProcID*>(retval) = static_cast<e_tk2_class*>(step->actor)->mBallID;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void before_collision(EnemySlowStep& step) {
    // The body is anchored to the lava height. Only its animation/turning moves.
    step.directCollision = nullptr;
    if (repeated_attack(step)) static_cast<e_tk2_class*>(step.actor)->mTKBallSpawned = false;
}

ModResult install() {
    return mods::hook::add_pre<CreateChildHook>(svc_hook, before_create_child);
}
}

const EnemySlowProfile& fire_toadpoli_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_TK2_e, eligible, prepare, nullptr, nullptr, install, nullptr,
        before_collision, nullptr, true
    };
    return profile;
}
}
