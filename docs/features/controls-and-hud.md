# Controls, touch UI and HUD

[← Dawnlight overview](../../README.md)

## Z item slot

Z item slot: bind a third item to Z, move Midna to the D-Pad prompt, and use
the Z slot from the item wheel. The separate Dawnlight Touch UI option keeps
the touch controls available when another mod provides the third item slot.

Improved item HUD support for extra item slots, including item icons, ammo,
oil, bottle contents, and combine prompts.

Dawnlight automatically skips its Z-slot hooks when Twilight HD HUD Z Items or
Twilit Essentials Custom Z Button is enabled. Your saved Dawnlight toggle stays
unchanged, and Dawnlight Touch UI remains independent. Restart Dusklight after
changing which mod provides the slot. See the
[compatibility guide](../../COMPATIBILITY.md) for configurations with
Twilit Essentials and Twilight HD HUD.

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
a full meter unless **Free Transform** is enabled; tapping again ends it early. All feature toggles, progression
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

The mod panel provides **Import Settings** and **Export Settings** directly below
**Open Dawnlight Settings**. Both use Dusklight's native file dialogs. The
`dawnlight_settings.json` export contains all registered Dawnlight settings,
including gameplay options, model selections, HUD settings, touch buttons and
their saved editor layouts. It contains configuration values, not model assets
or game save data. Restart the game after importing to apply startup-only options.
The importer validates the entire file and known setting types before applying
values. Missing settings keep their current values; unknown settings are ignored
for compatibility between versions. Full-settings files and HUD-only files are
separate formats; use their corresponding import buttons.

The Custom layout can move and scale supported HUD elements and can adjust item,
text, ammo, and button-backing offsets on the HUD buttons. `EXPORT HUD` opens
Dusklight's native export dialog to save `hud_layout_settings.json` to a destination
you choose. `IMPORT HUD` opens a JSON file picker and applies the selected layout,
including files selected through Android's document picker. Canceling either
dialog leaves the current HUD settings unchanged. Existing JSON exports remain
compatible; imports are limited to 1 MiB. The copy buttons can seed
Custom from the GameCube, X-Box, Wii-U, or Dawnlight presets.

**Custom Epona Spurs** moves the complete horse spur display with X/Y offsets
and scales it around the first spur, including spacing, used spurs, refill/use
flashes, and its A-button prompt. Select **Custom** to edit it. Values are saved
and included in HUD import/export and preset copy/reset; every preset starts
at the existing position and 100% scale. Spur counts and refill timing remain
controlled by the game.

**Custom Tears of Light** provides X/Y position and scale for Wolf Link's
Vessel of Light and its tears. Select the **Custom** layout to edit it. These
values are included in HUD save, preset copy/reset, and import/export; all
presets default to the existing position at 100% scale. The game still controls
when the vessel appears and how collection animations run.

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
