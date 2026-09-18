#pragma once

#include <cstdint>

namespace dawnlight {

// Returns zero if the disc resource could not be loaded; no substitute effect.
std::uint16_t bow_fire_effect();
void shutdown_bow_fire_resource();

}  // namespace dawnlight
