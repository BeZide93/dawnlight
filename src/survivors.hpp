#pragma once
#include "mods/api.h"

namespace dawnlight {
ModResult initialize_survivors(ModError* error);
void update_survivors();
void shutdown_survivors();
bool survivors_active();
bool survivors_arena_active();
} // namespace dawnlight
