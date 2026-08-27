#pragma once

#include "mods/api.h"

class daAlink_c;

namespace dawnlight {

ModResult initialize_bullet_time(ModError* error);
void mark_manual_jump_started(daAlink_c* link);
void clear_manual_jump(daAlink_c* link);
void update_bullet_time_before_jump(daAlink_c* link);
void update_bullet_time_after_jump(daAlink_c* link);
void slow_bullet_time_jump_speed_change(daAlink_c* link, float previousNormalSpeed);
bool bullet_time_active_for(const daAlink_c* link);
void bullet_time_tick();
void shutdown_bullet_time();

}  // namespace dawnlight
