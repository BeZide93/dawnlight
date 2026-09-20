#pragma once

#include "SSystem/SComponent/c_xyz.h"

namespace dawnlight {
inline constexpr unsigned kBossPortalSymbolCount = 18;
struct BossPortalSymbolSurface {
    cXyz center, right, up, normal;
};
// Snapshot queried only on the game thread, after a live hub mirror exists.
bool get_boss_portal_symbol(unsigned index, BossPortalSymbolSurface& surface, bool& defeated);
void initialize_boss_portal_symbols();
void shutdown_boss_portal_symbols();
}
