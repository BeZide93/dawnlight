# Compatibility

This document describes Dawnlight compatibility fixes for Dusklight forks and
tested configurations for mods that modify the third item slot, touch controls,
or the gameplay HUD.

## Lazy Tweaks startup compatibility

Lazy Tweaks adds fields to Dusklight's internal `UserSettings` structure.
Dawnlight 3.5.3 read the gyro setting through the upstream structure layout,
which could abort the app during its regular update, including immediately
after installation through the mod browser. This was confirmed in a Windows
crash dump from Lazy Tweaks `v3.1.0-309` (`9bf52f4ca8ba`).

Dawnlight now looks up `game.enableGyroAim` by name through
`dusk::config::GetConfigVar`, without depending on its position in
`UserSettings`. Runtime setting changes remain effective. If the lookup or
setting is unavailable, Dawnlight skips its Bullet Time gyro integration.
This uses the shared `ConfigVar<bool>` ABI of the inspected upstream and fork;
it does not guarantee compatibility with arbitrary changes to that type or
other private host interfaces.

Run `python3 tests/bullet_time_gyro_compat_test.py` for the focused regression
test. In-game verification should cover both app startup with Dawnlight
installed and installation through the mod browser on Lazy Tweaks, followed
by gyro aiming during Bullet Time on upstream Dusklight and Lazy Tweaks.

## Dual Wield Collection option

Enabling Dual Wield adds an Ordon Sword icon after the rightmost visible shield
slot. Selecting it equips Dual Wield; the setting itself only enables the menu
option. The existing equipped shield remains the underlying source of native
blocking behavior. Shield-slot actions leave Dual Wield and retain the other
mod's normal equip/unequip behavior.

The adapter reads the current `getItemTag` mapping and transformed pane bounds
after layout hooks, including the Hylian Shield's reserved position. The icon uses a
transparent separate screen. Selection reuses the actual Collection cursor,
including other mods' artwork and animation, and moves its rendered root after
cursor-update hooks. It does not replace grid entries, selection-pane pointers,
or another mod's menu root. Equipped shield frames are dimmed only during the
menu draw and restored afterward; the sword inherits their equipped tint.
HD HUD's equipment flourishes follow that highlight. Link's status-window
preview has its own equipment update/draw path for the second sword. Native and foreign actions pass through
except when interacting with the added sword. Shoulder-button page changes
release its focus. The icon follows the equipment row when it slides offscreen.

Source review for this adapter used:

| Source | Revision |
| --- | --- |
| Dusklight | `35cdedced6fb77c128be115429907cf0990e1cc7` |
| BeZide93/dawnlight-twilit-essentials (2.1.1) | `b0bc7b1dcb7ad7b1fc8b6472f0d2b8ff45c67654` |
| BeZide93/dawnlight-hd-hud (2.4.4) | `0e3a495e7df93b45afa4f1800e02291b89a4e8f8` |

These are source-inspected versions, **not an in-game compatibility claim**.
The inspected forks do not yet provide a shared Collection extension API;
Twilight HD HUD uses a fixed native-cell layout and hides other menu roots.
Dawnlight's separate icon survives that behavior, but does not make Essentials'
own extra entries compatible with that HUD. When Essentials exposes those
entries in a compatible shield row, Dawnlight follows their rendered positions.
Future layouts must still leave enough space to append the sword.

Run `python3 tests/collection_dual_wield_test.py` for the callback/geometry
regressions and `python3 tests/dual_wield_test.py` for combat regressions.
For a host-symbol check, pass `DUSKLIGHT_APK=/path/to/Dusklight.apk` to the
collection test. Visual/device QA remains necessary: native Collection, each
mod separately, both mods in both load orders, missing shields, extra shield
slots, Essentials page changes, menu scaling, mouse/touch/controller selection,
and normal-save/Boss-Rush/app-restart persistence.

## Previously tested versions

| Mod | Tested version |
| --- | --- |
| Dawnlight | 2.3.4 |
| Twilit Essentials | 1.1.9 |
| Twilight HD HUD | 2.1.1 |

Compatibility may change in later releases when two mods hook the same game or
Dusklight UI functions. After changing any item-slot or Dawnlight Touch UI
setting, fully restart Dusklight before testing the new configuration.

## Dawnlight Touch UI

`Dawnlight Touch UI` is an Android-only compatibility layer. It does not create
or own a third item slot. It:

- displays the active third-slot item on the touch Z button;
- displays supported ammo counts and lantern oil on that button;
- restores native Quick Transform (R+Y) and Sun Song (R+X, wolf form) while
  Dusklight touch controls are enabled, for both touch and controller input;
