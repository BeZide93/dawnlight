# Boss Rush

[← Dawnlight overview](../../README.md)

## Hub and encounters

Boss Rush Game Mode with the Garden of Twilight hub, individual boss portals,
a complete-run portal, Cave of Ordeals access, hardmode arena hazards, save
and resume support, and a Return to Hub Midna option.

Choose the Boss Rush **New Save Mode** in [General settings](general-settings.md)
when initializing an empty save slot. Progression-controlled abilities are
available immediately in Boss Rush; Gale capacity still follows maximum hearts.

The optional Hero's Shade encounter is described in the
[arena guide](../heroes-shade-arena.md). For enemy and boss difficulty options,
see [Hard Mode](hard-mode.md).

## Custom hub music

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

The included [loop_analysis.py](../../loop_analysis.py) helper searches a 16-bit PCM
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
