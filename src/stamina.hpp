#pragma once

#include "mods/api.h"

namespace dawnlight {

ModResult initialize_stamina(ModError* error);
void shutdown_stamina();

bool stamina_meter_visible();
bool stamina_available_for_bullet_time();
bool stamina_available_for_sprint();
void mark_sprint_stamina_active();
bool consume_flurry_rush_stamina();
bool consume_great_spin_stamina();
bool update_stamina(bool bulletTimeActive);

}  // namespace dawnlight
