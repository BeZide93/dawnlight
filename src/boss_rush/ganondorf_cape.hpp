// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
#pragma once

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "SSystem/SComponent/c_xyz.h"

ModResult init_ganondorf_cape(const HookService* hook_svc, const LogService*,
                              ModContext* mod_ctx);

void update_ganondorf_cape(bool statueVisible, const cXyz& gndPos, s16 gndYaw, f32 gndScale);

void shutdown_ganondorf_cape(const HookService* hook_svc);
