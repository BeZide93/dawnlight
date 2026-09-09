#pragma once

#include "profile.hpp"
#include "timing.hpp"
#include "m_Do/m_Do_ext.h"
#include "SSystem/SComponent/c_math.h"
#include <algorithm>

namespace dawnlight {
template <typename T, std::size_t N, std::size_t M>
void own_enemy_values(std::array<T*, N>& owned, T (&values)[M]) {
    auto slot = std::find(owned.begin(), owned.end(), nullptr);
    for (auto& value : values) {
        if (slot == owned.end()) break;
        *slot++ = &value;
    }
}

template <std::size_t N>
void own_enemy_joints(EnemySlowStep& step, csXyz (&joints)[N]) {
    auto slot = std::find(step.chaseAngles.begin(), step.chaseAngles.end(), nullptr);
    for (auto& joint : joints) {
        if (step.chaseAngles.end() - slot < 3) break;
        *slot++ = &joint.x;
        *slot++ = &joint.y;
        *slot++ = &joint.z;
    }
}

inline void finish_enemy_translation(EnemySlowStep& step, const cXyz& offset = cXyz::Zero) {
    auto& position = step.actor->current.pos;
    position.x = slow_enemy_position_axis(step.originalPosition.x, position.x, offset.x, step.scale);
    position.y = slow_enemy_position_axis(step.originalPosition.y, position.y, offset.y, step.scale);
    position.z = slow_enemy_position_axis(step.originalPosition.z, position.z, offset.z, step.scale);
    step.directCollision = nullptr;
}

// Native direct integrators add gravity either before or after translation.
// Keep impulses intact; only remove the unused fraction of this tick's gravity.
inline void slow_enemy_gravity(EnemySlowStep& step, float gravity, float terminal, bool beforePosition) {
    auto& actor = *step.actor;
    if (actor.speed.y > terminal) actor.speed.y -= gravity * (1.0f - step.scale);
    if (beforePosition) actor.current.pos.y -= gravity * (1.0f - step.scale);
}

// Relax the live stem, not a draw-only copy: native colliders and line rendering
// receive these same positions in the remainder of the actor execute.
template <std::size_t N>
void slow_enemy_chain(EnemySlowStep& step, cXyz (&points)[N], csXyz (&angles)[N]) {
    static_assert(N <= std::tuple_size<decltype(step.points)>::value);
    for (std::size_t i = 1; i + 1 < N; ++i)
        points[i] = step.points[i] + (points[i] - step.points[i]) * step.scale;
    for (std::size_t i = 0; i + 1 < N; ++i) {
        const cXyz delta = points[i] - points[i + 1];
        angles[i].y = cM_atan2s(delta.x, delta.z);
        angles[i].x = -cM_atan2s(delta.y, std::sqrt(delta.x * delta.x + delta.z * delta.z));
    }
}

inline void set_enemy_stem_matrix(J3DModel* model, const cXyz& point, const csXyz& angle,
                                  s16 twist, float scale) {
    if (model == nullptr) return;
    mDoMtx_stack_c::transS(point.x, point.y, point.z);
    mDoMtx_stack_c::YrotM(angle.y);
    mDoMtx_stack_c::XrotM(angle.x);
    mDoMtx_stack_c::ZrotM(twist);
    mDoMtx_stack_c::scaleM(scale, scale, scale);
    model->setBaseTRMtx(mDoMtx_stack_c::get());
}
}
