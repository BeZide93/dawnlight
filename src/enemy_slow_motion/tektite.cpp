#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_tt.h"
#include "SSystem/SComponent/c_lib.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_TT_c::execute, ExecuteHook);
DEFINE_HOOK(&cLib_chaseF, OffsetVelocityHook);

bool eligible(fopAc_ac_c* actor) {
    return static_cast<daE_TT_c*>(actor)->mpMorfSO != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<daE_TT_c*>(step.actor);
    step.animations[0] = actor.mpMorfSO;
    step.chaseFloats = {&actor.speedF, &actor.mDeathColor, &actor.mTransOffsetVelocity};
    step.chaseAngles = {&actor.shape_angle.y, &actor.mFootJoints[0], &actor.mFootJoints[1],
                       &actor.mFootJoints[2], &actor.mFootJoints[3]};
    if (!timerTick) {
        hold_enemy_timer(actor.mGenericTimer);
        hold_enemy_timer(actor.mAttackTimer);
        hold_enemy_timer(actor.mDamageCooldownTimer);
        hold_enemy_timer(actor.mPlayerCutTimer);
    }
}

// The native action integrates the water-landing body offset after this chase.
// Correct it before posMoveF/model/collision, retaining the landing impulse.
void after_offset_velocity(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor == nullptr || step->profile->name != fpcNm_E_TT_e) return;
    auto& actor = *static_cast<daE_TT_c*>(step->actor);
    if (mods::arg<float*>(args, 0) == &actor.mTransOffsetVelocity) {
        step->values[0] = actor.mTransOffset;
        step->values[1] = actor.mTransOffsetVelocity;
    }
}

void before_move(EnemySlowStep& step) {
    auto& actor = *static_cast<daE_TT_c*>(step.actor);
    actor.mTransOffset = std::min(0.0f, step.values[0] + step.values[1] * step.scale);
    actor.mTransOffsetVelocity = actor.mTransOffset < 0.0f ? step.values[1] : 0.0f;
}

ModResult install() {
    auto result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_post<OffsetVelocityHook>(svc_hook, after_offset_velocity);
    return result;
}
}

const EnemySlowProfile& tektite_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_TT_e, eligible, prepare, before_move, nullptr, install, nullptr
    };
    return profile;
}
}
