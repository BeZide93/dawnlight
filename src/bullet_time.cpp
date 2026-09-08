#include "bullet_time.hpp"

#include "aim_hooks.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "stamina.hpp"

#include "SSystem/SComponent/c_cc_d.h"
#include "SSystem/SComponent/c_cc_s.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_arrow.h"
#include "d/d_cc_s.h"
#include "d/d_cc_uty.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dawnlight {
namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint64_t kEnemySlowFrameInterval = 10;
constexpr std::uint64_t kFlurryLinkSlowFrameInterval = 4;
constexpr std::uint64_t kArrowSlowFrameInterval = 5;
constexpr float kLinkTimeScale = 0.1f;
constexpr auto kBulletTimeDuration = std::chrono::seconds(5);
constexpr auto kManualJumpTimeout = std::chrono::seconds(7);
constexpr auto kFlurryRushDuration = std::chrono::seconds(3);
constexpr float kPerfectDodgeMargin = 45.0f;
constexpr float kFlurryRushMeleeDistance = 120.0f;
constexpr auto kColliderCacheEntries = std::size_t{256};
constexpr auto kCollidersPerActor = std::size_t{64};
constexpr auto kHitActorEntries = std::size_t{32};
constexpr auto kArrowFlightEntries = std::size_t{8};
constexpr std::uint64_t kHitExecuteGraceFrames = 6;

#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__APPLE__)
DEFINE_HOOK_SYMBOL("_ZL13fopAc_ExecutePv", int(void*), ActorExecuteHook);
#else
DEFINE_HOOK_SYMBOL("fopAc_Execute", int(void*), ActorExecuteHook);
#endif
#if defined(__APPLE__)
DEFINE_HOOK(&fpcMtd_Method, ProcessMethodHook);
#else
DEFINE_HOOK(&fpcMtd_Execute, ProcessExecuteHook);
#endif
DEFINE_HOOK(&cCcS::Set, ColliderSetHook);
DEFINE_HOOK(&daArrow_c::atHitCallBack, ArrowHitHook);
DEFINE_HOOK(&daAlink_c::posMove, LinkPosMoveHook);
DEFINE_HOOK(&daAlink_c::checkDamageAction, LinkDamageActionHook);
DEFINE_HOOK(&cCcS::SetAtTgCommonHitInf, CommonAtTgHitHook);
DEFINE_HOOK(&cc_at_check, AtCheckHook);
DEFINE_HOOK(&at_power_check, FlurryAttackPowerHook);
DEFINE_HOOK(&J3DModel::calc, CombatModelCalcHook);
DEFINE_HOOK(&J3DModel::viewCalc, CombatModelViewCalcHook);

struct ColliderCacheEntry {
    fopAc_ac_c* actor = nullptr;
    std::array<cCcD_Obj*, kCollidersPerActor> colliders{};
    std::uint64_t lastSeenFrame = 0;
    u16 actorId = 0;
    std::size_t count = 0;
};

struct HitActorEntry {
    fopAc_ac_c* actor = nullptr;
    std::uint64_t frame = 0;
};

struct ArrowFlightEntry {
    daArrow_c* arrow = nullptr;
    u16 actorId = 0;
};

struct LinkPositionStep {
    daAlink_c* link = nullptr;
    cXyz startPosition{};
    float gravity = 0.0f;
    bool active = false;
};

struct DeferredFlurryDamage {
    fopAc_ac_c* targetActor = nullptr;
    fpc_ProcID targetActorId = fpcM_ERROR_PROCESS_ID_e;
    cCcD_Obj* attackCollider = nullptr;
    cCcD_Obj* targetCollider = nullptr;
    cXyz hitPosition{};
    std::uint64_t lastAttackSerial = 0;
    bool setAttackHit = false;
    bool pending = false;
};

using MatrixPose = std::array<float, 12>;
using VectorPose = std::array<float, 3>;

