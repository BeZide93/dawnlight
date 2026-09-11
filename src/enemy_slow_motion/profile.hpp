#pragma once

#include "../enemy_slow_motion.hpp"
#include "../service_imports.hpp"
#include "f_op/f_op_actor_mng.h"
#include "mods/svc/hook.hpp"
#include <array>
#include <cmath>
#include <limits>

class mDoExt_morf_c;
class dBgS_Acch;
class J3DFrameCtrl;
class Z2CreatureEnemy;

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
    bool timerTick = false;
    float timerFraction = 0.0f;
    bool freshAnimationFrame = false;
    cXyz originalPosition{};
    cXyz originalOldPosition{};
    csXyz originalAngles{};
    csXyz originalShapeAngles{};
    cXyz originalSpeed{};
    std::array<float, 16> values{};
    std::array<cXyz, 18> points{};
    std::array<float*, 32> chaseFloats{};
    std::array<s16*, 96> chaseAngles{};
    dBgS_Acch* directCollision = nullptr;
    std::array<mDoExt_morf_c*, 4> animations{};
    std::array<J3DFrameCtrl*, 8> controllers{};
    Z2CreatureEnemy* sound = nullptr;
    std::array<u32, 20> frameSounds{};
};

struct EnemySlowProfile {
    s16 name;
    bool (*eligible)(fopAc_ac_c*);
    void (*prepare)(EnemySlowStep&, bool timerTick);
    void (*beforeMove)(EnemySlowStep&);
    void (*observeExecute)(fopAc_ac_c*, float scale);
    ModResult (*install)();
    void (*reset)();
    void (*beforeCollision)(EnemySlowStep&) = nullptr;
    void (*afterExecute)(EnemySlowStep&) = nullptr;
    bool processExecute = false;
    void (*beforeFloatChase)(EnemySlowStep&, float*) = nullptr;
    void (*beforeAngleChase)(EnemySlowStep&, s16*) = nullptr;
};

// Returns only an active actor/profile pair, never an empty stack placeholder.
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
const EnemySlowProfile& keese_slow_profile();
const EnemySlowProfile& tektite_slow_profile();
const EnemySlowProfile& gibdo_slow_profile();
const EnemySlowProfile& goron_slow_profile();
const EnemySlowProfile& staltroop_slow_profile();
const EnemySlowProfile& aeralfos_slow_profile();
const EnemySlowProfile& chilfos_slow_profile();
const EnemySlowProfile& freezard_slow_profile();
const EnemySlowProfile& stalchild_slow_profile();
const EnemySlowProfile& bubble_slow_profile();
const EnemySlowProfile& rat_slow_profile();
const EnemySlowProfile& white_wolfos_slow_profile();
const EnemySlowProfile& puppet_slow_profile();
const EnemySlowProfile& bomskit_slow_profile();
const EnemySlowProfile& stalhound_slow_profile();
const EnemySlowProfile& fire_toadpoli_slow_profile();
const EnemySlowProfile& bulblin_slow_profile();
const EnemySlowProfile& lizalfos_slow_profile();
const EnemySlowProfile& dodongo_slow_profile();
const EnemySlowProfile& dynalfos_slow_profile();
const EnemySlowProfile& skulltula_slow_profile();
const EnemySlowProfile& baba_serpent_slow_profile();
const EnemySlowProfile& big_baba_slow_profile();
const EnemySlowProfile& deku_baba_slow_profile();
const EnemySlowProfile& stalfos_slow_profile();
}
