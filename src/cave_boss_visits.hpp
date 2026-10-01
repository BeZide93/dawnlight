#pragma once
#include "mods/api.h"
#include "SSystem/SComponent/c_xyz.h"
namespace dawnlight {
ModResult initialize_cave_boss_visits(ModError* error);
void shutdown_cave_boss_visits();
void reset_cave_boss_visits();
bool cave_boss_final_cleared();
bool return_from_cave_boss();
// Routes implemented by the existing Boss Rush warp/replay state machine.
bool cave_boss_warp_available();
bool warp_to_cave_boss(unsigned boss);
void warp_back_to_cave(const cXyz& position, short yaw, signed char room);
}
