#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_b_gg.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daB_GG_c::Execute, ExecuteHook);
DEFINE_HOOK(&daB_GG_c::F_AtHit, AttackCooldownHook);

bool eligible(fopAc_ac_c* actor) {
    auto& enemy = *static_cast<daB_GG_c*>(actor);
    // Demo/camera/death paths also manipulate Link, detached parts and scene state.
    return enemy.mpModelMorf != nullptr && enemy.mAction <= 2 && enemy.mSubAction != 4 &&
           enemy.mCamMode == 0 && enemy.field_0x5b1 != 1;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<daB_GG_c*>(step.actor);
    step.animations[0] = actor.mpModelMorf;
    step.chaseFloats = {&actor.speedF, &actor.speed.y, &actor.mModelPlaySpeed};
    step.chaseAngles = {&actor.current.angle.y, &actor.shape_angle.y,
                       &actor.field_0x6c4, &actor.field_0x6be};
    if (!timerTick) {
        for (auto& timer : actor.mTimers) hold_enemy_timer(timer);
        hold_enemy_timer(actor.field_0x5cc);
    }
    // Clawshot transport uses direct positioning; only the native velocity
    // integration is scaled, never the attachment point supplied by Link.
}

HookAction before_attack_cooldown(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* actor = mods::arg<daB_GG_c*>(args, 0);
    if (step != nullptr && step->actor == actor && !step->timerTick) {
        hold_enemy_timer(actor->field_0x65a);
    }
    return HOOK_CONTINUE;
}

ModResult install() {
    auto result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_pre<AttackCooldownHook>(svc_hook, before_attack_cooldown);
    return result;
}
}

const EnemySlowProfile& aeralfos_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_B_GG_e, eligible, prepare, nullptr, nullptr, install, nullptr
    };
    return profile;
}
}
