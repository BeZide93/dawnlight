#pragma once

#include "mods/api.h"

namespace dawnlight {
ModResult initialize_gale_counter(ModError* error);
void shutdown_gale_counter();
bool gale_charge_available();
void consume_gale_charge();
}  // namespace dawnlight
