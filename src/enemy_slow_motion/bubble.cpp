#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_bu.h"
#include "SSystem/SComponent/c_lib.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&cLib_addCalcAngleS2, JawHook);

e_bu_class& bubble(fopAc_ac_c* actor) {
    static_assert(offsetof(e_bu_class, enemy) == 0);
    return *reinterpret_cast<e_bu_class*>(actor);
}

bool eligible(fopAc_ac_c* actor) {
    return bubble(actor).modelMorf != nullptr && !fopAcM_checkHookCarryNow(actor);
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = bubble(step.actor);
    step.action = actor.action;
    step.subaction = actor.mode;
    step.animations[0] = actor.modelMorf;
    step.directCollision = &actor.acch;
    step.chaseFloats = {&step.actor->speedF, &actor.field_0x690, &actor.hit_speed, &actor.field_0x6a8};
    step.chaseAngles = {&step.actor->current.angle.x, &step.actor->current.angle.y, &step.actor->current.angle.z,
        &step.actor->shape_angle.x, &step.actor->shape_angle.y, &step.actor->shape_angle.z,
        &actor.head_rot_x, &actor.head_rot_y, &actor.jaw_rot};
    step.values = {static_cast<float>(actor.lifetime), static_cast<float>(actor.head_rot_x),
                   static_cast<float>(actor.head_rot_y)};
    if (!timerTick) {
        for (auto& timer : actor.timers) hold_enemy_timer(timer);
        hold_enemy_timer(actor.invulnerabilityTimer);
        actor.lifetime = static_cast<s16>(static_cast<u16>(actor.lifetime) - 1U);
    }
}

void before_collision(EnemySlowStep& step) {
    auto& actor = bubble(step.actor);
    if (fopAcM_checkHookCarryNow(step.actor)) {
        step.directCollision = nullptr;
        return;
    }
    if (actor.action == 10) step.actor->speed.y += 5.0f * (1.0f - step.scale);
    if (step.action == 15 || actor.action == 15 || step.action == 21) {
        step.actor->speed.y += 7.0f * (1.0f - step.scale);
    }
    if (actor.action == 15 && step.action == 15 && actor.mode == step.subaction && actor.mode <= 3) {
        actor.head_rot_x = slow_enemy_angle(static_cast<s16>(step.values[1]), actor.head_rot_x, step.scale);
        actor.head_rot_y = slow_enemy_angle(static_cast<s16>(step.values[2]), actor.head_rot_y, step.scale);
    }
    if (actor.is_dead && actor.hit_speed > 0.1f) {
        step.actor->shape_angle.y = slow_enemy_angle(step.originalShapeAngles.y, step.actor->shape_angle.y, step.scale);
        step.actor->shape_angle.z = slow_enemy_angle(step.originalShapeAngles.z, step.actor->shape_angle.z, step.scale);
    }
}

HookAction before_jaw(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor == nullptr || step->profile->name != fpcNm_E_BU_e) return HOOK_CONTINUE;
    auto& actor = bubble(step->actor);
    if (mods::arg<s16*>(args, 0) == &actor.jaw_rot && mods::arg<s16>(args, 1) != 0) {
        const float phase = actor.lifetime + step->timerFraction;
        mods::arg_ref<s16>(args, 1) = static_cast<s16>(-(3000.0f + 2000.0f * cM_ssin(
            static_cast<s16>(static_cast<int>(phase * 0x3100)))));
    }
    return HOOK_CONTINUE;
}

ModResult install() {
    return mods::hook::add_pre<JawHook>(svc_hook, before_jaw);
}
}

const EnemySlowProfile& bubble_slow_profile() {
    // E_BU contains normal, fire and ice resource variants.
    static const EnemySlowProfile profile{
        fpcNm_E_BU_e, eligible, prepare, nullptr, nullptr, install, nullptr,
        before_collision, nullptr, true
    };
    return profile;
}
}
