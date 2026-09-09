#include "integration.hpp"
#include "d/actor/d_a_e_rd.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
bool eligible(fopAc_ac_c* base) {
    const auto& a = *reinterpret_cast<e_rd_class*>(base);
    // Mounted King Bulblin and his scripted camera/boar paths are separate actors.
    return a.anm_p != nullptr && a.ride_mode == 0 && a.actor_set == 0 &&
        a.demo_mode == 0 && a.arg2 != 11 && a.field_0xaf0 == 0 &&
        a.action != 29 && !(a.action >= 40 && a.action <= 47) &&
        !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *reinterpret_cast<e_rd_class*>(step.actor);
    step.action = a.action;
    step.subaction = a.mode;
    step.animations = {a.anm_p, a.bow_anm, a.horn_anm};
    step.sound = &a.sound;
    step.frameSounds = {Z2SE_OBJ_ARROW_DRAW_NORMAL, Z2SE_EN_RD_V_RUNNING_BREATH,
        Z2SE_EN_RD_V_READY_WEAPON};
    step.directCollision = &a.Bgc;
    step.chaseFloats = {&a.enemy.speedF, &a.mount_jump_y, &a.field_0x9f0,
        &a.field_0xa24, &a.field_0xa2c, &a.field_0xab8, &a.field_0xaec, &a.field_0x96c};
    step.chaseAngles = {&a.enemy.current.angle.y, &a.enemy.shape_angle.x,
        &a.enemy.shape_angle.y, &a.enemy.shape_angle.z, &a.aim_angle_y, &a.aim_angle_x,
        &a.head_angle_y, &a.jump_angle.x, &a.field_0xa12.x, &a.field_0xa12.z,
        &a.field_0xade.x, &a.field_0xade.y, &a.field_0xaf8};
    own_enemy_joints(step, a.field_0xa32);
    own_enemy_joints(step, a.field_0xa74);
    if (!tick) {
        for (auto& timer : a.timer) hold_enemy_timer(timer);
        hold_enemy_timer(a.damage_timer);
        hold_enemy_timer(a.field_0xaf0);
        hold_enemy_timer(a.field_0x99a);
        hold_enemy_timer(a.attack_timer);
        hold_enemy_timer(a.yagura_timer);
        hold_enemy_timer(a.jump_timer);
        hold_enemy_timer(a.horn_timer);
        for (auto& timer : a.field_0xad2) hold_enemy_timer(timer);
        if (a.bow_anm != nullptr) hold_enemy_timer(a.bow_shake_timer);
    }
}

void before_collision(EnemySlowStep& step) {
    auto& a = *reinterpret_cast<e_rd_class*>(step.actor);
    if (a.ride_mode != 0) { step.directCollision = nullptr; return; }
    if (!step.freshAnimationFrame && a.action == step.action && a.mode == step.subaction)
        a.arrow_flag = 0;
    slow_enemy_gravity(step, a.enemy.gravity, -100.0f, false);
}
}

const EnemySlowProfile& bulblin_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_RD_e, eligible, prepare, nullptr,
        nullptr, nullptr, nullptr, before_collision, nullptr, true};
    return profile;
}
}
