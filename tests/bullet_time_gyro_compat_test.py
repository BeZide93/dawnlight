"""Exercise production gyro lookup and keep-alive logic against relocated host settings."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/bullet_time.cpp').read_text()


def function(signature):
    start = source.index(signature + '(')
    return source[start:source.index('\n}', start) + 2]


start = source.index('bool s_bulletTimeOwnsGyroKeepAlive')
state = source[start:source.index('#endif', start)]
fixture = r'''
#include <array>
#include <cassert>
#include <cstdlib>
#include <string_view>

namespace dusk::config {
struct ConfigVarBase { virtual ~ConfigVarBase() = default; };
template <typename T> struct ConfigVar : ConfigVarBase {
    bool registered = false;
    T value{};
    const T& getValue() const { if (!registered) std::abort(); return value; }
};
}
using BoolVar = dusk::config::ConfigVar<bool>;
struct UpstreamSettings { BoolVar gyro; } upstream;
struct ForkSettings {
    // Deliberately incompatible placement, as in Lazy Tweaks' UserSettings.
    std::array<char, 1024> additionalOptions{};
    BoolVar gyro;
} forkSettings;
BoolVar* registeredGyro = nullptr;
int lookups = 0, resolves = 0, writes = 0;
bool hostKeepAlive = false;
bool missingLookup = false, nullLookup = false, missingGyro = false;
bool s_bulletTimeActive = false;
constexpr int MOD_OK = 0, MOD_NOT_FOUND = 1;
void* mod_ctx = nullptr;

dusk::config::ConfigVarBase* host_lookup(std::string_view name) {
    assert(name == "game.enableGyroAim");
    ++lookups;
    return registeredGyro;
}
bool host_get_keep_alive() { return hostKeepAlive; }
void host_set_keep_alive(bool active) { hostKeepAlive = active; ++writes; }
void host_get_deltas(float&, float&) {}
int host_resolve(void*, const char* symbol, void** result, void*) {
    ++resolves;
    const std::string_view name(symbol);
    *result = nullptr;
    if (name == "dusk::config::GetConfigVar") {
        if (missingLookup) return MOD_NOT_FOUND;
        if (!nullLookup) *result = reinterpret_cast<void*>(&host_lookup);
    } else if (name == "dusk::gyro::get_sensor_keep_alive") {
        if (missingGyro) return MOD_NOT_FOUND;
        *result = reinterpret_cast<void*>(&host_get_keep_alive);
    } else if (name == "dusk::gyro::set_sensor_keep_alive") {
        if (missingGyro) return MOD_NOT_FOUND;
        *result = reinterpret_cast<void*>(&host_set_keep_alive);
    } else if (name == "dusk::gyro::getAimDeltas") {
        *result = reinterpret_cast<void*>(&host_get_deltas);
    } else {
        // No call to dusk::getSettings (or another layout-dependent fallback).
        assert(false);
    }
    return MOD_OK;
}
struct HookService { decltype(&host_resolve) resolve; } hooks{&host_resolve};
HookService* svc_hook = &hooks;
// STATE
// FUNCTIONS

void reset() {
    s_bulletTimeOwnsGyroKeepAlive = false;
    s_getGyroKeepAlive = nullptr;
    s_setGyroKeepAlive = nullptr;
    s_getGyroAimDeltas = nullptr;
    s_getConfigVar = nullptr;
    s_gyroKeepAliveSymbolsResolved = false;
    registeredGyro = nullptr;
    missingLookup = nullLookup = missingGyro = false;
    s_bulletTimeActive = hostKeepAlive = false;
    lookups = resolves = writes = 0;
    svc_hook = &hooks;
    hooks.resolve = &host_resolve;
}

int main() {
    for (BoolVar* gyro : {&upstream.gyro, &forkSettings.gyro}) {
        reset();
        gyro->registered = true;
        registeredGyro = gyro;
        gyro->value = false;
        assert(!bullet_time_gyro_enabled());
        gyro->value = true;
        assert(bullet_time_gyro_enabled());
        assert(resolves == 4 && lookups == 2);
        // An enabled setting alone must not take ownership outside Bullet Time.
        sync_bullet_time_gyro_keep_alive();
        assert(!hostKeepAlive && writes == 0);
        s_bulletTimeActive = true;
        sync_bullet_time_gyro_keep_alive();
        assert(hostKeepAlive && s_bulletTimeOwnsGyroKeepAlive && writes == 1);
        sync_bullet_time_gyro_keep_alive();
        assert(writes == 1);
        gyro->value = false; // A live settings toggle releases our own request.
        sync_bullet_time_gyro_keep_alive();
        assert(!hostKeepAlive && !s_bulletTimeOwnsGyroKeepAlive && writes == 2);
        gyro->value = true;
        sync_bullet_time_gyro_keep_alive();
        s_bulletTimeActive = false;
        sync_bullet_time_gyro_keep_alive();
        assert(!hostKeepAlive && !s_bulletTimeOwnsGyroKeepAlive && writes == 4);
    }

    reset();
    assert(!bullet_time_gyro_enabled()); // Absent CVar: do not call getValue().
    registeredGyro = &forkSettings.gyro;
    assert(bullet_time_gyro_enabled()); // Later registration is visible.
    s_bulletTimeActive = true;
    sync_bullet_time_gyro_keep_alive();
    registeredGyro = nullptr; // No stale cached CVar pointer after removal.
    sync_bullet_time_gyro_keep_alive();
    assert(!hostKeepAlive && writes == 2);

    reset();
    registeredGyro = &forkSettings.gyro;
    hostKeepAlive = true; // Preserve a request owned by the host or another mod.
    s_bulletTimeActive = true;
    sync_bullet_time_gyro_keep_alive();
    s_bulletTimeActive = false;
    registeredGyro = nullptr;
    sync_bullet_time_gyro_keep_alive();
    assert(hostKeepAlive && !s_bulletTimeOwnsGyroKeepAlive && writes == 0);

    for (int failure = 0; failure < 5; ++failure) {
        reset();
        registeredGyro = &forkSettings.gyro;
        s_bulletTimeActive = true;
        if (failure == 0) missingLookup = true;
        if (failure == 1) nullLookup = true;
        if (failure == 2) svc_hook = nullptr;
        if (failure == 3) hooks.resolve = nullptr;
        if (failure == 4) missingGyro = true;
        sync_bullet_time_gyro_keep_alive();
        assert(!hostKeepAlive && !s_bulletTimeOwnsGyroKeepAlive && writes == 0);
    }
}
'''
fixture = fixture.replace('// STATE', state)
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(signature) for signature in (
    'void resolve_bullet_time_gyro_symbols',
    'bool bullet_time_gyro_enabled',
    'void sync_bullet_time_gyro_keep_alive',
)))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Gyro compatibility passed: relocated settings, live changes, missing APIs/CVar, keep-alive ownership')
