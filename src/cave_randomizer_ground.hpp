#pragma once

#include <cmath>
#include <optional>

namespace dawnlight::cave {
struct Point { float x, y, z; };
struct Surface { bool valid = false; float y = 0; int room = -1; };

// Query returns the highest collision surface below a point. The cave stacks
// floors vertically: never accept a surface belonging to an adjacent room.
// Descend past ledges to the lowest same-room floor under the authored slot.
// A wall-mounted slot may need a small inward displacement to reach the arena.
template<class Query>
std::optional<Point> find_ground(Point authored, int room, Query query) {
    if (!std::isfinite(authored.x) || !std::isfinite(authored.y) ||
        !std::isfinite(authored.z)) return std::nullopt;
    constexpr float offsets[][2] = {
        {0, 0}, {120, 0}, {-120, 0}, {0, 120}, {0, -120},
        {120, 120}, {-120, 120}, {120, -120}, {-120, -120},
    };
    for (const auto& offset : offsets) {
        Point probe{authored.x + offset[0], authored.y + 50.0f, authored.z + offset[1]};
        std::optional<Point> floor;
        for (unsigned n = 0; n < 16; ++n) {
            const Surface surface = query(probe);
            if (!surface.valid || !std::isfinite(surface.y) || surface.y > probe.y) break;
            if (surface.room == room) floor = Point{probe.x, surface.y, probe.z};
            else if (floor) break;
            probe.y = surface.y - 1.0f;
        }
        if (floor) return floor;
    }
    return std::nullopt;
}
}
