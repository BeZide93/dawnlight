#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace dawnlight {
inline bool advance_enemy_timer(float& fraction, float scale) {
    fraction += scale;
    if (fraction < 1.0f - 0.000001f) return false;
    fraction = std::fmax(0.0f, fraction - 1.0f);
    return true;
}

template <std::size_t N>
float sample_enemy_motion(const float (&samples)[N], float frame) {
    static_assert(N > 0);
    frame = std::clamp(frame, 0.0f, static_cast<float>(N - 1));
    const auto index = static_cast<std::size_t>(frame);
    return samples[index] + (samples[std::min(index + 1, N - 1)] - samples[index]) * (frame - index);
}
}
