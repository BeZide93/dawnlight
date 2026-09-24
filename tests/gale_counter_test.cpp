#include "../src/gale_charges.hpp"
#include <cassert>
#include <cmath>
using dawnlight::GaleCharges;
int main() {
    GaleCharges c;
    c.update(1000, 3, 120);
    assert(c.available() == 3);
    assert(c.consume());
    c.update(1060, 3, 120);
    assert(c.available() == 2 && c.elapsed == 60);
    assert(c.consume()); // A second use must not reset the first recharge.
    c.update(1119, 3, 120);
    assert(c.available() == 1);
    c.update(1120, 3, 120);
    assert(c.available() == 2 && c.elapsed == 0);
    c.update(1239, 3, 120);
    assert(c.available() == 2);
    c.update(1240, 3, 120);
    assert(c.available() == 3);
    // Full time is not banked. Zero charges cannot go negative.
    c.update(10000, 3, 120);
    assert(c.consume() && c.consume() && c.consume() && !c.consume());
    c.update(10001, 3, 120);
    assert(c.available() == 0);
    // No calls while the HUD is hidden, during loading or a cutscene. The next
    // access must still restore all elapsed charges and retain partial time.
    c.update(10270, 3, 120);
    assert(c.available() == 2 && c.elapsed == 30);
    c.update(10360, 3, 120);
    assert(c.available() == 3);
    assert(c.consume());
    c.update(10420, 3, 120);
    c.update(10420, 5, 120);
    assert(c.available() == 4 && c.elapsed == 60);
    c.update(10420, 1, 120);
    assert(c.available() == 0 && c.elapsed == 60);
    c.update(10420, 1, 60); // Changing recovery keeps elapsed progress.
    assert(c.available() == 1);
    for (int capacity : {1,3,12}) for (int seconds : {1,120,3600}) {
        c = {};
        c.update(0, capacity, seconds);
        for (int i=0;i<capacity;++i) assert(c.consume());
        c.update(seconds - 0.01, capacity, seconds);
        assert(c.available() == 0);
        c.update(seconds, capacity, seconds);
        assert(c.available() == 1);
        c.update(1e9, capacity, seconds);
        assert(c.available() == capacity && c.elapsed == 0);
    }
    c = {}; // Fresh process/session is full again.
    c.update(0, 99, 120);
    assert(c.available() == 12);
    c.update(0, -10, 120);
    assert(c.available() == 1);
}
