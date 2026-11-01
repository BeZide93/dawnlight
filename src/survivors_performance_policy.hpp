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
struct HudBudget {
    unsigned bars=64,numbers=64,popups=96;
    static HudBudget for_mode(bool enabled,unsigned tier) {
        if(!enabled)return {};
        constexpr HudBudget budgets[]{{32,8,24},{16,4,12},{8,0,0}};
        return budgets[std::min(tier,2u)];
    }
};
// Main-thread counters. HUD values describe the last local overlay draw;
// shadow totals count submission decisions, not final GPU draws or unique actors.
struct NativeProjectileStats {
    unsigned normalEnemies=0,eliteEnemies=0,projectiles=0,arrows=0,attached=0,pending=0,retiring=0;
    unsigned normalArrows=0,fireArrows=0,bombArrows=0,eliteArrows=0;
    unsigned fullDraws=0,reducedDraws=0,limit=0;
    bool available=false;
};
inline unsigned native_arrow_detail_limit(unsigned tier) {
    constexpr unsigned limits[]{64,32,16};
    return limits[std::min(tier,2u)];
}
struct PresentationStats {
    NativeProjectileStats native{};
    unsigned bars=0,numbers=0,popups=0,textPasses=0,teMask=0;
    uint64_t shadowAllowed=0,shadowSuppressed=0;
    bool shadowAvailable=false;
};
inline PresentationStats presentation{};
inline bool suppress_normal_shadow(unsigned tier,float distance) {
    return tier>=2 || distance>(tier==1?600.f:1200.f);
}
struct Pressure {
    unsigned tier=0,quiet=0;
    // One observation per distinct completed-frame sample. Recovery needs sustained headroom.
    void observe(uint64_t used,uint64_t capacity) {
        if(!capacity)return;
        const unsigned wanted=used>=capacity*4/5?2:used>=capacity*7/10?1:tier;
        if(wanted>tier){tier=wanted;quiet=0;}
        else if(used<capacity*13/20) {if(++quiet>=120){if(tier)--tier;quiet=0;}}
        else quiet=0;
    }
    unsigned chain_per_ball() const {return tier==0?32:tier==1?24:16;}
    unsigned chain_total() const {return tier==0?128:tier==1?96:64;}
};
}
