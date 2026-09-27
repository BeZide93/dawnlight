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
#include <vector>

namespace dawnlight {
namespace {

DEFINE_HOOK(&dMeter2Draw_c::draw, StaminaMeterDrawHook);
DEFINE_HOOK(&daAlink_c::checkRestHPAnime, StaminaRestAnimationHook);
DEFINE_HOOK(&dMeter2Draw_c::drawKanteraScreen, LazySkillMeterScreenHook);
DEFINE_HOOK(&daAlink_c::procGuardAttackInit, StaminaGuardAttackHook);
DEFINE_HOOK(&daAlink_c::procCutFinishJumpUpInit, StaminaBackSliceHook);
DEFINE_HOOK(&daAlink_c::procCutHeadInit, StaminaHelmSplitterHook);
DEFINE_HOOK(&daAlink_c::setWolfLockDomeModel, StaminaMidnaChargeHook);

DEFINE_HOOK(&daAlink_c::checkDamageAction, StaminaDamageHook);
DEFINE_HOOK(&daAlink_c::setGuardSe, StaminaBlockHook);
DEFINE_HOOK(&daAlink_c::procGuardBreakInit, StaminaGuardBreakHook);

float maximum_stamina() {
    return static_cast<float>(stamina_capacity(stamina_setting(StaminaSetting::Amount),
        progression_system_enabled(), dComIfGs_getMaxLife()));
}

using Clock = std::chrono::steady_clock;

struct RuntimeState {
    daAlink_c* link = nullptr;
    u16 linkId = 0;
    float stamina = 100.0f;
    Clock::time_point lastUpdate{};
    bool sprintActive = false;
    bool glideActive = false;
    bool wolfSprintActive = false;
    bool exhausted = false;
};

RuntimeState s_state;
bool s_lazyTweaksDetected = false;

bool same_link(daAlink_c* link) {
    return link != nullptr && s_state.link == link && s_state.linkId == link->setID;
}

void reset_for_link(daAlink_c* link) {
    s_state.link = link;
    s_state.linkId = link != nullptr ? link->setID : 0;
    s_state.stamina = maximum_stamina();
    s_state.lastUpdate = Clock::now();
    s_state.sprintActive = false;
    s_state.glideActive = false;
    s_state.wolfSprintActive = false;
    s_state.exhausted = false;
}

daAlink_c* current_link() {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (!same_link(link)) {
        reset_for_link(link);
    }
    s_state.stamina = std::min(s_state.stamina, maximum_stamina());
    return link;
}

bool menu_or_pause_active() {
    return !combat_meter_hud_visible();
}

bool try_consume(float amount) {
    if (!stamina_enabled() || amount <= 0.0f) {
        return true;
    }
    daAlink_c* link = current_link();
    if (link == nullptr || link->checkDeadHP() || link->checkSceneChangeAreaStart() ||
        s_state.exhausted || s_state.stamina < amount)
    {
        return false;
    }
    s_state.stamina -= amount;
    if (s_state.stamina <= 0.0f) {
        s_state.stamina = 0.0f;
        s_state.exhausted = true;
    }
    return true;
}

bool can_consume(float amount) {
    if (!stamina_enabled() || amount <= 0.0f) {
        return true;
    }
    daAlink_c* link = current_link();
    return link != nullptr && !link->checkDeadHP() &&
           !link->checkSceneChangeAreaStart() && !s_state.exhausted &&
           s_state.stamina >= amount;
}

bool lazy_stamina_bridge_active() {
    return s_lazyTweaksDetected && stamina_meter_visible();
}

// Skill costs apply with or without Lazy Tweaks; the bridge only hides its
// duplicate meter. Failed initializers never spend stamina.
HookAction before_skill(void* retval, StaminaSetting setting) {
    if (can_consume(stamina_setting(setting))) return HOOK_CONTINUE;
    if (retval) *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

void after_skill(void* retval, StaminaSetting setting) {
    if (retval && *static_cast<int*>(retval)) try_consume(stamina_setting(setting));
}

HookAction before_guard_attack(ModContext*, void*, void* retval, void*) {
    return before_skill(retval, StaminaSetting::ShieldAttack);
}
void after_guard_attack(ModContext*, void*, void* retval, void*) {
    after_skill(retval, StaminaSetting::ShieldAttack);
}
HookAction before_back_slice(ModContext*, void*, void* retval, void*) {
    return before_skill(retval, StaminaSetting::BackSlice);
}
void after_back_slice(ModContext*, void*, void* retval, void*) {
    after_skill(retval, StaminaSetting::BackSlice);
}
HookAction before_helm_splitter(ModContext*, void*, void* retval, void*) {
    return before_skill(retval, StaminaSetting::HelmSplitter);
}
void after_helm_splitter(ModContext*, void*, void* retval, void*) {
    after_skill(retval, StaminaSetting::HelmSplitter);
}
HookAction before_midna_charge(ModContext*, void*, void*, void*) {
    return try_consume(stamina_setting(StaminaSetting::MidnaAttack)) ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL;
}

// Defensive reactions must still run if the remaining stamina cannot cover
// their cost. Spend the remainder, rather than making low-stamina blocks free.
void consume_defense(StaminaSetting setting) {
    if (!stamina_enabled()) return;
    auto* link = current_link();
    if (!link || link->checkDeadHP() || link->checkSceneChangeAreaStart()) return;
    const float cost = stamina_setting(setting);
    if (cost <= 0) return;
    s_state.stamina = std::max(0.0f, s_state.stamina - cost);
    if (s_state.stamina <= 0) s_state.exhausted = true;
}

struct GuardEvent {
    daAlink_c* link;
    void* args;
    bool blocked = false;
    bool broken = false;
};
std::vector<GuardEvent> s_guardEvents;

HookAction before_damage(ModContext*, void* args, void*, void*) {
    s_guardEvents.push_back({mods::arg<daAlink_c*>(args, 0), args});
    return HOOK_CONTINUE;
}
void after_block(ModContext*, void* args, void*, void*) {
    auto* hit = mods::arg<dCcD_GObjInf*>(args, 1);
    if (!s_guardEvents.empty() && s_guardEvents.back().link == mods::arg<daAlink_c*>(args, 0) &&
        hit && hit->ChkTgShieldHit()) s_guardEvents.back().blocked = true;
}
void after_guard_break(ModContext*, void* args, void* retval, void*) {
    if (!retval || !*static_cast<int*>(retval)) return;
    if (!s_guardEvents.empty() && s_guardEvents.back().link == mods::arg<daAlink_c*>(args, 0))
        s_guardEvents.back().broken = true;
    else consume_defense(StaminaSetting::GuardBreak);
}
void after_damage(ModContext*, void* args, void*, void*) {
    if (s_guardEvents.empty() || s_guardEvents.back().args != args) return;
    const auto event = s_guardEvents.back();
    s_guardEvents.pop_back();
    // Guard Break replaces Block, including the native fourth-block break.
    if (event.broken) consume_defense(StaminaSetting::GuardBreak);
    else if (event.blocked) consume_defense(StaminaSetting::Block);
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
    const CombatMeterStyle style = s_state.exhausted ?
        CombatMeterStyle::StaminaExhausted : CombatMeterStyle::Stamina;
    draw_combat_meter(
        mods::arg<dMeter2Draw_c*>(args, 0), s_state.stamina * 100.0f / maximum_stamina(), style, 0);
}

void after_check_rest_animation(ModContext*, void* args, void* retval, void*) {
    if (retval == nullptr || *static_cast<BOOL*>(retval) != FALSE ||
        !stamina_meter_visible() || !s_state.exhausted)
    {
        return;
    }

    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link != nullptr && !link->checkPlayerGuard() &&
        (link->checkNoUpperAnime() || link->checkHorseTiredAnime()) &&
        link->mTargetedActor == nullptr && !link->checkWindSpeedOnAngle() &&
        !link->checkPlayerDemoMode())
    {
        *static_cast<BOOL*>(retval) = TRUE;
    }
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
    ModResult result =
        mods::hook::add_post<StaminaMeterDrawHook>(svc_hook, after_meter_draw);
    if (result == MOD_OK) {
        result = mods::hook::add_post<StaminaRestAnimationHook>(
            svc_hook, after_check_rest_animation);
    }
    if (result == MOD_OK) result = mods::hook::add_pre<StaminaGuardAttackHook>(svc_hook, before_guard_attack);
    if (result == MOD_OK) result = mods::hook::add_post<StaminaGuardAttackHook>(svc_hook, after_guard_attack);
    if (result == MOD_OK) result = mods::hook::add_pre<StaminaBackSliceHook>(svc_hook, before_back_slice);
    if (result == MOD_OK) result = mods::hook::add_post<StaminaBackSliceHook>(svc_hook, after_back_slice);
    if (result == MOD_OK) result = mods::hook::add_pre<StaminaHelmSplitterHook>(svc_hook, before_helm_splitter);
    if (result == MOD_OK) result = mods::hook::add_post<StaminaHelmSplitterHook>(svc_hook, after_helm_splitter);
    if (result == MOD_OK) result = mods::hook::add_pre<StaminaMidnaChargeHook>(svc_hook, before_midna_charge);
    if (result == MOD_OK) result = mods::hook::add_pre<StaminaDamageHook>(svc_hook, before_damage);
    if (result == MOD_OK) result = mods::hook::add_post<StaminaDamageHook>(svc_hook, after_damage);
    if (result == MOD_OK) result = mods::hook::add_post<StaminaBlockHook>(svc_hook, after_block);
    if (result == MOD_OK) result = mods::hook::add_post<StaminaGuardBreakHook>(svc_hook, after_guard_break);
    if (result != MOD_OK) {
        return mods::set_error(error, result,
            "failed to install Dawnlight stamina hooks");
    }
    install_lazy_tweaks_bridge();
    reset_for_link(daAlink_getAlinkActorClass());
    return MOD_OK;
}

void shutdown_stamina() {
    s_state = {};
    s_lazyTweaksDetected = false;
    s_guardEvents.clear();
}

bool stamina_meter_visible() {
    // Blocking and native combat skills use stamina even if movement toggles are off.
    return stamina_enabled();
}

bool stamina_available_for_bullet_time() {
    return can_consume(stamina_setting(StaminaSetting::BulletTime) == 0 ? 0 : 0.0001f);
}

bool stamina_available_for_sprint() {
    return can_consume(stamina_setting(StaminaSetting::Sprint) == 0 ? 0 : 0.0001f);
}

bool stamina_available_for_wolf_sprint() {
    return can_consume(stamina_setting(StaminaSetting::WolfSprint) == 0 ? 0 : 0.0001f);
}

void mark_wolf_sprint_stamina_active() {
    if (stamina_enabled()) s_state.wolfSprintActive = true;
}

void mark_sprint_stamina_active() {
    if (stamina_enabled()) {
        s_state.sprintActive = true;
    }
}

bool consume_flurry_rush_stamina() {
    return try_consume(stamina_setting(StaminaSetting::FlurryRush));
}

bool stamina_available_for_glide() {
    return can_consume(stamina_setting(StaminaSetting::Glide) == 0 ? 0 : 0.0001f);
}

void set_glide_stamina_active(bool active) {
    current_link();
    // Retain this state between simulation ticks: stamina updates can also run
    // on presentation frames. Clear it when the owned carrier is retired.
    s_state.glideActive = active;
}

bool consume_great_spin_stamina() {
    return try_consume(stamina_setting(StaminaSetting::GreatSpin));
}

bool update_stamina(bool bulletTimeActive) {
    daAlink_c* link = current_link();
    if (link == nullptr) {
        return false;
    }

    const Clock::time_point now = Clock::now();
    const bool sprintActive = s_state.sprintActive;
    const bool wolfSprintActive = s_state.wolfSprintActive;
    s_state.sprintActive = s_state.wolfSprintActive = false;
    if (!stamina_enabled()) {
        const bool wasExhausted = s_state.exhausted;
        s_state.stamina = maximum_stamina();
        s_state.exhausted = false;
        s_state.lastUpdate = now;
        if (wasExhausted && dComIfGs_getLife() > 4) {
            if (link->mProcID == daAlink_c::PROC_TIRED_WAIT) {
                link->procWaitInit();
            } else if (link->mProcID == daAlink_c::PROC_WOLF_TIRED_WAIT) {
                link->procWolfWaitInit();
            }
        }
        return true;
    }
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
    const bool wasExhausted = s_state.exhausted;
    float drainPerSecond = bulletTimeActive ? stamina_setting(StaminaSetting::BulletTime) :
        (sprintActive ? stamina_setting(StaminaSetting::Sprint) :
        (wolfSprintActive ? stamina_setting(StaminaSetting::WolfSprint) : 0.0f));
    if (s_state.glideActive && glide_enabled()) drainPerSecond += stamina_setting(StaminaSetting::Glide);
    if (drainPerSecond > 0.0f && !s_state.exhausted) {
        s_state.stamina = std::max(
            0.0f, s_state.stamina - elapsed * drainPerSecond);
    } else {
        s_state.stamina = std::min(
            maximum_stamina(), s_state.stamina + elapsed * stamina_setting(
                s_state.exhausted ? StaminaSetting::ExhaustRecovery : StaminaSetting::Recovery));
    }
    if (!s_state.exhausted && s_state.stamina <= 0.0f) {
        s_state.stamina = 0.0f;
        s_state.exhausted = true;
    } else if (s_state.exhausted &&
               s_state.stamina > 0 && s_state.stamina >=
                   maximum_stamina() * stamina_setting(StaminaSetting::ExhaustThreshold) / 100.0f)
    {
        s_state.exhausted = false;
    }

    if (wasExhausted && !s_state.exhausted && dComIfGs_getLife() > 4) {
        if (link->mProcID == daAlink_c::PROC_TIRED_WAIT) {
            link->procWaitInit();
        } else if (link->mProcID == daAlink_c::PROC_WOLF_TIRED_WAIT) {
            link->procWolfWaitInit();
        }
    }
    return stamina_available_for_bullet_time();
}

}  // namespace dawnlight
