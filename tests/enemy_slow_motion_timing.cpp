#include "../src/enemy_slow_motion/timing.hpp"
#include <cassert>

int main() {
    for (float scale : {0.1f, 0.25f, 0.5f, 1.0f}) {
        float fraction = 0.0f;
        int ticks = 0;
        for (int frame = 0; frame < 300; ++frame) {
            ticks += dawnlight::advance_enemy_timer(fraction, scale);
        }
        assert(ticks == static_cast<int>(300 * scale));
        assert(fraction >= 0.0f && fraction < 1.0f);
    }
    float a = 0.0f, b = 0.0f;
    for (int i = 0; i < 9; ++i) assert(!dawnlight::advance_enemy_timer(a, 0.1f));
    assert(!dawnlight::advance_enemy_timer(b, 0.1f));
    assert(dawnlight::advance_enemy_timer(a, 0.1f));

    const float samples[]{0.0f, 10.0f, 30.0f};
    assert(dawnlight::sample_enemy_motion(samples, -2.0f) == 0.0f);
    assert(dawnlight::sample_enemy_motion(samples, 0.5f) == 5.0f);
    assert(dawnlight::sample_enemy_motion(samples, 1.5f) == 20.0f);
    assert(dawnlight::sample_enemy_motion(samples, 5.0f) == 30.0f);
    float total = 0.0f;
    float previous = 0.0f;
    for (int i = 0; i <= 20; ++i) {
        const float current = dawnlight::sample_enemy_motion(samples, i / 10.0f);
        total += current - previous;
        previous = current;
    }
    assert(std::fabs(total - 30.0f) < 0.0001f);
    for (int i = 20; i >= 0; --i) {
        const float current = dawnlight::sample_enemy_motion(samples, i / 10.0f);
        total += current - previous;
        previous = current;
    }
    assert(std::fabs(total) < 0.0001f);
}