struct QuaternionPose {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

struct TransformPose {
    VectorPose translation{};
    VectorPose scale{1.0f, 1.0f, 1.0f};
    QuaternionPose rotation{};
};

struct ModelVisualState {
    J3DModel* model = nullptr;
    fopAc_ac_c* actor = nullptr;
    u16 actorId = 0;
    std::uint64_t targetFrame = 0;
    std::uint64_t captureFrame = 0;
    std::uint64_t lastSeenFrame = 0;
    std::vector<MatrixPose> startJoints;
    std::vector<MatrixPose> targetJoints;
    std::vector<MatrixPose> backupJoints;
    std::vector<MatrixPose> startWeights;
    std::vector<MatrixPose> targetWeights;
    std::vector<MatrixPose> backupWeights;
    MatrixPose startBase{};
    MatrixPose targetBase{};
    MatrixPose backupBase{};
    VectorPose rootStartTangent{};
    bool initialized = false;
    bool viewApplied = false;
};

daAlink_c* s_manualJumpOwner = nullptr;
Clock::time_point s_manualJumpStarted{};
Clock::time_point s_bulletTimeStarted{};
daAlink_c* s_flurryRushOwner = nullptr;
fopAc_ac_c* s_flurryRushTarget = nullptr;
Clock::time_point s_flurryRushStarted{};
daAlink_c* s_dodgeOwner = nullptr;
u16 s_dodgeProc = daAlink_c::PROC_WAIT;
bool s_dodgeTriggered = false;
std::array<ColliderCacheEntry, kColliderCacheEntries> s_colliderCache{};
std::array<HitActorEntry, kHitActorEntries> s_hitActors{};
std::array<ArrowFlightEntry, kArrowFlightEntries> s_flyingArrows{};
std::array<fopAc_ac_c*, 8> s_actorExecuteStack{};
std::size_t s_actorExecuteDepth = 0;
std::uint64_t s_slowFrame = 0;
float s_previousGravity = 0.0f;
float s_previousMaxFallSpeed = 0.0f;
bool s_previousSpecialGravity = false;
bool s_bulletTimeActive = false;
bool s_flurryRushActive = false;
bool s_flurryLinkSlowed = false;
bool s_bulletTimeUsedForJump = false;
std::uint64_t s_flurrySwordAttackSerial = 0;
u16 s_flurryLastSwordProc = daAlink_c::PROC_WAIT;
u8 s_flurryLastCutCount = 0;
bool s_flurrySwordAttackWasActive = false;
LinkPositionStep s_linkPositionStep{};
DeferredFlurryDamage s_deferredFlurryDamage{};
std::array<ModelVisualState, 128> s_modelVisualStates{};
cCcD_Obj* s_heldFlurryPowerCollider = nullptr;
fopAc_ac_c* s_heldFlurryTarget = nullptr;
std::uint32_t s_heldFlurryAttackCount = 0;
thread_local bool s_flurryDamageCheckActive = false;
thread_local bool s_flurryDamageScaled = false;

bool combat_slow_active() {
    return s_bulletTimeActive || s_flurryRushActive;
}

void clear_model_visual_states() {
    for (ModelVisualState& state : s_modelVisualStates) {
        state = {};
    }
}

void clear_combat_time_caches() {
    s_colliderCache = {};
    s_hitActors = {};
    s_flyingArrows = {};
    s_slowFrame = 0;
    clear_model_visual_states();
}

void clear_deferred_flurry_damage() {
    s_deferredFlurryDamage = {};
}

void clear_held_flurry_damage() {
    s_heldFlurryPowerCollider = nullptr;
    s_heldFlurryTarget = nullptr;
    s_heldFlurryAttackCount = 0;
}

void release_deferred_flurry_damage() {
    const DeferredFlurryDamage deferred = s_deferredFlurryDamage;
    clear_deferred_flurry_damage();
    if (!deferred.pending || s_flurrySwordAttackSerial == 0 ||
        s_flurryRushOwner == nullptr ||
        daAlink_getAlinkActorClass() != s_flurryRushOwner ||
        fopAcM_SearchByID(deferred.targetActorId) != deferred.targetActor ||
        deferred.attackCollider == nullptr || deferred.targetCollider == nullptr ||
        deferred.attackCollider->GetAc() != s_flurryRushOwner ||
        deferred.targetCollider->GetAc() != deferred.targetActor)
    {
        return;
    }

    cCcD_Stts* attackStatus = deferred.attackCollider->GetStts();
    cCcD_Stts* targetStatus = deferred.targetCollider->GetStts();
    auto* attackInfo = static_cast<dCcD_GObjInf*>(
        deferred.attackCollider->GetGObjInf());
    auto* targetInfo = static_cast<dCcD_GObjInf*>(
        deferred.targetCollider->GetGObjInf());
    if (attackStatus == nullptr || targetStatus == nullptr ||
        attackInfo == nullptr || targetInfo == nullptr)
    {
        return;
    }

    clear_held_flurry_damage();
    if (deferred.setAttackHit) {
        deferred.attackCollider->SetAtHit(deferred.targetCollider);
    }
    deferred.targetCollider->SetTgHit(deferred.attackCollider);

    cXyz hitPosition = deferred.hitPosition;
    dComIfG_Ccsp()->SetAtTgGObjInf(
        deferred.setAttackHit, true,
        deferred.attackCollider, deferred.targetCollider,
        attackInfo, targetInfo,
        attackStatus, targetStatus,
        attackStatus->GetGStts(), targetStatus->GetGStts(),
        &hitPosition);
    s_heldFlurryPowerCollider = deferred.attackCollider;
    s_heldFlurryTarget = deferred.targetActor;
    s_heldFlurryAttackCount = static_cast<std::uint32_t>(s_flurrySwordAttackSerial);
}

void stop_bullet_time() {
    if (!s_bulletTimeActive) {
        return;
    }

    if (s_manualJumpOwner != nullptr &&
        daAlink_getAlinkActorClass() == s_manualJumpOwner)
    {
        s_manualJumpOwner->setSpecialGravity(
            s_previousGravity, s_previousMaxFallSpeed,
            s_previousSpecialGravity ? FALSE : TRUE);
    }
    s_bulletTimeActive = false;
    if (!combat_slow_active()) {
        clear_combat_time_caches();
    }
}

void stop_flurry_rush(bool releaseDamage = true) {
    if (!s_flurryRushActive) {
        clear_deferred_flurry_damage();
        return;
    }

    if (releaseDamage) {
        release_deferred_flurry_damage();
    } else {
        clear_deferred_flurry_damage();
        clear_held_flurry_damage();
    }
    s_flurryRushActive = false;
    s_flurryRushOwner = nullptr;
    s_flurryRushTarget = nullptr;
    s_flurryLinkSlowed = false;
    s_flurrySwordAttackSerial = 0;
    s_flurryLastSwordProc = daAlink_c::PROC_WAIT;
    s_flurryLastCutCount = 0;
    s_flurrySwordAttackWasActive = false;
    if (!combat_slow_active()) {
        clear_combat_time_caches();
    }
}

void start_bullet_time(daAlink_c* link) {
    if (s_bulletTimeActive || s_bulletTimeUsedForJump || link == nullptr ||
        !stamina_available_for_bullet_time())
    {
        return;
    }

    stop_flurry_rush();
    s_previousGravity = link->gravity;
    s_previousMaxFallSpeed = link->maxFallSpeed;
    s_previousSpecialGravity =
        link->checkNoResetFlg3(daPy_py_c::FLG3_UNK_4000) != 0;
    clear_combat_time_caches();
    s_bulletTimeStarted = Clock::now();
    link->setSpecialGravity(s_previousGravity, s_previousMaxFallSpeed, FALSE);
    s_bulletTimeActive = true;
    s_bulletTimeUsedForJump = true;
}

bool actor_is_exempt(fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return true;
    }

    switch (fopAcM_GetName(actor)) {
    case fpcNm_ALINK_e:
    case fpcNm_ARROW_e:
        return true;
    default:
        return false;
    }
}

ColliderCacheEntry* find_collider_entry(fopAc_ac_c* actor, bool create) {
    ColliderCacheEntry* oldest = &s_colliderCache.front();

    for (ColliderCacheEntry& entry : s_colliderCache) {
        if (entry.actor == actor) {
            if (entry.actorId != actor->setID) {
                entry = {};
                entry.actor = actor;
                entry.actorId = actor->setID;
            }
            return &entry;
        }
        if (entry.actor == nullptr && create) {
            entry.actor = actor;
            entry.actorId = actor->setID;
            return &entry;
        }
        if (entry.lastSeenFrame < oldest->lastSeenFrame) {
            oldest = &entry;
        }
    }

    if (!create) {
        return nullptr;
    }

    *oldest = {};
    oldest->actor = actor;
    oldest->actorId = actor->setID;
    return oldest;
}

void remember_collider(cCcD_Obj* collider) {
    if (!combat_slow_active() || collider == nullptr) {
        return;
    }

    fopAc_ac_c* actor = collider->GetAc();
    const bool cacheSlowedLink = actor != nullptr &&
                                  fopAcM_GetName(actor) == fpcNm_ALINK_e &&
                                  s_flurryRushActive && s_flurryLinkSlowed &&
                                  actor == s_flurryRushOwner;
    if (actor_is_exempt(actor) && !cacheSlowedLink) {
        return;
    }

    ColliderCacheEntry* entry = find_collider_entry(actor, true);
    entry->lastSeenFrame = s_slowFrame;
    for (std::size_t i = 0; i < entry->count; ++i) {
        if (entry->colliders[i] == collider) {
            return;
        }
    }
    if (entry->count < entry->colliders.size()) {
        entry->colliders[entry->count++] = collider;
    }
}

void prime_actor_colliders(fopAc_ac_c* actor) {
    ColliderCacheEntry* entry = find_collider_entry(actor, false);
    if (entry == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < entry->count; ++i) {
        if (entry->colliders[i] != nullptr) {
            dComIfG_Ccsp()->Set(entry->colliders[i]);
        }
    }
}

