#include "profile.hpp"
#include "timing.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_bs.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureSound", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, u32, s8), SoundHook);

e_bs_class& stalchild(fopAc_ac_c* actor) {
    static_assert(offsetof(e_bs_class, enemy) == 0);
    return *reinterpret_cast<e_bs_class*>(actor);
}

bool eligible(fopAc_ac_c* actor) {
    return stalchild(actor).modelMorf != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = stalchild(step.actor);
    step.animations = {actor.modelMorf, actor.weponModelMorf};
    step.directCollision = &actor.acch;
    step.gravity = step.actor->gravity;
    step.chaseFloats = {&step.actor->speedF, &actor.field_0x6b8, &actor.field_0x690};
    step.chaseAngles = {&step.actor->current.angle.y, &step.actor->shape_angle.x,
        &step.actor->shape_angle.y, &actor.field_0x6aa, &actor.head_rot_z,
        &actor.field_0x6b2, &actor.head_rot_y};
    step.values[0] = actor.counter;
    if (!timerTick) {
        for (auto& timer : actor.timers) hold_enemy_timer(timer);
        hold_enemy_timer(actor.invulnerabilityTimer);
        actor.counter = static_cast<s16>(enemy_periodic_counter_input(actor.counter, timerTick));
    }
}

void before_collision(EnemySlowStep& step) {
    step.actor->speed.y = std::max(-60.0f, step.originalSpeed.y + step.gravity * step.scale);
}

void after_execute(EnemySlowStep& step) {
    if (!step.timerTick) stalchild(step.actor).counter = static_cast<s16>(step.values[0]);
}

HookAction before_sound(ModContext*, void* args, void* retval, void*) {
    auto* step = current_enemy_slow_step();
    if (step != nullptr && step->actor != nullptr && step->profile->name == fpcNm_E_BS_e &&
        !step->freshAnimationFrame && mods::arg<Z2CreatureEnemy*>(args, 0) == &stalchild(step->actor).sound &&
        mods::arg<JAISoundID>(args, 1) == Z2SE_EN_BS_ATTACK_SPEAR) {
        *static_cast<Z2SoundHandlePool**>(retval) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

ModResult install() {
    return mods::hook::add_pre<SoundHook>(svc_hook, before_sound);
}
}

const EnemySlowProfile& stalchild_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_BS_e, eligible, prepare, nullptr, nullptr, install, nullptr,
        before_collision, after_execute, true
    };
    return profile;
}
}
