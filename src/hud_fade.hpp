#pragma once

#include "mods/api.h"

class J2DScreen;
class J2DGrafContext;

namespace dawnlight {
ModResult initialize_hud_fade(ModError* error);
void shutdown_hud_fade();
void draw_combat_meter_screen(J2DScreen* screen, J2DGrafContext* graf, bool stamina, float percentage);
} // namespace dawnlight
