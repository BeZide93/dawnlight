#pragma once

#include "mods/api.h"

namespace dawnlight {

ModResult initialize_fierce_deity(ModError* error);
void shutdown_fierce_deity();
bool fierce_deity_active();
bool fierce_deity_dark_visual_active();
ModResult initialize_fierce_deity_visual(ModError* error);
bool fierce_deity_model_reload_active();

}  // namespace dawnlight
