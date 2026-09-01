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
- Optional Bullet Time while aiming the Bow during a manual R jump.
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

## Compatibility

See [COMPATIBILITY.md](COMPATIBILITY.md) for tested Twilit Essentials and
Twilight HD HUD versions, compatible Z-item settings, and details about the
Android-only `Dawnlight Touch UI` option.

## Custom Boss Rush Hub Music

Dawnlight can load custom music for the Garden of Twilight Boss Rush hub. The
music is not included in `dawnlight_mod.dusk`; provide your own Nintendo AST
file named exactly `temp.ast`.

Place `temp.ast` next to `dawnlight_mod.dusk` in the active Dusklight mods
directory:

| OS | File path |
| --- | --- |
| Windows | `%APPDATA%\TwilitRealm\Dusklight\mods\temp.ast` |
| Linux | `~/.local/share/TwilitRealm/Dusklight/mods/temp.ast` |
| macOS | `~/Library/Application Support/TwilitRealm/Dusklight/mods/temp.ast` |
| Android | `<active Dusklight data folder>/mods/temp.ast` |

Restart Dusklight after adding or replacing the file. If `temp.ast` is absent
or cannot be read, Boss Rush remains playable but the custom hub music is
disabled. Only use audio that you have the right to use and distribute.

### Creating the AST file

[Nintendo AST Creator](https://github.com/gheskett/Nintendo-AST-Creator)
converts 16-bit PCM WAV files to Nintendo AST. Prepare the source audio as a
16-bit PCM WAV first; filenames passed to AST Creator should contain only ASCII
characters.

Create a looping file with loop boundaries expressed as sample positions:

```powershell
ASTCreate.exe music.wav -o temp.ast -s LOOP_START_SAMPLE -e LOOP_END_SAMPLE
```

Replace the two placeholders with the loop start and end samples. Then move
the resulting `temp.ast` to the platform-specific path above.

### Finding loop points

The included [loop_analysis.py](loop_analysis.py) helper searches a 16-bit PCM
WAV for musically repeating sections and refines the best candidate to
sample-aligned, click-resistant boundaries. It requires Python 3 and NumPy:

```powershell
python -m pip install numpy
python loop_analysis.py "music.wav"
```

The script prints several musical periods and a `Recommended sample-aligned
boundary`. Use its `start` and `end` sample values with AST Creator's `-s` and
`-e` arguments. To inspect a different period from the reported list, rerun it
with the period length in seconds:

```powershell
python loop_analysis.py "music.wav" --period 123.4
```

The analysis is a starting point: listen across the resulting loop boundary
before settling on the final AST file.

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
`hud_layout_settings.json` into Dawnlight's mod data directory provided by
Dusklight, and `IMPORT HUD` reads the same file from there. Existing exports in
the old `mods` folder are migrated automatically. The copy buttons can seed
Custom from the GameCube, X-Box, Wii-U, or Dawnlight presets.

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
