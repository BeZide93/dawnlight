// Adapted from F1mmel/dusklight-twilit-essentials, commit 1e7fb0afb828f924c165e8c57b7337304ed6cf4c.
#pragma once

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "SSystem/SComponent/c_xyz.h"

ModResult init_ganondorf_cape(const HookService* hook_svc, const LogService*,
                              ModContext* mod_ctx);

void update_ganondorf_cape(bool statueVisible, const cXyz& gndPos, s16 gndYaw, f32 gndScale);

void shutdown_ganondorf_cape(const HookService* hook_svc);
