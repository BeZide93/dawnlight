#pragma once
#include "mods/svc/hook.hpp"

namespace dawnlight {
// These run inside the existing fpcMtd_Method hook. Only a completed BG create
// may acquire ownership; subsequent mode changes cannot repurpose that actor.
HookAction before_roundtable_process(void* args, void* result, bool createHub);
void after_roundtable_process(void* args, void* result);
void shutdown_roundtable();
}
