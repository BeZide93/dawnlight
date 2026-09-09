#include "integration.hpp"
#include "d/actor/d_a_e_gb.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("e_gb_attack_1", void(e_gb_class*), LungeHook);
DEFINE_HOOK_SYMBOL("e_gb_attack_2", void(e_gb_class*), FallHook);

bool eligible(fopAc_ac_c* base) {
    auto& a = *reinterpret_cast<e_gb_class*>(base);
    return a.anmP != nullptr && a.flowerAnmP != nullptr && a.demoMode == 0 &&
        a.headAction < 5 && a.flowerAction != 10 && !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *reinterpret_cast<e_gb_class*>(step.actor);
    step.action = a.headAction;
    step.subaction = a.mode;
    step.animations = {a.anmP, a.flowerAnmP};
    if (a.brkAnmP != nullptr) step.controllers[0] = a.brkAnmP->getFrameCtrl();
    step.directCollision = &a.objAcch;
    std::copy(std::begin(a.field_0x6e4), std::end(a.field_0x6e4), step.points.begin());
    step.chaseFloats = {&a.actor.current.pos.x, &a.actor.current.pos.y, &a.actor.current.pos.z,
        &a.actor.speedF, &a.currentPosTargetStep, &a.field_0x940, &a.field_0x944, &a.field_0x94c};
    step.chaseAngles = {&a.actor.current.angle.x, &a.actor.current.angle.y,
        &a.actor.shape_angle.x, &a.actor.shape_angle.y, &a.xRot};
    if (!tick) {
        for (auto& timer : a.timer) hold_enemy_timer(timer);
        hold_enemy_timer(a.invulnerabilityTimer);
        hold_enemy_timer(a.flowerInvulnerabilityTimer);
        for (int i = 0; i < 3; ++i)
            if (a.field_0x92c[i] != 0) hold_enemy_timer(a.field_0x935[i]);
    }
}

HookAction before_lunge(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_gb_class*>(args, 0);
    if (step != nullptr && step->actor == &a->actor) step->values[0] = 1.0f;
    return HOOK_CONTINUE;
}
HookAction before_fall(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_gb_class*>(args, 0);
    if (step != nullptr && step->actor == &a->actor) step->values[0] = 2.0f;
    return HOOK_CONTINUE;
}

void before_collision(EnemySlowStep& step) {
    if (step.values[0] == 1.0f)
        step.actor->current.pos -= step.actor->speed * (1.0f - step.scale);
    step.directCollision = nullptr; // Do not scale the position chases a second time.
}

void before_float(EnemySlowStep& step, float* value) {
    auto& a = *reinterpret_cast<e_gb_class*>(step.actor);
    if (value == &a.field_0x940 && step.values[0] == 2.0f) {
        const float gravity = -(JREG_F(12) + 10.0f);
        cXyz velocity = a.actor.speed;
        velocity.y -= gravity;
        a.actor.current.pos -= velocity * (1.0f - step.scale);
        slow_enemy_gravity(step, gravity, -1.0e10f, false);
        a.field_0x93c = std::min(a.field_0x93c,
            a.field_0x940 * (a.actor.current.pos - a.field_0x6d4).abs() * (BREG_F(0) + 0.1f));
    }
    if (value != &a.field_0x944) return;
    slow_enemy_chain(step, a.field_0x6e4, a.field_0x7bc);
    // Big Baba uses X-then-Y stem rotations, unlike the smaller Baba actors.
    for (int i = 0; i < 17; ++i) {
        const cXyz delta = a.field_0x6e4[i] - a.field_0x6e4[i + 1];
        a.field_0x7bc[i].x = -cM_atan2s(delta.y, delta.z);
        a.field_0x7bc[i].y = cM_atan2s(delta.x, std::sqrt(delta.y * delta.y + delta.z * delta.z));
    }
}

ModResult install() {
    auto result = mods::hook::add_pre<LungeHook>(svc_hook, before_lunge);
    if (result == MOD_OK) result = mods::hook::add_pre<FallHook>(svc_hook, before_fall);
    return result;
}
}

const EnemySlowProfile& big_baba_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_GB_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, nullptr, true, before_float};
    return profile;
}
}
