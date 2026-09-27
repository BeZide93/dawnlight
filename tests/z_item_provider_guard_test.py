"""Run the production provider guard and deferred installer against host fixtures."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
items = (root / 'src/item_slot_hooks.cpp').read_text()
mod = (root / 'src/mod.cpp').read_text()
guard = items[items.index('using ZSlotGetConfigVarFn'):items.index('bool z_item_slot_active()')]
start = mod.index('MOD_EXPORT ModResult mod_update(')
update = mod[start:mod.index('\n}', start) + 2]
fixture = r'''
#include <cassert>
#include <map>
#include <string>
#include <string_view>
#include <vector>
using ModResult = int;
constexpr int MOD_OK = 0, MOD_ERROR = 1;
struct ModContext {};
struct ModError {};
ModContext* mod_ctx = nullptr;
namespace dusk::config {
struct ConfigVarBase {};
template<class T> struct ConfigVar : ConfigVarBase {
    T value = false;
    T getValue() const { return value; }
};
}
std::map<std::string, dusk::config::ConfigVar<bool>> vars;
dusk::config::ConfigVarBase* getVar(std::string_view key) {
    auto it = vars.find(std::string(key));
    return it == vars.end() ? nullptr : &it->second;
}
void set(const char* key, bool value) { vars[key].value = value; }
struct HookService {
    ModResult (*resolve)(ModContext*, const char*, void**, ModError*);
};
bool resolveFails = false, nullAddress = false;
int resolves = 0;
ModResult resolve(ModContext*, const char* symbol, void** address, ModError*) {
    assert(std::string_view(symbol) == "dusk::config::GetConfigVar");
    ++resolves;
    *address = nullAddress ? nullptr : reinterpret_cast<void*>(&getVar);
    return resolveFails ? MOD_ERROR : MOD_OK;
}
HookService hook{resolve};
HookService* svc_hook = &hook;
struct Log {
    std::vector<std::string> messages;
    void info(ModContext*, const char* text) { messages.emplace_back(text); }
    void warn(ModContext*, const char* text) { messages.emplace_back(text); }
} logService;
Log* svc_log = &logService;
void* svc_ui = nullptr;
bool savedToggle = true;
bool z_item_slot_enabled() { return savedToggle; }
// GUARD
bool s_itemSlotHooksInstalled = false, selected = false, touchInstalled = false;
int installs = 0, otherUpdates = 0;
bool failInstall = false;
namespace dawnlight {
ModResult install_item_slot_hooks(ModError*) {
    ++installs;
    if (failInstall) return MOD_ERROR;
    selected = select_dawnlight_z_slot();
    touchInstalled = true;
    return MOD_OK;
}
void update_new_save_modes() { ++otherUpdates; }
void update_progression() {}
void bullet_time_tick() {}
void update_update_service(Log*, ModContext*, void*) {}
}
#define MOD_EXPORT
// UPDATE
int main() {
    const char* hd = "mod.org_twilight_hd__hud.enabled";
    const char* hdSlot = "mod.org_twilight_hd__hud.third-item-slot";
    const char* te = "mod.com_dusklight_twilit__essentials.enabled";
    const char* teSlot = "mod.com_dusklight_twilit__essentials.customZButtonEnabled";
    assert(select_dawnlight_z_slot()); // absent providers
    for (int mask = 0; mask < 16; ++mask) {
        vars.clear();
        set(hd, mask & 1); set(hdSlot, mask & 2);
        set(te, mask & 4); set(teSlot, mask & 8);
        bool conflict = ((mask & 1) && (mask & 2)) || ((mask & 4) && (mask & 8));
        assert(select_dawnlight_z_slot() == !conflict);
        assert(savedToggle); // no writes to the user's preference
    }
    // Stale feature settings alone cannot make a disabled/absent mod own Z.
    vars.clear(); set(hdSlot, true); set(teSlot, true);
    assert(select_dawnlight_z_slot());
    // Older versions lacking a feature toggle retain their default-on behavior.
    for (auto key : {hd, te}) {
        vars.clear(); set(key, true); assert(!select_dawnlight_z_slot());
        set(key, false); assert(select_dawnlight_z_slot());
    }
    vars.clear(); savedToggle = false; resolves = 0;
    assert(!select_dawnlight_z_slot() && resolves == 0);
    savedToggle = true; svc_hook = nullptr; assert(!select_dawnlight_z_slot());
    svc_hook = &hook; hook.resolve = nullptr; assert(!select_dawnlight_z_slot());
    hook.resolve = resolve; resolveFails = true; assert(!select_dawnlight_z_slot());
    resolveFails = false; nullAddress = true; assert(!select_dawnlight_z_slot());
    nullAddress = false;
    // Other mods can register before OR after Dawnlight initialization. Nothing
    // installs until update, when the provider's saved false value is available.
    for (auto provider : {hd, te}) {
        auto slot = provider == hd ? hdSlot : teSlot;
        for (bool earlier : {false, true}) for (bool enabled : {false, true}) {
            vars.clear(); s_itemSlotHooksInstalled = false; installs = 0;
            auto registerProvider = [&] { set(provider, true); set(slot, enabled); };
            if (earlier) registerProvider();
            assert(installs == 0);
            if (!earlier) registerProvider();
            assert(mod_update(nullptr) == MOD_OK);
            assert(selected == !enabled && touchInstalled && installs == 1);
            assert(mod_update(nullptr) == MOD_OK && installs == 1);
        }
    }
    // Hook errors propagate through mod_update without falsely marking success.
    s_itemSlotHooksInstalled = false; failInstall = true;
    int previous = otherUpdates;
    assert(mod_update(nullptr) == MOD_ERROR && !s_itemSlotHooksInstalled);
    assert(otherUpdates == previous);
}
'''
fixture = fixture.replace('// GUARD', guard).replace('// UPDATE', update)
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
# Lifecycle wiring: no early snapshot and no stale flag on reload.
initialize = mod[mod.index('MOD_EXPORT ModResult mod_initialize'):start]
assert 'install_item_slot_hooks(error)' not in initialize
shutdown = mod[mod.index('MOD_EXPORT ModResult mod_shutdown'):]
assert 's_itemSlotHooksInstalled = false;' in shutdown
assert 's_zItemSlotSessionEnabled = select_dawnlight_z_slot();' in items
assert 's_dawnlightTouchUiSessionEnabled = dawnlight_touch_ui_enabled();' in items
print('Z-slot guards passed: provider combinations, missing settings, lookup failures, both startup orders, touch independence, install-once and failure propagation')
