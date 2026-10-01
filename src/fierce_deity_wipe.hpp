#pragma once

#include <algorithm>

namespace dawnlight {
// Link's native human warp scrolls -0.5 <-> 5.5 at 0.06 per simulation tick,
// with horizontal noise scrolling at 0.15. Both rates are doubled, not the
// player's animation/game speed. No cutscene hold or teleport is involved.
struct FierceWarpWipe {
    static constexpr float low = -0.5f;
    static constexpr float high = 5.5f;
    static constexpr float step = 0.06f * 2.0f;
    static constexpr float noiseStep = 0.15f * 2.0f;
    bool entering = true;
    unsigned ticks = 0;
    float height() const {
        const float distance = std::min(float(ticks) * step, high - low);
        return entering ? high - distance : low + distance;
    }
    float scroll() const {
        const float value = float(ticks) * noiseStep;
        return value - static_cast<unsigned>(value);
    }
    bool inverse(bool incoming) const { return entering == incoming; }
    bool advance() { return float(++ticks) * step >= high - low; }
};
} // namespace dawnlight
