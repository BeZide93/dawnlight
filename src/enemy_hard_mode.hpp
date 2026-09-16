#pragma once

class fopAc_ac_c;

namespace dawnlight {
struct EnemySlowStep;

bool enemy_hard_mode_applies(int profileName);
float enemy_hard_mode_turn_scale(const EnemySlowStep& step);
void prepare_enemy_hard_mode(EnemySlowStep& step);
void reset_enemy_hard_mode();
}
