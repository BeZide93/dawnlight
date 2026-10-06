#include "boss.hpp"
#include "d/actor/d_a_e_vt.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daE_VA_c*>(actor);
    return a.mpMorf && a.mAction >= daE_VA_c::ACTION_OPACI_FLY_e && a.mAction < daE_VA_c::ACTION_OPACI_DEATH_e && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daE_VA_c*>(step.actor);
    step.animations = {a.mpMorf, a.mpEndEfMorf};
    own_boss_controller(step, a.mpWeaponBrk);
    own_boss_controller(step, a.mpEndEfBrk);
    step.chaseFloats = {&a.current.pos.y, &a.field_0x14a8, &a.field_0x14b0, &a.speed.y, &a.speedF};
    step.chaseAngles = {&a.current.angle.y, &a.field_0x1238.x, &a.field_0x1238.y, &a.field_0x1238.z, &a.field_0x1304.x, &a.field_0x1304.z, &a.field_0x1334, &a.field_0x1336, &a.field_0x1394, &a.field_0x1396, &a.field_0x1398, &a.field_0x14ac, &a.shape_angle.y};
    step.chasePositions = {&a.field_0x122c};
    own_enemy_values(step.chaseFloats, a.field_0x1104);
    own_enemy_values(step.chasePositions, a.field_0x1140);
    hold_boss_timers(tick, a.mDemoModeTimer, a.mDownTimer, a.mFadeAwayTimer, a.field_0x1350, a.mAttackSphIFrameTimer, a.mNeckSphIFrameTimer, a.mBodyCylIFrameTimer, a.mOffTgTimer, a.field_0x1348, a.field_0x1354, a.field_0x1358);
}
void before_magic_collision(EnemySlowStep& step, dBgS_Acch* collision) {
    auto& a = *static_cast<daE_VA_c*>(step.actor);
    for (int i = 0; i < 2; ++i) {
        if (collision == &a.mMagicAcch[i]) {
            // calcMagicMove sets OldPos immediately before velocity integration,
            // including a newly released bolt. Do not interpolate its spawn point.
            a.mMagicPos[i] = a.mMagicOldPos[i] +
                (a.mMagicPos[i] - a.mMagicOldPos[i]) * step.scale;
        }
    }
}
}
const EnemySlowProfile& death_sword_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_VT_e, eligible, prepare,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true,
        nullptr, nullptr, before_magic_collision};
    return profile;
}
}
