# Controls, touch UI and HUD

[← Dawnlight overview](../../README.md)

## Android touch buttons

Under **Controls → Touch Buttons**, enable **Midna**, **LB**, **L3**, **R3**, **Jump**, **Dark Link**, and each of the four **D-Pad**
directions individually (all default to off). LB supplies the native left-bumper
input for compatible mods, including Twilight HD HUD; the mod decides its action. Both Dusklight Touch Controls and
Dawnlight Touch UI must be enabled. Open **Dusklight's normal Touch Layout
Editor** in Dusklight's settings to edit the nine vanilla elements and ten Dawnlight
buttons together: drag to move, resize with the edge/corner handles, then **Save**.
Existing Dawnlight positions are loaded automatically. **Save** applies both
layouts; **Cancel** discards changes; **Reset** restores both groups' defaults in
the editor until you save. No separate touch overlay is raised over the editor
or its Reset confirmation dialog. Dawnlight keeps its edit state separate from
the native controls and leaves other mods' button events and normal save hooks intact.
The dedicated Midna button matches Twilit Essentials' rounded purple button,
centered icon, pressed feedback, and green hint pulse with its chime. A fresh/reset
layout uses TE's 78×46 dp button at 24/72 dp; saved custom positions and sizes stay
intact. Each tap calls Midna once when available, without sending Z, D-Pad or Skip.
Cancelled touches and missed input frames cannot queue a delayed call. Skip stays
reserved for cutscenes; enable Midna separately in Touch Buttons.

**L3/R3** send real left/right stick clicks, including for mods that read SDL
buttons directly. **Jump** (upward arrow above a take-off line) is a separate input for Manual Jump,
Revali's Gale and Glide, regardless of the selected **Jump Button**. Tap to jump;
hold through landing to charge Gale and release when ready, or use it in the air
to glide. **Dark Link** (two red outlined eyes with pupils) toggles the mode without
using the configured activation combo or spin attack. Activation still requires
a full meter; tapping again ends it early. All feature toggles, progression
unlocks, stamina costs and gameplay restrictions continue to apply.
Both action icons use the same bundled Material Symbols Rounded font as the
vanilla utility controls and TE bottle button, with matching thin outlines.
They scale with the button and also appear in the editor.

When Twilit Essentials and its **Quick Access** feature are enabled, the editor
also includes a **Quick Access** button. Move or resize it and **Save** to apply
that layout to Essentials' existing gameplay button. **Cancel** and **Reset**
work the same way as for the Dawnlight buttons; Essentials still controls the
button's action and visibility.

For touch requirements, restart behavior, input integration and tested mod
combinations, see [Dawnlight Touch UI](../../COMPATIBILITY.md#dawnlight-touch-ui).

## HUD editing

Open `Mod Manager -> Dawnlight -> Open Dawnlight Settings -> HUD`.

**HUD Auto Fade**, at the top of the editor, is Off by default and independent
of the layout preset. After Link stands still for 3 seconds, the game HUD fades
out over 1 second. Automatic idle gestures and low-health idle animations in
human and wolf form keep it faded. Movement or actions bring it back in 0.15 seconds. Menus,
dialogue and scene transitions reset the idle timer.

The HUD layout setting has five modes:

- GameCube: vanilla-style HUD placement and backing.
- X-Box: Dawnlight's X-Box-style HUD placement with hidden button backing.
- Wii-U: Wii-U inspired HUD placement with hidden button backing.
- Dawnlight: Dawnlight's compact custom layout with hidden button backing.
- Custom: editable layout, initialized from the X-Box preset.

The Custom layout can move and scale supported HUD elements and can adjust item,
text, ammo, and button-backing offsets on the HUD buttons. `EXPORT HUD` writes
`hud_layout_settings.json` into Dawnlight's mod data directory provided by
Dusklight, and `IMPORT HUD` reads the same file from there. Existing exports in
the old `mods` folder are migrated automatically. The copy buttons can seed
Custom from the GameCube, X-Box, Wii-U, or Dawnlight presets.

The `hud_layout_settings.json` format is compatible with the Dawnlight fork's
HUD layout export where the same fields are available.

**Stamina Bar Auto Fade** is in **Controls → Stamina Settings** and hides the bar
when full. **Gauge Auto Fade** is in **Gameplay → Dark Link Settings** and hides
that bar when empty. Both default to Off, work with every HUD preset, and fade
back in when their condition no longer applies. Existing saved toggle values are
retained. HUD preset copy/reset/export/import no longer changes these gameplay
settings or HUD Auto Fade. When TE owns stamina, its own bar settings take priority.

The editor also supports D-Pad arrows and shadows, single-row hearts, round
X/Y buttons, and [Gale Counter](movement-and-abilities.md#gale-counter) offsets
and scale. See [Twilight HD HUD compatibility](../../COMPATIBILITY.md#twilight-hd-hud)
when using that mod's artwork and base layout.

## Custom models

Dawnlight supports external custom model overlays for Link's outfits, Wolf Link, Sumo Link, the
horse, items, animations, and shields, plus shield visibility and eye movement
controls.

For the Glider's model and texture replacement options, see the
[glider guide](movement-and-abilities.md#glider-model-and-textures).

## Movement bindings

See [movement and abilities](movement-and-abilities.md) for Sprint, Wolf Sprint,
Manual Jump, Disable Auto Jump, Glide and Revali's Gale controls.
