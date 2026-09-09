#include "profile.hpp"
#include "timing.hpp"
#include "SSystem/SComponent/c_lib.h"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_ms.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(static_cast<void (*)(s16*, s16, s16, s16)>(&cLib_addCalcAngleS2), TurnHook);

bool eligible(fopAc_ac_c* base) {
    const auto& actor = *static_cast<e_ms_class*>(base);
    return actor.mpModelMorf != nullptr && actor.mAction != 5;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<e_ms_class*>(step.actor);
    step.animations = {actor.mpModelMorf};
    step.directCollision = &actor.mAcch;
    step.chaseFloats = {&actor.speedF, &actor.field_0x694};
    step.chaseAngles = {&actor.current.angle.y, &actor.shape_angle.x,
        &actor.shape_angle.y, &actor.shape_angle.z};
    step.values[0] = actor.mLifetime;
    if (!timerTick) {
        for (auto& timer : actor.mActionTimer) hold_enemy_timer(timer);
        hold_enemy_timer(actor.mCooldown1);
        hold_enemy_timer(actor.mCooldown2);
        actor.mLifetime = static_cast<s16>(enemy_periodic_counter_input(actor.mLifetime, false));
    }
    step.values[1] = static_cast<s16>(actor.mLifetime + 1);
}

HookAction before_physics(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step != nullptr && step->actor != nullptr && step->profile != nullptr &&
        step->profile->name == fpcNm_E_MS_e) {
        auto& actor = *static_cast<e_ms_class*>(step->actor);
        if (mods::arg<s16*>(args, 0) == &actor.shape_angle.z && actor.mAction != 5) {
            // This turn is immediately before native integration. Preserve new
            // attack impulses and the zero gravity selected while swimming.
            actor.gravity *= step->scale;
        }
    }
    return HOOK_CONTINUE;
}

void before_collision(EnemySlowStep& step) {
    if (static_cast<e_ms_class*>(step.actor)->mAction == 5) step.directCollision = nullptr;
}

void after_execute(EnemySlowStep& step) {
    auto& actor = *static_cast<e_ms_class*>(step.actor);
    // Normal-state initialization can reseed this counter. Do not undo it.
    if (!step.timerTick && actor.mLifetime == static_cast<s16>(step.values[1])) {
        actor.mLifetime = static_cast<s16>(step.values[0]);
    }
}

ModResult install() {
    return mods::hook::add_pre<TurnHook>(svc_hook, before_physics);
}
}

const EnemySlowProfile& rat_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_MS_e, eligible, prepare, nullptr, nullptr, install, nullptr,
        before_collision, after_execute, true
    };
    return profile;
}
}
