#pragma once
#include "mods/svc/actor.h"
namespace dawnlight {
ModResult initialize_roundtable_props(ModError* error);
void update_roundtable_props(ActorId owner);
void release_roundtable_props(ActorId owner);
void shutdown_roundtable_props();
}
