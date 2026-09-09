#pragma once

#include "mods/api.h"

class fopAc_ac_c;

namespace dawnlight {
float enemy_slow_motion_scale(fopAc_ac_c* actor);
bool enemy_uses_continuous_slow(fopAc_ac_c* actor);
ModResult initialize_enemy_slow_motion();
void reset_enemy_slow_motion();
}
