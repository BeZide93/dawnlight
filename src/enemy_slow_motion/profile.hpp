#pragma once

#include "../enemy_slow_motion.hpp"
#include "../service_imports.hpp"
#include "f_op/f_op_actor_mng.h"
#include "mods/svc/hook.hpp"
#include <array>
#include <cmath>
#include <limits>

class mDoExt_morf_c;

namespace dawnlight {
struct EnemySlowProfile;
struct EnemySlowStep {
    fopAc_ac_c* actor = nullptr;
    const EnemySlowProfile* profile = nullptr;
    float scale = 1.0f;
    int action = 0;
    int subaction = 0;
    s16 facing = 0;
    s16 neck = 0;
    s16 waist = 0;
    float speed = 0.0f;
    cXyz position{};
    float gravity = 0.0f;
    bool moving = false;
    std::array<mDoExt_morf_c*, 4> animations{};
};

struct EnemySlowProfile {
    s16 name;
    bool (*eligible)(fopAc_ac_c*);
    void (*prepare)(EnemySlowStep&, bool timerTick);
    void (*beforeMove)(EnemySlowStep&);
    void (*observeExecute)(fopAc_ac_c*, float scale);
    ModResult (*install)();
    void (*reset)();
};

EnemySlowStep* current_enemy_slow_step();
HookAction before_enemy_slow_execute(ModContext*, void*, void*, void*);
void after_enemy_slow_execute(ModContext*, void*, void*, void*);

template <typename Hook>
ModResult install_enemy_execute_hook() {
    ModResult result = mods::hook::add_pre<Hook>(svc_hook, before_enemy_slow_execute);
    if (result == MOD_OK) {
        result = mods::hook::add_post<Hook>(svc_hook, after_enemy_slow_execute);
    }
    return result;
}

template <typename T>
void hold_enemy_timer(T& timer) {
    // Only for audited unconditional decrements in the upcoming execute.
    if (timer != 0 && timer < std::numeric_limits<T>::max()) ++timer;
}

inline s16 slow_enemy_angle(s16 before, s16 after, float scale) {
    return static_cast<s16>(before + std::lround(static_cast<s16>(after - before) * scale));
}

inline void slow_enemy_steering(EnemySlowStep& step, bool sameState) {
    if (!sameState) return;
    auto* actor = step.actor;
    const s16 desiredFacing = actor->shape_angle.y;
    actor->shape_angle.y = slow_enemy_angle(step.facing, desiredFacing, step.scale);
    actor->current.angle.y += static_cast<s16>(actor->shape_angle.y - desiredFacing);
    actor->speedF = step.speed + (actor->speedF - step.speed) * step.scale;
}

const EnemySlowProfile& darknut_slow_profile();
const EnemySlowProfile& bokoblin_slow_profile();
const EnemySlowProfile& mini_freezard_slow_profile();
}
