#pragma once
#include "general_modes.hpp"
#include "mods/api.h"

namespace dawnlight {
ModResult initialize_progression(ModError* error);
void update_progression();
void shutdown_progression();
ProgressionState progression_state();
} // namespace dawnlight