void suppress_flurry_link_hits(cCcD_Obj* collider) {
    if (!s_flurryRushActive || s_flurryRushOwner == nullptr || collider == nullptr ||
        collider->GetAc() != s_flurryRushOwner)
    {
        return;
    }

    for (dCcD_Cyl& body : s_flurryRushOwner->mTgCyls) {
        if (collider == &body) {
            body.OffTgSetBit();
            body.ResetTgHit();
            return;
        }
    }

    if (collider == &s_flurryRushOwner->mAtSph) {
        s_flurryRushOwner->mAtSph.OffTgSetBit();
        s_flurryRushOwner->mAtSph.ResetTgHit();
    }
}

void disable_flurry_link_targets(daAlink_c* link) {
    if (link == nullptr) {
        return;
    }

    for (dCcD_Cyl& body : link->mTgCyls) {
        body.OffTgSetBit();
        body.ResetTgHit();
    }
    link->mAtSph.OffTgSetBit();
    link->mAtSph.ResetTgHit();
}

bool actor_has_hit_grace(fopAc_ac_c* actor) {
    for (const HitActorEntry& entry : s_hitActors) {
        if (entry.actor == actor && s_slowFrame - entry.frame <= kHitExecuteGraceFrames) {
            return true;
        }
    }
    return false;
}

void mark_actor_hit(fopAc_ac_c* actor) {
    if (!combat_slow_active() || actor_is_exempt(actor)) {
        return;
    }

    HitActorEntry* oldest = &s_hitActors.front();
    for (HitActorEntry& entry : s_hitActors) {
        if (entry.actor == actor || entry.actor == nullptr) {
            entry.actor = actor;
            entry.frame = s_slowFrame;
            return;
        }
        if (entry.frame < oldest->frame) {
            oldest = &entry;
        }
    }
    oldest->actor = actor;
    oldest->frame = s_slowFrame;
}

bool arrow_flight_was_initialized(daArrow_c* arrow) {
    if (arrow == nullptr) {
        return false;
    }

    if (arrow->checkWait()) {
        for (ArrowFlightEntry& entry : s_flyingArrows) {
            if (entry.arrow == arrow) {
                entry = {};
            }
        }
        return false;
    }

    ArrowFlightEntry* freeEntry = nullptr;
    for (ArrowFlightEntry& entry : s_flyingArrows) {
        if (entry.arrow == arrow) {
            if (entry.actorId == arrow->setID) {
                return true;
            }
            entry = {};
        }
        if (entry.arrow == nullptr && freeEntry == nullptr) {
            freeEntry = &entry;
        }
    }

    if (freeEntry == nullptr) {
        freeEntry = &s_flyingArrows.front();
    }
    *freeEntry = {.arrow = arrow, .actorId = arrow->setID};
    return false;
}

bool flurry_dodge_active(const daAlink_c* link) {
    return link != nullptr &&
           (link->mProcID == daAlink_c::PROC_SIDESTEP ||
               link->mProcID == daAlink_c::PROC_BACK_JUMP);
}

bool actor_uses_visual_slowdown(fopAc_ac_c* actor) {
    if (!combat_slow_active() || actor == nullptr) {
        return false;
    }

    if (fopAcM_GetName(actor) == fpcNm_ALINK_e) {
        auto* link = static_cast<daAlink_c*>(actor);
        return s_flurryRushActive && s_flurryLinkSlowed &&
               s_flurryRushOwner == link && flurry_dodge_active(link);
    }

    return !actor_is_exempt(actor) && !actor_has_hit_grace(actor);
}

std::uint64_t actor_visual_slow_frame_interval(fopAc_ac_c* actor) {
    return fopAcM_GetName(actor) == fpcNm_ALINK_e
               ? kFlurryLinkSlowFrameInterval
               : kEnemySlowFrameInterval;
}

void read_matrix(MtxP matrix, MatrixPose& pose) {
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            pose[row * 4 + column] = matrix[row][column];
        }
    }
}

void write_matrix(const MatrixPose& pose, MtxP matrix) {
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            matrix[row][column] = pose[row * 4 + column];
        }
    }
}

void write_interpolated_matrix(const MatrixPose& start, const MatrixPose& target,
                               float progress, MtxP matrix) {
    MatrixPose pose{};
    for (std::size_t i = 0; i < pose.size(); ++i) {
        pose[i] = start[i] + (target[i] - start[i]) * progress;
    }
    write_matrix(pose, matrix);
}

float vector_length(float x, float y, float z) {
    return std::sqrt(x * x + y * y + z * z);
}

bool decompose_transform(const MatrixPose& pose, TransformPose& transform) {
    constexpr float kMinimumScale = 0.0001f;
    constexpr float kMaximumAxisDot = 0.001f;

    float scaleX = vector_length(pose[0], pose[4], pose[8]);
    const float scaleY = vector_length(pose[1], pose[5], pose[9]);
    const float scaleZ = vector_length(pose[2], pose[6], pose[10]);
    if (scaleX < kMinimumScale || scaleY < kMinimumScale ||
        scaleZ < kMinimumScale)
    {
        return false;
    }

    float r00 = pose[0] / scaleX;
    float r10 = pose[4] / scaleX;
    float r20 = pose[8] / scaleX;
    const float r01 = pose[1] / scaleY;
    const float r11 = pose[5] / scaleY;
    const float r21 = pose[9] / scaleY;
    const float r02 = pose[2] / scaleZ;
    const float r12 = pose[6] / scaleZ;
    const float r22 = pose[10] / scaleZ;

    if (std::abs(r00 * r01 + r10 * r11 + r20 * r21) > kMaximumAxisDot ||
        std::abs(r00 * r02 + r10 * r12 + r20 * r22) > kMaximumAxisDot ||
        std::abs(r01 * r02 + r11 * r12 + r21 * r22) > kMaximumAxisDot)
    {
        return false;
    }

    const float determinant =
        r00 * (r11 * r22 - r12 * r21) -
        r01 * (r10 * r22 - r12 * r20) +
        r02 * (r10 * r21 - r11 * r20);
    if (determinant < 0.0f) {
        scaleX = -scaleX;
        r00 = -r00;
        r10 = -r10;
        r20 = -r20;
    }

    QuaternionPose rotation;
    const float trace = r00 + r11 + r22;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        rotation.w = 0.25f * s;
        rotation.x = (r21 - r12) / s;
        rotation.y = (r02 - r20) / s;
        rotation.z = (r10 - r01) / s;
    } else if (r00 > r11 && r00 > r22) {
        const float s = std::sqrt(1.0f + r00 - r11 - r22) * 2.0f;
        rotation.w = (r21 - r12) / s;
        rotation.x = 0.25f * s;
        rotation.y = (r01 + r10) / s;
        rotation.z = (r02 + r20) / s;
    } else if (r11 > r22) {
        const float s = std::sqrt(1.0f + r11 - r00 - r22) * 2.0f;
        rotation.w = (r02 - r20) / s;
        rotation.x = (r01 + r10) / s;
        rotation.y = 0.25f * s;
        rotation.z = (r12 + r21) / s;
    } else {
        const float s = std::sqrt(1.0f + r22 - r00 - r11) * 2.0f;
        rotation.w = (r10 - r01) / s;
        rotation.x = (r02 + r20) / s;
        rotation.y = (r12 + r21) / s;
        rotation.z = 0.25f * s;
    }

    const float rotationLength = vector_length(
        rotation.x, rotation.y, rotation.z);
    const float quaternionLength =
        std::sqrt(rotationLength * rotationLength + rotation.w * rotation.w);
    if (quaternionLength < kMinimumScale) {
        return false;
    }
    rotation.x /= quaternionLength;
    rotation.y /= quaternionLength;
    rotation.z /= quaternionLength;
    rotation.w /= quaternionLength;

    transform.translation = {pose[3], pose[7], pose[11]};
    transform.scale = {scaleX, scaleY, scaleZ};
    transform.rotation = rotation;
    return true;
}