- allows the touch Z button to assign the selected item from the item wheel;
- keeps touch L available on field and dungeon maps, and routes it to the
  field map's portal action after Twilight HD HUD's physical-L mapping;
- offers a separate Midna button with her icon, enabled under Controls → Touch Buttons; and
- leaves the normal Skip button behavior intact during cutscenes.

This toggle works independently from Dawnlight's `Z Item Slot` setting so that
another mod can own the third item slot while Dawnlight supplies the compatible
touch controls. Restart Dusklight after changing it.

### Extra touch buttons

**Controls → Touch Buttons** contains independent switches for Midna, LB, D-Pad Up,
Down, Left and Right, all off by default. They require both native Touch Controls
and Dawnlight Touch UI on Android. The switches and layout changes apply live;
changing the parent Dawnlight Touch UI setting still requires a restart.

Dusklight's normal **Touch Layout Editor** now edits all nine native elements and
six Dawnlight buttons in one document. Dawnlight's **Open Dusklight Touch Layout
Editor** button is a shortcut to that same editor. Dawnlight owns separate
selection, gesture and layout state for its six buttons. The native element array,
control metadata and working layout are never swapped or repurposed. Drag, edge
resizing, corner scaling and docking follow the pinned editor's geometry.

Events targeting other mods' buttons pass through unchanged. Layout updates run
as a post-hook, and successful saves continue the original hook chain, including
other mods' pre-hooks and replacements. Dawnlight handles only its own buttons,
resize handles and active pointers.

Existing Dawnlight layouts, including old numeric positions, load automatically.
Save persists the native/foreign working layout through the normal editor and
Dawnlight's six layouts through ConfigService. Cancel discards unsaved changes;
Reset restores both sets of defaults in the editor and requires Save to persist.
If another mod vetoes Save without closing the editor, Dawnlight rolls back its
writes too. The native touch overlay stays suppressed throughout editing and
reset confirmation. During mod unload, editor/modal documents are closed and
mod-owned callbacks are removed before their code can unload.

The adapter uses the pinned Android host's private touch ABI. If required editor
methods or hook targets cannot be resolved, its launch button is disabled while
Dawnlight and the extra buttons remain active. Partial editor hook installation is
rolled back. No `end_edit` hook is required. Tests cover a simulated foreign button,
extended metadata, both Save hook orders, veto/error rollback, and native gesture
geometry. Actual interaction with the user's other mod still requires runtime
verification on the supported Dusklight/Lazy Tweaks build.

LB exposes SDL's left-shoulder button (LB/L1) through `SDL_GetGamepadButton` for
the player-one controller and `PADGetNativeButtonPressed(PAD_1)` when no physical
button is reported. Twilight HD HUD uses these two paths for its physical LB
bindings, including Midna in TPHD Fixed Bindings. No fixed Midna action or analog
trigger is injected. Physical inputs and other player ports are preserved.
Mods that insist on a connected device and provide no native-query fallback
still need their own touch support; no synthetic controller is attached.
The old `touch-button-zl-*` config keys remain internal storage for LB, preserving
existing enabled state, position and size. D-Pad buttons send normal logical pad
directions. Additional inputs are merged with native touch
input before the host combines it with controller input. Multiple fingers are
tracked independently; hiding/disabling controls or clearing native touch input
releases the extra buttons. Extras are hidden during game menus, dialogue and
cutscenes. No new APK is needed.

Run `python3 tests/touch_buttons_test.py` for input ownership, pad merging and
layout bounds, and `python3 tests/touch_midna_test.py` for Midna taps, cancellation,
availability and coexistence with native Skip/Z and the D-pad shortcut.
Run `python3 tests/touch_button_editor_test.py` after fetching the
pinned Dusklight source (or set `DUSKLIGHT_DIR`) for the viewport contract, native
native drag/resize/scale, group switching, layout migration, failed-save rollback
and combined Save/Cancel/Reset behavior. Device QA should cover all six buttons, simultaneous LB + face
buttons and stick movement, disabling a held button, app/menu transitions,
orientation/safe-area changes, drag and edge/corner resizing, Save/Cancel/Reset,
both editor entry points, persistent layouts after restart, and mod unload with
the editor or its reset confirmation open.

