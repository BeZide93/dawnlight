#pragma once
#include "mods/api.h"
#include "mods/svc/ui.h"
#include "general_modes.hpp"

namespace dawnlight {
ModResult initialize_survivors(ModError* error);
void update_survivors();
void shutdown_survivors();
bool survivors_active();
bool survivors_switch_attack_lock();
bool survivors_mobility_override(ModeSetting setting, int64_t& value);
float survivors_sprint_multiplier();
bool survivors_dual_wield();
bool survivors_arena_active();
bool survivors_save_screen_active();
bool survivors_dark_link_active();
ModResult build_survivors_settings(ModContext*, UiWindowHandle, UiElementHandle, UiElementHandle, void*, ModError*);
bool survivors_hub_active();
void survivors_open_talents();
} // namespace dawnlight