QuaternionPose interpolate_rotation(
    const QuaternionPose& start, const QuaternionPose& target, float progress)
{
    QuaternionPose end = target;
    const float dot = start.x * end.x + start.y * end.y +
                      start.z * end.z + start.w * end.w;
    if (dot < 0.0f) {
        end.x = -end.x;
        end.y = -end.y;
        end.z = -end.z;
        end.w = -end.w;
    }

    QuaternionPose result{
        start.x + (end.x - start.x) * progress,
        start.y + (end.y - start.y) * progress,
        start.z + (end.z - start.z) * progress,
        start.w + (end.w - start.w) * progress,
    };
    const float length = std::sqrt(result.x * result.x + result.y * result.y +
                                   result.z * result.z + result.w * result.w);
    if (length > 0.0001f) {
        result.x /= length;
        result.y /= length;
        result.z /= length;
        result.w /= length;
    }
    return result;
}

float interpolate_root_translation(float start, float target, float startTangent,
                                   float progress) {
    const float progress2 = progress * progress;
    const float progress3 = progress2 * progress;
    const float endTangent = target - start;
    const float value = (2.0f * progress3 - 3.0f * progress2 + 1.0f) * start +
                        (progress3 - 2.0f * progress2 + progress) * startTangent +
                        (-2.0f * progress3 + 3.0f * progress2) * target +
                        (progress3 - progress2) * endTangent;
    return std::clamp(value, std::min(start, target), std::max(start, target));
}

void compose_transform(const TransformPose& transform, MatrixPose& pose) {
    const QuaternionPose& q = transform.rotation;
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float xw = q.x * q.w;
    const float yw = q.y * q.w;
    const float zw = q.z * q.w;

    pose[0] = (1.0f - 2.0f * (yy + zz)) * transform.scale[0];
    pose[4] = (2.0f * (xy + zw)) * transform.scale[0];
    pose[8] = (2.0f * (xz - yw)) * transform.scale[0];
    pose[1] = (2.0f * (xy - zw)) * transform.scale[1];
    pose[5] = (1.0f - 2.0f * (xx + zz)) * transform.scale[1];
    pose[9] = (2.0f * (yz + xw)) * transform.scale[1];
    pose[2] = (2.0f * (xz + yw)) * transform.scale[2];
    pose[6] = (2.0f * (yz - xw)) * transform.scale[2];
    pose[10] = (1.0f - 2.0f * (xx + yy)) * transform.scale[2];
    pose[3] = transform.translation[0];
    pose[7] = transform.translation[1];
    pose[11] = transform.translation[2];
}

void write_interpolated_transform(const MatrixPose& start, const MatrixPose& target,
                                  float progress, MtxP matrix,
                                  const VectorPose* rootStartTangent = nullptr) {
    TransformPose startTransform;
    TransformPose targetTransform;
    if (!decompose_transform(start, startTransform) ||
        !decompose_transform(target, targetTransform))
    {
        write_interpolated_matrix(start, target, progress, matrix);
        return;
    }

    TransformPose result;
    for (std::size_t i = 0; i < result.translation.size(); ++i) {
        result.translation[i] = rootStartTangent == nullptr ?
            startTransform.translation[i] +
                (targetTransform.translation[i] - startTransform.translation[i]) * progress :
            interpolate_root_translation(startTransform.translation[i],
                targetTransform.translation[i], (*rootStartTangent)[i], progress);
        result.scale[i] = startTransform.scale[i] +
                          (targetTransform.scale[i] - startTransform.scale[i]) * progress;
    }
    result.rotation = interpolate_rotation(
        startTransform.rotation, targetTransform.rotation, progress);

    MatrixPose pose{};
    compose_transform(result, pose);
    write_matrix(pose, matrix);
}

ModelVisualState* find_model_visual_state(J3DModel* model, bool create,
                                          fopAc_ac_c* actor = nullptr) {
    ModelVisualState* oldest = &s_modelVisualStates.front();
    for (ModelVisualState& state : s_modelVisualStates) {
        if (state.model == model) {
            if (actor != nullptr &&
                (state.actor != actor || state.actorId != actor->setID))
            {
                state = {};
                state.model = model;
                state.actor = actor;
                state.actorId = actor->setID;
            }
            return &state;
        }
        if (state.model == nullptr && create) {
            state.model = model;
            state.actor = actor;
            state.actorId = actor == nullptr ? 0 : actor->setID;
            return &state;
        }
        if (state.lastSeenFrame < oldest->lastSeenFrame) {
            oldest = &state;
        }
    }

    if (!create) {
        return nullptr;
    }

    *oldest = {};
    oldest->model = model;
    oldest->actor = actor;
    oldest->actorId = actor == nullptr ? 0 : actor->setID;
    return oldest;
}

bool prepare_model_pose_buffers(ModelVisualState& state) {
    J3DModelData* modelData = state.model == nullptr
                                  ? nullptr
                                  : state.model->getModelData();
    if (modelData == nullptr) {
        return false;
    }

    const std::size_t jointCount = modelData->getJointNum();
    const std::size_t weightCount = modelData->getWEvlpMtxNum();
    state.startJoints.resize(jointCount);
    state.targetJoints.resize(jointCount);
    state.backupJoints.resize(jointCount);
    state.startWeights.resize(weightCount);
    state.targetWeights.resize(weightCount);
    state.backupWeights.resize(weightCount);
    return true;
}

void read_model_pose(J3DModel* model, std::vector<MatrixPose>& joints,
                     std::vector<MatrixPose>& weights) {
    for (std::size_t i = 0; i < joints.size(); ++i) {
        read_matrix(model->getAnmMtx(static_cast<int>(i)), joints[i]);
    }
    for (std::size_t i = 0; i < weights.size(); ++i) {
        read_matrix(model->getWeightAnmMtx(static_cast<int>(i)), weights[i]);
    }
}

void write_interpolated_model_pose(ModelVisualState& state, float progress) {
    read_matrix(state.model->getBaseTRMtx(), state.backupBase);
    read_model_pose(state.model, state.backupJoints, state.backupWeights);
    write_interpolated_transform(state.startBase, state.targetBase, progress,
                                 state.model->getBaseTRMtx(),
                                 &state.rootStartTangent);
    for (std::size_t i = 0; i < state.startJoints.size(); ++i) {
        write_interpolated_transform(state.startJoints[i], state.targetJoints[i],
                                     progress,
                                     state.model->getAnmMtx(static_cast<int>(i)));
    }
    for (std::size_t i = 0; i < state.startWeights.size(); ++i) {
        write_interpolated_matrix(state.startWeights[i], state.targetWeights[i],
                                  progress,
                                  state.model->getWeightAnmMtx(static_cast<int>(i)));
    }
    state.viewApplied = true;
}

