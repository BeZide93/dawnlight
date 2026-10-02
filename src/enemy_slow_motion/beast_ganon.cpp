#include "boss.hpp"
#include "d/actor/d_a_b_mgn.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daB_MGN_c*>(actor);
    return a.mpMgnModelMorf && a.mActionMode != 0 && a.mActionMode != 3 && a.mActionMode != 6 && a.mActionMode != 9 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daB_MGN_c*>(step.actor);
    step.animations = {a.mpMgnModelMorf};
    own_boss_controller(step, a.mpMgnCoreBrk);
    own_boss_controller(step, a.mpMgnBtk);
    step.chaseFloats = {&a.field_0xadc, &a.field_0xae8, &a.mBlurRate, &a.mJewelColorStrength, &a.mKankyoBlend, &a.speedF};
    step.chaseAngles = {&a.current.angle.y, &a.field_0xa92, &a.field_0xae0, &a.field_0xae2, &a.field_0xb14, &a.field_0xb16, &a.shape_angle.y};
    own_enemy_values(step.chaseFloats, a.field_0x940);
    own_enemy_values(step.chaseAngles, a.mGdgateAngle);
    hold_boss_timers(tick, a.field_0xa9c, a.mDamageInvulnerabilityTimer, a.field_0xaa0, a.mBloodEffTimer, a.field_0xaa8, a.field_0xaac, a.mHeadHitEffTimer);
    for (std::size_t i = 0; i < 4; ++i) {
        step.animations[1 + i] = a.mpGdgateModelMorf[i];
        own_boss_controller(step, a.mpGdgateStartBrk[i]);
        own_boss_controller(step, a.mpGdgateAppearBrk[i]);
        own_boss_controller(step, a.mpGdgateBtk[i]);
    }
}
}
const EnemySlowProfile& beast_ganon_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_MGN_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
