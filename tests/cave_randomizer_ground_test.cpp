#include "../src/cave_randomizer_ground.hpp"

#include <cassert>
#include <limits>
#include <vector>

using namespace dawnlight::cave;

int main() {
    // A ceiling enemy above a ledge must reach this room's arena floor, not
    // retain its ceiling height or drop into the next floor of the dungeon.
    const std::vector<Surface> surfaces{{true, 1400, 6}, {true, 500, 7},
        {true, 0, 7}, {true, -1200, 8}};
    auto ray = [&](Point point) {
        for (const auto& surface : surfaces) if (surface.y <= point.y) return surface;
        return Surface{};
    };
    auto floor = find_ground({10, 1000, 20}, 7, ray);
    assert(floor && floor->x == 10 && floor->y == 0 && floor->z == 20);
    floor = find_ground({10, 0, 20}, 7, ray);
    assert(floor && floor->y == 0);

    // Collision from another loaded room is not a usable spawn point.
    assert(!find_ground({0, 100, 0}, 9, ray));
    assert(!find_ground({0, 100, 0}, 7, [](Point) { return Surface{}; }));

    // Move a wall slot inward when there is no same-room floor directly below.
    floor = find_ground({0, 1000, 0}, 7, [](Point p) {
        if (p.x == 120 && p.y >= 0) return Surface{true, 0, 7};
        return Surface{};
    });
    assert(floor && floor->x == 120 && floor->y == 0);

    // Bad collision data must not yield NaN or an ever-ascending ray.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    assert(!find_ground({0, nan, 0}, 7, ray));
    assert(!find_ground({0, 0, 0}, 7, [=](Point) { return Surface{true, nan, 7}; }));
    assert(!find_ground({0, 0, 0}, 7, [](Point p) { return Surface{true, p.y + 1, 7}; }));
}
