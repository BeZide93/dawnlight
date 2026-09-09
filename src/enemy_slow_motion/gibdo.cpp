#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_gi.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_GI_c::execute, ExecuteHook);

bool eligible(fopAc_ac_c* actor) {
    return static_cast<daE_GI_c*>(actor)->mpModelMorf != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<daE_GI_c*>(step.actor);
    step.animations[0] = actor.mpModelMorf;
    step.chaseFloats = {&actor.speedF, &actor.mWallCheckRadius, &actor.mBodyDamageColor};
    step.chaseAngles = {&actor.shape_angle.y, &actor.field_0x67e, &actor.field_0x6a2};
    if (!timerTick) {
        hold_enemy_timer(actor.field_0x684);
        hold_enemy_timer(actor.field_0x688);
        hold_enemy_timer(actor.mInvulnerabilityTimer);
        hold_enemy_timer(actor.mContinuousHitTimer);
    }
    // Stun/cry ownership and button-mashing belong to Link's real-time input,
    // not the enemy's animation clock. Leave their timers and release logic native.
}
}

const EnemySlowProfile& gibdo_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_GI_e, eligible, prepare, nullptr, nullptr,
        install_enemy_execute_hook<ExecuteHook>, nullptr
    };
    return profile;
}
}
