#pragma once

#include <mods/api.h>

#include <array>
#include <cstddef>

namespace dawnlight {

inline constexpr std::array<const char*, 28> kEnemySpawnerProfileLabels{
    "Darknut", "Bokoblin", "Mini Freezard", "Keese", "Tektite", "Gibdo", "Goron",
    "Staltroop", "Aeralfos", "Chilfos", "Freezard", "Stalchild", "Bubble", "Rat",
    "White Wolfos", "Puppet", "Bomskit", "Stalhound (night only)", "Fire Toadpoli", "Bulblin",
    "Lizalfos", "Dodongo", "Dynalfos", "Skulltula", "Baba Serpent", "Big Baba",
    "Deku Baba", "Stalfos",
};

ModResult spawn_enemy_for_testing(int profileIndex);

}  // namespace dawnlight
