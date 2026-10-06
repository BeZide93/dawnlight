#pragma once

#include "mods/api.h"

namespace dawnlight {
ModResult install_cave_randomizer(ModError* error);
void update_cave_randomizer();
void shutdown_cave_randomizer();
}
