#include "profile.hpp"
#include "../enemy_hard_mode.hpp"
#include "d/actor/d_a_e_tk2.h"
#include "d/d_com_inf_game.h"
#include <array>

namespace dawnlight {
namespace {
DEFINE_HOOK(&fopAcM_createChild, CreateChildHook);

struct ShotClock {
    fpc_ProcID id = fpcM_ERROR_PROCESS_ID_e;
    u8 shots = 0;
};

std::array<ShotClock, 16> s_shotClocks{};
csXyz s_ballAngle{};
bool s_spreadShot = false;
bool s_spawningSpread = false;
int s_ballCount = 0;

ShotClock& shot_clock(fopAc_ac_c* actor) {
    const auto id = fopAcM_GetID(actor);
    auto& clock = s_shotClocks[id % s_shotClocks.size()];
    if (clock.id != id) clock = {id, 0};
    return clock;
}

void* count_balls(void* process, void*) {
    if (fopAcM_IsActor(process) && fopAcM_GetName(process) == fpcNm_E_TK_BALL_e) ++s_ballCount;
    return nullptr;
}

bool eligible(fopAc_ac_c* base) {
    return static_cast<e_tk2_class*>(base)->mpMorf != nullptr;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<e_tk2_class*>(step.actor);
    step.animations = {actor.mpMorf};
    step.directCollision = &actor.mAcch;
    step.chaseFloats = {&actor.mAnimSpeed};
    step.chaseAngles = {&actor.shape_angle.y};
    step.action = actor.mAction;
    step.subaction = actor.mMode;
    if (!timerTick) {
        for (auto& timer : actor.mActionTimer) hold_enemy_timer(timer);
        hold_enemy_timer(actor.mInvincibilityTimer);
    }
}

bool repeated_attack(const EnemySlowStep& step) {
    const auto& actor = *static_cast<e_tk2_class*>(step.actor);
    return !step.freshAnimationFrame && step.action == 2 && step.subaction == 1 &&
        actor.mAction == step.action && actor.mMode == step.subaction;
}

HookAction before_create_child(ModContext*, void* args, void* retval, void*) {
    if (s_spawningSpread) return HOOK_CONTINUE;
    auto* step = current_enemy_slow_step();
    if (step != nullptr && step->actor != nullptr && step->profile != nullptr &&
        step->profile->name == fpcNm_E_TK2_e && repeated_attack(*step) &&
        mods::arg<s16>(args, 0) == fpcNm_E_TK_BALL_e &&
        mods::arg<fpc_ProcID>(args, 1) == fopAcM_GetID(step->actor)) {
        // Keep the existing suspended ball ID instead of replacing it with an error.
        *static_cast<fpc_ProcID*>(retval) = static_cast<e_tk2_class*>(step->actor)->mBallID;
        return HOOK_SKIP_ORIGINAL;
    }
    const bool isBall = step != nullptr && step->actor != nullptr &&
        step->profile != nullptr && step->profile->name == fpcNm_E_TK2_e &&
        mods::arg<s16>(args, 0) == fpcNm_E_TK_BALL_e &&
        mods::arg<fpc_ProcID>(args, 1) == fopAcM_GetID(step->actor);
    if (isBall && enemy_hard_mode_applies(fpcNm_E_TK2_e)) {
        auto& clock = shot_clock(step->actor);
        ++clock.shots;
        const auto* source = mods::arg<const csXyz*>(args, 5);
        s_ballAngle = source != nullptr ? *source : step->actor->shape_angle;
        auto* player = dComIfGp_getPlayer(0);
        if (player != nullptr && (clock.shots & 1U) == 0) {
            const cXyz target = player->current.pos + player->speed * 10.0f;
            const cXyz delta = target - step->actor->current.pos;
            s_ballAngle.y = cM_atan2s(delta.x, delta.z);
        }
        mods::arg_ref<const csXyz*>(args, 5) = &s_ballAngle;

        s_ballCount = 0;
        fpcM_Search(count_balls, nullptr);
        s_spreadShot = clock.shots % 4 == 0 && s_ballCount <= 3;
    }
    return HOOK_CONTINUE;
}

void after_create_child(ModContext*, void* args, void* retval, void*) {
    if (!s_spreadShot || s_spawningSpread ||
        mods::arg<s16>(args, 0) != fpcNm_E_TK_BALL_e ||
        *static_cast<fpc_ProcID*>(retval) == fpcM_ERROR_PROCESS_ID_e) return;

    s_spreadShot = false;
    s_spawningSpread = true;
    for (const s16 offset : {-0x900, 0x900}) {
        csXyz angle = s_ballAngle;
        angle.y = static_cast<s16>(angle.y + offset);
        fopAcM_createChild(mods::arg<s16>(args, 0), mods::arg<fpc_ProcID>(args, 1),
            mods::arg<u32>(args, 2), mods::arg<const cXyz*>(args, 3),
            mods::arg<int>(args, 4), &angle, mods::arg<const cXyz*>(args, 6),
            mods::arg<s8>(args, 7), mods::arg<createFunc>(args, 8));
    }
    s_spawningSpread = false;
}

void before_collision(EnemySlowStep& step) {
    // The body is anchored to the lava height. Only its animation/turning moves.
    step.directCollision = nullptr;
    if (repeated_attack(step)) static_cast<e_tk2_class*>(step.actor)->mTKBallSpawned = false;
}

ModResult install() {
    auto result = mods::hook::add_pre<CreateChildHook>(svc_hook, before_create_child);
    if (result == MOD_OK) result = mods::hook::add_post<CreateChildHook>(svc_hook, after_create_child);
    return result;
}

void reset() {
    s_shotClocks = {};
    s_spreadShot = false;
    s_spawningSpread = false;
}
}

const EnemySlowProfile& fire_toadpoli_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_TK2_e, eligible, prepare, nullptr, nullptr, install, reset,
        before_collision, nullptr, true
    };
    return profile;
}
}
