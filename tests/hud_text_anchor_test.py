"""Exercise copied GameCube text alignment with native and HD HUD bindings."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()
config = (root / 'src/config.cpp').read_text()
layout = (root / 'src/hud_layout.hpp').read_text()


def function(text, signature):
    start = text.index(signature + '(')
    return text[start:text.index('\n}', start) + 2]


def declaration(text, signature, end='\n};'):
    start = text.index(signature)
    return text[start:text.index(end, start) + len(end)]


fixture = r'''
#include <array>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
using u8 = unsigned char;
enum J2DTextBoxHBinding { HBIND_LEFT, HBIND_CENTER, HBIND_RIGHT };
struct J2DTextBox { u8 mFlags = 0; };
struct CPaneMgr { J2DTextBox* pane; };
J2DTextBox* text_box_ptr(CPaneMgr* pane) { return pane ? pane->pane : nullptr; }
enum class HudPaneSlot : std::size_t { TextA, TextB, TextX, TextY, Count };
constexpr std::size_t kHudButtonCount = 5;
// DECLARATIONS
using HudButtonDefaultArray = std::array<HudButtonDefaults, kHudButtonCount>;
// DEFAULTS
std::array<std::array<HudTextBoxFlagState, 5>, 4> s_hudTextBoxFlags;
// FUNCTIONS
int main() {
    J2DTextBox boxes[4][5];
    CPaneMgr managers[4][5];
    CPaneMgr* a[5]; CPaneMgr* b[5]; CPaneMgr* xy[5][3] = {};
    for (int slot = 0; slot < 4; ++slot) for (int layer = 0; layer < 5; ++layer) {
        managers[slot][layer] = {&boxes[slot][layer]};
        if (slot == 0) a[layer] = &managers[slot][layer];
        else if (slot == 1) b[layer] = &managers[slot][layer];
        else xy[layer][slot - 2] = &managers[slot][layer];
    }
    auto apply = [&](bool enabled, int anchor) {
        apply_hud_text_box_group_binding(HudPaneSlot::TextA, a, 5, enabled, anchor);
        apply_hud_text_box_group_binding(HudPaneSlot::TextB, b, 5, enabled, anchor);
        apply_hud_xy_text_box_group_binding(HudPaneSlot::TextX, xy, 0, enabled, anchor);
        apply_hud_xy_text_box_group_binding(HudPaneSlot::TextY, xy, 1, enabled, anchor);
    };
    // Native right binding, centered HD labels, and left-bound replacement HUDs.
    for (auto binding : {HBIND_RIGHT, HBIND_CENTER, HBIND_LEFT}) {
        for (int frame = 0; frame < 300; ++frame) {
            restore_hud_text_box_bindings();
            const u8 flags = 0xA1 | (binding << 2);
            for (auto& group : boxes) for (auto& box : group) box.mFlags = flags;
            // Copying GameCube must not move any of the five label layers.
            for (int slot = 0; slot < 4; ++slot) {
                const auto& preset = kGameCubeHudButtonDefaults[slot];
                assert(preset.textOffsetX == 0 && preset.textOffsetY == 0);
                assert(preset.textScale == 100);
                assert(preset.textAnchor == kHudTextAnchorOriginal);
            }
            apply(true, kGameCubeHudButtonDefaults[0].textAnchor);
            for (auto& group : boxes) for (auto& box : group) assert(box.mFlags == flags);
            // Explicit custom anchors work; Original restores the base binding.
            for (int anchor : {kHudTextAnchorLeft, kHudTextAnchorRight}) {
                apply(true, anchor);
                for (auto& group : boxes) for (auto& box : group) {
                    assert(((box.mFlags >> 2) & 3) ==
                        (anchor == kHudTextAnchorRight ? HBIND_LEFT : HBIND_RIGHT));
                    box.mFlags ^= 1; // unrelated vertical flag changed by another owner
                }
                apply(true, kHudTextAnchorOriginal);
                for (auto& group : boxes) for (auto& box : group) {
                    assert(box.mFlags == (flags ^ 1));
                    box.mFlags = flags;
                }
            }
            // Restore before the next HUD presentation, including live toggles.
            apply(true, kHudTextAnchorRight);
            restore_hud_text_box_bindings();
            for (auto& group : boxes) for (auto& box : group) assert(box.mFlags == flags);
            apply(true, kHudTextAnchorLeft);
            apply(false, kHudTextAnchorLeft);
            for (auto& group : boxes) for (auto& box : group) assert(box.mFlags == flags);
        }
    }
    apply_hud_text_box_binding(HudPaneSlot::TextA, 0, nullptr, true, kHudTextAnchorOriginal);
    apply_hud_text_box_binding(HudPaneSlot::TextA, 99, a[0], true, kHudTextAnchorOriginal);
    assert(std::strcmp(text_anchor_name(0), "Left") == 0);
    assert(std::strcmp(text_anchor_name(1), "Right") == 0);
    assert(std::strcmp(text_anchor_name(2), "Original") == 0);
}
'''
constants = '\n'.join(line for line in layout.splitlines()
                      if line.startswith('constexpr int kHudTextAnchor'))
fixture = fixture.replace('// DECLARATIONS', constants + '\n' +
    declaration(source, 'struct HudTextBoxFlagState {') + '\n' +
    declaration(config, 'struct HudButtonDefaults {'))
fixture = fixture.replace('// DEFAULTS',
    declaration(config, 'constexpr HudButtonDefaultArray kGameCubeHudButtonDefaults', '\n}};') + '\n' +
    next(line for line in config.splitlines() if line.startswith('constexpr const char* kTextAnchorNames')))
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(source, name) for name in [
    'void set_text_box_h_binding', 'J2DTextBoxHBinding hud_text_anchor_binding',
    'void apply_hud_text_box_binding', 'void restore_hud_text_box_bindings',
    'void apply_hud_text_box_group_binding', 'void apply_hud_xy_text_box_group_binding',
]) + '\n' + function(config, 'const char* text_anchor_name'))

# The restored state must reach the real draw hook, and Original must survive
# config reads and JSON import rather than silently clamping to Right.
assert 'restore_hud_text_box_bindings();' in function(source, 'HookAction before_meter_draw_restore_hud')
assert '0, 2)' in function(config, 'int hud_custom_button_text_anchor')
assert 'kTextAnchorNames, std::size(kTextAnchorNames)' in config
assert '"Original"' in declaration((root / 'src/ui.cpp').read_text(),
                                  'constexpr const char* kHudTextAnchorOptions')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('HUD text anchors passed: GameCube copy, native/HD labels, custom anchors and restoration')
