#pragma once

#include "mods/api.h"

#include <cstdint>

namespace dawnlight {

ModResult initialize_save_state(ModError* error);
void shutdown_save_state();

bool save_state_is_new_game_plus();
unsigned save_state_new_game_plus_count();
bool save_state_intro_skipped();
void save_state_set_intro_skipped(bool enabled);

bool save_state_boss_rush_active();
void save_state_set_boss_rush_active(bool enabled);
uint8_t save_state_boss_rush_index();
void save_state_set_boss_rush_index(uint8_t index);
uint8_t save_state_boss_rush_loop();
void save_state_set_boss_rush_loop(uint8_t loop);
void save_state_increment_boss_rush_loop();
uint8_t save_state_boss_rush_state();
void save_state_set_boss_rush_state(uint8_t state);
bool save_state_boss_defeated(uint8_t index);
void save_state_mark_boss_defeated(uint8_t index);
void save_state_clear_boss_defeated();

}  // namespace dawnlight
