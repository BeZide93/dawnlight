"""Run the production provider guard and deferred installer against host fixtures."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
items = (root / 'src/item_slot_hooks.cpp').read_text()
mod = (root / 'src/mod.cpp').read_text()
te = (root / 'src/twilit_stamina.cpp').read_text()
te_features = te[te.index('static bool twilit_feature_enabled('):te.index('void shutdown_twilit_stamina(')]
guard = items[items.index('using ZSlotGetConfigVarFn'):items.index('bool z_item_slot_active()')]
start = mod.index('MOD_EXPORT ModResult mod_update(')
update = mod[start:mod.index('\n}', start) + 2]
fixture = r'''
#include <cassert>
#include <algorithm>
#include <cmath>
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
namespace te_fixture {
using GetConfigVarFn = ZSlotGetConfigVarFn;
GetConfigVarFn s_getConfigVar = nullptr;
bool active = true;
bool twilit_stamina_active() { return active; }
// TE FEATURES
}
bool s_itemSlotHooksInstalled = false, selected = false, touchInstalled = false;
int installs = 0, otherUpdates = 0, kh2Updates = 0;
bool failInstall = false;
namespace dawnlight {
ModResult install_item_slot_hooks(ModError*) {
    ++installs;
    if (failInstall) return MOD_ERROR;
    selected = select_dawnlight_z_slot();
    touchInstalled = true;
    return MOD_OK;
}
void update_cave_randomizer() {}
void update_new_save_modes() { ++otherUpdates; }
void update_progression() {}
void update_twilit_aim() {}
void bullet_time_tick() {}
// Stamina ownership/UI behavior is covered by the stamina and mode fixtures.
void update_stamina_ui() {}
// KH2 behavior is covered by kh2_hud_compat_test.py; track the frame dispatch here.
bool update_kh2_drive() { ++kh2Updates; return false; }
void update_update_service(Log*, ModContext*, void*) {}
}
#define MOD_EXPORT
// UPDATE
int main() {
    // The actual TE feature reader must use the host's escaped mod ID:
    // dots become '_', while the literal underscore in twilit_essentials becomes '__'.
    const char* teMod = "mod.com_dusklight_twilit__essentials.enabled";
    const char* sprint = "mod.com_dusklight_twilit__essentials.staminaSprint";
    const char* wolf = "mod.com_dusklight_twilit__essentials.staminaWolfSprint";
    const char* bullet = "mod.com_dusklight_twilit__essentials.bulletTimeEnabled";
    const char* speedDrain = "mod.com_dusklight_twilit__essentials.staminaSprintDrainBySpeed";
    for (int mask = 0; mask < 8; ++mask) {
        vars.clear();set(teMod,true);set(sprint,mask & 1);set(wolf,mask & 2);set(bullet,mask & 4);
        assert(te_fixture::twilit_sprint_enabled(false) == bool(mask & 1));
        assert(te_fixture::twilit_sprint_enabled(true) == bool(mask & 2));
        assert(te_fixture::twilit_bullet_time_enabled() == bool(mask & 4));
    }
    vars.clear();assert(!te_fixture::twilit_sprint_enabled(false));
    set(teMod,true);set(sprint,true);assert(te_fixture::twilit_sprint_enabled(false));
    set(sprint,false);assert(!te_fixture::twilit_sprint_enabled(false));
    set(sprint,true);te_fixture::active=false;
    assert(te_fixture::twilit_sprint_enabled(false)); // Sprint also works with TE stamina off.
    set(bullet,true);assert(te_fixture::twilit_bullet_time_enabled()); // No stamina dependency.
    set(teMod,false);assert(!te_fixture::twilit_bullet_time_enabled());
    set(teMod,false);assert(!te_fixture::twilit_sprint_enabled(false));
    vars.erase(teMod);assert(!te_fixture::twilit_sprint_enabled(false));
    set(teMod,true);set(wolf,true);assert(te_fixture::twilit_sprint_enabled(true));
    set(teMod,false);assert(!te_fixture::twilit_sprint_enabled(true));
    set(teMod,true);
    te_fixture::active=true;
    assert(te_fixture::twilit_sprint_drain_multiplier(31,20)==1);
    set(speedDrain,true);
    assert(te_fixture::twilit_sprint_drain_multiplier(31,20)==1);
    assert(te_fixture::twilit_sprint_drain_multiplier(0,20)==.25f);
    assert(te_fixture::twilit_sprint_drain_multiplier(100,20)==2);
    assert(te_fixture::twilit_sprint_drain_multiplier(100,0)==1);
    vars.clear();
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
        const char* notice=z_slot_provider_notice();
        assert((notice!=nullptr)==conflict);
        if(notice)assert(std::string(notice)==((mask&1)&&(mask&2)?
            "Z Items: Twilight HD HUD.":"Z Items: Twilit Essentials."));
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
    set(te,true);set(teSlot,true);
    assert(std::string(z_slot_provider_notice())=="Z Items: Twilit Essentials.");
    vars.clear();
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
            const int previousKh2 = kh2Updates;
            assert(mod_update(nullptr) == MOD_OK);
            assert(selected == !enabled && touchInstalled && installs == 1);
            assert(mod_update(nullptr) == MOD_OK && installs == 1);
            assert(kh2Updates == previousKh2 + 2);
        }
    }
    // Hook errors propagate through mod_update without falsely marking success.
    s_itemSlotHooksInstalled = false; failInstall = true;
    int previous = otherUpdates, previousKh2 = kh2Updates;
    assert(mod_update(nullptr) == MOD_ERROR && !s_itemSlotHooksInstalled);
    assert(otherUpdates == previous && kh2Updates == previousKh2);
}
'''
fixture = fixture.replace('// GUARD', guard).replace('// UPDATE', update).replace('// TE FEATURES', te_features)
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
