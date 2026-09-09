#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_zs.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_ZS_c::execute, ExecuteHook);
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureVoice", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, s8), VoiceHook);

bool eligible(fopAc_ac_c* actor) {
    return static_cast<daE_ZS_c*>(actor)->mpMorf != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<daE_ZS_c*>(step.actor);
    step.animations[0] = actor.mpMorf;
    step.action = actor.mAction;
    step.subaction = actor.mMode;
    step.chaseFloats = {&actor.field_0x65c};
    step.chaseAngles = {&actor.current.angle.y};
    if (!timerTick) {
        hold_enemy_timer(actor.field_0x670);
        hold_enemy_timer(actor.field_0x671);
    }
}

HookAction before_voice(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor == nullptr || step->profile->name != fpcNm_E_ZS_e) return HOOK_CONTINUE;
    auto& actor = *static_cast<daE_ZS_c*>(step->actor);
    // Native wait checks int(frame)==0 instead of crossing frame zero.
    if (mods::arg<Z2CreatureEnemy*>(args, 0) == &actor.mSound &&
        actor.mAction == daE_ZS_c::ACT_WAIT && step->action == actor.mAction &&
        step->subaction == actor.mMode && !step->freshAnimationFrame) return HOOK_SKIP_ORIGINAL;
    return HOOK_CONTINUE;
}

ModResult install() {
    auto result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_pre<VoiceHook>(svc_hook, before_voice);
    return result;
}
}

const EnemySlowProfile& staltroop_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_ZS_e, eligible, prepare, nullptr, nullptr, install, nullptr
    };
    return profile;
}
}