void restore_model_pose(ModelVisualState& state) {
    write_matrix(state.backupBase, state.model->getBaseTRMtx());
    for (std::size_t i = 0; i < state.backupJoints.size(); ++i) {
        write_matrix(state.backupJoints[i],
                     state.model->getAnmMtx(static_cast<int>(i)));
    }
    for (std::size_t i = 0; i < state.backupWeights.size(); ++i) {
        write_matrix(state.backupWeights[i],
                     state.model->getWeightAnmMtx(static_cast<int>(i)));
    }
    state.viewApplied = false;
}

bool should_skip_actor(fopAc_ac_c* actor) {
    if (!combat_slow_active() || actor == nullptr) {
        return false;
    }

    if (fopAcM_GetName(actor) == fpcNm_ARROW_e) {
        if (!s_bulletTimeActive) {
            return false;
        }
        if (!arrow_flight_was_initialized(static_cast<daArrow_c*>(actor))) {
            return false;
        }
        return s_slowFrame % kArrowSlowFrameInterval != 0;
    }

    if (fopAcM_GetName(actor) == fpcNm_ALINK_e) {
        auto* link = static_cast<daAlink_c*>(actor);
        return s_flurryRushActive && s_flurryLinkSlowed &&
               s_flurryRushOwner == link && flurry_dodge_active(link) &&
               s_slowFrame % kFlurryLinkSlowFrameInterval != 0;
    }

    return !actor_is_exempt(actor) && !actor_has_hit_grace(actor) &&
           s_slowFrame % kEnemySlowFrameInterval != 0;
}

void update_dodge_attempt(daAlink_c* link) {
    if (!flurry_dodge_active(link)) {
        s_dodgeOwner = nullptr;
        s_dodgeProc = daAlink_c::PROC_WAIT;
        s_dodgeTriggered = false;
        return;
    }

    if (s_dodgeOwner != link || s_dodgeProc != link->mProcID) {
        s_dodgeOwner = link;
        s_dodgeProc = link->mProcID;
        s_dodgeTriggered = false;
    }
}

float axis_gap(float minA, float maxA, float minB, float maxB) {
    if (maxA < minB) {
        return minB - maxA;
    }
    if (maxB < minA) {
        return minA - maxB;
    }
    return 0.0f;
}

bool collider_near_link(cCcD_Obj* attack, daAlink_c* link) {
    cCcD_ShapeAttr* attackShape = attack->GetShapeAttr();
    if (attackShape == nullptr) {
        return false;
    }
    attackShape->CalcAabBox();
    const cM3dGAab& attackBounds = attackShape->GetWorkAab();
    const float marginSquared = kPerfectDodgeMargin * kPerfectDodgeMargin;

    for (dCcD_Cyl& body : link->mTgCyls) {
        if (!body.ChkTgSet() || (attack->GetAtGrp() & body.GetTgGrp()) == 0 ||
            (attack->GetAtType() & body.GetTgType()) == 0)
        {
            continue;
        }

        cCcD_ShapeAttr* bodyShape = body.GetShapeAttr();
        if (bodyShape == nullptr) {
            continue;
        }
        bodyShape->CalcAabBox();
        const cM3dGAab& bodyBounds = bodyShape->GetWorkAab();
        const float x = axis_gap(attackBounds.GetMinX(), attackBounds.GetMaxX(),
                                 bodyBounds.GetMinX(), bodyBounds.GetMaxX());
        const float y = axis_gap(attackBounds.GetMinY(), attackBounds.GetMaxY(),
                                 bodyBounds.GetMinY(), bodyBounds.GetMaxY());
        const float z = axis_gap(attackBounds.GetMinZ(), attackBounds.GetMaxZ(),
                                 bodyBounds.GetMinZ(), bodyBounds.GetMaxZ());
        if (x * x + y * y + z * z <= marginSquared) {
            return true;
        }
    }
    return false;
}

void try_start_flurry_rush(cCcD_Obj* attack) {
    if (attack == nullptr || !attack->ChkAtSet() || attack->GetAtType() == AT_TYPE_0 ||
        s_bulletTimeActive || s_flurryRushActive || !flurry_rush_enabled())
    {
        return;
    }

    daAlink_c* link = daAlink_getAlinkActorClass();
    update_dodge_attempt(link);
    if (!flurry_dodge_active(link) || s_dodgeTriggered || !link->checkAttentionLock()) {
        return;
    }

    fopAc_ac_c* target = link->mTargetedActor;
    fopAc_ac_c* attacker = attack->GetAc();
    if (target == nullptr || attacker == nullptr || attacker == link ||
        !daAlink_c::checkEnemyGroup(target) ||
        (attacker != target && !daAlink_c::checkEnemyGroup(attacker)) ||
        !collider_near_link(attack, link))
    {
        return;
    }
    if (!consume_flurry_rush_stamina()) {
        s_dodgeTriggered = true;
        return;
    }

    clear_combat_time_caches();
    s_flurryRushOwner = link;
    s_flurryRushTarget = target;
    s_flurryRushStarted = Clock::now();
    s_flurryRushActive = true;
    s_flurryLinkSlowed = true;
    s_dodgeTriggered = true;
    s_flurrySwordAttackSerial = 0;
    s_flurryLastSwordProc = link->mProcID;
    s_flurryLastCutCount = link->getCutCount();
    s_flurrySwordAttackWasActive = false;
    clear_deferred_flurry_damage();
    disable_flurry_link_targets(link);
}

bool sword_attack_active(const daAlink_c* link) {
    if (link == nullptr || link->mEquipItem != 0x103) {
        return false;
    }

    switch (link->mProcID) {
    case daAlink_c::PROC_CUT_NORMAL:
    case daAlink_c::PROC_CUT_FINISH:
    case daAlink_c::PROC_CUT_FINISH_JUMP_UP:
    case daAlink_c::PROC_CUT_REVERSE:
    case daAlink_c::PROC_CUT_JUMP:
    case daAlink_c::PROC_CUT_TURN:
    case daAlink_c::PROC_CUT_DOWN:
    case daAlink_c::PROC_CUT_HEAD:
    case daAlink_c::PROC_CUT_LARGE_JUMP:
        return true;
    default:
        return false;
    }
}

void track_flurry_sword_attack(daAlink_c* link) {
    if (!s_flurryRushActive || link == nullptr || link != s_flurryRushOwner) {
        return;
    }

    const bool attackActive = sword_attack_active(link);
    const u8 cutCount = link->getCutCount();
    const u16 swordProc = link->mProcID;
    const bool newAttack = attackActive &&
                           (!s_flurrySwordAttackWasActive ||
                               cutCount != s_flurryLastCutCount ||
                               swordProc != s_flurryLastSwordProc);
    if (newAttack) {
        const int attackPower = swordProc == daAlink_c::PROC_CUT_TURN
                                    ? link->mAtSph.GetAtAtp()
                                    : link->mAtCps[0].GetAtAtp();
        if (attackPower > 0) {
            ++s_flurrySwordAttackSerial;
            if (s_flurrySwordAttackSerial == 0) {
                ++s_flurrySwordAttackSerial;
            }
        }
    }

    s_flurrySwordAttackWasActive = attackActive;
    s_flurryLastCutCount = cutCount;
    s_flurryLastSwordProc = swordProc;
}

