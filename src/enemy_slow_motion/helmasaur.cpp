#include "integration.hpp"
#include "d/actor/d_a_e_mm.h"
#include "d/actor/d_a_e_mm_mt.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureSound", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, u32, s8), SoundHook);
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureVoice", Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, s8), VoiceHook);

e_mm_class& helmasaur(fopAc_ac_c* base) {
    static_assert(offsetof(e_mm_class, enemy) == 0);
    return *reinterpret_cast<e_mm_class*>(base);
}
e_mm_mt_class& armor(fopAc_ac_c* base) {
    static_assert(offsetof(e_mm_mt_class, enemy) == 0);
    return *reinterpret_cast<e_mm_mt_class*>(base);
}
bool eligible(fopAc_ac_c* base) { return helmasaur(base).modelMorf != nullptr; }
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = helmasaur(step.actor);
    step.animations = {a.modelMorf};
    step.directCollision = &a.acch;
    step.chaseFloats = {&step.actor->speedF, &a.field_0x6a8};
    step.chaseAngles = {&step.actor->current.angle.y, &step.actor->shape_angle.y};
    if (!tick) {
        for (auto& timer : a.timers) hold_enemy_timer(timer);
        hold_enemy_timer(a.field_0x6a4);
    }
}
void before_collision(EnemySlowStep& step) {
    // action integrates fixed -3 gravity before translation, for both sizes.
    slow_enemy_gravity(step, -3.0f, -std::numeric_limits<float>::infinity(), true);
}
HookAction before_sound(ModContext*, void* args, void* result, void*) {
    const auto* step = current_enemy_slow_step();
    if (!step || step->profile->name != fpcNm_E_MM_e || step->freshAnimationFrame ||
        mods::arg<Z2CreatureEnemy*>(args, 0) != &helmasaur(step->actor).sound) return HOOK_CONTINUE;
    const auto id = mods::arg<JAISoundID>(args, 1);
    if (id == Z2SE_EN_MM_WALK_LND || id == Z2SE_EN_MM_WALK_WTR ||
        id == Z2SE_EN_MM_RUN_LND || id == Z2SE_EN_MM_RUN_WTR ||
        id == Z2SE_CM_BODYFALL_S || id == Z2SE_CM_BODYFALL_ASASE_S) {
        *static_cast<Z2SoundHandlePool**>(result) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
HookAction before_voice(ModContext*, void* args, void* result, void*) {
    const auto* step = current_enemy_slow_step();
    if (!step || step->profile->name != fpcNm_E_MM_e || step->freshAnimationFrame ||
        mods::arg<Z2CreatureEnemy*>(args, 0) != &helmasaur(step->actor).sound) return HOOK_CONTINUE;
    const auto id = mods::arg<JAISoundID>(args, 1);
    // Only the integer-frame events, never damage/death/state-entry voices.
    if (id == Z2SE_EN_MM_V_KYORO || id == Z2SE_EN_MM_V_FIND || id == Z2SE_EN_MM_V_SURPRISE) {
        *static_cast<Z2SoundHandlePool**>(result) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
ModResult install() {
    auto result = mods::hook::add_pre<SoundHook>(svc_hook, before_sound);
    return result == MOD_OK ? mods::hook::add_pre<VoiceHook>(svc_hook, before_voice) : result;
}
bool armor_eligible(fopAc_ac_c* base) {
    auto& a = armor(base);
    auto* parent = fopAcM_SearchByID(base->parentActorID);
    // Attached armor copies the parent's live joint matrix every frame. Carried
    // or detached shells keep their native/player-owned behavior.
    return a.mp_model && a.m_action == 0 && parent &&
        fopAcM_GetName(parent) == fpcNm_E_MM_e && helmasaur(parent).modelMorf;
}
void prepare_armor(EnemySlowStep& step, bool tick) {
    if (!tick) {
        auto& a = armor(step.actor);
        for (auto& timer : a.m_timer) hold_enemy_timer(timer);
        hold_enemy_timer(a.m_invulnerabilityTimer);
    }
}
}
const EnemySlowProfile& helmasaur_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_MM_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, nullptr, true};
    return profile;
}
const EnemySlowProfile& helmasaur_armor_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_MM_MT_e, armor_eligible, prepare_armor,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true};
    return profile;
}
}
