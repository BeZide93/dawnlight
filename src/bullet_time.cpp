#include "bullet_time.hpp"

#include "aim_hooks.hpp"
#include "config.hpp"
#include "service_imports.hpp"

#include "SSystem/SComponent/c_cc_d.h"
#include "SSystem/SComponent/c_cc_s.h"
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
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace dawnlight {
namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint64_t kSlowFrameInterval = 4;
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
    std::uint32_t damage = 0;
    std::uint64_t lastAttackSerial = 0;
    bool setAttackHit = false;
    bool pending = false;
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
cCcD_Obj* s_heldFlurryPowerCollider = nullptr;
fopAc_ac_c* s_heldFlurryTarget = nullptr;
std::uint32_t s_heldFlurryAttackCount = 0;
thread_local bool s_flurryDamageCheckActive = false;
thread_local bool s_flurryDamageScaled = false;

bool combat_slow_active() {
    return s_bulletTimeActive || s_flurryRushActive;
}

void clear_combat_time_caches() {
    s_colliderCache = {};
    s_hitActors = {};
    s_flyingArrows = {};
    s_slowFrame = 0;
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
    char logMessage[160] = {};
    std::snprintf(logMessage, sizeof(logMessage),
                  "Dawnlight Flurry: release attacks=%llu raw_total=%u pending=%u",
                  static_cast<unsigned long long>(s_flurrySwordAttackSerial),
                  static_cast<unsigned>(deferred.damage), deferred.pending ? 1U : 0U);
    svc_log->info(mod_ctx, logMessage);
    if (!deferred.pending || deferred.damage == 0 || s_flurryRushOwner == nullptr ||
        daAlink_getAlinkActorClass() != s_flurryRushOwner ||
        fopAcM_SearchByID(deferred.targetActorId) != deferred.targetActor ||
        deferred.attackCollider == nullptr || deferred.targetCollider == nullptr ||
        deferred.attackCollider->GetAc() != s_flurryRushOwner ||
        deferred.targetCollider->GetAc() != deferred.targetActor)
    {
        svc_log->warn(mod_ctx, "Dawnlight Flurry: release validation failed");
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
        svc_log->warn(mod_ctx, "Dawnlight Flurry: release collider state missing");
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
    if (s_bulletTimeActive || s_bulletTimeUsedForJump || link == nullptr) {
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
               s_slowFrame % kSlowFrameInterval != 0;
    }

    return !actor_is_exempt(actor) && !actor_has_hit_grace(actor) &&
           s_slowFrame % kSlowFrameInterval != 0;
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
    svc_log->info(mod_ctx, "Dawnlight Flurry: started");
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
            s_deferredFlurryDamage.damage = std::min<std::uint32_t>(
                s_deferredFlurryDamage.damage +
                    static_cast<std::uint32_t>(attackPower),
                0xFFU);
            char logMessage[192] = {};
            std::snprintf(logMessage, sizeof(logMessage),
                          "Dawnlight Flurry: attack=%llu cut_count=%u proc=%u raw=%d total=%u",
                          static_cast<unsigned long long>(s_flurrySwordAttackSerial),
                          static_cast<unsigned>(cutCount), static_cast<unsigned>(swordProc),
                          attackPower,
                          static_cast<unsigned>(s_deferredFlurryDamage.damage));
            svc_log->info(mod_ctx, logMessage);
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

    char logMessage[160] = {};
    std::snprintf(logMessage, sizeof(logMessage),
                  "Dawnlight Flurry: damage base=%u attacks=%u total=%u",
                  static_cast<unsigned>(baseDamage),
                  static_cast<unsigned>(s_heldFlurryAttackCount),
                  static_cast<unsigned>(atInfo->mAttackPower));
    svc_log->info(mod_ctx, logMessage);
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
            svc_log->info(mod_ctx,
                          !flurry_rush_enabled()
                              ? "Dawnlight Flurry: stopping (disabled)"
                              : (!targetStillLocked
                                        ? "Dawnlight Flurry: stopping (lock lost)"
                                        : "Dawnlight Flurry: stopping (timeout)"));
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
}

}  // namespace dawnlight
