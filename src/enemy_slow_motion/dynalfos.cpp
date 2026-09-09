#include "integration.hpp"
#include "d/actor/d_a_e_mf.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("e_mf_fight_run", void(e_mf_class*), FightRunHook);
bool eligible(fopAc_ac_c* base) {
    const auto& a = *reinterpret_cast<e_mf_class*>(base);
    return a.mpModelMorf != nullptr && a.field_0x728 == 0 && a.field_0x820 == 0 &&
        a.mAction != 24 && !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *reinterpret_cast<e_mf_class*>(step.actor);
    step.action = a.mAction;
    step.subaction = a.field_0x5b4;
    step.animations = {a.mpModelMorf};
    step.sound = &a.mSound;
    step.frameSounds = {Z2SE_EN_MF_TAIL, Z2SE_EN_MF_V_TAIL, Z2SE_EN_MF_KNIFE,
        Z2SE_EN_MF_KNIFE2_A, Z2SE_EN_MF_KNIFE2_B, Z2SE_EN_MF_FN_L, Z2SE_EN_MF_FN_R,
        Z2SE_EN_MF_FN_RUN_L, Z2SE_EN_MF_FN_RUN_R, Z2SE_EN_MF_V_BREATH,
        Z2SE_EN_MF_V_SEARCH, Z2SE_EN_MF_V_SEARCH2};
    step.directCollision = &a.mObjAcch;
    step.chaseFloats = {&a.actor.speedF, &a.field_0x700, &a.field_0x6e8,
        &a.field_0x72c, &a.field_0x734, &a.field_0x73c, &a.field_0x7c8, &a.field_0x81c, &a.field_0x6a8};
    step.chaseAngles = {&a.actor.current.angle.y, &a.actor.shape_angle.x,
        &a.actor.shape_angle.y, &a.actor.shape_angle.z, &a.field_0x6d6, &a.field_0x6d8,
        &a.field_0x6da, &a.field_0x704.x, &a.field_0x70a.x, &a.field_0x70a.z,
        &a.field_0x810, &a.field_0x812, &a.field_0x806, &a.field_0x80a};
    own_enemy_joints(step, a.field_0x742);
    own_enemy_joints(step, a.field_0x784);
    if (!tick) {
        for (auto& timer : a.field_0x6c0) hold_enemy_timer(timer);
        hold_enemy_timer(a.field_0x6c8);
        hold_enemy_timer(a.field_0x6ca);
        hold_enemy_timer(a.field_0x6cc);
        hold_enemy_timer(a.field_0x820);
        hold_enemy_timer(a.field_0x6e0);
        hold_enemy_timer(a.field_0x6de);
        hold_enemy_timer(a.field_0x716);
        hold_enemy_timer(a.field_0x808);
        for (auto& timer : a.field_0x7e2) hold_enemy_timer(timer);
    }
}

HookAction before_fight_run(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_mf_class*>(args, 0);
    if (step == nullptr || step->actor != &a->actor) return HOOK_CONTINUE;
    const int frame = static_cast<int>(a->mpModelMorf->getFrame());
    step->values[10] = !step->freshAnimationFrame && a->field_0x5b4 == 2 ? frame : -1;
    step->values[11] = a->actor.speed.y;
    step->values[12] = a->actor.speedF;
    step->values[13] = a->actor.current.angle.y;
    step->values[14] = a->field_0x10c4;
    return HOOK_CONTINUE;
}

void after_fight_run(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_mf_class*>(args, 0);
    if (step == nullptr || step->actor != &a->actor || a->field_0x5b4 != 2 || a->mAction != 3) return;
    const int frame = static_cast<int>(step->values[10]);
    if (frame == 7 || frame == 21) {
        // The sidestep sets a new impulse and random heading at these frames.
        // Repeating it throughout a fractional frame would keep the actor airborne.
        a->actor.speed.y = step->values[11];
        a->actor.speedF = step->values[12];
        a->actor.current.angle.y = static_cast<s16>(step->values[13]);
    } else if (frame == 13 || frame == 27) {
        a->field_0x10c4 = static_cast<s8>(step->values[14]);
    }
}

ModResult install() {
    auto result = mods::hook::add_pre<FightRunHook>(svc_hook, before_fight_run);
    if (result == MOD_OK) result = mods::hook::add_post<FightRunHook>(svc_hook, after_fight_run);
    return result;
}

void before_angle(EnemySlowStep& step, s16* value) {
    auto& a = *reinterpret_cast<e_mf_class*>(step.actor);
    if (value == &a.field_0x80a) {
        const s16 correction = std::lround(a.field_0x80a * (1.0f - step.scale));
        a.actor.current.angle.y -= correction;
        a.actor.shape_angle.y -= correction;
    }
    if (value != &a.actor.shape_angle.z) return;
    step.values[0] = a.field_0x5d8 != 0;
    step.gravity = a.field_0x6e4 != 0.0f ? -4.0f : a.actor.gravity;
}

void before_collision(EnemySlowStep& step) {
    auto& a = *reinterpret_cast<e_mf_class*>(step.actor);
    if (step.values[0] == 0.0f) {
        slow_enemy_gravity(step, step.gravity, -100.0f, true);
    } else {
        const cXyz pos = step.originalPosition +
            (a.actor.current.pos - step.originalPosition) * step.scale;
        const float length = (a.field_0x5bc - a.field_0x5c8).abs();
        if (length > 0.001f)
            a.field_0x700 = std::min(length * 0.3f, 250.0f) *
                cM_ssin(((a.field_0x5bc - pos).abs() / length) * 32768.0f);
    }
}
}

const EnemySlowProfile& dynalfos_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_MF_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, nullptr, true, nullptr, before_angle};
    return profile;
}
}
