#pragma once

#include "mods/api.h"

class daAlink_c;

namespace dawnlight {

enum class FierceDeityTint { None, Dark, White, Gold };
FierceDeityTint fierce_deity_displayed_tint();

ModResult initialize_fierce_deity(ModError* error);
void shutdown_fierce_deity();
bool fierce_deity_active();
// Also cancels an already charged Gale when its R hold is used for this combo.
bool fierce_deity_input_consumed();
void fierce_deity_touch_button(uint32_t button, bool pressed);
bool fierce_deity_dark_visual_active();
ModResult initialize_fierce_deity_visual(ModError* error);
bool fierce_deity_model_reload_active();

// Called only at the native player-update boundary; never changes gameplay.
bool fierce_deity_transition_busy();
void fierce_deity_transition_prepare(daAlink_c*, FierceDeityTint fromTint, FierceDeityTint toTint, bool entering);
void fierce_deity_transition_commit(daAlink_c*);
void fierce_deity_transition_tick(daAlink_c*);
void fierce_deity_transition_cancel(daAlink_c*);

}  // namespace dawnlight
