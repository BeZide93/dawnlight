#include "integration.hpp"
#include "d/actor/d_a_e_dt.h"
#include "d/d_com_inf_game.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_DT_c::action, ActionHook);

bool eligible(fopAc_ac_c* actor) {
    const auto& boss = *static_cast<daE_DT_c*>(actor);
    // Native actions 9/10 own the death/opening camera and actor placement.
    return boss.mpMorf != nullptr && boss.mAction >= 0 && boss.mAction < 9 &&
           boss.mDemoMode == 0 && !boss.mLinkPressed && !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& boss = *static_cast<daE_DT_c*>(step.actor);
    step.animations[0] = boss.mpMorf;
    step.chaseFloats = {&boss.speedF, &boss.current.pos.x, &boss.current.pos.z,
        &boss.field_0x748, &boss.mSpitOffset};
    own_enemy_values(step.chaseFloats, boss.mSpitScale);
    own_enemy_values(step.chaseFloats, boss.mSpitFade);
    step.chaseAngles = {&boss.shape_angle.y, &boss.field_0x734,
        &boss.field_0x742, &boss.field_0x744, &boss.field_0x74e, &boss.field_0x752};
    own_enemy_joints(step, boss.mSpitAngle);
    if (!timerTick) {
        hold_enemy_timer(boss.mTimer);
        hold_enemy_timer(boss.mWalkTimer);
        hold_enemy_timer(boss.mDamageTimer);
        hold_enemy_timer(boss.mDemoTimer);
        hold_enemy_timer(boss.mQuakeTimer);
        hold_enemy_timer(boss.mBodyDamageTimer);
        hold_enemy_timer(boss.mLegLDamageTimer);
        hold_enemy_timer(boss.mLegRDamageTimer);
        hold_enemy_timer(boss.mSpitTimer);
        hold_enemy_timer(boss.field_0x714);
    }
}

HookAction before_action(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* boss = mods::arg<daE_DT_c*>(args, 0);
    if (step == nullptr || step->actor != boss) return HOOK_CONTINUE;
    // action() advances these three procedural damage oscillators before
    // drawing their joints. Damage may reset a phase later in this same call.
    const s16 correction = static_cast<s16>(std::lround(0x2000 * (1.0f - step->scale)));
    boss->field_0x74c -= correction;
    boss->field_0x750 -= correction;
    boss->field_0x754 -= correction;
    return HOOK_CONTINUE;
}

ModResult install() {
    return mods::hook::add_pre<ActionHook>(svc_hook, before_action);
}
}

const EnemySlowProfile& deku_toad_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_DT_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, nullptr, nullptr, true};
    return profile;
}
}
