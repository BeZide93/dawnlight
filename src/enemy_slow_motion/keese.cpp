#include "profile.hpp"
#include "timing.hpp"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_ba.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
e_ba_class* keese(fopAc_ac_c* base) {
    // The SDK declares this actor using composition, with mEnemy at offset zero.
    static_assert(offsetof(e_ba_class, mEnemy) == 0);
    return reinterpret_cast<e_ba_class*>(base);
}

bool attached(const e_ba_class* actor) {
    return actor->mAction == e_ba_class::ACT_WIND ||
        (actor->mAction == e_ba_class::ACT_WOLFBITE && actor->mMode < 2);
}

bool eligible(fopAc_ac_c* base) {
    return keese(base)->mpMorf != nullptr && !attached(keese(base));
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto* actor = keese(step.actor);
    step.action = actor->mAction;
    step.subaction = actor->mMode;
    step.animations = {actor->mpMorf};
    step.directCollision = &actor->mAcch;
    step.chaseFloats = {&step.actor->speedF, &actor->mSpeedRatio, &actor->mKnockbackSpeed};
    step.chaseAngles = {&step.actor->current.angle.x, &step.actor->current.angle.y,
        &step.actor->current.angle.z, &step.actor->shape_angle.x,
        &step.actor->shape_angle.y, &step.actor->shape_angle.z};
    step.values[0] = actor->mCounter;
    if (!timerTick) {
        for (auto& timer : actor->mTimer) hold_enemy_timer(timer);
        hold_enemy_timer(actor->mIFrames);
        // Only modulo-16/32 decisions read this counter. Expose an odd value
        // on fractional ticks, then restore the real slow counter after execute.
        actor->mCounter = static_cast<s16>(enemy_periodic_counter_input(actor->mCounter, timerTick));
    }
}

void before_collision(EnemySlowStep& step) {
    auto* actor = keese(step.actor);
    if (attached(actor)) {
        step.directCollision = nullptr;
        return;
    }
    // These two branches subtract fixed gravity after position integration.
    if (actor->mAction == e_ba_class::ACT_CHANCE) step.actor->speed.y += 2.0f * (1.0f - step.scale);
    if (actor->mAction == e_ba_class::ACT_WOLFBITE) step.actor->speed.y += 4.0f * (1.0f - step.scale);
    if (actor->mIsDying) {
        step.actor->shape_angle.y = slow_enemy_angle(step.originalShapeAngles.y, step.actor->shape_angle.y, step.scale);
        step.actor->shape_angle.z = slow_enemy_angle(step.originalShapeAngles.z, step.actor->shape_angle.z, step.scale);
    }
}

void after_execute(EnemySlowStep& step) {
    if (!step.timerTick) keese(step.actor)->mCounter = static_cast<s16>(step.values[0]);
}
}

const EnemySlowProfile& keese_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_BA_e, eligible, prepare, nullptr,
        nullptr, nullptr, nullptr, before_collision, after_execute, true};
    return profile;
}
}
