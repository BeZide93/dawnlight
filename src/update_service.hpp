#pragma once

#include "mods/api.h"
#include "mods/svc/log.h"
#include "mods/svc/ui.h"
#include "mods/svc/config.h"

namespace dawnlight {

extern bool g_configCheckForUpdatesEnabled;

ModResult init_update_service(const LogService* log_svc, ModContext* mod_ctx, const UiService* ui_svc, const ConfigService* config_svc, ConfigVarHandle var_handle);
void update_update_service(const LogService* log_svc, ModContext* mod_ctx, const UiService* ui_svc);
void shutdown_update_service();

}  // namespace dawnlight
