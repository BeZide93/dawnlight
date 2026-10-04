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

Under **Controls → Touch Buttons**, enable **Midna**, **LB**, and each of the four **D-Pad**
directions individually (all default to off). LB supplies the native left-bumper
input for compatible mods, including Twilight HD HUD; the mod decides its action. Both Dusklight Touch Controls and
Dawnlight Touch UI must be enabled. Open **Dusklight's normal Touch Layout
Editor**, or use **Open Dusklight Touch Layout Editor** in Dawnlight's Touch Buttons
window. Both entry points edit the nine vanilla elements and six Dawnlight buttons
together: drag to move, resize with the edge/corner handles, then **Save**.
Existing Dawnlight positions are loaded automatically. **Save** applies both
layouts; **Cancel** discards changes; **Reset** restores both groups' defaults in
the editor until you save. No separate touch overlay is raised over the editor
or its Reset confirmation dialog. Dawnlight keeps its edit state separate from
the native controls and leaves other mods' button events and normal save hooks intact.
The dedicated Midna button shows her icon and calls her when available. Skip is
reserved for cutscenes; enable Midna separately in Touch Buttons.

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

Under **Custom Stamina Bar**, **Fade when full** hides the bar at full stamina;
under **Custom Dark Link Bar**, **Fade when empty** hides it at zero charge.
Both fade out smoothly and reappear when their condition no longer applies.
Both are Off in every built-in preset and after copying/resetting a preset.
Custom exports/imports preserve these two choices; they never change HUD Auto Fade.

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
