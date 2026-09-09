#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace dawnlight {
inline bool advance_enemy_timer(float& fraction, float scale) {
    fraction += scale;
    if (fraction < 1.0f - 0.000001f) return false;
    fraction = std::fmax(0.0f, fraction - 1.0f);
    return true;
}

// For counters whose ONLY consumers are modulo-16/32 decisions and whose
// native execute increments them once. The caller restores held counters.
inline std::uint16_t enemy_periodic_counter_input(std::uint16_t counter, bool timerTick) {
    return timerTick ? counter : static_cast<std::uint16_t>((counter | 1U) - 1U);
}

inline float slow_enemy_position_axis(float original, float integrated, float collisionOffset, float scale) {
    return original + collisionOffset + (integrated - collisionOffset - original) * scale;
}

template <std::size_t N>
float sample_looped_enemy_motion(const float (&samples)[N], float frame) {
    static_assert(N > 0);
    frame = std::fmod(std::max(0.0f, frame), static_cast<float>(N));
    const auto index = static_cast<std::size_t>(frame);
    return samples[index] + (samples[(index + 1) % N] - samples[index]) * (frame - index);
}

template <std::size_t N>
float sample_enemy_motion(const float (&samples)[N], float frame) {
    static_assert(N > 0);
    frame = std::clamp(frame, 0.0f, static_cast<float>(N - 1));
    const auto index = static_cast<std::size_t>(frame);
    return samples[index] + (samples[std::min(index + 1, N - 1)] - samples[index]) * (frame - index);
}
}
