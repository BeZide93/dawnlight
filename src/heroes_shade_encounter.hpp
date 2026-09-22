#pragma once
#include "mods/api.h"
namespace dawnlight {
ModResult initialize_heroes_shade_encounter(ModError* error);
void update_heroes_shade_arena();
void update_heroes_shade_audio();
void shutdown_heroes_shade_encounter();
}
