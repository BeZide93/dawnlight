#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace dawnlight {

inline constexpr auto kNoBowVolley = std::numeric_limits<std::uint32_t>::max();

constexpr bool same_bow_volley(std::uint32_t first, std::uint32_t second) {
    return first != kNoBowVolley && first == second;
}

// Rotate the final aimed velocity, keeping its pitch and magnitude (also on horseback).
template <class Vec>
Vec bow_spread_velocity(const Vec& center, std::int16_t yaw) {
    constexpr float radiansPerUnit = 6.2831853071795864769f / 65536.0f;
    const float sine = std::sin(yaw * radiansPerUnit);
    const float cosine = std::cos(yaw * radiansPerUnit);
    return Vec(center.x * cosine + center.z * sine, center.y,
               center.z * cosine - center.x * sine);
}

}  // namespace dawnlight
