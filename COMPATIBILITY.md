# Compatibility

This document describes tested Dawnlight configurations for other Dusklight
mods that modify the third item slot, touch controls, or the gameplay HUD.

## Tested versions

| Mod | Tested version |
| --- | --- |
| Dawnlight | 2.3.4 |
| Twilit Essentials | 1.1.9 |
| Twilight HD HUD | 1.5.1 |

Compatibility may change in later releases when two mods hook the same game or
Dusklight UI functions. After changing any item-slot or Dawnlight Touch UI
setting, fully restart Dusklight before testing the new configuration.

## Dawnlight Touch UI

`Dawnlight Touch UI` is an Android-only compatibility layer. It does not create
or own a third item slot. It:

- displays the active third-slot item on the touch Z button;
- displays supported ammo counts and lantern oil on that button;
- allows the touch Z button to assign the selected item from the item wheel;
- moves the Midna touch action and Midna's head to the Skip button outside
  cutscenes; and
- leaves the normal Skip button behavior intact during cutscenes.

This toggle works independently from Dawnlight's `Z Item Slot` setting so that
another mod can own the third item slot while Dawnlight supplies the compatible
touch controls. Restart Dusklight after changing it.

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

Tested with Twilight HD HUD 1.5.1.

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

## All three mods

When Dawnlight, Twilit Essentials, and Twilight HD HUD are active together,
Twilight HD HUD must be the only owner of the third item slot:

- Dawnlight `Z Item Slot`: Off
- Twilit Essentials `Custom Z Button`: Off
- Dawnlight `Dawnlight Touch UI`: On on Android when its touch layout is wanted

Twilit Essentials features unrelated to its Custom Z Button can remain enabled.
