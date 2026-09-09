#include "profile.hpp"
#include "timing.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/d_s_play.h"
#include "d/d_bg_s_acch.h"
#include "d/d_cc_d.h"
#include "d/actor/d_a_e_sh.h"

namespace dawnlight {
namespace {
e_sh_class& stalhound(fopAc_ac_c* base) {
    static_assert(offsetof(e_sh_class, enemy) == 0);
    return *reinterpret_cast<e_sh_class*>(base);
}

bool eligible(fopAc_ac_c* base) {
    return stalhound(base).mAnm_p != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = stalhound(step.actor);
    step.animations = {actor.mAnm_p};
    step.directCollision = &actor.mObjAcch;
    step.action = actor.field_0x676;
    step.chaseFloats = {&step.actor->speedF, &actor.field_0x6b4,
        &actor.field_0x6a4, &actor.field_0x66c};
    step.chaseAngles = {&step.actor->current.angle.y, &step.actor->shape_angle.y,
        &step.actor->shape_angle.x, &actor.field_0x6ac, &actor.field_0x6ae};
    step.values[0] = actor.field_0x674;
    if (!timerTick) {
        for (auto& timer : actor.field_0x698) hold_enemy_timer(timer);
        hold_enemy_timer(actor.field_0x6a0);
        actor.field_0x674 = static_cast<s16>(enemy_periodic_counter_input(actor.field_0x674, false));
    }
}

void before_collision(EnemySlowStep& step) {
    auto& actor = stalhound(step.actor);
    if (step.action == 5 && actor.field_0x676 == 0) {
        // Disappearance relocates the actor to its spawn, not along a trajectory.
        step.directCollision = nullptr;
    } else {
        step.actor->speed.y += (JREG_F(5) + 5.0f) * (1.0f - step.scale);
    }
    // Damage may have started this mouth effect in the current execute.
    if (!step.timerTick) hold_enemy_timer(actor.field_0xceb);
}

void after_execute(EnemySlowStep& step) {
    if (!step.timerTick) stalhound(step.actor).field_0x674 = static_cast<s16>(step.values[0]);
}
}

const EnemySlowProfile& stalhound_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_SH_e, eligible, prepare, nullptr, nullptr, nullptr, nullptr,
        before_collision, after_execute, true
    };
    return profile;
}
}
