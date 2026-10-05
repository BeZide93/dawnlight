#pragma once

#include <algorithm>

namespace dawnlight {

// Real seconds, independent of simulation FPS. Long gaps (menus,
// scene loads, backgrounded app) start visible rather than completing an unseen fade.
struct HudFadeEnvelope {
    double last = -1.0;
    float level = 1.0f;

    float update(double now, bool hide, bool enabled = true) {
        const double elapsed = last < 0.0 ? 0.0 : now - last;
        last = now;
        if (!enabled || elapsed < 0.0 || elapsed > 0.25) level = 1.0f;
        else {
            const float step = static_cast<float>(elapsed / (hide ? 1.0 : 0.15));
            level = std::clamp(level + (hide ? -step : step), 0.0f, 1.0f);
        }
        return level * level * (3.0f - 2.0f * level);
    }
};

struct HudIdleFade {
    HudFadeEnvelope fade;
    double idleSince = -1.0;

    float update(double now, bool enabled, bool idle) {
        if (!enabled || !idle || fade.last < 0.0 || now < fade.last || now - fade.last > 0.25)
            idleSince = now;
        if (idleSince < 0.0) idleSince = now;
        const bool hide = idle && now - idleSince >= 3.0;
        if (hide) fade.last = std::max(fade.last, idleSince + 3.0);
        return fade.update(now, hide, enabled);
    }
};

} // namespace dawnlight
