#pragma once
#include "dawnlight/fierce_deity_hud.h"

namespace dawnlight {
ModResult initialize_fierce_deity_hud(ModError* error);
void shutdown_fierce_deity_hud();
DawnlightFierceDeityHudState fierce_deity_hud_state();
bool fierce_deity_hud_renderer_registered();
bool draw_external_fierce_deity_hud(const DawnlightFierceDeityHudFrame& frame);
}
