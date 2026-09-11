#pragma once
#include "integration.hpp"
#include "d/d_cc_d.h"

namespace dawnlight {
template <typename Actor>
void finish_baba_stem(EnemySlowStep& step, Actor& actor, cXyz (&points)[12],
                     csXyz (&angles)[12], bool detached = false) {
    std::array<cXyz, 12> nativePoints;
    std::copy(std::begin(points), std::end(points), nativePoints.begin());
    slow_enemy_chain(step, points, angles);
    if (detached) {
        points[11] = step.points[11] + (nativePoints[11] - step.points[11]) * step.scale;
        for (int i = 1; i < 12; ++i) {
            const cXyz delta = points[i] - points[i - 1];
            angles[i].y = cM_atan2s(delta.x, delta.z);
            angles[i].x = -cM_atan2s(delta.y, std::sqrt(delta.x * delta.x + delta.z * delta.z));
        }
    }
    auto* line = actor.stalkLine.getPos(0);
    for (int i = 0; i < 12; ++i) {
        line[i] = points[i];
#if TARGET_PC
        actor.mStalkLineInterpCurr[i] = points[i];
#endif
        if (i > 0 && i < 11)
            set_enemy_stem_matrix(actor.thornModel[i], points[i], angles[i], i << 13, actor.thorn_size[i]);
    }
    // cCcS::Set queues pointers; collision processing runs after actor execute.
    // Retain vanilla's hide offsets and only move the four stem sphere centers.
    for (int i = 0; i < 4; ++i) {
        const int segment = 3 + i * 2;
        auto& sphere = actor.kukiSph[i];
        sphere.SetC(sphere.GetC() + points[segment] - nativePoints[segment]);
    }
}
}
