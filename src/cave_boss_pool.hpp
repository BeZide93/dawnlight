#pragma once
#include <cstdint>
namespace dawnlight {
struct CaveBossPool {
    uint32_t used = 0;
    uint8_t clearedRooms = 0;
    static constexpr unsigned count = 18;
    static int floor_index(int room) { return room >= 9 && room <= 49 && room % 10 == 9 ? room / 10 : -1; }
    bool cleared(int room) const {
        const int floor = floor_index(room);
        return floor >= 0 && (clearedRooms & (1u << floor));
    }
    int choose(unsigned roll) const {
        unsigned remaining = 0;
        for (unsigned i=0; i<count; ++i) if (!(used & (1u << i))) ++remaining;
        if (!remaining) return -1;
        roll %= remaining;
        for (unsigned i=0; i<count; ++i) if (!(used & (1u << i)) && roll-- == 0) return i;
        return -1;
    }
    void reserve(unsigned boss) { if (boss < count) used |= 1u << boss; }
    void complete(int room) {
        const int floor = floor_index(room);
        if (floor >= 0) clearedRooms |= 1u << floor;
    }
};
}
