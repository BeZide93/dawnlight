#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace dawnlight::survivor_perf {
enum class Effect : unsigned { Burn, Wave, Lightning, Explosion, Boomerang, Other, Count };
// Reservations count emitter slots, not particles. Native/world emitters are outside this budget.
struct Budget {
    struct Lease { const void* owner=nullptr; Effect kind=Effect::Other; unsigned cost=0; };
    std::array<Lease,128> leases{};
    unsigned tier=0;
    unsigned limit(Effect kind) const {
        constexpr unsigned limits[3][6]{{32,8,8,4,8,128},{24,6,6,3,6,96},{16,4,4,2,4,64}};
        return limits[std::min(tier,2u)][unsigned(kind)];
    }
    unsigned total_limit() const { return tier==0?128:tier==1?96:64; }
    unsigned used() const {unsigned n=0;for(const auto& l:leases)n+=l.cost;return n;}
    unsigned count(Effect kind) const {unsigned n=0;for(const auto& l:leases)n+=l.owner&&l.kind==kind;return n;}
    void release(const void* owner) {for(auto& l:leases)if(l.owner==owner)l={};}
    bool acquire(const void* owner,Effect kind,unsigned cost) {
        for(const auto& l:leases)if(l.owner==owner)return l.kind==kind&&l.cost==cost;
        if(!owner || !cost || count(kind)>=limit(kind) || cost>total_limit()-std::min(used(),total_limit()))return false;
        for(auto& l:leases)if(!l.owner){l={owner,kind,cost};return true;}
        return false;
    }
    void clear() { leases={}; }
};
struct Pressure {
    unsigned tier=0,quiet=0;
    // One observation per distinct completed-frame sample. Recovery needs sustained headroom.
    void observe(uint64_t used,uint64_t capacity) {
        if(!capacity)return;
        const unsigned wanted=used>=capacity*9/10?2:used>=capacity*4/5?1:tier;
        if(wanted>tier){tier=wanted;quiet=0;}
        else if(used<capacity*13/20) {if(++quiet>=120){if(tier)--tier;quiet=0;}}
        else quiet=0;
    }
    unsigned chain_per_ball() const {return tier==0?32:tier==1?24:16;}
    unsigned chain_total() const {return tier==0?128:tier==1?96:64;}
};
}
