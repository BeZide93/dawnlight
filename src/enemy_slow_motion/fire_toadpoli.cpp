#include "profile.hpp"
#include "../enemy_hard_mode.hpp"
#include "d/actor/d_a_e_tk2.h"
#include "d/actor/d_a_e_tk_ball.h"
#include "d/d_com_inf_game.h"
#include <array>

namespace dawnlight {
namespace {
DEFINE_HOOK(&fopAcM_createChild, CreateChildHook);

struct ShotClock {
    fpc_ProcID id = fpcM_ERROR_PROCESS_ID_e;
    u8 shots = 0;
    fpc_ProcID primaryBall = fpcM_ERROR_PROCESS_ID_e;
    std::array<fpc_ProcID, 2> spreadBalls{
        fpcM_ERROR_PROCESS_ID_e, fpcM_ERROR_PROCESS_ID_e};
    bool leadShot = false;
    bool primaryLaunched = false;
};

std::array<ShotClock, 16> s_shotClocks{};
ShotClock* s_creatingShot = nullptr;
bool s_createSpread = false;
bool s_spawningSpread = false;
int s_ballCount = 0;

ShotClock& shot_clock(fopAc_ac_c* actor) {
    const auto id = fopAcM_GetID(actor);
    auto& clock = s_shotClocks[id % s_shotClocks.size()];
    if (clock.id != id) {
        clock = {};
        clock.id = id;
    }
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
        clock.primaryBall = fpcM_ERROR_PROCESS_ID_e;
        clock.spreadBalls = {fpcM_ERROR_PROCESS_ID_e, fpcM_ERROR_PROCESS_ID_e};
        clock.leadShot = (clock.shots & 1U) == 0;
        clock.primaryLaunched = false;
        s_creatingShot = &clock;

        s_ballCount = 0;
        fpcM_Search(count_balls, nullptr);
        s_createSpread = clock.shots % 4 == 0 && s_ballCount <= 3;
    }
    return HOOK_CONTINUE;
}

void after_create_child(ModContext*, void* args, void* retval, void*) {
    if (s_spawningSpread) return;

    auto* shot = s_creatingShot;
    const bool createSpread = s_createSpread;
    s_creatingShot = nullptr;
    s_createSpread = false;
    if (shot == nullptr || mods::arg<s16>(args, 0) != fpcNm_E_TK_BALL_e ||
        *static_cast<fpc_ProcID*>(retval) == fpcM_ERROR_PROCESS_ID_e) {
        return;
    }

    shot->primaryBall = *static_cast<fpc_ProcID*>(retval);
    if (!createSpread) return;

    s_spawningSpread = true;
    for (std::size_t i = 0; i < shot->spreadBalls.size(); ++i) {
        shot->spreadBalls[i] = fopAcM_createChild(
            mods::arg<s16>(args, 0), mods::arg<fpc_ProcID>(args, 1),
            mods::arg<u32>(args, 2), mods::arg<const cXyz*>(args, 3),
            mods::arg<int>(args, 4), mods::arg<const csXyz*>(args, 5),
            mods::arg<const cXyz*>(args, 6),
            mods::arg<s8>(args, 7), mods::arg<createFunc>(args, 8));
    }
    s_spawningSpread = false;
}

bool launch_ball(fpc_ProcID id, e_tk2_class& parent, bool leadShot, s16 yawOffset) {
    auto* ball = static_cast<e_tk_ball_class*>(fopAcM_SearchByID(id));
    auto* player = dComIfGp_getPlayer(0);
    if (ball == nullptr || ball->mpModel == nullptr || player == nullptr) return false;

    ball->current.pos = parent.eyePos;
    ball->old.pos = ball->current.pos;
    ball->home.pos = ball->current.pos;

    cXyz target = player->eyePos;
    target.y -= 20.0f;
    if (leadShot) target += player->speed * 10.0f;
    cXyz direction = target - ball->current.pos;
    f32 distance = direction.abs();
    if (distance < 0.001f) {
        direction.set(0.0f, 0.0f, 1.0f);
        distance = 1.0f;
    }

    direction *= 50.0f / distance;
    const f32 sinYaw = cM_ssin(yawOffset);
    const f32 cosYaw = cM_scos(yawOffset);
    const f32 speedX = direction.x * cosYaw + direction.z * sinYaw;
    const f32 speedZ = direction.z * cosYaw - direction.x * sinYaw;
    direction.x = speedX;
    direction.z = speedZ;

    ball->speed = direction;
    ball->current.angle.y = cM_atan2s(direction.x, direction.z);
    ball->current.angle.x = -cM_atan2s(
        direction.y, JMAFastSqrt(direction.x * direction.x + direction.z * direction.z));
    ball->mInitalPosition = ball->current.pos;
    ball->mInitalDistance = distance < 10.0f ? 10.0f : distance;
    ball->mArcHeight = 0.0f;
    ball->mAction = 0;
    ball->mMode = 1;
    ball->mActionTimer[0] = 100;
    ball->mActionTimer[1] = 0;
    ball->mAtSph.OnAtVsPlayerBit();
    ball->mAtSph.OffAtVsEnemyBit();
    ball->mAtSph.StartCAt(ball->current.pos);
    ball->mPreviousPosition = ball->current.pos;
    ball->mSuspended = false;
    return true;
}

void after_execute(EnemySlowStep& step) {
    auto& parent = *static_cast<e_tk2_class*>(step.actor);
    auto& shot = shot_clock(step.actor);
    if (shot.primaryBall == fpcM_ERROR_PROCESS_ID_e) return;

    auto* primary = static_cast<e_tk_ball_class*>(fopAcM_SearchByID(shot.primaryBall));
    if (primary == nullptr || primary->mpModel == nullptr || primary->mSuspended) return;

    if (!shot.primaryLaunched) {
        shot.primaryLaunched = launch_ball(shot.primaryBall, parent, shot.leadShot, 0);
    }

    static constexpr std::array<s16, 2> offsets{-0x900, 0x900};
    for (std::size_t i = 0; i < shot.spreadBalls.size(); ++i) {
        if (shot.spreadBalls[i] != fpcM_ERROR_PROCESS_ID_e &&
            launch_ball(shot.spreadBalls[i], parent, shot.leadShot, offsets[i])) {
            shot.spreadBalls[i] = fpcM_ERROR_PROCESS_ID_e;
        }
    }

    if (shot.primaryLaunched &&
        shot.spreadBalls[0] == fpcM_ERROR_PROCESS_ID_e &&
        shot.spreadBalls[1] == fpcM_ERROR_PROCESS_ID_e) {
        shot.primaryBall = fpcM_ERROR_PROCESS_ID_e;
        shot.primaryLaunched = false;
    }
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
    s_creatingShot = nullptr;
    s_createSpread = false;
    s_spawningSpread = false;
}
}

const EnemySlowProfile& fire_toadpoli_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_TK2_e, eligible, prepare, nullptr, nullptr, install, reset,
        before_collision, after_execute, true
    };
    return profile;
}
}
