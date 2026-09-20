#pragma once

#include "boss_portal_symbols.hpp"
#include "mods/svc/actor.h"
#include "SSystem/SComponent/c_sxyz.h"

namespace dawnlight {
ModResult initialize_boss_portal_mirrors(ModError* error);
void shutdown_boss_portal_mirrors();
ActorId create_boss_portal_mirror(unsigned index, const cXyz& position, s16 facing, s8 room);
bool boss_portal_mirror_surface(ActorId id, BossPortalSymbolSurface& surface);
}
