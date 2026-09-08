#include "stamina.hpp"

#include "combat_meter.hpp"
#include "config.hpp"
#include "service_imports.hpp"

#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_object.h"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"

#include <algorithm>
#include <chrono>

namespace dawnlight {
namespace {

DEFINE_HOOK(&dMeter2Draw_c::draw, StaminaMeterDrawHook);
DEFINE_HOOK(&dMeter2Draw_c::drawKanteraScreen, LazySkillMeterScreenHook);
DEFINE_HOOK(&daAlink_c::procGuardAttackInit, LazyGuardAttackHook);
DEFINE_HOOK(&daAlink_c::procCutFinishJumpUpInit, LazyBackSliceHook);
DEFINE_HOOK(&daAlink_c::procCutHeadInit, LazyHelmSplitterHook);
DEFINE_HOOK(&daAlink_c::setWolfLockDomeModel, LazyMidnaChargeHook);

constexpr float kMaximumStamina = 100.0f;
constexpr float kFlurryRushCost = 50.0f;
constexpr float kGreatSpinCost = 40.0f;
constexpr float kBulletTimeDrainPerSecond = 20.0f;
constexpr float kRecoveryPerSecond = 5.0f;
constexpr float kLazySkillCost = 50.0f;
constexpr float kLazyMidnaChargeCost = 100.0f;

using Clock = std::chrono::steady_clock;

struct RuntimeState {
    daAlink_c* link = nullptr;
    u16 linkId = 0;
    float stamina = kMaximumStamina;
    Clock::time_point lastUpdate{};
};

RuntimeState s_state;
bool s_lazyTweaksDetected = false;

bool same_link(daAlink_c* link) {
    return link != nullptr && s_state.link == link && s_state.linkId == link->setID;
}

void reset_for_link(daAlink_c* link) {
    s_state.link = link;
    s_state.linkId = link != nullptr ? link->setID : 0;
    s_state.stamina = kMaximumStamina;
    s_state.lastUpdate = Clock::now();
}

daAlink_c* current_link() {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (!same_link(link)) {
        reset_for_link(link);
    }
    return link;
}

bool menu_or_pause_active() {
    return dMeter2Info_getWindowStatus() != 0 || dMeter2Info_getPauseStatus() != 0 ||
           dComIfGp_isPauseFlag() || dComIfGp_event_runCheck() ||
           dMeter2Info_isShopTalkFlag() || dMsgObject_isTalkNowCheck();
}

bool try_consume(float amount) {
    daAlink_c* link = current_link();
    if (link == nullptr || link->checkDeadHP() || link->checkSceneChangeAreaStart() ||
        s_state.stamina < amount)
    {
        return false;
    }
    s_state.stamina -= amount;
    return true;
}

bool can_consume(float amount) {
    daAlink_c* link = current_link();
    return link != nullptr && !link->checkDeadHP() &&
           !link->checkSceneChangeAreaStart() && s_state.stamina >= amount;
}

bool lazy_stamina_bridge_active() {
    return s_lazyTweaksDetected && stamina_meter_visible();
}

HookAction before_lazy_skill(ModContext*, void*, void* retval, float cost) {
    if (!lazy_stamina_bridge_active() || can_consume(cost)) {
        return HOOK_CONTINUE;
    }
    if (retval != nullptr) {
        *static_cast<int*>(retval) = 0;
    }
    return HOOK_SKIP_ORIGINAL;
}

void after_lazy_skill(ModContext*, void*, void* retval, float cost) {
    if (!lazy_stamina_bridge_active() || retval == nullptr ||
        *static_cast<int*>(retval) == 0)
    {
        return;
    }
    try_consume(cost);
}

HookAction before_lazy_guard_attack(ModContext* ctx, void* args, void* retval, void*) {
    return before_lazy_skill(ctx, args, retval, kLazySkillCost);
}

void after_lazy_guard_attack(ModContext* ctx, void* args, void* retval, void*) {
    after_lazy_skill(ctx, args, retval, kLazySkillCost);
}

HookAction before_lazy_back_slice(ModContext* ctx, void* args, void* retval, void*) {
    return before_lazy_skill(ctx, args, retval, kLazySkillCost);
}

void after_lazy_back_slice(ModContext* ctx, void* args, void* retval, void*) {
    after_lazy_skill(ctx, args, retval, kLazySkillCost);
}

HookAction before_lazy_helm_splitter(ModContext* ctx, void* args, void* retval, void*) {
    return before_lazy_skill(ctx, args, retval, kLazySkillCost);
}

void after_lazy_helm_splitter(ModContext* ctx, void* args, void* retval, void*) {
    after_lazy_skill(ctx, args, retval, kLazySkillCost);
}

HookAction before_lazy_midna_charge(ModContext*, void*, void*, void*) {
    if (!lazy_stamina_bridge_active()) {
        return HOOK_CONTINUE;
    }
    return try_consume(kLazyMidnaChargeCost) ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL;
}

HookAction before_lazy_skill_meter(ModContext*, void* args, void*, void*) {
    if (lazy_stamina_bridge_active() && mods::arg<u8>(args, 1) == 0) {
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void after_meter_draw(ModContext*, void* args, void*, void*) {
    if (!stamina_meter_visible() || menu_or_pause_active()) {
        return;
    }
    draw_combat_meter(mods::arg<dMeter2Draw_c*>(args, 0), s_state.stamina,
        CombatMeterStyle::Stamina, 0);
}

bool resolve_lazy_tweaks_symbol(const char* name) {
    if (svc_hook == nullptr || svc_hook->resolve == nullptr) {
        return false;
    }
    void* address = nullptr;
    HookSymbolFlags flags{};
    return svc_hook->resolve(mod_ctx, name, &address, &flags) == MOD_OK &&
           address != nullptr && (flags & HOOK_SYMBOL_CODE) != 0;
}

ModResult install_lazy_tweaks_bridge() {
    if (!resolve_lazy_tweaks_symbol("dMeter2_c::moveSkill") ||
        !resolve_lazy_tweaks_symbol("dMeter2Draw_c::drawSkill"))
    {
        return MOD_OK;
    }

    ModResult result =
        mods::hook::add_pre<LazySkillMeterScreenHook>(svc_hook, before_lazy_skill_meter);
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LazyGuardAttackHook>(svc_hook, before_lazy_guard_attack);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LazyGuardAttackHook>(svc_hook, after_lazy_guard_attack);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LazyBackSliceHook>(svc_hook, before_lazy_back_slice);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LazyBackSliceHook>(svc_hook, after_lazy_back_slice);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LazyHelmSplitterHook>(svc_hook, before_lazy_helm_splitter);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LazyHelmSplitterHook>(svc_hook, after_lazy_helm_splitter);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LazyMidnaChargeHook>(svc_hook, before_lazy_midna_charge);
    }
    if (result != MOD_OK) {
        svc_log->warn(mod_ctx,
            "Dawnlight stamina: Lazy Tweaks detected, but compatibility hooks failed");
        return MOD_OK;
    }

    s_lazyTweaksDetected = true;
    svc_log->info(mod_ctx, "Dawnlight stamina: Lazy Tweaks compatibility enabled");
    return MOD_OK;
}

}  // namespace

ModResult initialize_stamina(ModError* error) {
    const ModResult result =
        mods::hook::add_post<StaminaMeterDrawHook>(svc_hook, after_meter_draw);
    if (result != MOD_OK) {
        return mods::set_error(error, result,
            "failed to install Dawnlight stamina meter hook");
    }
    install_lazy_tweaks_bridge();
    reset_for_link(daAlink_getAlinkActorClass());
    return MOD_OK;
}

void shutdown_stamina() {
    s_state = {};
    s_lazyTweaksDetected = false;
}

bool stamina_meter_visible() {
    return bullet_time_enabled() || flurry_rush_enabled() ||
           great_spin_projectile_enabled();
}

bool stamina_available_for_bullet_time() {
    daAlink_c* link = current_link();
    return link != nullptr && !link->checkDeadHP() &&
           !link->checkSceneChangeAreaStart() && s_state.stamina > 0.0f;
}

bool consume_flurry_rush_stamina() {
    return try_consume(kFlurryRushCost);
}

bool consume_great_spin_stamina() {
    return try_consume(kGreatSpinCost);
}

bool update_stamina(bool bulletTimeActive) {
    daAlink_c* link = current_link();
    if (link == nullptr) {
        return false;
    }

    const Clock::time_point now = Clock::now();
    if (link->checkDeadHP() || link->checkSceneChangeAreaStart()) {
        reset_for_link(link);
        return true;
    }
    if (menu_or_pause_active()) {
        s_state.lastUpdate = now;
        return s_state.stamina > 0.0f;
    }
    if (s_state.lastUpdate.time_since_epoch().count() == 0) {
        s_state.lastUpdate = now;
        return s_state.stamina > 0.0f;
    }

    const float elapsed = std::clamp(
        std::chrono::duration<float>(now - s_state.lastUpdate).count(), 0.0f, 0.25f);
    s_state.lastUpdate = now;
    if (bulletTimeActive) {
        s_state.stamina = std::max(
            0.0f, s_state.stamina - elapsed * kBulletTimeDrainPerSecond);
    } else {
        s_state.stamina = std::min(
            kMaximumStamina, s_state.stamina + elapsed * kRecoveryPerSecond);
    }
    return s_state.stamina > 0.0f;
}

}  // namespace dawnlight
