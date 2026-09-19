#pragma once
#include <cstddef>
bool is_boss_rush_chamber_room();
bool is_in_boss_rush_chamber();
bool boss_rush_scene_load_stable();
size_t boss_rush_get_active_gallery_count();
size_t boss_rush_get_active_gallery_table_index(size_t slot);
size_t boss_rush_get_circle_slot_for_table_index(size_t index);
void release_boss_gallery_archives();
