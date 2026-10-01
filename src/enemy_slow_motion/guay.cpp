#include "integration.hpp"
#include "d/actor/d_a_e_ge.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_GE_c::executeFly, FlyHook);
DEFINE_HOOK(&daE_GE_c::executeAttack, AttackHook);
DEFINE_HOOK(&daE_GE_c::executeBack, BackHook);
DEFINE_HOOK(&daE_GE_c::calcCircleFly, CircleHook);
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
    step.values[1] = 0; // vertical-speed checkpoint from calcCircleFly
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
HookAction before_circle(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    if (!step || step->actor != a) return HOOK_CONTINUE;
    auto& angle = mods::arg_ref<s16>(args, 3);
    const auto old = static_cast<s16>(step->values[0]);
    // Only an actual orbit increment, never a new target/home bearing.
    if (angle == a->field_0xb8c && static_cast<s16>(angle - old) == a->field_0xb8a &&
        (a->mActionMode == kFly || (a->mActionMode == kAttack && a->mMode == 1))) {
        angle = a->field_0xb8c = slow_enemy_angle(old, angle, step->scale);
    }
    return HOOK_CONTINUE;
}
void after_circle(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    if (step && step->actor == a) { step->values[1] = 1; step->values[2] = a->speed.y; }
}
void after_attack(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    auto* a = mods::arg<daE_GE_c*>(args, 0);
    // calcCircleFly has already slowed chase acceleration. Only scale the
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
    if (result == MOD_OK) result = mods::hook::add_pre<CircleHook>(svc_hook, before_circle);
    if (result == MOD_OK) result = mods::hook::add_post<CircleHook>(svc_hook, after_circle);
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
