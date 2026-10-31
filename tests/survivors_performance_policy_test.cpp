#include "survivors_performance_policy.hpp"
#include <cassert>
#include <iostream>
using namespace dawnlight::survivor_perf;
int main(){
    Budget b;int owners[160]{};
    for(unsigned i=0;i<32;++i)assert(b.acquire(&owners[i],Effect::Burn,2));
    assert(!b.acquire(&owners[32],Effect::Burn,2));
    // Updating a living emitter never consumes another slot.
    for(unsigned i=0;i<32;++i)assert(b.acquire(&owners[i],Effect::Burn,2));
    assert(b.used()==64);
    for(unsigned i=32;i<40;++i)assert(b.acquire(&owners[i],Effect::Wave,6));
    assert(b.used()==112&&!b.acquire(&owners[40],Effect::Wave,6));
    assert(b.acquire(&owners[41],Effect::Explosion,9));
    assert(!b.acquire(&owners[42],Effect::Explosion,9)); // Global cap before per-type cap.
    b.release(&owners[41]);assert(b.used()==112);
    for(unsigned i=42;i<50;++i)assert(b.acquire(&owners[i],Effect::Lightning,2));
    assert(b.used()==128);
    assert(!b.acquire(&owners[50],Effect::Other,1));
    b.clear();assert(b.used()==0);
    for(unsigned tier=0;tier<3;++tier) {
        b.clear();b.tier=tier;
        for(unsigned i=0;i<b.limit(Effect::Explosion);++i)assert(b.acquire(&owners[i],Effect::Explosion,9));
        assert(!b.acquire(&owners[159],Effect::Explosion,9));
    }
    Pressure p;p.observe(79,100);assert(p.tier==0);
    p.observe(80,100);assert(p.tier==1);p.observe(90,100);assert(p.tier==2);
    for(unsigned i=0;i<119;++i)p.observe(64,100);
    assert(p.tier==2);
    p.observe(64,100);assert(p.tier==1);
    p.observe(70,100);assert(p.quiet==0);
    for(unsigned i=0;i<120;++i)p.observe(10,100);
    assert(p.tier==0);
    p.observe(100,0);assert(p.tier==0); // Missing telemetry cannot invent pressure.
    assert(p.chain_per_ball()==32&&p.chain_total()==128);
    std::cout<<"Performance budget: lifetime reservations, combined caps, pressure and recovery passed\n";
}
