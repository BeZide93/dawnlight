#pragma once

#include "SSystem/SComponent/c_xyz.h"

namespace dawnlight {
inline constexpr unsigned kBossPortalSymbolCount = 18;
// Snapshot queried only on the game thread, after a live hub portal exists.
bool get_boss_portal_symbol(unsigned index, cXyz& position, bool& defeated);
void initialize_boss_portal_symbols();
void shutdown_boss_portal_symbols();
}