The portal shortcut uses a touch press edge, so holding L does not repeatedly
toggle portals. It is scoped to field-map input processing and also works
with Twilight HD HUD's Fixed controller bindings, which rebuild logical L/R
from physical triggers. Physical controller shortcuts remain independent.
Run `python3 tests/touch_map_l_test.py` and
`python3 tests/touch_map_portal_test.py` for the focused regression checks.
Run `python3 tests/native_shortcut_compat_test.py` for shortcut eligibility and
HUD restoration checks. Test both native shortcuts on-device with touch alone
and with a controller while both touch settings are enabled. Normal progression
and gameplay restrictions still apply.
Device validation should cover opening the field map, toggling portals with
touch L, releasing/repeating the press, and returning to normal gameplay.

## Twilit Essentials

Tested with Twilit Essentials 1.1.9.

Only one mod should own the Z item implementation. The recommended setup for
using Twilit Essentials' Custom Z Button is:

| Setting | Value |
| --- | --- |
| Dawnlight `Z Item Slot` | Off |
| Dawnlight `Dawnlight Touch UI` | On on Android |
| Twilit Essentials `Custom Z Button` | On |

In this configuration, Twilit Essentials owns the item slot and its gameplay
behavior. Dawnlight supplies the Android touch integration: item-wheel
assignment through touch Z, the item icon and counters on touch Z, and Midna's
head and action on the separate Midna button (enable it under Controls → Touch Buttons).

Alternatively, Dawnlight's `Z Item Slot` can be enabled when Twilit Essentials'
`Custom Z Button` is disabled. Enabling both Z item implementations at the same
time is unsupported because both mods hook the same item-slot, input, and HUD
paths. Other Twilit Essentials features can remain enabled.

## Twilight HD HUD

Tested with Twilight HD HUD 2.1.1.

Twilight HD HUD currently owns its third-item behavior and does not expose a
toggle that disables only that feature. When Twilight HD HUD is active, use:

| Setting | Value |
| --- | --- |
| Dawnlight `Z Item Slot` | Off |
| Twilit Essentials `Custom Z Button` | Off, if Essentials is installed |
| Dawnlight `Dawnlight Touch UI` | On on Android, if Dawnlight touch controls are desired |

This avoids competing third-item implementations. `Dawnlight Touch UI` may
remain enabled because it does not create another slot; it adapts the active
third slot to Dawnlight's Android touch layout. Enable the separate Midna button
under Controls → Touch Buttons to call her; Skip remains a cutscene-only button.

Dawnlight's HUD Layout Editor is compatible with Twilight HD HUD's gameplay HUD
for the supported elements. Twilight HD HUD owns its artwork and base layout;
Dawnlight applies the saved Custom HUD positions and scales afterward. Use the
HUD Editor's copy action to initialize a Custom layout before editing it.

Ammo offsets and scales also compose with Twilight HD HUD: X/Y counts receive
the editor transform after the HD layout, and its third-slot count uses the
Z ammo settings. These adjustments are restored after each draw so repeated
presentation frames and live edits do not accumulate offsets or scaling.

## Arrow Modes and ZR input

Dawnlight 3.1.0 adds `Arrow Modes` under `Gameplay -> Combat`. While enabled,
ZR cycles Normal, Fire, and Triple Shot only during active Bow/Hawkeye aiming.
Dawnlight's R Jump, Sprint, and Manual Shielding do not also activate from that
press. Holding the Bow without aiming keeps the normal ZR actions available.

Arrow Modes do not change the Z item slot settings described above. They work
with Dawnlight's Vanilla, 3rd Person, and Cinema aiming modes; bomb arrows and
the native Hawkeye zoom button keep their normal behavior.

If another mod also assigns an action to ZR while aiming, disable one of the
overlapping features. The `Arrow Modes` toggle applies without restarting and
returns the Bow to normal arrows. Already-fired arrows keep their effects.

## All three mods

When Dawnlight, Twilit Essentials, and Twilight HD HUD are active together,
Twilight HD HUD must be the only owner of the third item slot:

- Dawnlight `Z Item Slot`: Off
- Twilit Essentials `Custom Z Button`: Off
- Dawnlight `Dawnlight Touch UI`: On on Android when its touch layout is wanted

Twilit Essentials features unrelated to its Custom Z Button can remain enabled.

The editor's required symbols have also been checked against the official Android
v2.0.1 APK, build ID `8431032be1f483cf884995a28bddd81556950b09`.
Run `python3 tests/touch_editor_install_test.py` for missing-symbol and hook-failure
rollback tests; set `DUSKLIGHT_APK` to a local APK for the real symbol check.

Run `python3 tests/touch_lb_test.py` for SDL/native LB query behavior without a
controller, physical/touch combinations, other-player isolation, disable/menu
release and controller reconnect. Direct SDL-only mods without a no-controller
fallback are outside this adapter's controller-free compatibility.