void move_link_to_flurry_target(daAlink_c* link) {
    if (link == nullptr || link->mTargetedActor == nullptr ||
        link->mTargetedActor != s_flurryRushTarget)
    {
        return;
    }

    const cXyz targetPosition = s_flurryRushTarget->current.pos;
    const float deltaX = link->current.pos.x - targetPosition.x;
    const float deltaZ = link->current.pos.z - targetPosition.z;
    const float distance = cXyz(deltaX, 0.0f, deltaZ).abs();
    if (distance <= kFlurryRushMeleeDistance || distance < 0.001f) {
        return;
    }

    const float scale = kFlurryRushMeleeDistance / distance;
    cXyz destination(
        targetPosition.x + deltaX * scale,
        link->current.pos.y,
        targetPosition.z + deltaZ * scale);
    link->current.pos = destination;
    link->old.pos = destination;
    link->mCcStts.ClrCcMove();
}

void update_flurry_link_attack(fopAc_ac_c* actor) {
    if (!s_flurryRushActive || !s_flurryLinkSlowed || actor != s_flurryRushOwner) {
        return;
    }

    auto* link = static_cast<daAlink_c*>(actor);
    if (!sword_attack_active(link)) {
        return;
    }

    s_flurryLinkSlowed = false;
    move_link_to_flurry_target(link);
}

void apply_bullet_time_fall(daAlink_c* link) {
    if (!s_bulletTimeActive || link == nullptr) {
        return;
    }

    link->setSpecialGravity(s_previousGravity, s_previousMaxFallSpeed, FALSE);
    if (link->speed.y < s_previousMaxFallSpeed) {
        link->speed.y = s_previousMaxFallSpeed;
    }
}

HookAction before_link_pos_move(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_linkPositionStep = {};
    if (!bullet_time_active_for(link)) {
        return HOOK_CONTINUE;
    }

    s_linkPositionStep = {
        .link = link,
        .startPosition = link->current.pos,
        .gravity = link->gravity,
        .active = true,
    };
    link->gravity *= kLinkTimeScale;
    return HOOK_CONTINUE;
}

void after_link_pos_move(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_linkPositionStep.active || s_linkPositionStep.link != link) {
        return;
    }

    link->current.pos.x = s_linkPositionStep.startPosition.x +
                          (link->current.pos.x - s_linkPositionStep.startPosition.x) *
                              kLinkTimeScale;
    link->current.pos.y = s_linkPositionStep.startPosition.y +
                          (link->current.pos.y - s_linkPositionStep.startPosition.y) *
                              kLinkTimeScale;
    link->current.pos.z = s_linkPositionStep.startPosition.z +
                          (link->current.pos.z - s_linkPositionStep.startPosition.z) *
                              kLinkTimeScale;
    link->gravity = s_linkPositionStep.gravity;
    s_linkPositionStep = {};
}

HookAction before_actor_execute(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth < s_actorExecuteStack.size()) {
        s_actorExecuteStack[s_actorExecuteDepth++] =
            static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    }
    return HOOK_CONTINUE;
}

void after_actor_execute(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth == 0) {
        return;
    }

    auto* actor = static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    if (actor == s_flurryRushOwner) {
        track_flurry_sword_attack(static_cast<daAlink_c*>(actor));
    }
    update_flurry_link_attack(actor);
    if (s_actorExecuteStack[s_actorExecuteDepth - 1] == actor) {
        s_actorExecuteStack[--s_actorExecuteDepth] = nullptr;
    }
}

HookAction before_combat_model_calc(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth == 0) {
        return HOOK_CONTINUE;
    }

    auto* model = mods::arg<J3DModel*>(args, 0);
    fopAc_ac_c* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    if (model == nullptr || actor == nullptr) {
        return HOOK_CONTINUE;
    }

    if (!actor_uses_visual_slowdown(actor)) {
        if (ModelVisualState* state = find_model_visual_state(model, false);
            state != nullptr)
        {
            *state = {};
        }
        return HOOK_CONTINUE;
    }

    ModelVisualState* state = find_model_visual_state(model, true, actor);
    if (!prepare_model_pose_buffers(*state)) {
        *state = {};
        return HOOK_CONTINUE;
    }

    if (!state->initialized) {
        read_matrix(model->getBaseTRMtx(), state->startBase);
        state->targetBase = state->startBase;
        read_model_pose(model, state->startJoints, state->startWeights);
        state->targetJoints = state->startJoints;
        state->targetWeights = state->startWeights;
        state->initialized = true;
    } else if (state->captureFrame != s_slowFrame) {
        state->rootStartTangent = {
            state->targetBase[3] - state->startBase[3],
            state->targetBase[7] - state->startBase[7],
            state->targetBase[11] - state->startBase[11],
        };
        state->startBase = state->targetBase;
        state->startJoints = state->targetJoints;
        state->startWeights = state->targetWeights;
    }

    read_matrix(model->getBaseTRMtx(), state->targetBase);
    state->captureFrame = s_slowFrame;
    state->lastSeenFrame = s_slowFrame;
    return HOOK_CONTINUE;
}

void after_combat_model_calc(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth == 0) {
        return;
    }

    auto* model = mods::arg<J3DModel*>(args, 0);
    fopAc_ac_c* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    ModelVisualState* state = find_model_visual_state(model, false);
    if (state == nullptr || !state->initialized || state->actor != actor ||
        !actor_uses_visual_slowdown(actor))
    {
        return;
    }

    read_matrix(model->getBaseTRMtx(), state->targetBase);
    read_model_pose(model, state->targetJoints, state->targetWeights);
    state->targetFrame = s_slowFrame;
    state->lastSeenFrame = s_slowFrame;
}

HookAction before_combat_model_view_calc(ModContext*, void* args, void*, void*) {
    auto* model = mods::arg<J3DModel*>(args, 0);
    ModelVisualState* state = find_model_visual_state(model, false);
    if (state == nullptr || !state->initialized || state->viewApplied) {
        return HOOK_CONTINUE;
    }

    if (state->actor == nullptr ||
        fopAcM_SearchByID(state->actorId) != state->actor ||
        !actor_uses_visual_slowdown(state->actor))
    {
        *state = {};
        return HOOK_CONTINUE;
    }

    J3DModelData* modelData = model->getModelData();
    if (modelData == nullptr ||
        state->startJoints.size() != modelData->getJointNum() ||
        state->startWeights.size() != modelData->getWEvlpMtxNum())
    {
        *state = {};
        return HOOK_CONTINUE;
    }

    const std::uint64_t elapsedFrames = s_slowFrame >= state->targetFrame
                                            ? s_slowFrame - state->targetFrame
                                            : 0;
    const float progress = std::min(
        static_cast<float>(elapsedFrames + 1) /
            static_cast<float>(actor_visual_slow_frame_interval(state->actor)),
        1.0f);
    write_interpolated_model_pose(*state, progress);
    return HOOK_CONTINUE;
}

