#include "../src/cave_boss_pool.hpp"
#include <cassert>
#include <iostream>

int main() {
    using dawnlight::CaveBossPool;
    for (unsigned roll : {0u, 1u, 7u, 17u, 100u}) {
        CaveBossPool pool;
        unsigned seen = 0;
        for (unsigned i = 0; i < CaveBossPool::count; ++i) {
            const int boss = pool.choose(roll);
            assert(boss >= 0 && boss < int(CaveBossPool::count));
            assert(!(seen & (1u << boss)));
            seen |= 1u << boss;
            pool.reserve(boss);
        }
        assert(pool.choose(roll) == -1);
        assert(seen == (1u << CaveBossPool::count) - 1);
        pool = {};
        assert(pool.choose(0) == 0);
    }
    CaveBossPool pool;
    for (int room : {9, 19, 29, 39, 49}) {
        assert(!pool.cleared(room));
        pool.complete(room);
        assert(pool.cleared(room));
    }
    for (int room : {-11, -1, 0, 8, 10, 48, 50, 59, 99}) {
        assert(CaveBossPool::floor_index(room) == -1);
        assert(!pool.cleared(room));
        pool.complete(room);
    }
    assert(pool.clearedRooms == 31);
    pool.reserve(100);
    assert(pool.used == 0);
    std::cout << "Cave boss selection and fairy-floor completion: OK\n";
}
