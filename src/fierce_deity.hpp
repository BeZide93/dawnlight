#pragma once

#include "mods/api.h"

class daAlink_c;

namespace dawnlight {

ModResult initialize_fierce_deity(ModError* error);
void shutdown_fierce_deity();
bool fierce_deity_active();
bool fierce_deity_dark_visual_active();
ModResult initialize_fierce_deity_visual(ModError* error);
bool fierce_deity_model_reload_active();

// Called only at the native player-update boundary; never changes gameplay.
bool fierce_deity_transition_busy();
void fierce_deity_transition_prepare(daAlink_c*, bool fromDark, bool toDark, bool entering);
void fierce_deity_transition_commit(daAlink_c*);
void fierce_deity_transition_tick(daAlink_c*);
void fierce_deity_transition_cancel(daAlink_c*);

}  // namespace dawnlight
