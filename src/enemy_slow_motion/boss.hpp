#pragma once

#include "integration.hpp"
#include "d/d_com_inf_game.h"

namespace dawnlight {
bool boss_conditional_timers_available();
void install_boss_conditional_timers();
// Only enter an actor's native combat scope. In particular, a child executing
// inside a boss callback must never inherit the parent's physics corrections.
template <typename Actor>
EnemySlowStep* boss_step(Actor* actor, s16 name) {
    auto* step = current_enemy_slow_step();
    return step && step->profile->name == name &&
        static_cast<void*>(step->actor) == static_cast<void*>(actor) ? step : nullptr;
}

template <typename Animation>
void own_boss_controller(EnemySlowStep& step, Animation* animation) {
    if (!animation) return;
    auto slot = std::find(step.controllers.begin(), step.controllers.end(), nullptr);
    if (slot != step.controllers.end()) *slot = animation->getFrameCtrl();
}

template <typename... Timers>
void hold_boss_timers(bool tick, Timers&... timers) {
    if (!tick) (hold_enemy_timer(timers), ...);
}

template <typename Timer, std::size_t N>
void hold_boss_timer_array(bool tick, Timer (&timers)[N]) {
    if (!tick) for (auto& timer : timers) hold_enemy_timer(timer);
}

// Used at audited action boundaries, before the native base matrix and attack
// colliders are built. CrrPos may already have consumed directCollision.
inline void finish_boss_translation(EnemySlowStep& step) {
    if (step.directCollision) finish_enemy_translation(step);
}
}
