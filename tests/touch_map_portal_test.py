"""Exercise touch portal input with TPHD's physical-only L mapping and Fixed mode.

Run with python3 tests/touch_map_portal_test.py; no game assets required.
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
#include <cassert>
#include <initializer_list>
using u32 = unsigned int;
constexpr u32 PAD_1 = 0, PAD_TRIGGER_L = 0x40, PAD_TRIGGER_Z = 0x10, A = 0x100;
struct ModContext {};
enum HookAction { HOOK_CONTINUE };
struct PADStatus { u32 button = 0; };
struct Pad { u32 mButtonFlags = 0, mPressedButtonFlags = 0; } pad;
struct mDoCPd_c { static Pad& getCpadInfo(u32) { return pad; } };
namespace mods {
template<class T> T arg(void* args, int index) { return *static_cast<T*>(static_cast<void**>(args)[index]); }
}
bool enabled = true;
int windowStatus = 4;
bool dawnlight_touch_ui_active() { return enabled; }
int dMeter2Info_getWindowStatus() { return windowStatus; }
// STATE
// FUNCTIONS

void touch(bool held, u32 port = PAD_1) {
    PADStatus status{held ? PAD_TRIGGER_L : 0};
    const PADStatus* ptr = &status;
    void* args[]{&port, &ptr};
    before_pad_set_virtual_status(nullptr, args, nullptr, nullptr);
}
void clear_touch(u32 port = PAD_1) {
    void* args[]{&port};
    before_pad_clear_virtual_status(nullptr, args, nullptr, nullptr);
}
void read(bool accepted, bool fixed) {
    pad = {accepted ? PAD_TRIGGER_L : 0, 0};
    observe_map_touch_input(nullptr, nullptr, nullptr, nullptr);
    // TPHD Fixed mode replaces logical L with physical trigger input.
    if (fixed) pad.mButtonFlags &= ~PAD_TRIGGER_L;
}
void map(bool tphd, bool physicalPortal, bool expectedHeld, bool expectedPressed) {
    const auto original = pad;
    if (tphd) {
        // TPHD's before_fmap_move replaces Z, using physical L only.
        pad.mButtonFlags = (pad.mButtonFlags & ~PAD_TRIGGER_Z) |
            (physicalPortal ? PAD_TRIGGER_Z : 0);
        pad.mPressedButtonFlags = (pad.mPressedButtonFlags & ~PAD_TRIGGER_Z) |
            (physicalPortal ? PAD_TRIGGER_Z : 0);
    }
    const auto mapped = pad;
    before_touch_fmap_move(nullptr, nullptr, nullptr, nullptr);
    assert(bool(pad.mButtonFlags & PAD_TRIGGER_Z) == expectedHeld);
    assert(bool(pad.mPressedButtonFlags & PAD_TRIGGER_Z) == expectedPressed);
    // Changes to unrelated buttons during the native map update survive.
    pad.mButtonFlags |= A;
    after_touch_fmap_move(nullptr, nullptr, nullptr, nullptr);
    assert(pad.mButtonFlags == (mapped.mButtonFlags | A));
    assert(pad.mPressedButtonFlags == mapped.mPressedButtonFlags);
    if (tphd) {
        pad.mButtonFlags = (pad.mButtonFlags & ~PAD_TRIGGER_Z) | (original.mButtonFlags & PAD_TRIGGER_Z);
        pad.mPressedButtonFlags = (pad.mPressedButtonFlags & ~PAD_TRIGGER_Z) |
            (original.mPressedButtonFlags & PAD_TRIGGER_Z);
    }
    assert((pad.mButtonFlags & PAD_TRIGGER_Z) == (original.mButtonFlags & PAD_TRIGGER_Z));
}
int main() {
    for (bool tphd : {false, true}) {
        for (bool fixed : {false, true}) {
            touch(true); read(true, fixed);
            map(tphd, false, true, true);
            map(tphd, false, true, false); // Consume each edge once.
            read(true, fixed);
            map(tphd, false, true, false); // Holding cannot toggle repeatedly.
            clear_touch(); read(false, fixed);
            map(tphd, false, false, false);
            touch(true); read(true, fixed);
            map(tphd, true, true, true); // Simultaneous controller and touch.
            clear_touch(); read(false, fixed);
        }
    }
    map(true, true, true, true); // Physical shortcut still works alone.
    touch(true); read(false, true); // Host blocked the input.
    map(true, false, false, false);
    clear_touch(); read(true, false); // Physical GameCube L alone is not touch.
    map(true, false, false, false);
    for (int status : {0, 2, 3, 5, 10}) {
        windowStatus = status;
        touch(true); read(true, true);
        map(false, false, false, false);
    }
    windowStatus = 4;
    enabled = false;
    read(true, true);
    map(true, false, false, false);
    enabled = true;
    clear_touch();
    touch(true, 1); // Other ports cannot activate our shortcut.
    read(true, true);
    map(true, false, false, false);
    touch(true);
    clear_touch(1);
    read(true, true);
    map(true, false, true, true);
    clear_touch(); read(false, false);
    pad = {PAD_TRIGGER_Z, PAD_TRIGGER_Z};
    map(false, false, true, true); // Native Z remains intact without TPHD.
}
'''
start = source.index('bool s_touchMapLRawHeld')
fixture = fixture.replace('// STATE', source[start:source.index('#endif', start)])
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(name) for name in (
    'before_pad_set_virtual_status', 'before_pad_clear_virtual_status',
    'observe_map_touch_input', 'before_touch_fmap_move', 'after_touch_fmap_move',
)))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Touch portals passed: TPHD mapping, Fixed/Follow input, edges, blocking, restoration')