void after_combat_model_view_calc(ModContext*, void* args, void*, void*) {
    auto* model = mods::arg<J3DModel*>(args, 0);
    ModelVisualState* state = find_model_visual_state(model, false);
    if (state != nullptr && state->viewApplied) {
        restore_model_pose(*state);
    }
}

HookAction before_process_execute(ModContext*, void* args, void* retval, void*) {
    if (s_actorExecuteDepth == 0) {
        return HOOK_CONTINUE;
    }

    auto* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    if (mods::arg<void*>(args, 1) != actor || !should_skip_actor(actor)) {
        return HOOK_CONTINUE;
    }

    prime_actor_colliders(actor);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

#if defined(__APPLE__)
HookAction before_process_method(ModContext*, void* args, void* retval, void*) {
    if (s_actorExecuteDepth == 0) {
        return HOOK_CONTINUE;
    }

    auto* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    const auto method = mods::arg<process_method_func>(args, 0);
    const auto* methods = actor == nullptr
                              ? nullptr
                              : reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (actor == nullptr || actor->sub_method == nullptr ||
        mods::arg<void*>(args, 1) != actor ||
        method != methods->execute_method ||
        !should_skip_actor(actor)) {
        return HOOK_CONTINUE;
    }

    prime_actor_colliders(actor);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}
#endif

HookAction before_collider_set(ModContext*, void* args, void*, void*) {
    cCcD_Obj* collider = mods::arg<cCcD_Obj*>(args, 1);
    try_start_flurry_rush(collider);
    suppress_flurry_link_hits(collider);
    remember_collider(collider);
    return HOOK_CONTINUE;
}

HookAction before_link_damage_action(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_flurryRushActive || link == nullptr || link != s_flurryRushOwner) {
        return HOOK_CONTINUE;
    }

    disable_flurry_link_targets(link);
    *static_cast<BOOL*>(retval) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_arrow_hit(ModContext*, void* args, void*, void*) {
    mark_actor_hit(mods::arg<fopAc_ac_c*>(args, 2));
    return HOOK_CONTINUE;
}

HookAction before_at_check(ModContext*, void* args, void*, void*) {
    auto* enemy = mods::arg<fopAc_ac_c*>(args, 0);
    auto* atInfo = mods::arg<dCcU_AtInfo*>(args, 1);
    s_flurryDamageCheckActive = atInfo != nullptr &&
                                enemy == s_heldFlurryTarget &&
                                atInfo->mpCollider == s_heldFlurryPowerCollider &&
                                s_heldFlurryAttackCount > 0;
    s_flurryDamageScaled = false;
    return HOOK_CONTINUE;
}

void after_flurry_attack_power(ModContext*, void* args, void*, void*) {
    auto* atInfo = mods::arg<dCcU_AtInfo*>(args, 0);
    if (!s_flurryDamageCheckActive || s_flurryDamageScaled || atInfo == nullptr ||
        atInfo->mpCollider != s_heldFlurryPowerCollider || atInfo->mAttackPower == 0)
    {
        return;
    }

    const u16 baseDamage = atInfo->mAttackPower;
    atInfo->mAttackPower = static_cast<u16>(std::min<std::uint32_t>(
        static_cast<std::uint32_t>(baseDamage) * s_heldFlurryAttackCount,
        0xFFFFU));
    s_flurryDamageScaled = true;
}

void after_at_check(ModContext*, void*, void*, void*) {
    if (s_flurryDamageCheckActive) {
        clear_held_flurry_damage();
    }
    s_flurryDamageCheckActive = false;
    s_flurryDamageScaled = false;
}

bool is_flurry_sword_collider(cCcD_Obj* collider) {
    if (s_flurryRushOwner == nullptr || collider == nullptr) {
        return false;
    }

    for (dCcD_Cps& sword : s_flurryRushOwner->mAtCps) {
        if (collider == &sword) {
            return true;
        }
    }
    return collider == &s_flurryRushOwner->mAtSph;
}

void preserve_flurry_attack_hit(cCcD_Obj* attack, cCcD_Obj* target,
                                cXyz* hitPosition) {
    attack->SetAtHit(target);
    auto* attackInfo = static_cast<dCcD_GObjInf*>(attack->GetGObjInf());
    cCcD_Stts* targetStatus = target->GetStts();
    if (attackInfo == nullptr || targetStatus == nullptr) {
        return;
    }

    attackInfo->SetAtHitApid(targetStatus->GetApid());
    attackInfo->SetAtHitPos(*hitPosition);
}

HookAction before_common_at_tg_hit(ModContext*, void* args, void*, void*) {
    if (!s_flurryRushActive || s_flurrySwordAttackSerial == 0) {
        return HOOK_CONTINUE;
    }

    auto* attack = mods::arg<cCcD_Obj*>(args, 1);
    auto* target = mods::arg<cCcD_Obj*>(args, 2);
    if (attack == nullptr || target == nullptr ||
        attack->GetAc() != s_flurryRushOwner ||
        !is_flurry_sword_collider(attack))
    {
        return HOOK_CONTINUE;
    }

    fopAc_ac_c* targetActor = target->GetAc();
    cXyz* hitPosition = mods::arg<cXyz*>(args, 3);
    const u8 damage = attack->GetAtAtp();
    if (targetActor == nullptr || targetActor != s_flurryRushTarget ||
        !daAlink_c::checkEnemyGroup(targetActor) || hitPosition == nullptr || damage == 0)
    {
        return HOOK_CONTINUE;
    }

    preserve_flurry_attack_hit(attack, target, hitPosition);
    if (s_deferredFlurryDamage.pending &&
        s_deferredFlurryDamage.lastAttackSerial == s_flurrySwordAttackSerial)
    {
        return HOOK_SKIP_ORIGINAL;
    }

    s_deferredFlurryDamage.targetActor = targetActor;
    s_deferredFlurryDamage.targetActorId = fopAcM_GetID(targetActor);
    s_deferredFlurryDamage.attackCollider = attack;
    s_deferredFlurryDamage.targetCollider = target;
    s_deferredFlurryDamage.hitPosition = *hitPosition;
    s_deferredFlurryDamage.lastAttackSerial = s_flurrySwordAttackSerial;
    s_deferredFlurryDamage.setAttackHit = true;
    s_deferredFlurryDamage.pending = true;

    return HOOK_SKIP_ORIGINAL;
}

bool manual_jump_is_airborne(daAlink_c* link) {
    return link != nullptr && link->mProcID == daAlink_c::PROC_AUTO_JUMP &&
           !link->mLinkAcch.ChkGroundHit();
}

bool selected_bow_button_requested(daAlink_c* link) {
    if (link == nullptr) {
        return false;
    }

    const u32 buttons = static_cast<u32>(link->mItemTrigger) |
                        static_cast<u32>(link->mItemButton);
    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X,
        SELECT_ITEM_Y,
        SELECT_ITEM_DOWN,
    };
    for (const u8 slot : slots) {
        if ((buttons & (1u << slot)) != 0 &&
            daPy_py_c::checkBowItem(dComIfGp_getSelectItem(slot)))
        {
            return true;
        }
    }
    return false;
}

