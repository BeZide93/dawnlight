#pragma once
#include "survivors_performance_policy.hpp"
#include "survivors_buffer_diagnostics.hpp"
#include "mods/api.h"
#include <cstdint>
namespace dawnlight::survivor_perf {
struct Sample {
    uint64_t sequence=0,epoch=0;
    uint32_t vertices=0,indices=0,uniforms=0,storage=0,draws=0;
    uint64_t vertexCapacity=0,indexCapacity=0,uniformCapacity=0,storageCapacity=0;
    uint64_t request=0,requestedUs=0,readUs=0,dropped=0;
    std::array<uint32_t,4> peaks{};
};
ModResult initialize();
void shutdown();
bool enabled();
bool monitor_enabled();
void set_enabled(bool);
void set_monitor(bool);
void tick(bool active);
void reset_run();
unsigned tier();
Sample sample();
uint32_t peak();
void status(char* text,unsigned size);
void counts(unsigned enemies,unsigned projectiles,unsigned burns,unsigned bolts,unsigned emitters);
void set_diagnostics(bool enabled);
void set_diagnostic_providers(BufferScene (*scene)(),FogInventory (*fog)());
}
