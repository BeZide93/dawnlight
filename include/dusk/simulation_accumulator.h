#pragma once

#include <algorithm>

namespace dusk::game_clock {
// Fixed simulation ticks, fractional presentation time. Changing the scale
// never rounds away the remainder or holds the rendered pose for a whole tick.
class SimulationAccumulator {
public:
    explicit SimulationAccumulator(float period) : period_(period) {}
    void reset(float seconds = 0.0f) { seconds_ = seconds; }
    int advance(float dt, float scale, int max_ticks) {
        seconds_ = std::min(seconds_ + std::max(0.0f, dt) * scale,
                            period_ * (max_ticks + 1));
        return std::min(static_cast<int>(seconds_ / period_), max_ticks);
    }
    void commit() { seconds_ = std::max(0.0f, seconds_ - period_); }
    float interpolation() const { return std::clamp(seconds_ / period_, 0.0f, 1.0f); }
private:
    float period_;
    float seconds_ = 0.0f;
};
}
