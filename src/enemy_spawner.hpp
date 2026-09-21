#pragma once

#include <mods/api.h>
#include "mods/svc/actor.h"

#include <array>
#include <cstddef>

namespace dawnlight {

inline constexpr std::array<const char*, 33> kEnemySpawnerProfileLabels{
    "Darknut", "Bokoblin", "Mini Freezard", "Keese", "Fire Keese", "Ice Keese",
    "Tektite", "Gibdo", "Goron", "Staltroop", "Aeralfos", "Chilfos", "Freezard",
    "Stalchild", "Bubble", "Fire Bubble", "Ice Bubble", "Rat", "White Wolfos",
    "Puppet", "Bomskit", "Stalhound (night only)", "Water Toadpoli", "Fire Toadpoli",
    "Bulblin", "Lizalfos", "Dodongo", "Dynalfos", "Skulltula", "Baba Serpent",
    "Big Baba", "Deku Baba", "Stalfos",
};

// Scene ownership plus per-process room-state isolation for standalone actors.
ModResult create_standalone_actor(ProfileName profile, const ActorSpawnParams& params, ActorId& id);
ModResult spawn_enemy_for_testing(int profileIndex);
bool enemy_spawner_blocked_in_bossrush_hub();
// True only inside a process method belonging to a tracked standalone actor.
bool enemy_spawner_process_active();

}  // namespace dawnlight
