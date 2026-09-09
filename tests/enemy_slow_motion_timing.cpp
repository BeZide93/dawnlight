#include "../src/enemy_slow_motion/timing.hpp"
#include "../src/enemy_slow_motion/scope.hpp"
#include "../src/enemy_slow_motion/events.hpp"
#include <cassert>

int main() {
    struct Scope {
        const void* actor = nullptr;
        const void* profile = nullptr;
    };
    std::array<Scope, 2> scopes{};
    int actor = 0, profile = 0;
    assert(dawnlight::active_enemy_scope(scopes, 0) == nullptr);
    assert(dawnlight::active_enemy_scope(scopes, 1) == nullptr); // slow motion off
    scopes[0].actor = &actor;
    assert(dawnlight::active_enemy_scope(scopes, 1) == nullptr);
    scopes[0] = {nullptr, &profile};
    assert(dawnlight::active_enemy_scope(scopes, 1) == nullptr);
    scopes[0] = {&actor, &profile};
    assert(dawnlight::active_enemy_scope(scopes, 1) == &scopes[0]);
    assert(dawnlight::active_enemy_scope(scopes, 2) == nullptr); // inactive child masks parent
    scopes[1] = {&actor, &profile};
    assert(dawnlight::active_enemy_scope(scopes, 2) == &scopes[1]);
    assert(dawnlight::active_enemy_scope(scopes, 3) == nullptr); // nesting overflow
    scopes[1] = {};
    assert(dawnlight::active_enemy_scope(scopes, 1) == &scopes[0]); // child returned
    scopes = {};
    assert(dawnlight::active_enemy_scope(scopes, 1) == nullptr); // reset

    dawnlight::EnemyFrameEvents events, otherActor;
    events.update(&actor, 4);
    assert(events.claim(42));
    assert(!events.claim(42));
    assert(events.claim(43)); // distinct events on the same frame remain valid
    otherActor.update(&actor, 4);
    assert(otherActor.claim(42));
    events.update(&actor, 5); // animation->play crossed an integer boundary
    assert(events.claim(42));
    events.update(&actor, 5); // following execute starts on that same frame
    assert(!events.claim(42));
    events.update(&actor, 6); // silent frame must reset a single-event loop
    events.update(&actor, 5);
    assert(events.claim(42));
    events.update(&profile, 5); // different animation at the same frame
    assert(events.claim(42));
    events = {};
    events.update(&actor, 5);
    assert(events.claim(42)); // reset/new actor lifetime

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

    assert(dawnlight::sample_looped_enemy_motion(samples, 2.5f) == 15.0f);
    assert(dawnlight::sample_looped_enemy_motion(samples, 3.5f) == 5.0f);
    assert(dawnlight::sample_looped_enemy_motion(samples, -1.0f) == 0.0f);
    assert(std::fabs(dawnlight::slow_enemy_position_axis(100.0f, 72.0f, -30.0f, 0.1f) + 30.0f - 100.2f) < 0.001f);
    assert(dawnlight::slow_enemy_position_axis(100.0f, 102.0f, 0.0f, 1.0f) == 102.0f);
    assert(dawnlight::slow_enemy_position_axis(100.0f, 72.0f, -30.0f, 0.0f) == 70.0f);

    for (std::uint16_t start : {0, 65520}) {
        std::uint16_t counter = start;
        float phase = 0.0f;
        int events4 = 0, events8 = 0, events16 = 0, events32 = 0;
        for (int frame = 0; frame < 320; ++frame) {
            const auto saved = counter;
            const bool tick = dawnlight::advance_enemy_timer(phase, 0.1f);
            counter = dawnlight::enemy_periodic_counter_input(counter, tick);
            ++counter;
            events4 += (counter & 3U) == 0;
            events8 += (counter & 7U) == 0;
            events16 += (counter & 15U) == 0;
            events32 += (counter & 31U) == 0;
            if (!tick) counter = saved;
        }
        assert(counter == static_cast<std::uint16_t>(start + 32));
        assert(events4 == 8 && events8 == 4 && events16 == 2 && events32 == 1);
    }
}
