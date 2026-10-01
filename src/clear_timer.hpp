#pragma once
#include "mods/api.h"
#include "SSystem/SComponent/c_xyz.h"
#include "clear_timer_state.hpp"

namespace dawnlight {
struct ClearTimerContext {
    bool active = false; // A Boss Rush save is playing (not title/file select).
    bool counting = false;
    bool dead = false;
    int encounter = -1;
};
ClearTimerContext clear_timer_context();
bool clear_timer_anchor(unsigned index, cXyz& position);
bool clear_timer_near(const cXyz& interactionPosition);
bool heroes_shade_timer_anchor(cXyz& position);
bool heroes_shade_timer_active();
ModResult initialize_clear_timer(ModError* error);
void shutdown_clear_timer();
void update_clear_timer();
void begin_clear_timer(int target);
void finish_clear_timer(int target);
void cancel_clear_timer(int target = -1);
}
