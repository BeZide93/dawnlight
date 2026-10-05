#pragma once

#include "mods/api.h"

namespace dawnlight {

ModResult initialize_stamina(ModError* error);
void shutdown_stamina();

bool stamina_meter_visible();
bool stamina_available_for_sprint();
bool stamina_available_for_wolf_sprint();
void mark_wolf_sprint_stamina_active();
void mark_sprint_stamina_active();
bool stamina_available_for_glide();
void set_glide_stamina_active(bool active);
bool consume_great_spin_stamina();
void update_stamina();
void update_stamina_ui();

}  // namespace dawnlight
