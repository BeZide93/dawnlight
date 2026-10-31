#pragma once
#include "mods/api.h"
class fopAc_ac_c;
class cXyz;
namespace dawnlight {
bool heroes_shade_combat_slow_eligible(fopAc_ac_c* actor);
ModResult initialize_heroes_shade_encounter(ModError* error);
void update_heroes_shade_arena();
void update_heroes_shade_audio();
void shutdown_heroes_shade_encounter();
void update_survivors_equipment_chest(bool enabled,const cXyz& position,short yaw);
bool survivors_equipment_chest_anchor(cXyz& position);
}
