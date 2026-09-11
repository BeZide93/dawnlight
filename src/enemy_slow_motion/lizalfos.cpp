#include "integration.hpp"
#include "d/actor/d_a_e_dn.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("e_dn_fight_run", void(e_dn_class*), FightRunHook);
bool eligible(fopAc_ac_c* base) {
    const auto& a = *reinterpret_cast<e_dn_class*>(base);
    return a.anm_p != nullptr && a.status == 0 && a.unk_timer_5 == 0 &&
        a.action != 24 && a.action != 60 && !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *reinterpret_cast<e_dn_class*>(step.actor);
    step.action = a.action;
    step.subaction = a.mode;
    step.animations = {a.anm_p};
    step.sound = &a.sound;
    step.frameSounds = {Z2SE_EN_DN_TAIL, Z2SE_EN_DN_V_TAIL, Z2SE_EN_DN_KNIFE,
        Z2SE_EN_DN_KNIFE2_A, Z2SE_EN_DN_KNIFE2_B, Z2SE_EN_DN_FN_L, Z2SE_EN_DN_FN_R,
        Z2SE_EN_DN_FN_RUN_L, Z2SE_EN_DN_FN_RUN_R, Z2SE_EN_DN_V_BREATH,
        Z2SE_EN_DN_V_SEARCH, Z2SE_EN_DN_V_SEARCH2};
    step.directCollision = &a.objacch;
    step.chaseFloats = {&a.actor.speedF, &a.cur_pos_y_offset, &a.field_0x708,
        &a.field_0x74c, &a.field_0x754, &a.field_0x75c, &a.field_0x7e8, &a.field_0x83c, &a.color};
    step.chaseAngles = {&a.actor.current.angle.y, &a.actor.shape_angle.x,
        &a.actor.shape_angle.y, &a.actor.shape_angle.z, &a.field_0x6f6, &a.field_0x6f8,
        &a.field_0x6fa, &a.field_0x724.x, &a.field_0x72a.x, &a.field_0x72a.z,
        &a.field_0x830.x, &a.field_0x830.y, &a.jnt_tail_y_rot_offset};
    own_enemy_joints(step, a.field_0x762);
    own_enemy_joints(step, a.field_0x7a4);
    if (!tick) {
        for (auto& timer : a.timer) hold_enemy_timer(timer);
        hold_enemy_timer(a.invulnerability_timer);
        hold_enemy_timer(a.unk_timer_1);
        hold_enemy_timer(a.unk_timer_2);
        hold_enemy_timer(a.unk_timer_5);
        hold_enemy_timer(a.field_0x700);
        hold_enemy_timer(a.field_0x6fe);
        hold_enemy_timer(a.unk_timer_3);
        hold_enemy_timer(a.unk_timer_4);
        for (auto& timer : a.field_0x802) hold_enemy_timer(timer);
    }
}

HookAction before_fight_run(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_dn_class*>(args, 0);
    if (step == nullptr || step->actor != &a->actor) return HOOK_CONTINUE;
    const int frame = static_cast<int>(a->anm_p->getFrame());
    step->values[10] = !step->freshAnimationFrame && a->mode == 2 ? frame : -1;
    step->values[11] = a->actor.speed.y;
    step->values[12] = a->actor.speedF;
    step->values[13] = a->actor.current.angle.y;
    step->values[14] = a->set_smoke_flag;
    return HOOK_CONTINUE;
}

void after_fight_run(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_dn_class*>(args, 0);
    if (step == nullptr || step->actor != &a->actor || a->mode != 2 || a->action != 3) return;
    const int frame = static_cast<int>(step->values[10]);
    if (frame == 7 || frame == 21) {
        // The sidestep sets a new impulse and random heading at these frames.
        // Repeating it throughout a fractional frame would keep the actor airborne.
        a->actor.speed.y = step->values[11];
        a->actor.speedF = step->values[12];
        a->actor.current.angle.y = static_cast<s16>(step->values[13]);
    } else if (frame == 13 || frame == 27) {
        a->set_smoke_flag = static_cast<s8>(step->values[14]);
    }
}

ModResult install() {
    auto result = mods::hook::add_pre<FightRunHook>(svc_hook, before_fight_run);
    if (result == MOD_OK) result = mods::hook::add_post<FightRunHook>(svc_hook, after_fight_run);
    return result;
}

void before_angle(EnemySlowStep& step, s16* value) {
    auto& a = *reinterpret_cast<e_dn_class*>(step.actor);
    if (value != &a.actor.shape_angle.z) return;
    // This chase immediately precedes action()'s direct physics integration.
    step.values[0] = a.field_0x5d8 != 0;
    step.gravity = a.field_0x704 != 0.0f ? -4.0f : a.actor.gravity;
}

void before_collision(EnemySlowStep& step) {
    auto& a = *reinterpret_cast<e_dn_class*>(step.actor);
    if (step.values[0] == 0.0f) {
        slow_enemy_gravity(step, step.gravity, -100.0f, true);
    } else {
        // The leap's visual arc is a function of distance travelled, not time.
        const cXyz pos = step.originalPosition +
            (a.actor.current.pos - step.originalPosition) * step.scale;
        const float length = (a.field_0x5bc - a.field_0x5c8).abs();
        if (length > 0.001f)
            a.cur_pos_y_offset = std::min(length * 0.3f, 250.0f) *
                cM_ssin(((a.field_0x5bc - pos).abs() / length) * 32768.0f);
    }
}
}

const EnemySlowProfile& lizalfos_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_DN_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, nullptr, true, nullptr, before_angle};
    return profile;
}
}
