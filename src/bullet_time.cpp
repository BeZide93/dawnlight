#include "bullet_time.hpp"

#include "aim_hooks.hpp"
#include "config.hpp"
#include "service_imports.hpp"

#include "SSystem/SComponent/c_cc_d.h"
#include "SSystem/SComponent/c_cc_s.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_arrow.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace dawnlight {
namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint64_t kSlowFrameInterval = 4;
constexpr std::uint64_t kArrowSlowFrameInterval = 5;
constexpr float kLinkTimeScale = 0.1f;
constexpr auto kBulletTimeDuration = std::chrono::seconds(5);
constexpr auto kManualJumpTimeout = std::chrono::seconds(7);
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

daAlink_c* s_manualJumpOwner = nullptr;
Clock::time_point s_manualJumpStarted{};
Clock::time_point s_bulletTimeStarted{};
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
bool s_bulletTimeUsedForJump = false;
LinkPositionStep s_linkPositionStep{};

void clear_combat_time_caches() {
    s_colliderCache = {};
    s_hitActors = {};
    s_flyingArrows = {};
    s_slowFrame = 0;
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
    clear_combat_time_caches();
}

void start_bullet_time(daAlink_c* link) {
    if (s_bulletTimeActive || s_bulletTimeUsedForJump || link == nullptr) {
        return;
    }

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
    if (!s_bulletTimeActive || collider == nullptr) {
        return;
    }

    fopAc_ac_c* actor = collider->GetAc();
    if (actor_is_exempt(actor)) {
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

bool actor_has_hit_grace(fopAc_ac_c* actor) {
    for (const HitActorEntry& entry : s_hitActors) {
        if (entry.actor == actor && s_slowFrame - entry.frame <= kHitExecuteGraceFrames) {
            return true;
        }
    }
    return false;
}

void mark_actor_hit(fopAc_ac_c* actor) {
    if (!s_bulletTimeActive || actor_is_exempt(actor)) {
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

bool should_skip_actor(fopAc_ac_c* actor) {
    if (!s_bulletTimeActive || actor == nullptr) {
        return false;
    }

    if (fopAcM_GetName(actor) == fpcNm_ARROW_e) {
        if (!arrow_flight_was_initialized(static_cast<daArrow_c*>(actor))) {
            return false;
        }
        return s_slowFrame % kArrowSlowFrameInterval != 0;
    }

    return !actor_is_exempt(actor) && !actor_has_hit_grace(actor) &&
           s_slowFrame % kSlowFrameInterval != 0;
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
    remember_collider(mods::arg<cCcD_Obj*>(args, 1));
    return HOOK_CONTINUE;
}

HookAction before_arrow_hit(ModContext*, void* args, void*, void*) {
    mark_actor_hit(mods::arg<fopAc_ac_c*>(args, 2));
    return HOOK_CONTINUE;
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
    if (result != MOD_OK) {
        return mods::set_error(error, result,
                               "failed to install Dawnlight Bullet Time hooks");
    }
    return MOD_OK;
}

void mark_manual_jump_started(daAlink_c* link) {
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
    if (s_manualJumpOwner == nullptr) {
        return;
    }

    daAlink_c* currentLink = daAlink_getAlinkActorClass();
    if (currentLink != s_manualJumpOwner || !bullet_time_enabled() || !r_jump_enabled()) {
        clear_manual_jump(nullptr);
        return;
    }

    const auto now = Clock::now();
    if (!manual_jump_is_airborne(currentLink) ||
        now - s_manualJumpStarted >= kManualJumpTimeout)
    {
        clear_manual_jump(currentLink);
        return;
    }

    if (s_bulletTimeActive && now - s_bulletTimeStarted >= kBulletTimeDuration) {
        stop_bullet_time();
    } else if (s_bulletTimeActive) {
        ++s_slowFrame;
    }
}

void shutdown_bullet_time() {
    clear_manual_jump(nullptr);
    s_linkPositionStep = {};
    s_actorExecuteStack = {};
    s_actorExecuteDepth = 0;
}

}  // namespace dawnlight
