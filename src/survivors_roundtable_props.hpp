#pragma once
#include "mods/svc/actor.h"
#include "mods/svc/hook.hpp"
namespace dawnlight {
HookAction before_roundtable_prop_process(void* args,void* result);
void update_roundtable_props(ActorId owner);
void release_roundtable_props(ActorId owner);
void shutdown_roundtable_props();
}
