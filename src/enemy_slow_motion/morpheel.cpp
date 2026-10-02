#include "boss.hpp"
#include "d/actor/d_a_b_ob.h"
#include "SSystem/SComponent/c_lib.h"
#include "d/d_s_play.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_b_ob.cpp#fish_move", void(b_ob_class*), FishHook);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_b_ob.cpp#core_action", void(b_ob_class*), CoreHook);
DEFINE_HOOK(&MtxPosition, VectorHook);

struct HistoryScope {
    b_ob_class* actor = nullptr;
    int index = 0;
    std::array<cXyz, 512> positions{};
    std::array<csXyz, 512> angles{};
};
std::array<HistoryScope, 8> s_history{};
std::size_t s_depth = 0;

bool eligible(fopAc_ac_c* actor) {
    const auto& a = *static_cast<b_ob_class*>(actor);
    return a.mpCoreMorf && a.mDemoAction == 0 &&
        a.mAction != OB_ACTION_CORE_START && a.mAction != OB_ACTION_CORE_HOOK &&
        a.mAction != OB_ACTION_CORE_END && a.mAction != OB_ACTION_FISH_END &&
        !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = *static_cast<b_ob_class*>(step.actor);
    step.animations[0] = a.mpCoreMorf;
    std::size_t slot = 1;
    for (auto& part : a.mBodyParts) {
        step.animations[slot++] = part.mpMorf;
        step.animations[slot++] = part.mpFinMorf;
        step.animations[slot++] = part.mpFinUnkMorf;
        step.animations[slot++] = part.mpFinBMorf;
        step.animations[slot++] = part.mpFinCMorf;
    }
    own_boss_controller(step, a.mpSuiBtk);
    own_boss_controller(step, a.mpSuiBrk);
    step.chaseFloats = {&a.speedF, &a.field_0x479c, &a.field_0x47c0,
        &a.field_0x5d04, &a.mBossLightScale, &a.mColsetBlend, &a.mSuiBrkFrame};
    step.chaseAngles = {&a.current.angle.x, &a.current.angle.y, &a.current.angle.z,
        &a.shape_angle.x, &a.shape_angle.y, &a.shape_angle.z,
        &a.field_0x476a, &a.field_0x47ac, &a.field_0x47ae, &a.field_0x47bc,
        &a.mBlureRate, &a.mMoveAngle.x};
    if (a.mCoreBattleMode && a.mAction == OB_ACTION_CORE_CHANCE)
        step.directCollision = &a.mAcch;
    else if (a.mCoreBattleMode) {
        step.chaseFloats[7] = &a.current.pos.x;
        step.chaseFloats[8] = &a.current.pos.y;
        step.chaseFloats[9] = &a.current.pos.z;
    }
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.mHitIFrameTimer, a.field_0x4794);
}
HookAction before_fish(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<b_ob_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_OB_e);
    if (s_depth < s_history.size()) {
        auto& saved = s_history[s_depth];
        saved.actor = step && eligible(a) ? a : nullptr;
        if (saved.actor) {
            saved.index = a->field_0x2320;
            std::copy(std::begin(a->field_0x2324), std::end(a->field_0x2324), saved.positions.begin());
            std::copy(std::begin(a->field_0x3b24), std::end(a->field_0x3b24), saved.angles.begin());
            const float fraction = step->timerTick ? 1.0f : step->timerFraction;
            // Read interpolated history, then restore its simulation samples.
            // Advancing all 512 samples at display rate shortens the body.
            for (std::size_t i = 0; i < saved.positions.size(); ++i) {
                const auto prev = (i + saved.positions.size() - 1) % saved.positions.size();
                a->field_0x2324[i] = saved.positions[prev] +
                    (saved.positions[i] - saved.positions[prev]) * fraction;
                a->field_0x3b24[i].x = slow_enemy_angle(saved.angles[prev].x, saved.angles[i].x, fraction);
                a->field_0x3b24[i].y = slow_enemy_angle(saved.angles[prev].y, saved.angles[i].y, fraction);
                a->field_0x3b24[i].z = slow_enemy_angle(saved.angles[prev].z, saved.angles[i].z, fraction);
            }
            hold_boss_timers(step->timerTick, a->mAttnOffTimer);
        }
    }
    ++s_depth;
    return HOOK_CONTINUE;
}
void after_fish(ModContext*, void* args, void*, void*) {
    if (!s_depth || --s_depth >= s_history.size()) return;
    auto& saved = s_history[s_depth];
    auto* a = mods::arg<b_ob_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_OB_e);
    if (saved.actor != a || !step) return;
    const cXyz newest = a->field_0x2324[saved.index];
    const csXyz newestAngle = a->field_0x3b24[saved.index];
    std::copy(saved.positions.begin(), saved.positions.end(), std::begin(a->field_0x2324));
    std::copy(saved.angles.begin(), saved.angles.end(), std::begin(a->field_0x3b24));
    if (step->timerTick) {
        a->field_0x2324[saved.index] = newest;
        a->field_0x3b24[saved.index] = newestAngle;
    } else {
        a->field_0x2320 = saved.index;
    }
    if (step->values[0]) a->speed = step->points[0];
    saved.actor = nullptr;
}
void after_vector(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (!step || step->profile->name != fpcNm_B_OB_e ||
        mods::arg<cXyz*>(args, 1) != &step->actor->speed) return;
    auto& a = *static_cast<b_ob_class*>(step->actor);
    if (a.mFishBattleMode && s_depth && s_depth <= s_history.size() &&
        s_history[s_depth - 1].actor == &a) {
        step->points[0] = a.speed;
        step->values[0] = 1;
        a.speed *= step->scale; // Before history, model matrices AND colliders.
    }
}
void after_core(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<b_ob_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_B_OB_e);
    if (step && eligible(a)) finish_boss_translation(*step);
}
void before_collision(EnemySlowStep& step) {
    if (!eligible(step.actor)) { step.directCollision = nullptr; return; }
    slow_enemy_gravity(step, -(VREG_F(0) + 3.0f), -std::numeric_limits<float>::infinity(), false);
}
void reset() { s_depth = 0; for (auto& frame : s_history) frame.actor = nullptr; }
ModResult install() {
    auto result = mods::hook::add_pre<FishHook>(svc_hook, before_fish);
    if (result == MOD_OK) result = mods::hook::add_post<FishHook>(svc_hook, after_fish);
    if (result == MOD_OK) result = mods::hook::add_post<VectorHook>(svc_hook, after_vector);
    if (result == MOD_OK) result = mods::hook::add_post<CoreHook>(svc_hook, after_core);
    return result;
}
}
const EnemySlowProfile& morpheel_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_B_OB_e, eligible, prepare,
        nullptr, nullptr, install, reset, before_collision, nullptr, true};
    return profile;
}
}
