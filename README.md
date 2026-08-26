# Dawnlight

Dawnlight is a Dusklight mod package that adds optional gameplay, controller,
aiming, boss, and HUD features for Twilight Princess.

> [!IMPORTANT]
> Dawnlight does not include or provide copyrighted game assets. You need a
> legal Dusklight installation and your own dumped copy of Twilight Princess.

## Features

- Z item slot: bind a third item to Z, move Midna to the D-Pad prompt, and use
  the Z slot from the item wheel.
- Improved item HUD support for extra item slots, including item icons, ammo,
  oil, bottle contents, and combine prompts.
- Aim Movement and Aim Mode settings with Vanilla, 3rd Person, and Cinema
  options.
- Touch and gyro aiming support for the modded aiming modes.
- Manual Shielding and R Jump quality-of-life options.
- Intro Skip new-save mode. 
- Boss Rush Game Mode with hub (Garden of Twilight), individual boss portals, Boss Rush run portal and Return to hub Midna option.
- HUD Layout Editor for supported HUD elements, item/text/ammo offsets, button
  backing, round X/Y buttons, and HUD import/export.
- Optional update checks against the Dawnlight GitHub releases.

## Installation

1. Download `dawnlight_mod.dusk` from the Releases page.
2. Move the file into your Dusklight mods directory:

| OS | Path |
| --- | --- |
| Windows | `%APPDATA%\TwilitRealm\Dusklight\mods` |
| Linux | `~/.local/share/TwilitRealm/Dusklight/mods` |
| macOS | `~/Library/Application Support/TwilitRealm/Dusklight/mods` |
| Android | `<active Dusklight data folder>/mods` |

3. Enable Dawnlight in the in-game Mod Manager menu.

On Android, the active data folder is the folder currently selected by
Dusklight. If you changed it with `Change Data Folder`, create or use the
`mods` folder inside that selected location.

## HUD Editing

Open `Mod Manager -> Dawnlight -> Open Dawnlight Settings -> HUD`.

The HUD layout setting has five modes:

- GameCube: vanilla-style HUD placement and backing.
- X-Box: Dawnlight's X-Box-style HUD placement with hidden button backing.
- Wii-U: Wii-U inspired HUD placement with hidden button backing.
- Dawnlight: Dawnlight's compact custom layout with hidden button backing.
- Custom: editable layout, initialized from the X-Box preset.

The Custom layout can move and scale supported HUD elements and can adjust item,
text, ammo, and button-backing offsets on the HUD buttons. `EXPORT HUD` writes
`hud_layout_settings.json` into the `mods` folder, and `IMPORT HUD` reads the
same file from that folder. The copy buttons can seed Custom from the GameCube,
X-Box, Wii-U, or Dawnlight presets.

The `hud_layout_settings.json` format is compatible with the Dawnlight fork's
HUD layout export where the same fields are available.

## LLM Disclaimer

Parts of Dawnlight's source code and documentation were created or modified
with assistance from large language model (LLM) tools. LLM-assisted output can
contain mistakes even after review and testing. Use Dawnlight at your own risk
and report reproducible issues through the project's issue tracker.

## License

Dawnlight is released under CC0 1.0 Universal. See [LICENSE.md](LICENSE.md) for
the full license text.

## Credits

Dawnlight is maintained by BeZide93 and builds on the Dusklight mod API.

Special thanks to the [Dusklight](https://github.com/TwilitRealm/dusklight)
project, the TP decompilation team, the GC/Wii decompilation community, the
Aurora developers, the TP speedrunning community, and all contributors.
