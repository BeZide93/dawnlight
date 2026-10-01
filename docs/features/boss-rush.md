# Boss Rush

[← Dawnlight overview](../../README.md)

## Hub and encounters

Boss Rush Game Mode with the Garden of Twilight hub, individual boss portals,
a complete-run portal, Cave of Ordeals access, hardmode arena hazards, save
and resume support, and a Return to Hub Midna option.

Select **Boss Rush** in Dusklight's game-mode menu, then create or load its
separate save file. **Dawnlight Settings → Boss Rush → Boss Rush** controls
whether the mode is available (default: on). Turning it off while active
returns to the mode selection; save first. Disabling it does not delete saves.
Progression-controlled abilities are available immediately in Boss Rush;
Gale capacity still follows maximum hearts.

### Clear Timer

**Boss Rush → Clear Timer** is off by default. Enable it before starting an
encounter or entering a run portal:

- A bottom-center timer uses the game's native minigame number textures.
- The last successful clear time floats above each boss mirror, the complete-run
  portal, the Cave of Ordeals portal and Hero's Shade's Master Sword. An encounter
  without a recorded time displays `--:--.--`.
- Individual boss replays and Hero's Shade stop at victory. A complete Boss Rush
  records each boss as well as the total, including the horseback leg in the total.
- The Cave timer spans all floors and stops when the native door unlocks after the
  last combat floor (49), before the Great Fairy floor (50).
- Loading, menus, pauses and cutscenes are excluded. Time follows real seconds,
  independent of frame rate. Long runs retain minutes beyond 99.
- Death, a manual return, disabling the timer, or loading another save cancels the
  attempt without replacing previous records. Enabling the option mid-fight does
  not record a partial clear. After loading a saved run, start a new run from its
  portal to obtain a complete-run time.

Records belong to the Boss Rush save slot and use Dusklight's mod save storage.
They are updated on victory and **written to disk when the game is saved**, just
like the other Boss Rush progress. Save before quitting or disabling the mode.
This records the latest clear, not a personal best.

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
