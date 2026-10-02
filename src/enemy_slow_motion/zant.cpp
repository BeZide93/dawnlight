#include "boss.hpp"
#include "d/actor/d_a_b_zant.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daB_ZANT_c::executeIceJump, IceJumpHook);
DEFINE_HOOK(&daB_ZANT_c::executeFly, FlyHook);
DEFINE_HOOK(&daB_ZANT_c::executeLastAttack, SpinHook);
bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<daB_ZANT_c*>(actor);
    return a.mpModelMorf && a.mAction != daB_ZANT_c::ACT_OPENING && a.mAction != daB_ZANT_c::ACT_ICE_DEMO && a.mAction != daB_ZANT_c::ACT_LAST_START_DEMO && a.mAction != daB_ZANT_c::ACT_LAST_END_DEMO && a.mAction != daB_ZANT_c::ACT_ROOM_CHANGE && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daB_ZANT_c*>(step.actor);
    step.animations = {a.mpModelMorf};
    own_boss_controller(step, a.mpMahojinEndBrk);
    own_boss_controller(step, a.mpMahojinBtk);
    own_boss_controller(step, a.mpMahojinStartBtk);
    own_boss_controller(step, a.mpMahojinBrk2);
    own_boss_controller(step, a.mpMahojinStartBtk2);
    step.chaseFloats = {&a.current.pos.y, &a.field_0x6bc, &a.field_0x6cc, &a.field_0x77c, &a.mKankyoBlend, &a.mMahojin2Size, &a.mModelScaleXZ, &a.mModelScaleY, &a.mSwordSize, &a.speed.y, &a.speedF};
    step.chaseAngles = {&a.current.angle.y, &a.field_0x6ba, &a.field_0x6f8, &a.mBackboneRotZ, &a.mNeckRotX, &a.mNeckRotZ, &a.shape_angle.x, &a.shape_angle.y};
    step.chasePositions = {&a.current.pos};
    hold_boss_timers(tick, a.mModeTimer, a.field_0x6ec, a.field_0x6e4, a.field_0x6f0, a.field_0x6f4);
}
HookAction before_motion(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<daB_ZANT_c*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_ZANT_e);
    if (!step) return HOOK_CONTINUE;
    step->points[10] = a->current.pos;
    step->facing = a->shape_angle.y;
    step->action = a->mAction;
    step->subaction = a->mMode;
    step->values[10] = a->field_0x6f8;
    return HOOK_CONTINUE;
}
void after_ice_jump(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<daB_ZANT_c*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_ZANT_e);
    if (!step || a->mAction != step->action) return;
    if (step->subaction == 5 || step->subaction == 6)
        a->current.pos = step->points[10] + (a->current.pos - step->points[10]) * step->scale;
}
void after_spin(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<daB_ZANT_c*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_ZANT_e);
    if (!step || a->mAction != step->action) return;
    const bool falling = step->action == daB_ZANT_c::ACT_FLY &&
        step->subaction >= 10 && step->subaction <= 13 && step->values[10] != 0;
    const bool spinning = step->action == daB_ZANT_c::ACT_LAST_ATTACK &&
        ((step->subaction >= 10 && step->subaction <= 13) || step->subaction == 31);
    if (falling || spinning)
        a->shape_angle.y = slow_enemy_angle(step->facing, a->shape_angle.y, step->scale);
}
ModResult install() {
    auto result = mods::hook::add_pre<IceJumpHook>(svc_hook, before_motion);
    if (result == MOD_OK) result = mods::hook::add_post<IceJumpHook>(svc_hook, after_ice_jump);
    if (result == MOD_OK) result = mods::hook::add_pre<FlyHook>(svc_hook, before_motion);
    if (result == MOD_OK) result = mods::hook::add_post<FlyHook>(svc_hook, after_spin);
    if (result == MOD_OK) result = mods::hook::add_pre<SpinHook>(svc_hook, before_motion);
    if (result == MOD_OK) result = mods::hook::add_post<SpinHook>(svc_hook, after_spin);
    return result;
}
}
const EnemySlowProfile& zant_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_ZANT_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, nullptr, nullptr, true};
    return profile;
}
}
