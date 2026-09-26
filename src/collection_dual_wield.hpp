#pragma once
#include "mods/api.h"
namespace dawnlight {
ModResult install_collection_dual_wield(ModError* error);
void shutdown_collection_dual_wield();
bool dual_wield_equipped();
}
