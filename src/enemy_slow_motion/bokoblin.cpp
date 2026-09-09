#include "profile.hpp"
#include "timing.hpp"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_oc.h"
#include "f_pc/f_pc_name.h"
#include <algorithm>

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_OC_c::execute, ExecuteHook);
DEFINE_HOOK(&daE_OC_c::executeAttack, AttackHook);

// Values from d_a_e_oc.cpp's private E_OC_ACTION enum.
constexpr int kDeath = 9;
constexpr int kFall = 14;
bool eligible(fopAc_ac_c* base) {
    auto* actor = static_cast<daE_OC_c*>(base);
    return actor->mpMorf != nullptr && actor->field_0x6c8 == 0 &&
        (actor->mActionMode < kDeath || actor->mActionMode >= kFall);
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto* actor = static_cast<daE_OC_c*>(step.actor);
    step.action = actor->mActionMode;
    step.subaction = actor->mOcState;
    step.neck = actor->field_0x6d2;
    step.animations = {actor->mpMorf};
    if (timerTick) return;
    hold_enemy_timer(actor->field_0x6c0);
    hold_enemy_timer(actor->field_0x6c2);
    hold_enemy_timer(actor->field_0x6c4);
    hold_enemy_timer(actor->field_0x6c6);
    hold_enemy_timer(actor->field_0x6cc);
    hold_enemy_timer(actor->field_0x6ca);
    hold_enemy_timer(actor->field_0x6d6);
    hold_enemy_timer(actor->field_0x6ce);
}

void before_move(EnemySlowStep& step) {
    auto* actor = static_cast<daE_OC_c*>(step.actor);
    slow_enemy_steering(step, actor->mActionMode == step.action && actor->mOcState == step.subaction);
    actor->field_0x6d2 = slow_enemy_angle(step.neck, actor->field_0x6d2, step.scale);
}

struct AttackStep {
    daE_OC_c* actor = nullptr;
    float translation = 0.0f;
};
AttackStep s_attack{};

HookAction before_attack(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_OC_c*>(args, 0);
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor != actor || actor->mOcState < 1 || actor->mOcState > 3) {
        return HOOK_CONTINUE;
    }
    const float frame = actor->mpMorf->getFrame() - 9.0f;
    const auto& table = actor->checkBck(5) ? E_OC_n::oc_attackb_trans : E_OC_n::oc_attackc_trans;
    s_attack = {actor, sample_enemy_motion(table, frame)};
    return HOOK_CONTINUE;
}

void after_attack(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_OC_c*>(args, 0);
    if (s_attack.actor != actor) return;
    // Native code has applied integer-sampled root motion and stored it in
    // field_0x6a0. Correct to the fractional sample before background collision.
    const float correction = s_attack.translation - actor->field_0x6a0;
    actor->current.pos.x += correction * cM_ssin(actor->shape_angle.y);
    actor->current.pos.z += correction * cM_scos(actor->shape_angle.y);
    actor->field_0x6a0 = s_attack.translation;
    s_attack = {};
}

ModResult install() {
    ModResult result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_pre<AttackHook>(svc_hook, before_attack);
    if (result == MOD_OK) result = mods::hook::add_post<AttackHook>(svc_hook, after_attack);
    return result;
}
void reset() { s_attack = {}; }
}

const EnemySlowProfile& bokoblin_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_OC_e, eligible, prepare, before_move,
                                          nullptr, install, reset};
    return profile;
}
}
