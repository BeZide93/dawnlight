#include "progression.hpp"

#include "config.hpp"
#include "glider_reward.hpp"
#include "save_state.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_save.h"
#include "mods/service.hpp"

#include <cstdio>

namespace dawnlight {
namespace {
SaveObserverHandle s_observer = 0;
ConfigSubscriptionHandle s_modeSubscription = 0;
ConfigSubscriptionHandle s_progressionSubscription = 0;
ProgressionNotifications s_notifications;

void on_save_started(ModContext*, uint32_t, void*) {
    cancel_glider_reward();
    s_notifications = {};
}

void on_mode_changed(ModContext*, ConfigVarHandle, const ConfigVarValue*,
    const ConfigVarValue*, void*) {
    s_notifications = {};
}

void notify(const char* message) {
    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.title_rml = "Dawnlight Progression";
    toast.body_rml = message;
    toast.duration_ms = 4000;
    svc_ui->push_toast(mod_ctx, &toast);
}
} // namespace

ProgressionState progression_state() {
    // Read the installed save, never copy unlocks into the global config.
    // The game maintains these flags across scenes and save/load cycles.
    ProgressionState state;
    const bool bossRush = save_state_boss_rush_active();
    state.glide = bossRush || dComIfGs_isEventBit(dSv_event_flag_c::F_0026);
    state.gale = bossRush || dComIfGs_isEventBit(dSv_event_flag_c::M_016);
    state.fierceDeity = bossRush || dComIfGs_isEventBit(dSv_event_flag_c::M_019);
    state.charges = progression_charges(dComIfGs_getMaxLife());
    return state;
}

ModResult initialize_progression(ModError* error) {
    s_notifications = {};
    auto result = svc_save->observe_saves(
        mod_ctx, on_save_started, on_save_started, nullptr, nullptr, &s_observer);
    if (result == MOD_OK) result = svc_config->subscribe(mod_ctx, dawnlight_mode_config_var(),
        on_mode_changed, nullptr, &s_modeSubscription);
    if (result == MOD_OK) result = svc_config->subscribe(mod_ctx, progression_system_config_var(),
        on_mode_changed, nullptr, &s_progressionSubscription);
    if (result == MOD_OK) result = initialize_glider_reward(error);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result,
        "failed to observe Dawnlight progression saves");
}

void update_progression() {
    update_glider_reward();
    if (!progression_system_enabled()) {
        s_notifications = {};
        return;
    }
    // Loading screens/title have no active player. Keep the baseline across
    // normal scene changes; SaveService explicitly resets it on save changes.
    if (!daAlink_getAlinkActorClass()) return;
    const auto state = progression_state();
    const unsigned unlocked = s_notifications.update(true, state);
    if (unlocked & 1) {
        notify("Glide unlocked! Dawnlight's Glider is ready.");
        queue_glider_reward();
    }
    if (unlocked & 2) notify("Revali's Gale unlocked! The Gale counter is now available.");
    if (unlocked & 4) notify("Fierce Deity unlocked!");
    if (unlocked & 8) {
        char message[96];
        std::snprintf(message, sizeof(message), "Gale capacity increased: %d charges.", state.charges);
        notify(message);
    }
}

void shutdown_progression() {
    shutdown_glider_reward();
    if (s_observer && svc_save) svc_save->unobserve_saves(mod_ctx, s_observer);
    s_observer = 0;
    if (s_modeSubscription) svc_config->unsubscribe(mod_ctx, s_modeSubscription);
    if (s_progressionSubscription) svc_config->unsubscribe(mod_ctx, s_progressionSubscription);
    s_modeSubscription = s_progressionSubscription = 0;
    s_notifications = {};
}
} // namespace dawnlight
