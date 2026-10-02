#include "boss.hpp"
#include "d/actor/d_a_e_mk.h"
#include "d/d_s_play.h"

namespace dawnlight {
namespace {
e_mk_class& boss(fopAc_ac_c* actor) {
    static_assert(offsetof(e_mk_class, actor) == 0);
    return *reinterpret_cast<e_mk_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boss(actor);
    return a.anmP && a.demoMode == 0 && a.action < 20 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boss(step.actor);
    step.animations = {a.anmP, a.crownAnmP};
    own_boss_controller(step, a.btpP);
    step.directCollision = &a.acch;
    step.chaseFloats = {&a.actor.speedF, &a.field_0x604};
    step.chaseAngles = {&a.actor.current.angle.y, &a.actor.shape_angle.y, &a.unkRotation.x};
    hold_boss_timer_array(tick, a.timer);
    hold_boss_timers(tick, a.invulnerabilityTimer, a.unkTimer1);
    step.action = a.action;
    step.subaction = a.mode;
    step.values[4] = a.unkFlag4;
    step.points[2] = a.prevPos;
    step.points[3] = a.prevPosTarget;
}
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mk.cpp#action", void(e_mk_class*), ActionHook);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mk.cpp#e_mk_move", void(e_mk_class*), JumpHook);
void after_jump(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_E_MK_e);
    if (step && !step->freshAnimationFrame && a->mode == 3 &&
        static_cast<int>(a->anmP->getFrame()) == TREG_S(0) + 9) {
        a->actor.speed = step->originalSpeed;
        a->prevPos = step->points[2];
        a->prevPosTarget = step->points[3];
        a->setSmokeFlag = 0;
    }
}
void after_action(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_mk_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_E_MK_e);
    if (!step || !eligible(step->actor)) return;
    if (!step->freshAnimationFrame && a->action == e_mk_class::ACT_SHOOT &&
        step->action == a->action && step->subaction == a->mode && a->unkFlag4 == 7)
        a->unkFlag4 = static_cast<s8>(step->values[4]);
    if (!step->directCollision) return; // Ground correction already consumed it.
    if (a->unkFlag3 != 0)
        slow_enemy_gravity(*step, step->actor->gravity, -std::numeric_limits<float>::infinity(), false);
    // State-entry placement is native; ordinary flight is integrated before
    // the base matrix and every attack/target collider in execute.
    if (step->action == a->action &&
        step->subaction == a->mode) finish_enemy_translation(*step);
}
ModResult install() {
    auto result = mods::hook::add_post<ActionHook>(svc_hook, after_action);
    if (result == MOD_OK) result = mods::hook::add_post<JumpHook>(svc_hook, after_jump);
    return result;
}
void before_collision(EnemySlowStep& step) {
    if (!eligible(step.actor)) { step.directCollision = nullptr; return; }
    slow_enemy_gravity(step, step.actor->gravity, -std::numeric_limits<float>::infinity(), false);
}
}
const EnemySlowProfile& ook_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_MK_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, before_collision, nullptr, true};
    return profile;
}
}
