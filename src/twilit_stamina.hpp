#pragma once

#include "stamina_settings.hpp"

namespace dawnlight {
bool twilit_stamina_active();
float twilit_stamina_cost(StaminaSetting setting);
bool twilit_stamina_available(float amount);
bool twilit_stamina_consume(float amount);
bool twilit_stamina_drain(float amount);
bool twilit_stamina_gameplay();
// Sprint ownership is independent of TE's optional stamina meter.
bool twilit_sprint_enabled(bool wolf = false);
bool twilit_bullet_time_enabled();
float twilit_sprint_drain_multiplier(float speed, float baseSpeed);
constexpr bool twilit_owns_stamina_setting(StaminaSetting setting) {
    return setting != StaminaSetting::Glide && setting != StaminaSetting::FlurryRush &&
        setting != StaminaSetting::GreatSpin && setting != StaminaSetting::MidnaAttack;
}
void shutdown_twilit_stamina();
} // namespace dawnlight
