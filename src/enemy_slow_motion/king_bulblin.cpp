#include "boss.hpp"
#include "d/actor/d_a_e_rdb.h"

namespace dawnlight {
namespace {
e_rdb_class& boss(fopAc_ac_c* actor) {
    static_assert(offsetof(e_rdb_class, enemy) == 0);
    return *reinterpret_cast<e_rdb_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boss(actor);
    return a.mpModelMorf && a.mDemoMode == 0 && a.field_0xfe6 == 0 && a.mAction > 0 && a.mAction < 7 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boss(step.actor);
    step.animations = {a.mpModelMorf};
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.enemy.speedF, &a.field_0x5cc, &a.field_0x6e4, &a.mBlend};
    step.chaseAngles = {&a.enemy.current.angle.y, &a.enemy.shape_angle.x, &a.enemy.shape_angle.y, &a.enemy.shape_angle.z, &a.field_0x6a2, &a.field_0x6ca, &a.field_0x6cc};
    hold_boss_timer_array(tick, a.field_0x6b8);
    hold_boss_timers(tick, a.field_0x6c0, a.field_0x6c2);
}
void before_collision(EnemySlowStep& step) {
    if (!eligible(step.actor)) { step.directCollision = nullptr; return; }
    slow_enemy_gravity(step, -5.0f, -80.0f, false);
}
}
const EnemySlowProfile& king_bulblin_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_RDB_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, before_collision, nullptr, true};
    return profile;
}
}
