#pragma once

#include "mods/api.h"

namespace dawnlight {

ModResult install_model_hooks(ModError* error);
void initialize_model_overlays();
void shutdown_model_overlays();

}  // namespace dawnlight
