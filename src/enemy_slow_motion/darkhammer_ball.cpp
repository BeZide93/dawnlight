#include "boss.hpp"
#include "d/actor/d_a_e_th_ball.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    [[maybe_unused]] const auto& a = *static_cast<e_th_ball_class*>(actor);
    return (a.mpBallModel && !a.mPlayerGet && a.mDemoMode == 0 && fopAcM_SearchByID(a.parentActorID)) && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<e_th_ball_class*>(step.actor);
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.current.pos.x, &a.current.pos.y, &a.current.pos.z, &a.field_0xdc8, &a.field_0xdd4, &a.field_0xde4, &a.speedF};
    step.chaseAngles = {&a.current.angle.x, &a.current.angle.y, &a.field_0x15c4, &a.field_0xdd0, &a.shape_angle.x, &a.shape_angle.y};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.field_0x658);
}
void before_collision(EnemySlowStep& step) {
    slow_enemy_gravity(step, -5.0f, -std::numeric_limits<float>::infinity(), false);
}
}
const EnemySlowProfile& darkhammer_ball_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_TH_BALL_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, before_collision, nullptr, true};
    return profile;
}
}
