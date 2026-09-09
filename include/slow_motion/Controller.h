#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace slow_motion {

class Controller {
public:
    void start(float scale = 0.2f) {
        if (!std::isfinite(scale) || scale < 0.1f || scale > 1.0f)
            throw std::invalid_argument("Slow-motion scale must be in [0.1, 1].");
        active_ = true;
        target_scale_ = scale;
    }

    void stop() {
        active_ = false;
        target_scale_ = 1.0f;
        scale_ = 1.0f;
    }

    void reset() { *this = {}; }

    void update(float real_dt) {
        if (!std::isfinite(real_dt) || real_dt < 0.0f)
            throw std::invalid_argument("Presentation delta must be finite and nonnegative.");
        const float dt = std::min(real_dt, 0.05f);
        scale_ += (target_scale_ - scale_) * (1.0f - std::exp(-dt * 18.0f));
        edges_ += ((active_ ? 1.0f : 0.0f) - edges_) * (1.0f - std::exp(-dt * 12.0f));
        if (std::abs(scale_ - target_scale_) < 0.001f) scale_ = target_scale_;
        if (!active_ && edges_ < 0.001f) edges_ = 0.0f;
    }

    bool active() const { return active_; }
    bool needs_interpolation() const { return active_ || scale_ < 0.999f || edges_ > 0.001f; }
    float time_scale() const { return scale_; }
    float edge_strength() const { return edges_; }
    float audio_rate() const { return 0.55f + 0.45f * scale_; }

private:
    bool active_ = false;
    float target_scale_ = 1.0f;
    float scale_ = 1.0f;
    float edges_ = 0.0f;
};

inline float blur_weight(float x, float y) {
    const float dx = std::abs(x - 0.5f) * 2.0f;
    const float dy = std::abs(y - 0.5f) * 2.0f;
    const float radius = std::sqrt(dx * dx + dy * dy);
    const float t = std::clamp((radius - 0.35f) / 0.85f, 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

}
