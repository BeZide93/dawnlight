#pragma once

#include "mods/api.h"

class fopAc_ac_c;

namespace dawnlight {
struct EnemySlowStep;

bool enemy_hard_mode_applies(int profileName);
ModResult install_enemy_hard_mode_hooks();
float enemy_hard_mode_turn_scale(const EnemySlowStep& step);
float enemy_hard_mode_chase_scale(const EnemySlowStep& step, const float* value);
void prepare_enemy_hard_mode(EnemySlowStep& step);
void finish_enemy_hard_mode(EnemySlowStep& step);
void reset_enemy_hard_mode();
}
