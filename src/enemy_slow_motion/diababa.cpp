#include "boss.hpp"
#include "d/actor/d_a_b_bq.h"

namespace dawnlight {
namespace {
b_bq_class& boss(fopAc_ac_c* actor) {
    return *static_cast<b_bq_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boss(actor);
    return a.mpMorf && a.mDemoMode == 0 && a.mAction > 0 && a.mAction < 4 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boss(step.actor);
    step.animations = {a.mpMorf};
    own_boss_controller(step, a.mpDeadBrk);
    own_boss_controller(step, a.mpTodomeBtk);
    step.chaseFloats = {&a.field_0x1178, &a.field_0x11d8, &a.mColpatBlend, &a.mDeadColor};
    step.chaseAngles = {&a.field_0x138c, &a.field_0x138e, &a.field_0x5dc, &a.field_0x6f6, &a.mBlureRate, &a.mHeadRot};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.field_0x6de, a.field_0x6fc, a.field_0x6fe);
}
}
const EnemySlowProfile& diababa_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_BQ_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
