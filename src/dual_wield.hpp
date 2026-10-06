#pragma once
#include "mods/api.h"
class daAlink_c;
class J3DModel;
namespace dawnlight {
// Identity only; models remain owned and released by Dual Wield.
bool dual_wield_owns_model(const daAlink_c* link, const J3DModel* model);
ModResult install_dual_wield_hooks(ModError* error);
void shutdown_dual_wield();
}
