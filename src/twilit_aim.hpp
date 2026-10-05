#pragma once

namespace dawnlight {

// TE owns the airborne input/slow-motion loop; Dawnlight supplies its camera.
bool twilit_bullet_time_aim_enabled();
void update_twilit_aim();
void shutdown_twilit_aim();

}  // namespace dawnlight
