#include "integration.hpp"
#include "d/actor/d_a_e_dd.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* base) {
    return reinterpret_cast<e_dd_class*>(base)->mpModelMorf != nullptr &&
        !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *reinterpret_cast<e_dd_class*>(step.actor);
    step.action = a.mAction;
    step.subaction = a.field_0x68c;
    step.animations = {a.mpModelMorf};
    step.sound = &a.mSound;
    step.frameSounds = {Z2SE_EN_DD_DIE_BOMB, Z2SE_CM_BODYFALL_M};
    for (std::size_t i = 0; i < 5; ++i)
        if (a.mpBrkAnms[i] != nullptr) step.controllers[i] = a.mpBrkAnms[i]->getFrameCtrl();
    step.directCollision = &a.mObjAcch;
    step.values[0] = a.field_0xe5c;
    step.chaseFloats = {&a.actor.speedF, &a.field_0x6c4};
    step.chaseAngles = {&a.actor.current.angle.y, &a.actor.shape_angle.y,
        &a.actor.shape_angle.x, &a.actor.shape_angle.z, &a.field_0x6b8, &a.field_0x6bc};
    if (!tick) {
        for (auto& timer : a.field_0x6aa) hold_enemy_timer(timer);
        hold_enemy_timer(a.field_0x6b2);
        hold_enemy_timer(a.field_0x6d3);
    }
}

void before_collision(EnemySlowStep& step) {
    slow_enemy_gravity(step, -5.0f, -100.0f, false);
}

void before_angle(EnemySlowStep& step, s16* value) {
    auto& a = *reinterpret_cast<e_dd_class*>(step.actor);
    // Wall walking has no CrrPos call. This common chase follows its integration.
    if (value == &a.actor.shape_angle.y && a.field_0x6d4 != 0 &&
        step.directCollision != nullptr) finish_enemy_translation(step);
}

void after_execute(EnemySlowStep& step) {
    auto& a = *reinterpret_cast<e_dd_class*>(step.actor);
    if (a.field_0xe59 != 0) {
        a.field_0xe5c = step.values[0] + 40.0f * step.scale;
        if (a.field_0xe5c >= 250.0f) a.field_0xe5c = 0.0f;
    }
}
}

const EnemySlowProfile& dodongo_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_DD_e, eligible, prepare, nullptr,
        nullptr, nullptr, nullptr, before_collision, after_execute, true, nullptr, before_angle};
    return profile;
}
}
