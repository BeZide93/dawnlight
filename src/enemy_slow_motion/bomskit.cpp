#include "profile.hpp"
#include "timing.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_cr.h"

namespace dawnlight {
namespace {
e_cr_class& bomskit(fopAc_ac_c* base) {
    static_assert(offsetof(e_cr_class, enemy) == 0);
    return *reinterpret_cast<e_cr_class*>(base);
}

bool eligible(fopAc_ac_c* base) {
    return bomskit(base).modelMorf != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = bomskit(step.actor);
    step.animations = {actor.modelMorf};
    step.directCollision = &actor.acch;
    step.chaseFloats = {&step.actor->speedF};
    step.chaseAngles = {&step.actor->current.angle.y, &step.actor->shape_angle.y,
        &actor.field_0x68e, &actor.head_rot};
    step.values[0] = actor.lifetime;
    if (!timerTick) {
        for (auto& timer : actor.timers) hold_enemy_timer(timer);
        hold_enemy_timer(actor.invulnerabilityTimer);
        // Eggs are emitted on lifetime % 4. Fractional ticks must not lay extras.
        actor.lifetime = static_cast<s16>(enemy_periodic_counter_input(actor.lifetime, false));
    }
}

void before_collision(EnemySlowStep& step) {
    auto& actor = bomskit(step.actor);
    // Applied after native impulses, before collision can clear vertical speed.
    step.actor->speed.y += 5.0f * (1.0f - step.scale);
    if (actor.action == 10) {
        step.actor->shape_angle.y = slow_enemy_angle(step.originalShapeAngles.y,
            step.actor->shape_angle.y, step.scale);
    }
    if (!step.timerTick && actor.field_0x690 != 0) hold_enemy_timer(actor.field_0x692);
}

void after_execute(EnemySlowStep& step) {
    if (!step.timerTick) bomskit(step.actor).lifetime = static_cast<s16>(step.values[0]);
}
}

const EnemySlowProfile& bomskit_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_CR_e, eligible, prepare, nullptr, nullptr, nullptr, nullptr,
        before_collision, after_execute, true
    };
    return profile;
}
}
