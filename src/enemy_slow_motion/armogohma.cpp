#include "boss.hpp"
#include "d/actor/d_a_b_gm.h"

namespace dawnlight {
namespace {
b_gm_class& boss(fopAc_ac_c* actor) {
    return *static_cast<b_gm_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boss(actor);
    return a.mpModelMorf && !a.mIsDisappear && a.mDemoMode == 0 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boss(step.actor);
    step.animations = {a.mpModelMorf, a.mpBeamModelMorf};
    own_boss_controller(step, a.mpZoomBtk);
    own_boss_controller(step, a.mpBeamBtk);
    own_boss_controller(step, a.mpSpotLightBtk);
    step.chaseFloats = {&a.speedF, &a.field_0x5d4, &a.field_0x6c0, &a.field_0x6c4, &a.mBodyColorIntensity, &a.mKankyoBlend, &a.mZoomBtkFrame};
    step.chaseAngles = {&a.current.angle.y, &a.field_0x1ad8, &a.field_0x1ada, &a.field_0x1adc, &a.field_0x6c8, &a.shape_angle.x, &a.shape_angle.y};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.mInvincibilityTimer);
    step.action = a.mAction;
    step.subaction = a.mMode;
}
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_b_gm.cpp#action", void(b_gm_class*), ActionHook);
void after_action(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<b_gm_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_GM_e);
    if (!step || !eligible(step->actor)) return;
    // State-entry placement is native; ordinary flight is integrated before
    // the base matrix and every attack/target collider in execute.
    if (step->action == a->mAction &&
        step->subaction == a->mMode) finish_enemy_translation(*step);
}
ModResult install() { return mods::hook::add_post<ActionHook>(svc_hook, after_action); }
}
const EnemySlowProfile& armogohma_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_GM_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, nullptr, nullptr, true};
    return profile;
}
}
