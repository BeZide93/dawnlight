#include "../src/bow_volley.hpp"

#include <array>
#include <cassert>
#include <cmath>

struct Velocity {
    float x, y, z;
    Velocity(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

float length_squared(const Velocity& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

int main() {
    using namespace dawnlight;

    // Every pair of the three overlapping arrows must be ignored, in both
    // attack/target directions. Enemy hits and separate volleys remain eligible.
    const std::array<std::uint32_t, 3> volley{42, 42, 42};
    for (auto attack : volley) {
        for (auto target : volley) {
            assert(same_bow_volley(attack, target));
        }
        assert(!same_bow_volley(attack, kNoBowVolley));
        assert(!same_bow_volley(kNoBowVolley, attack));
        assert(!same_bow_volley(attack, 43));
    }
    assert(!same_bow_volley(kNoBowVolley, kNoBowVolley));

    // Charged speeds, steep pitch, and off-axis camera aim must retain speed
    // and vertical motion, and produce distinct left/right trajectories.
    for (const auto& center : {Velocity(0, 0, 100), Velocity(0, 180, 300),
                               Velocity(110, -250, -70), Velocity(-90, 5, 160)})
    {
        const auto left = bow_spread_velocity(center, -0x900);
        const auto right = bow_spread_velocity(center, 0x900);
        for (const auto& side : {left, right}) {
            assert(side.y == center.y);
            assert(std::abs(length_squared(side) - length_squared(center)) < 0.1f);
        }
        assert(center.z * left.x - center.x * left.z < 0);
        assert(center.z * right.x - center.x * right.z > 0);
        const auto recovered = bow_spread_velocity(left, 0x900);
        assert(std::abs(recovered.x - center.x) < 0.001f);
        assert(std::abs(recovered.z - center.z) < 0.001f);
    }
}
