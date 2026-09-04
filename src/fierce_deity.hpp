#pragma once

#include "mods/api.h"

namespace dawnlight {

ModResult initialize_fierce_deity(ModError* error);
void shutdown_fierce_deity();
bool fierce_deity_active();

}  // namespace dawnlight
