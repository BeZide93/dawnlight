"""Run the production TE aim bridge against live provider/config lifecycles."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/twilit_aim.cpp").read_text()
body = source[source.index("namespace dawnlight {"):]
fixture = r'''
#include <cassert>
#include <string_view>
constexpr int MOD_OK = 0, MOD_UNAVAILABLE = 1;
void* mod_ctx = nullptr;
bool notifiedFirstPerson = false;
int writes = 0;
namespace dusk::config {
enum class ConfigVarLayer { Default, Value, Override, Speedrun };
struct ConfigVarBase {
    bool has_subscribers() const;
    void notify_changed(const void*);
};
template<class T> struct ConfigVar : ConfigVarBase {
    bool registered = true;
    T saved = false, effective = false;
    ConfigVarLayer layer = ConfigVarLayer::Value;
    T getValue() const { assert(registered); return effective; }
    ConfigVarLayer getLayer() const { assert(registered); return layer; }
    void setOverrideValue(T v) {
        assert(registered); effective = v; layer = ConfigVarLayer::Override;
        notifiedFirstPerson = v; ++writes;
    }
    void clearOverride() {
        assert(registered); effective = saved; layer = ConfigVarLayer::Value;
        notifiedFirstPerson = saved; ++writes;
    }
};
}
using Var = dusk::config::ConfigVar<bool>;
using Layer = dusk::config::ConfigVarLayer;
Var first, replacement;
Var* live = nullptr;
bool provider = false, missingResolver = false;
dusk::config::ConfigVarBase* lookup(std::string_view key) {
    assert(key == "mod.com_dusklight_twilit__essentials.bulletTimeFirstPerson");
    return live;
}
bool host_has_subscribers(const dusk::config::ConfigVarBase*) { return true; }
void host_notify_changed(dusk::config::ConfigVarBase*, const void*) {}
int resolve(void*, const char* symbol, void** address, void*) {
    const std::string_view name(symbol);
    if (name == "dusk::config::GetConfigVar") *address = reinterpret_cast<void*>(&lookup);
    else if (name == "dusk::config::ConfigVarBase::has_subscribers")
        *address = reinterpret_cast<void*>(&host_has_subscribers);
    else if (name == "dusk::config::ConfigVarBase::notify_changed")
        *address = reinterpret_cast<void*>(&host_notify_changed);
    else assert(false);
    if (missingResolver) *address = nullptr;
    return missingResolver ? MOD_UNAVAILABLE : MOD_OK;
}
struct Hook { int (*resolve)(void*, const char*, void**, void*); } hook{resolve};
Hook* svc_hook = &hook;
namespace dawnlight {
enum class AimMode { Vanilla, Cinema, ThirdPerson };
AimMode mode = AimMode::Cinema;
AimMode aim_mode() { return mode; }
bool twilit_bullet_time_enabled() { return provider; }
}
// PRODUCTION
int main() {
    using namespace dawnlight;
    for (auto camera : {AimMode::Cinema, AimMode::ThirdPerson}) {
        mode = camera; provider = false; live = nullptr;
        update_twilit_aim(); assert(!twilit_bullet_time_aim_enabled());
        // Provider may initialize after Dawnlight's first update.
        provider = true; first = {}; live = &first;
        update_twilit_aim();
        assert(first.effective && !first.saved && notifiedFirstPerson);
        assert(twilit_bullet_time_aim_enabled());
        const int count = writes;
        update_twilit_aim(); assert(writes == count); // no notification churn
        mode = AimMode::Vanilla; update_twilit_aim();
        assert(!first.effective && !first.saved && !notifiedFirstPerson);
        mode = camera; update_twilit_aim();
        provider = false; update_twilit_aim();
        assert(!first.effective && !twilit_bullet_time_aim_enabled());
        provider = true; update_twilit_aim();
        // A saved preference edited underneath the override survives release.
        first.saved = true; mode = AimMode::Vanilla; update_twilit_aim();
        assert(first.effective && first.saved);
        shutdown_twilit_aim();
    }
    provider = true; mode = AimMode::ThirdPerson;
    // Existing launch/other-mod and speedrun overrides are never taken over.
    for (auto layer : {Layer::Override, Layer::Speedrun}) for (bool value : {false, true}) {
        first = {}; first.layer = layer; first.effective = value; live = &first;
        const int count = writes;
        update_twilit_aim(); shutdown_twilit_aim();
        assert(writes == count && first.layer == layer && first.effective == value);
    }
    // An already-enabled backend needs no override, including the default layer.
    first = {}; first.effective = true; first.layer = Layer::Default; live = &first;
    const int count = writes;
    update_twilit_aim(); shutdown_twilit_aim(); assert(writes == count);
    // Removal/re-registration must not dereference a stale provider CVar.
    first = {}; live = &first; update_twilit_aim(); first.registered = false;
    replacement = {}; live = &replacement; update_twilit_aim();
    assert(replacement.effective); shutdown_twilit_aim(); assert(!replacement.effective);
    first = {}; live = &first; update_twilit_aim(); first.registered = false;
    live = nullptr; shutdown_twilit_aim();
    // Another owner replaces our override: do not clear that owner's value.
    first = {}; live = &first; update_twilit_aim();
    first.effective = false; first.layer = Layer::Speedrun;
    shutdown_twilit_aim(); assert(first.layer == Layer::Speedrun && !first.effective);
    // Missing host lookup is a no-op, retried when it becomes available.
    missingResolver = true; first = {}; live = &first; update_twilit_aim();
    assert(!twilit_bullet_time_aim_enabled() && !first.effective);
    missingResolver = false; update_twilit_aim(); assert(first.effective);
    shutdown_twilit_aim(); assert(!first.effective && !first.saved);
}
'''
with tempfile.TemporaryDirectory(prefix="dawnlight-te-aim-") as directory:
    cpp, exe = Path(directory) / "test.cpp", Path(directory) / "test"
    cpp.write_text(fixture.replace("// PRODUCTION", body))
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("TE aim bridge passed: runtime override, saved preferences, ownership, unload/reload")
