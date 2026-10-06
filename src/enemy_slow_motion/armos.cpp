#include "integration.hpp"
#include "JSystem/JParticle/JPAEmitter.h"
#include "d/actor/d_a_e_ai.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&e_ai_class::e_ai_attack, AttackHook);
DEFINE_HOOK(&e_ai_class::action, ActionHook);
DEFINE_HOOK(&e_ai_class::e_ai_damage, DamageHook);

HookAction before_damage(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_ai_class*>(args, 0);
    if (a->field_0x692 != 0 || a->m_mode != 1 || a->m_timers[1] != 0)
        return HOOK_CONTINUE;

    // Native e_ai_damage unconditionally dereferences the emitter returned for
    // this scene-specific flash. It may be absent in a spawner/randomizer room,
    // or allocation may fail. Start the same flash/countdown safely, then let
    // native movement, emitter tracking, explosion, switch and deletion run.
    // This safety hook deliberately does not depend on an active slow step.
    a->m_sound.startCreatureSound(Z2SE_EN_AI_FLASH, 0, -1);
    a->mpEmitter = dComIfGp_particle_set(0x81ED, &a->current.pos, &a->tevStr,
                                       &a->shape_angle, nullptr);
    if (a->mpEmitter) a->mpEmitter->becomeImmortalEmitter();
    a->m_timers[1] = 1000; // Bypass only the unsafe native flash-start block.
    a->m_timers[2] = 56;
    return HOOK_CONTINUE;
}
bool eligible(fopAc_ac_c* base) {
    const auto& a = *static_cast<e_ai_class*>(base);
    return a.m_modelMorf && a.m_brk;
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<e_ai_class*>(step.actor);
    step.animations = {a.m_modelMorf};
    step.controllers = {a.m_brk->getFrameCtrl()};
    step.directCollision = &a.m_acch;
    step.chaseAngles = {&a.current.angle.y, &a.shape_angle.y};
    step.values[0] = a.m_lifetime;
    step.values[1] = a.m_timers[2];
    step.values[2] = -1; // scoped integer-frame event suppression
    if (!tick) {
        a.m_lifetime = static_cast<s16>(static_cast<u16>(a.m_lifetime) - 1);
        for (auto& timer : a.m_timers) hold_enemy_timer(timer);
        hold_enemy_timer(a.m_invulnerabilityTimer);
        hold_enemy_timer(a.field_0x6bc);
        hold_enemy_timer(a.field_0x6ba);
        // Awake voice and explosion are equality-at-one events. Do not replay
        // them while a fractional frame holds the last countdown value.
        if (step.values[1] == 1) ++a.m_timers[2];
    }
}
void before_collision(EnemySlowStep& step) {
    slow_enemy_gravity(step, step.actor->gravity, -std::numeric_limits<float>::infinity(), false);
}
void before_angle(EnemySlowStep& step, s16* value) {
    auto& a = *static_cast<e_ai_class*>(step.actor);
    if (value == &a.shape_angle.y && a.m_action == e_ai_class::ACTION_DAMAGE &&
        a.field_0x692 == 0 && a.m_mode == 1)
        a.current.angle.y -= static_cast<s16>(std::lround(a.field_0x6a8 * (1.0f - step.scale)));
}
HookAction before_attack(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_ai_class*>(args, 0);
    if (step && step->actor == a && !step->freshAnimationFrame && a->m_mode == 1 &&
        static_cast<int>(a->m_modelMorf->getFrame()) == 4) {
        // This method only reads this frame for the impact event and isStop.
        // Restore it before native animation playback; keep the hitbox active.
        step->values[2] = a->m_modelMorf->getFrame();
        a->m_modelMorf->setFrame(3.0f);
    }
    return HOOK_CONTINUE;
}
void after_attack(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_ai_class*>(args, 0);
    if (step && step->actor == a && step->values[2] >= 0) {
        a->m_modelMorf->setFrame(step->values[2]);
        step->values[2] = -1;
    }
}
void after_action(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<e_ai_class*>(args, 0);
    if (!step || step->actor != a) return;
    const float phase = a->m_lifetime + step->timerFraction;
    const auto wave = [phase](int frequency) { return cM_ssin(static_cast<s16>(static_cast<int>(phase * frequency))); };
    if (a->field_0x6bc != 0) a->shape_angle.z = (200.0f + TREG_F(18)) * wave(TREG_S(5) + 15000);
    else if (a->m_action == e_ai_class::ACTION_MOVE && a->m_timers[2] != 0)
        a->shape_angle.z = (500.0f + TREG_F(18)) * wave(TREG_S(5) + 10000);
    else if (a->m_action == e_ai_class::ACTION_DAMAGE && a->field_0x692 == 0 && a->m_mode == 1) {
        a->shape_angle.z = (2000.0f + TREG_F(16)) * wave(TREG_S(7) + 6000);
    }
    if (a->field_0x6ba != 0) {
        const float left = std::max(0.0f, a->field_0x6ba - step->timerFraction);
        a->field_0x6c0 = 0.5f + 0.5f * cM_ssin(static_cast<s16>(static_cast<int>(left * (BREG_S(5) + 8000))));
    }
}
void after_execute(EnemySlowStep& step) {
    auto& a = *static_cast<e_ai_class*>(step.actor);
    if (!step.timerTick && step.values[1] == 1 && a.m_timers[2] == 2) a.m_timers[2] = 1;
}
ModResult install() {
    auto result = mods::hook::add_pre<AttackHook>(svc_hook, before_attack);
    if (result == MOD_OK) result = mods::hook::add_post<AttackHook>(svc_hook, after_attack);
    if (result == MOD_OK) result = mods::hook::add_post<ActionHook>(svc_hook, after_action);
    if (result == MOD_OK) result = mods::hook::add_pre<DamageHook>(svc_hook, before_damage);
    return result;
}
}
const EnemySlowProfile& armos_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_AI_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, after_execute, true, nullptr, before_angle};
    return profile;
}
}
