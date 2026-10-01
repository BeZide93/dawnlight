#include "../src/clear_timer_state.hpp"
#include <cassert>
#include <iostream>
using namespace dawnlight::timing;

void second(Attempt& clock, double start, int encounter) {
    clock.tick(start, true, encounter);
    clock.tick(start + 1, true, encounter);
}
int main() {
    Records records;
    Attempt timer;
    assert(!timer.finish(0, records));
    timer.begin(0);
    second(timer, 10, 0);
    assert(timer.elapsed == 1000);
    timer.tick(12, false, 0); // opening the pause menu
    timer.tick(90, false, 0);
    second(timer, 100, 0); // resume: don't charge the paused interval
    assert(timer.elapsed == 2000);
    assert(timer.finish(0, records));
    assert(records.values[0] == 2000);
    timer.tick(200, true, 0);
    assert(timer.elapsed == 2000);
    assert(!timer.finish(0, records)); // repeated clear callbacks don't overwrite

    timer.begin(run);
    second(timer, 0, 0);
    assert(timer.finish(0, records));
    timer.tick(2, false, 0);
    timer.tick(600, false, 1); // save prompt, transition, load
    second(timer, 700, 1);
    assert(timer.elapsed == 2000);
    assert(timer.finish(1, records));
    second(timer, 800, 15);
    assert(timer.finish(15, records));
    second(timer, 900, 16);
    assert(timer.finish(16, records));
    timer.tick(902, false, -1);
    second(timer, 1000, run); // horseback counts only towards the full run
    assert(timer.segment == -1);
    assert(!timer.finish(15, records));
    second(timer, 1100, 17);
    assert(timer.finish(17, records));
    assert(timer.finish(run, records));
    assert(records.values[run] == 6000);
    for (int key : {0, 1, 15, 16, 17}) assert(records.values[key] == 1000);

    timer.begin(cave);
    second(timer, 0, cave);
    assert(!timer.finish(0, records));
    timer.tick(2, false, cave);
    second(timer, 10, cave);
    assert(timer.finish(cave, records));
    assert(records.values[cave] == 2000);
    timer.begin(cave);
    second(timer, 10, cave);
    timer = {}; // death, disabled timer, slot reload or return to hub
    assert(!timer.finish(cave, records));
    assert(records.values[cave] == 2000);

    timer.begin(shade);
    timer.tick(0, false, shade); // intro/howl
    second(timer, 20, shade);
    assert(timer.finish(shade, records));
    assert(records.values[shade] == 1000);
    auto bytes = records.encode();
    Records loaded;
    assert(loaded.decode(bytes));
    assert(loaded.values == records.values);
    assert(!loaded.decode(std::span(bytes.data(), bytes.size()-1)));
    assert(loaded.values == Records{}.values);
    bytes[0] = 99;
    assert(!loaded.decode(bytes));
    records.values[cave] = 6000000; // >99 minutes remains representable
    assert(loaded.decode(records.encode()));
    assert(loaded.values[cave] == 6000000);

    timer.begin(shade);
    second(timer, 0, shade);
    timer.tick(1.0e10, true, shade);
    assert(timer.finish(shade, records));
    assert(records.values[shade] == max_ms); // saturates instead of overflowing
    std::cout << "Clear timer state tests passed\n";
}
