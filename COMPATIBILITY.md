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

## Tested versions

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
- moves the Midna touch action and Midna's head to the Skip button outside
  cutscenes; and
- leaves the normal Skip button behavior intact during cutscenes.

This toggle works independently from Dawnlight's `Z Item Slot` setting so that
another mod can own the third item slot while Dawnlight supplies the compatible
touch controls. Restart Dusklight after changing it.

### Extra touch buttons

**Controls → Touch Buttons** contains independent switches for ZL, D-Pad Up,
Down, Left and Right, all off by default. They require both native Touch Controls
and Dawnlight Touch UI on Android. The switches and layout changes apply live;
changing the parent Dawnlight Touch UI setting still requires a restart.

The **Layout Editor** tab selects one extra button and edits its X/Y position
(0–100% of the safe area available to that button) and size (28–120 dp), with a
scaled preview and per-button reset. Values are saved through ConfigService.
The native touch editor has a fixed-size control array, so Dawnlight keeps its
additional layouts separate and does not extend that array or the host settings
struct. The native editor continues to edit the original controls.

ZL sends the independent left analog trigger used by the game's lock-trigger
checks, including Dawnlight manual shielding; it does not send digital L.
D-Pad buttons send normal logical pad directions. Physical-only input readers in
other mods are not emulated. Additional inputs are merged with native touch
input before the host combines it with controller input. Multiple fingers are
tracked independently; hiding/disabling controls or clearing native touch input
releases the extra buttons. Extras are hidden during game menus, dialogue and
cutscenes. No new APK is needed.

Run `python3 tests/touch_buttons_test.py` for input ownership, pad merging and
layout bounds. Device QA should cover all five buttons, simultaneous ZL + face
buttons and stick movement, disabling a held button, app/menu transitions,
orientation/safe-area changes, persistent layouts after restart, and mod unload.

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
head and action on the touch Skip button.

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
third slot to Dawnlight's Android touch layout and keeps Midna on the touch Skip
button outside cutscenes.

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
