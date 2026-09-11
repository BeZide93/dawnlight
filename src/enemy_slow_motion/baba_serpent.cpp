#include "plant.hpp"
#include "d/actor/d_a_e_hb.h"
#include "d/actor/d_a_e_hb_leaf.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("e_hb_s_damage", void(e_hb_class*), SmallDamageHook);

bool eligible(fopAc_ac_c* base) {
    auto& a = *reinterpret_cast<e_hb_class*>(base);
    return a.modelMorf != nullptr && a.field_0x850 == 0 && a.enemy.health != 1000 &&
        a.field_0x852 == 0 && !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *reinterpret_cast<e_hb_class*>(step.actor);
    step.action = a.action;
    step.subaction = a.mode;
    step.animations = {a.modelMorf};
    auto* leaf = fopAcM_SearchByID(a.leaf_actor_id);
    if (leaf != nullptr && fopAcM_GetName(leaf) == fpcNm_E_HB_LEAF_e)
        step.animations[1] = static_cast<e_hb_leaf_class*>(leaf)->mpMorf;
    step.directCollision = &a.acch;
    std::copy(std::begin(a.field_0x6a0), std::end(a.field_0x6a0), step.points.begin());
    step.chaseFloats = {&a.enemy.current.pos.x, &a.enemy.current.pos.y, &a.enemy.current.pos.z,
        &a.field_0x674.y, &a.field_0x690, &a.size, &a.field_0x848, &a.field_0x84c,
        &a.field_0x854, &a.field_0x124c};
    step.chaseAngles = {&a.enemy.shape_angle.x, &a.enemy.shape_angle.y, &a.enemy.shape_angle.z};
    if (!tick) {
        for (auto& timer : a.timers) hold_enemy_timer(timer);
        hold_enemy_timer(a.invulnerabilityTimer);
    }
}

HookAction before_small_damage(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_hb_class*>(args, 0);
    if (step != nullptr && step->actor == &a->enemy) {
        step->values[0] = a->mode == 1;
        step->position = a->enemy.current.pos;
        step->originalSpeed = a->enemy.speed;
    }
    return HOOK_CONTINUE;
}

void after_small_damage(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_hb_class*>(args, 0);
    if (step == nullptr || step->actor != &a->enemy || step->values[0] == 0) return;
    a->enemy.current.pos = step->position + (a->enemy.current.pos - step->position) * step->scale;
    if (a->enemy.speed.abs2() > 0.0f)
        a->enemy.speed = step->originalSpeed * std::pow(0.92f, step->scale);
}

void before_collision(EnemySlowStep& step) {
    auto& a = *reinterpret_cast<e_hb_class*>(step.actor);
    if (a.action != 7) { step.directCollision = nullptr; return; }
    const float gravity = a.arg0 == 1 ? 0.0f : (a.arg0 == 2 ? -(1.0f + AREG_F(0)) : -5.0f);
    slow_enemy_gravity(step, gravity, -1.0e10f, false);
}

void after_execute(EnemySlowStep& step) {
    auto& a = *reinterpret_cast<e_hb_class*>(step.actor);
    finish_baba_stem(step, a, a.field_0x6a0, a.field_0x730);
}

ModResult install() {
    auto result = mods::hook::add_pre<SmallDamageHook>(svc_hook, before_small_damage);
    if (result == MOD_OK) result = mods::hook::add_post<SmallDamageHook>(svc_hook, after_small_damage);
    return result;
}
}

const EnemySlowProfile& baba_serpent_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_HB_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, nullptr, true};
    return profile;
}
}
