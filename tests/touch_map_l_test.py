"""Exercise the map L override lifecycle without game assets.

Run with python3 tests/touch_map_l_test.py.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()


def function(name):
    start = source.index(name + '(')
    start = source.rfind('\n', 0, start) + 1
    return source[start:source.index('\n}', start) + 2]


fixture = r'''
#include <array>
#include <cassert>
using u32 = unsigned int;
namespace dusk::ui {
enum class Control { L, R, Z };
enum class ControlOverride { Default, Action };
}
using dusk::ui::Control;
using dusk::ui::ControlOverride;
struct ModContext {};
enum HookAction { HOOK_CONTINUE };
struct Args { Control control; ControlOverride requested; };
namespace mods {
template<class T> T arg(void* args, int) { return static_cast<Args*>(args)->control; }
template<class T> T& arg_ref(void* args, int) { return static_cast<Args*>(args)->requested; }
}
bool enabled = true;
int windowStatus = 0, writes = 0;
bool dawnlight_touch_ui_active() { return enabled; }
int dMeter2Info_getWindowStatus() { return windowStatus; }
std::array<ControlOverride, 3> overrides{};
void host_set_override(Control control, ControlOverride value) {
    overrides[static_cast<unsigned>(control)] = value;
    ++writes;
}
struct TouchSetControlOverrideHook {
    static inline decltype(&host_set_override) g_orig = host_set_override;
};
// STATE
// FUNCTIONS

void request(Control control, ControlOverride value) {
    Args args{control, value};
    assert(before_touch_set_control_override(nullptr, &args, nullptr, nullptr) == HOOK_CONTINUE);
    host_set_override(args.control, args.requested);
}
void sync() {
    assert(before_touch_sync_visual_state(nullptr, nullptr, nullptr, nullptr) == HOOK_CONTINUE);
}
ControlOverride l() { return overrides[0]; }

int main() {
    // No writes outside maps, including the item wheel and collection menu.
    for (int status = 0; status <= 10; ++status) {
        if (status == 4 || status == 5) continue;
        windowStatus = status;
        sync();
        assert(writes == 0 && l() == ControlOverride::Default);
    }
    for (int map : {4, 5}) {
        windowStatus = map;
        sync();
        assert(l() == ControlOverride::Action);
        // Host refreshes must not disable L while the map remains open.
        request(Control::L, ControlOverride::Default);
        assert(l() == ControlOverride::Action);
        sync();
        assert(l() == ControlOverride::Action);
        assert(overrides[1] == ControlOverride::Default);
        assert(overrides[2] == ControlOverride::Default);
        windowStatus = 0;
        sync();
        assert(l() == ControlOverride::Default);
        const int afterClose = writes;
        sync();
        assert(writes == afterClose);
    }
    // Preserve the latest L action requested by another menu/mod.
    windowStatus = 4;
    sync();
    request(Control::L, ControlOverride::Action);
    request(Control::R, ControlOverride::Action);
    request(Control::Z, ControlOverride::Action);
    windowStatus = 10;
    sync();
    assert(l() == ControlOverride::Action);
    assert(overrides[1] == ControlOverride::Action);
    assert(overrides[2] == ControlOverride::Action);
    request(Control::L, ControlOverride::Default);
    assert(l() == ControlOverride::Default);
    // Disabling/shutting down during an open map releases our override.
    windowStatus = 5;
    sync();
    enabled = false;
    sync();
    assert(l() == ControlOverride::Default && !s_mapLTouchOverrideActive);
    const int afterDisable = writes;
    sync();
    assert(writes == afterDisable);
    // Partial hook installation must be safe during shutdown.
    TouchSetControlOverrideHook::g_orig = nullptr;
    sync();
}
'''
start = source.index('dusk::ui::ControlOverride s_hostLTouchOverride')
state = source[start:source.index('#endif', start)]
fixture = fixture.replace('// STATE', state)
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(name) for name in (
    'map_l_touch_override_needed', 'sync_map_l_touch_override',
    'before_touch_set_control_override', 'before_touch_sync_visual_state',
)))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Map L touch override passed: both maps, other menus, host overrides, shutdown')
