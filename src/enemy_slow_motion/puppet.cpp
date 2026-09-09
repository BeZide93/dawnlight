#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_fs.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureSound", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, u32, s8), SoundHook);

e_fs_class& puppet(fopAc_ac_c* base) {
    static_assert(offsetof(e_fs_class, mEnemy) == 0);
    return *reinterpret_cast<e_fs_class*>(base);
}

bool eligible(fopAc_ac_c* base) {
    const auto& actor = puppet(base);
    return actor.mpMorf != nullptr && actor.mAction != e_fs_class::ACT_DEMOWAIT &&
        !(actor.mAction == e_fs_class::ACT_APPEAR && actor.mMode < 2);
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = puppet(step.actor);
    step.animations = {actor.mpMorf};
    step.directCollision = &actor.mAcch;
    step.gravity = step.actor->gravity;
    step.chaseFloats = {&step.actor->speedF};
    step.chaseAngles = {&step.actor->current.angle.y};
    step.action = actor.mAction;
    step.subaction = actor.mMode;
    if (!timerTick) {
        for (auto& timer : actor.mTimer) hold_enemy_timer(timer);
        hold_enemy_timer(actor.mIFrameTimer);
    }
}

bool repeated_attack(const EnemySlowStep& step) {
    const auto& actor = puppet(step.actor);
    return !step.freshAnimationFrame && step.action == e_fs_class::ACT_ATTACK &&
        step.subaction == 1 && actor.mAction == step.action && actor.mMode == step.subaction;
}

void before_collision(EnemySlowStep& step) {
    auto& actor = puppet(step.actor);
    if (!eligible(step.actor)) {
        step.directCollision = nullptr;
        return;
    }
    step.actor->speed.y = std::max(-80.0f, step.originalSpeed.y + step.gravity * step.scale);
    // Start the swept attack once, then follow the hand through frame 31.
    if (actor.field_0x692 == 1 && repeated_attack(step)) actor.field_0x692 = 2;
}

HookAction before_sound(ModContext*, void* args, void* retval, void*) {
    auto* step = current_enemy_slow_step();
    if (step != nullptr && step->actor != nullptr && step->profile != nullptr &&
        step->profile->name == fpcNm_E_FS_e && repeated_attack(*step) &&
        mods::arg<Z2CreatureEnemy*>(args, 0) == &puppet(step->actor).mCreatureSound &&
        mods::arg<JAISoundID>(args, 1) == Z2SE_EN_FS_ATTACK) {
        *static_cast<Z2SoundHandlePool**>(retval) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

ModResult install() {
    return mods::hook::add_pre<SoundHook>(svc_hook, before_sound);
}
}

const EnemySlowProfile& puppet_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_FS_e, eligible, prepare, nullptr, nullptr, install, nullptr,
        before_collision, nullptr, true
    };
    return profile;
}
}
