#include "config.hpp"
#include "save_compat.hpp"
#include "service_imports.hpp"
#include "update_check.hpp"

#include "mods/service.hpp"
#include "mods/svc/config.h"
#include "mods/svc/flow.h"
#include "mods/svc/game_mode.h"
#include "mods/svc/hook.h"
#include "mods/svc/host.h"
#include "mods/svc/log.h"
#include "mods/svc/message.h"
#include "mods/svc/texture.h"
#include "mods/svc/ui.h"

DEFINE_MOD();
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(FlowService, svc_flow);
IMPORT_SERVICE(GameModeService, svc_game_mode);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(MessageService, svc_message);
IMPORT_SERVICE(TextureService, svc_texture);
IMPORT_SERVICE(UiService, svc_ui);

namespace dawnlight {
ModResult install_aim_hooks(ModError* error);
ModResult install_enemy_scaling_hooks(ModError* error);
ModResult install_item_integrity_hooks(ModError* error);
ModResult install_item_slot_hooks(ModError* error);
ModResult install_jump_hooks(ModError* error);
ModResult install_manual_shield_hooks(ModError* error);
ModResult register_new_save_modes(ModError* error);
ModResult register_ui(ModError* error);
void shutdown_item_slot_hooks();
void shutdown_new_save_modes();
}

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* error) {
    if (const ModResult result = dawnlight::register_config(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::register_ui(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_save_compat_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_aim_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_item_integrity_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_item_slot_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_manual_shield_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::register_new_save_modes(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_jump_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_enemy_scaling_hooks(error); result != MOD_OK) {
        return result;
    }

    svc_log->info(mod_ctx, "Dawnlight portable feature pack initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    dawnlight::update_check_tick();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    dawnlight::shutdown_new_save_modes();
    dawnlight::shutdown_update_check();
    dawnlight::shutdown_item_slot_hooks();
    svc_log->info(mod_ctx, "Dawnlight portable feature pack stopped");
    return MOD_OK;
}

}  // extern "C"
