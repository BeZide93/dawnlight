#pragma once
#include "d/actor/d_a_obj_gb.h"

namespace dawnlight {
// The original final-duel circle, also used by the Boss Rush hub.
inline constexpr u32 ganondorf_barrier_parameters=0xF0069600;
inline void configure_ganondorf_barrier(obj_gb_class* barrier) {
    barrier->mIsFinalBattle=0;
    barrier->mBrkFrame=29.f;
    barrier->mColorAlpha=0xF0;
    barrier->scale.x=1.5f;
    barrier->scale.y=1.f;
}
}
