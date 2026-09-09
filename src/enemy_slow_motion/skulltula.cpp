#include "integration.hpp"
#include "SSystem/SComponent/c_lib.h"
#include "d/actor/d_a_e_st.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("bg_pos_get", BOOL(e_st_class*), WallPositionHook);
DEFINE_HOOK(&MtxPosition, VectorHook);
DEFINE_HOOK(&fopAcM_createChild, SilkHook);

EnemySlowStep* skulltula_step() {
    auto* step = current_enemy_slow_step();
    return step != nullptr && step->profile->name == fpcNm_E_ST_e ? step : nullptr;
}

bool eligible(fopAc_ac_c* base) {
    auto& a = *reinterpret_cast<e_st_class*>(base);
    return a.mpModelMorf != nullptr && a.mAction != 21 &&
        !(a.mAction == 15 && (a.mActionPhase == 3 || a.mActionPhase == 4)) &&
        !dComIfGp_event_runCheck();
}

void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *reinterpret_cast<e_st_class*>(step.actor);
    step.action = a.mAction;
    step.subaction = a.mActionPhase;
    step.animations = {a.mpModelMorf};
    step.directCollision = &a.mBgc;
    step.chaseFloats = {&a.actor.speedF, &a.actor.speed.y, &a.actor.current.pos.x,
        &a.actor.current.pos.y, &a.actor.current.pos.z, &a.field_0x6b0.y,
        &a.field_0x7e0, &a.field_0x7ec, &a.field_0x7f4, &a.field_0x75c,
        &a.field_0x724, &a.field_0x764, &a.mColor};
    step.chaseAngles = {&a.actor.current.angle.x, &a.actor.current.angle.y,
        &a.actor.current.angle.z, &a.field_0x69c.x, &a.field_0x69c.y,
        &a.field_0x6a2, &a.field_0x6a4, &a.field_0x7d4};
    for (std::size_t i = 0; i < 8; ++i) {
        step.chaseFloats[13 + i] = &a.mStFeet[i].field_0x0;
        own_enemy_values(step.chaseAngles, a.mStFeet[i].mAngles);
    }
    if (!tick) {
        for (auto& timer : a.mTimers) hold_enemy_timer(timer);
        hold_enemy_timer(a.mInvulnerabilityTimer);
        hold_enemy_timer(a.mDefTimer);
    }
}

HookAction before_wall_position(ModContext*, void* args, void*, void*) {
    auto* step = skulltula_step();
    auto* a = mods::arg<e_st_class*>(args, 0);
    if (step != nullptr && step->actor == &a->actor && a->mAction <= 3)
        a->mBgPos = a->actor.old.pos + (a->mBgPos - a->actor.old.pos) * step->scale;
    return HOOK_CONTINUE;
}

void after_wall_position(ModContext*, void* args, void*, void*) {
    auto* step = skulltula_step();
    auto* a = mods::arg<e_st_class*>(args, 0);
    if (step == nullptr || step->actor != &a->actor) return;
    // Any subsequent CrrPos corrects knockback, not this already slowed crawl.
    step->originalPosition = a->actor.current.pos;
    step->originalOldPosition = a->actor.old.pos;
}

void after_vector(ModContext*, void* args, void*, void*) {
    auto* step = skulltula_step();
    if (step == nullptr) return;
    auto& a = *reinterpret_cast<e_st_class*>(step->actor);
    if (mods::arg<cXyz*>(args, 1) == &a.actor.speed && a.mAction == 15 &&
        (a.mActionPhase == 2 || a.mActionPhase == 5)) {
        // Fresh homing velocity, before movement AND the native arrival test.
        a.actor.speed *= step->scale;
    }
}

HookAction before_silk(ModContext*, void* args, void* retval, void*) {
    auto* step = skulltula_step();
    if (step != nullptr && !step->freshAnimationFrame &&
        mods::arg<s16>(args, 0) == fpcNm_E_ST_LINE_e &&
        mods::arg<fpc_ProcID>(args, 1) == fopAcM_GetID(step->actor)) {
        auto& a = *reinterpret_cast<e_st_class*>(step->actor);
        --a.mParameters; // The caller increments after each create attempt.
        *static_cast<fpc_ProcID*>(retval) = fpcM_ERROR_PROCESS_ID_e;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void before_collision(EnemySlowStep& step) {
    auto& a = *reinterpret_cast<e_st_class*>(step.actor);
    if (a.mAction >= 50 && a.mAction <= 57)
        slow_enemy_gravity(step, -5.0f, -80.0f, false);
    else if (a.mAction == 12)
        slow_enemy_gravity(step, -(YREG_F(7) + 3.0f), -1.0e10f, false);
    else if (a.mAction == 20)
        slow_enemy_gravity(step, -(YREG_F(8) + 5.0f), -1.0e10f, false);
}

ModResult install() {
    auto result = mods::hook::add_pre<WallPositionHook>(svc_hook, before_wall_position);
    if (result == MOD_OK) result = mods::hook::add_post<WallPositionHook>(svc_hook, after_wall_position);
    if (result == MOD_OK) result = mods::hook::add_post<VectorHook>(svc_hook, after_vector);
    if (result == MOD_OK) result = mods::hook::add_pre<SilkHook>(svc_hook, before_silk);
    return result;
}
}

const EnemySlowProfile& skulltula_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_ST_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, nullptr, true};
    return profile;
}
}
