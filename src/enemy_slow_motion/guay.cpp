#include "integration.hpp"
#include "d/actor/d_a_e_ge.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_GE_c::executeFly, FlyHook);
DEFINE_HOOK(&daE_GE_c::executeAttack, AttackHook);
DEFINE_HOOK(&daE_GE_c::executeBack, BackHook);
// Do not hook calcCircleFly: it returns cXyz by value. On the Microsoft ABI,
// the member function takes `this` before the hidden return buffer, while the
// SDK's free-function trampoline puts that buffer first. Even an inactive
// callback would forward the wrong actor pointer and corrupt memory.
DEFINE_HOOK(&daE_GE_c::checkCircleSpeedAdd, OrbitCheckHook);
// References use the pointer ABI; spelling them as pointers also avoids the
// pinned SDK's invalid const-reference-to-void* argument marshalling.
DEFINE_HOOK_SYMBOL("daE_GE_c::setAddCalcSpeed",
    void(daE_GE_c*, cXyz*, const cXyz*, float, float, float, float), SpeedHook);
DEFINE_HOOK(&daE_GE_c::mtx_set, MatrixHook);
constexpr int kFly = 1, kAttack = 2, kBack = 3, kDown = 4, kWind = 7;
bool attached(const daE_GE_c& a) { return a.mActionMode == kWind && a.mMode < 2; }
bool eligible(fopAc_ac_c* base) {
    auto& a = *static_cast<daE_GE_c*>(base);
    return a.mpMorfSO && !attached(a);
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<daE_GE_c*>(step.actor);
    step.action = a.mActionMode;
    step.subaction = a.mMode;
    step.animations = {a.mpMorfSO};
    step.directCollision = &a.mObjAcch;
    step.chaseFloats = {&a.speedF, &a.speed.y, &a.field_0xb58, &a.field_0xb5c};
    step.chaseAngles = {&a.current.angle.y, &a.shape_angle.x, &a.shape_angle.y,
        &a.shape_angle.z, &a.field_0xb8a};
    step.values[0] = a.field_0xb8c;
    step.values[1] = 0; // vertical-speed checkpoint from setAddCalcSpeed
    if (!tick) {
        for (auto& timer : a.field_0xb8e) hold_enemy_timer(timer);
        hold_enemy_timer(a.mDamageCooldownTimer);
        hold_enemy_timer(a.mAnmChangeTimer);
        hold_enemy_timer(a.mSurpriseTime);
        hold_enemy_timer(a.mBackAnimeTimer);
    }
}
HookAction before_back(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    if (step && step->actor == a && !step->timerTick && a->mMode == 1 && a->field_0xb8a > 8)
        hold_enemy_timer(a->field_0xb8a);
    return HOOK_CONTINUE;
}
HookAction before_flight(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    if (step && step->actor == a && a->mMode == 1) {
        // These are raw altitude accumulators, read by the orbit target in
        // this very call. Compensate before the native +=4 / -=3.
        if (a->mSubMode == 0) a->field_0xb5c -= 4.0f * (1.0f - step->scale);
        else if (a->mActionMode == kFly || a->mSubMode == 1)
            a->field_0xb5c += 3.0f * (1.0f - step->scale);
    }
    return HOOK_CONTINUE;
}
void after_orbit_check(ModContext*, void* args, void* result, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    if (!step || step->actor != a || !result || !*static_cast<bool*>(result)) return;
    if (!((a->mActionMode == kFly && (a->mMode == 1 || a->mMode == 2)) ||
          (a->mActionMode == kAttack && a->mMode == 1))) return;
    // Each native caller immediately adds field_0xb8a after a successful check.
    // Remove the unused fraction now, without changing the check's input or
    // return value, the persistent angular speed, or any target/home bearing.
    const s16 next = static_cast<s16>(a->field_0xb8c + a->field_0xb8a);
    a->field_0xb8c = static_cast<s16>(
        slow_enemy_angle(a->field_0xb8c, next, step->scale) - a->field_0xb8a);
}
void after_speed(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    if (step && step->actor == a) { step->values[1] = 1; step->values[2] = a->speed.y; }
}
void after_attack(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    // setAddCalcSpeed has already slowed chase acceleration. Only scale the
    // additional raw dive acceleration after that call, without doing it twice.
    if (step && step->actor == a && step->values[1] != 0)
        a->speed.y = step->values[2] + (a->speed.y - step->values[2]) * step->scale;
}
void before_collision(EnemySlowStep& step) {
    auto& a = *static_cast<daE_GE_c*>(step.actor);
    if (attached(a)) { step.directCollision = nullptr; return; }
    if (a.mActionMode == kDown && a.mMode == 1)
        a.shape_angle.z -= static_cast<s16>(std::lround(0x800 * (1.0f - step.scale)));
    if (a.mActionMode == kWind && a.mMode == 2)
        a.shape_angle.y -= static_cast<s16>(std::lround(a.field_0xb8a * (1.0f - step.scale)));
    if (a.mActionMode == kBack && a.mMode == 1 &&
        static_cast<s16>(a.field_0xb8c - static_cast<s16>(step.values[0])) == 0x190)
        a.field_0xb8c = slow_enemy_angle(static_cast<s16>(step.values[0]), a.field_0xb8c, step.scale);
}
HookAction before_matrix(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    // Perched/landing branches can omit CrrPos; still finish before model and
    // collider calculation. A boomerang attachment must remain absolute.
    if (step && step->actor == a && step->directCollision && !attached(*a))
        finish_enemy_translation(*step);
    return HOOK_CONTINUE;
}
ModResult install() {
    auto result = mods::hook::add_pre<FlyHook>(svc_hook, before_flight);
    if (result == MOD_OK) result = mods::hook::add_pre<BackHook>(svc_hook, before_back);
    if (result == MOD_OK) result = mods::hook::add_pre<AttackHook>(svc_hook, before_flight);
    if (result == MOD_OK) result = mods::hook::add_post<AttackHook>(svc_hook, after_attack);
    if (result == MOD_OK) result = mods::hook::add_post<OrbitCheckHook>(svc_hook, after_orbit_check);
    if (result == MOD_OK) result = mods::hook::add_post<SpeedHook>(svc_hook, after_speed);
    if (result == MOD_OK) result = mods::hook::add_pre<MatrixHook>(svc_hook, before_matrix);
    return result;
}
}
const EnemySlowProfile& guay_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_GE_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, nullptr, true};
    return profile;
}
}
