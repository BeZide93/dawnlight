"""Link real CVar mutators and exercise host notification forwarding without host libraries."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / "dusklight").resolve()
source = (root / "src/twilit_aim.cpp").read_text()
body = source[source.index("namespace dawnlight {"):]
fixture = r'''
#include "dusk/config_var.hpp"
#include "mods/svc/hook.h"
#include <cassert>
#include <string_view>

// Only construction/registration is supplied locally. The production bridge
// must provide the mutators' notification methods and dispatch to the host.
namespace dusk::config {
ConfigVarBase::ConfigVarBase(std::string n, const ConfigImplBase* i)
    : name(std::move(n)), registered(true), layer(ConfigVarLayer::Default), impl(i) {}
ConfigVarBase::~ConfigVarBase() = default;
template<> const ConfigImplBase* GetConfigImpl<bool>() { return nullptr; }
}
using Var = dusk::config::ConfigVar<bool>;
using Base = dusk::config::ConfigVarBase;
using Layer = dusk::config::ConfigVarLayer;
Var first("firstPerson", true);
ModContext* mod_ctx = nullptr;
bool provider = true, subscribed = true;
bool notified = false, previous = false;
int notifications = 0;
std::string_view missing;
bool nullAddress = false;
Base* lookup(std::string_view key) {
    assert(key == "mod.com_dusklight_twilit__essentials.bulletTimeFirstPerson");
    return &first;
}
bool host_has_subscribers(const Base* var) {
    assert(var == &first);
    return subscribed;
}
void host_notify_changed(Base* var, const void* old) {
    assert(var == &first);
    previous = *static_cast<const bool*>(old);
    notified = first.getValue();
    assert(previous != notified);
    ++notifications;
}
ModResult resolve(ModContext*, const char* symbol, void** address, HookSymbolFlags*) {
    const std::string_view name(symbol);
    *address = nullptr;
    if (name == missing) return nullAddress ? MOD_OK : MOD_UNAVAILABLE;
    if (name == "dusk::config::GetConfigVar") *address = reinterpret_cast<void*>(&lookup);
    else if (name == "dusk::config::ConfigVarBase::has_subscribers")
        *address = reinterpret_cast<void*>(&host_has_subscribers);
    else if (name == "dusk::config::ConfigVarBase::notify_changed")
        *address = reinterpret_cast<void*>(&host_notify_changed);
    else assert(false);
    return MOD_OK;
}
HookService hook{};
const HookService* svc_hook = &hook;
namespace dawnlight {
enum class AimMode { Vanilla, Cinema, ThirdPerson };
AimMode mode = AimMode::Cinema;
AimMode aim_mode() { return mode; }
bool twilit_bullet_time_enabled() { return provider; }
}
// PRODUCTION
int main() {
    using namespace dawnlight;
    hook.resolve = resolve;
    first.setValue(false);
    notifications = 0;
    for (auto camera : {AimMode::Cinema, AimMode::ThirdPerson}) {
        mode = camera;
        update_twilit_aim();
        assert(first.getLayer() == Layer::Override && first.getValue());
        assert(!first.getValueForSave() && notified && !previous);
        const int count = notifications;
        update_twilit_aim(); assert(notifications == count);
        mode = AimMode::Vanilla; update_twilit_aim();
        assert(first.getLayer() == Layer::Value && !first.getValue());
        assert(!first.getValueForSave() && !notified && previous);
        assert(notifications == count + 1);
        mode = camera; update_twilit_aim();
        shutdown_twilit_aim();
        assert(!first.getValue() && !notified);
    }
    // Either missing method must prevent mutation, even if the resolver
    // incorrectly reports success with a null address. Retry on recovery.
    for (auto symbol : {"dusk::config::ConfigVarBase::has_subscribers",
                        "dusk::config::ConfigVarBase::notify_changed"}) {
        for (bool nullResult : {false, true}) {
            missing = symbol; nullAddress = nullResult;
            const int count = notifications;
            update_twilit_aim();
            assert(first.getLayer() == Layer::Value && !first.getValue());
            assert(notifications == count);
            missing = {}; update_twilit_aim();
            assert(first.getValue() && notified);
            shutdown_twilit_aim();
            assert(!first.getValue() && !notified);
        }
    }
    const int count = notifications;
    subscribed = false;
    update_twilit_aim(); assert(first.getValue());
    shutdown_twilit_aim(); assert(!first.getValue());
    assert(notifications == count);
}
'''
with tempfile.TemporaryDirectory(prefix="dawnlight-te-sdk-") as directory:
    cpp, exe = Path(directory) / "test.cpp", Path(directory) / "test"
    cpp.write_text(fixture.replace("// PRODUCTION", body))
    subprocess.run(["c++", "-std=c++20", "-DTARGET_PC", "-Wall", "-Wextra", "-Werror",
                    "-isystem", str(sdk / "src"), "-isystem", str(sdk / "sdk/include"),
                    "-isystem", str(sdk / "extern/aurora/include"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("TE SDK link/notifications passed: override, restore, missing symbols, retry")
