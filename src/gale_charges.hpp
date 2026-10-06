#pragma once

#include <algorithm>
#include <cmath>

namespace dawnlight {
// Session-owned resource state. Neither Link nor HUD/scene lifetimes own this.
// All callers use the same monotonic seconds; invisible HUDs and cutscenes do
// not interrupt recovery. Full capacity never banks spare recovery time.
struct GaleCharges {
    int missing = 0;
    int capacity = 3;
    double elapsed = 0;
    double lastTime = 0;
    bool initialized = false;

    void update(double now, int maximum, double recoverySeconds) {
        capacity = std::clamp(maximum, 1, 12);
        missing = std::clamp(missing, 0, capacity);
        const double delta = initialized ? std::max(0.0, now - lastTime) : 0.0;
        initialized = true;
        lastTime = now;
        if (missing == 0) {
            elapsed = 0;
            return;
        }
        elapsed += delta;
        const double interval = std::clamp(recoverySeconds, 1.0, 3600.0);
        const int restored = static_cast<int>(std::min<double>(missing, std::floor(elapsed / interval)));
        missing -= restored;
        elapsed = missing == 0 ? 0 : elapsed - restored * interval;
    }
    int available() const { return capacity - missing; }
    bool consume() {
        if (available() <= 0) return false;
        if (missing == 0) elapsed = 0;
        ++missing;
        return true;
    }
};
}  // namespace dawnlight
