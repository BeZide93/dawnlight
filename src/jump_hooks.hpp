#pragma once

class daAlink_c;

namespace dawnlight {
// The manual shield must not consume B while this opt-in jump is airborne.
bool air_combo_jump_active(const daAlink_c* link);
void shutdown_jump_hooks();
}