bool bow_aim_requested(daAlink_c* link) {
    return selected_bow_button_requested(link) ||
           (link != nullptr && daPy_py_c::checkBowItem(link->mEquipItem) &&
               link->checkReadyItem() && (link->itemTrigger() || link->itemButton()));
}

void prepare_bow_aim(daAlink_c* link) {
    if (link == nullptr || !daPy_py_c::checkBowItem(link->mEquipItem) ||
        !link->checkReadyItem())
    {
        return;
    }

    if (!link->checkBowAnime()) {
        prepare_bullet_time_bow_aim(link);
        link->setBowReadyAnime();
        link->mItemMode = 0;
    }
    link->setBowOrSlingStatus();
}

}  // namespace

ModResult initialize_bullet_time(ModError* error) {
    ModResult result = mods::hook::add_pre<ActorExecuteHook>(svc_hook, before_actor_execute);
    if (result == MOD_OK) {
        result = mods::hook::add_post<ActorExecuteHook>(svc_hook, after_actor_execute);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<CombatModelCalcHook>(
            svc_hook, before_combat_model_calc);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<CombatModelCalcHook>(
            svc_hook, after_combat_model_calc);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<CombatModelViewCalcHook>(
            svc_hook, before_combat_model_view_calc);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<CombatModelViewCalcHook>(
            svc_hook, after_combat_model_view_calc);
    }
    if (result == MOD_OK) {
#if defined(__APPLE__)
        result = mods::hook::add_pre<ProcessMethodHook>(svc_hook, before_process_method);
#else
        result = mods::hook::add_pre<ProcessExecuteHook>(svc_hook, before_process_execute);
#endif
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<ColliderSetHook>(svc_hook, before_collider_set);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<ArrowHitHook>(svc_hook, before_arrow_hit);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LinkPosMoveHook>(svc_hook, before_link_pos_move);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LinkPosMoveHook>(svc_hook, after_link_pos_move);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LinkDamageActionHook>(
            svc_hook, before_link_damage_action);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<CommonAtTgHitHook>(
            svc_hook, before_common_at_tg_hit);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<AtCheckHook>(svc_hook, before_at_check);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<AtCheckHook>(svc_hook, after_at_check);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<FlurryAttackPowerHook>(
            svc_hook, after_flurry_attack_power);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result,
                               "failed to install Dawnlight Bullet Time hooks");
    }
    return MOD_OK;
}

void mark_manual_jump_started(daAlink_c* link) {
    stop_flurry_rush();
    stop_bullet_time();
    s_manualJumpOwner = link;
    s_manualJumpStarted = Clock::now();
    s_bulletTimeUsedForJump = false;
}

void clear_manual_jump(daAlink_c* link) {
    if (link != nullptr && s_manualJumpOwner != link) {
        return;
    }

    stop_bullet_time();
    s_manualJumpOwner = nullptr;
    s_bulletTimeUsedForJump = false;
}

void update_bullet_time_before_jump(daAlink_c* link) {
    if (link == nullptr || s_manualJumpOwner != link) {
        return;
    }

    if (!bullet_time_enabled() || !r_jump_enabled() || !manual_jump_is_airborne(link) ||
        Clock::now() - s_manualJumpStarted >= kManualJumpTimeout)
    {
        clear_manual_jump(link);
        return;
    }

    if (s_bulletTimeActive &&
        (link->doTrigger() || Clock::now() - s_bulletTimeStarted >= kBulletTimeDuration))
    {
        stop_bullet_time();
    }

    if (!s_bulletTimeActive && !s_bulletTimeUsedForJump && bow_aim_requested(link)) {
        start_bullet_time(link);
    }

    if (s_bulletTimeActive) {
        link->setDoStatus(BUTTON_STATUS_BACK);
        apply_bullet_time_fall(link);
        prepare_bow_aim(link);
    }
}

void update_bullet_time_after_jump(daAlink_c* link) {
    if (!bullet_time_active_for(link)) {
        return;
    }

    if (!manual_jump_is_airborne(link)) {
        clear_manual_jump(link);
        return;
    }

    link->setDoStatus(BUTTON_STATUS_BACK);
    apply_bullet_time_fall(link);
    if (daPy_py_c::checkBowItem(link->mEquipItem) &&
        !update_bullet_time_bow_aim(link) && link->setBodyAngleToCamera())
    {
        link->setBowSight();
    }
}

void slow_bullet_time_jump_speed_change(daAlink_c* link, float previousNormalSpeed) {
    if (!bullet_time_active_for(link)) {
        return;
    }

    link->mNormalSpeed = previousNormalSpeed +
                         (link->mNormalSpeed - previousNormalSpeed) * kLinkTimeScale;
}

bool bullet_time_active_for(const daAlink_c* link) {
    return s_bulletTimeActive && s_manualJumpOwner == link;
}

void bullet_time_tick() {
    daAlink_c* currentLink = daAlink_getAlinkActorClass();
    update_dodge_attempt(currentLink);

    if (s_flurryRushActive) {
        const bool targetStillLocked = currentLink == s_flurryRushOwner &&
                                       currentLink != nullptr &&
                                       currentLink->checkAttentionLock() &&
                                       currentLink->mTargetedActor == s_flurryRushTarget &&
                                       s_flurryRushTarget != nullptr;
        const bool timedOut = Clock::now() - s_flurryRushStarted >= kFlurryRushDuration;
        if (!flurry_rush_enabled() || !targetStillLocked || timedOut) {
            stop_flurry_rush();
        }
    }

    if (s_manualJumpOwner != nullptr) {
        if (currentLink != s_manualJumpOwner || !bullet_time_enabled() || !r_jump_enabled()) {
            clear_manual_jump(nullptr);
        } else {
            const auto now = Clock::now();
            if (!manual_jump_is_airborne(currentLink) ||
                now - s_manualJumpStarted >= kManualJumpTimeout)
            {
                clear_manual_jump(currentLink);
            } else if (s_bulletTimeActive &&
                       now - s_bulletTimeStarted >= kBulletTimeDuration)
            {
                stop_bullet_time();
            }
        }
    }

    if (!update_stamina(s_bulletTimeActive) && s_bulletTimeActive) {
        stop_bullet_time();
    }

    if (combat_slow_active()) {
        ++s_slowFrame;
    }
}

void shutdown_bullet_time() {
    stop_flurry_rush(false);
    clear_manual_jump(nullptr);
    s_dodgeOwner = nullptr;
    s_dodgeProc = daAlink_c::PROC_WAIT;
    s_dodgeTriggered = false;
    s_linkPositionStep = {};
    s_actorExecuteStack = {};
    s_actorExecuteDepth = 0;
    clear_model_visual_states();
}

}  // namespace dawnlight
