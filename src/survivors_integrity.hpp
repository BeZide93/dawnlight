#pragma once
#include "mods/api.h"
#include "survivors_buffer_diagnostics.hpp"
namespace dawnlight {
ModResult initialize_survivors_integrity(ModError*);
void begin_survivors_integrity(bool allow_cheats, bool& modded);
void update_survivors_integrity(bool& modded);
void end_survivors_integrity();
void shutdown_survivors_integrity();
survivor_perf::FogInventory survivors_fog_inventory();
}
