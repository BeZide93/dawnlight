# Dawnlight

Dawnlight is a Dusklight mod package that adds optional gameplay, controller,
aiming, boss, and HUD features for Twilight Princess.

> [!IMPORTANT]
> Dawnlight does not include or provide copyrighted game assets. You need a
> legal Dusklight installation and your own dumped copy of Twilight Princess.

## General settings

The first tab in `Mod Manager -> Dawnlight -> Open Dawnlight Settings` is **General**.
Both new options default to **Off**, preserving existing configurations.

**Dawnlight Mode** applies the intended preset: Sprint at 150%, R Jump at 110%,
Flurry Rush, BOTW Bullet Time, Enemy Hard Mode, Boss Hard Mode, 300% HP Scaling,
Manual Shielding, 60-second Gale recovery, Arrow Modes, Great Spin Projectile,
No Normal-Hit Invulnerability, and the Progression System.
Controlled settings display their effective values and are grayed out.
Your personal Off-mode settings remain saved separately: switching Dawnlight Mode
Off restores them, including your previous Progression System choice. This also
works after restarting Dusklight. Settings outside the preset remain editable.

**Progression System** can also be enabled independently. Unlocks follow the
currently loaded save's story flags:

| Milestone | Unlock |
| --- | --- |
| Start of the game | Sprint |
| Give Talo the Wooden Sword in Ordon | Glide, with Glider selected |
| Free Ordona | Revali's Gale and its counter |
| Free Faron | Fierce Deity |
| Each three full heart containers | One Gale charge: 3 hearts = 1, 6 = 2, 9 = 3, etc. |

Charge capacity uses maximum heart containers, not current health; damage and
partial heart containers do not lower or prematurely increase it. Progression
controls Sprint, Glide, Glide Item, Revali's Gale, Gale Counter, Gale Charges,
and Fierce Deity, locking these settings until progression is disabled. A short
Dusklight toast announces newly reached unlocks and charge-capacity increases.
Loading a save or enabling progression applies existing progress silently;
changing saves does not carry unlocks or spent Gale charges into the next save.
Disabling progression restores the corresponding personal settings.

## Features

- Z item slot: bind a third item to Z, move Midna to the D-Pad prompt, and use
  the Z slot from the item wheel. The separate Dawnlight Touch UI option keeps
  the touch controls available when another mod provides the third item slot.
- Improved item HUD support for extra item slots, including item icons, ammo,
  oil, bottle contents, and combine prompts.
- Aim Movement and Aim Mode settings with Vanilla, 3rd Person, and Cinema
  options.
- Optional [Arrow Modes](#arrow-modes): press ZR while aiming the Bow to cycle
  normal arrows, fire arrows, and a three-arrow spread, with a visual mode indicator.
- Touch and gyro aiming support for the modded aiming modes.
- Bullet Time while aiming the Bow during a manual jump or Gale: **Off** disables
  it, **Always** preserves the previous On behavior, and **BOTW** requires a ground
  clearance of twice the original 100% jump height when activating. The threshold
  is independent of Jump Height/Gale Height. A 100% jump from level ground cannot
  trigger BOTW Bullet Time; jumping off a ledge or using Gale can. A 200% jump
  reaches the threshold near its apex. Once activated, duration, stamina and
  cancellation work as before. Existing On/Off configurations migrate to Always/Off.
- Optional Flurry Rush after a perfectly timed evade and a ranged Great Spin projectile.
- Shared Stamina meter for Bullet Time, Flurry Rush, Sprint, Glide, and the Great Spin
  projectile, including compatibility with Lazy Tweaks stamina actions. Emptying
  the meter causes exhaustion until it recovers to 50%. The Stamina Bar setting
  can disable both the meter and all Dawnlight stamina costs.
- Fierce Deity mode with a charge meter, temporary Magic Armor model swap, and
  doubled sword damage.
- Manual Shielding, R Jump, and a stamina-powered Sprint option with adjustable
  speed (100–300%, default 150%). Sprint animation uses half the actual movement
  speed bonus: 150% movement gives 125% playback, including native indoor slowdown.
  Manual jumps during sprinting carry the earned speed bonus into horizontal
  speed and distance, independently of the selected jump height. Set **Sprint Speed** beside
  **Sprint** in the Controls settings.
- **Jump Height**, directly below **R Jump** in Controls, sets manual jump height
  from 100% (the original height and default) to 500%.
- Optional **Glide**: press ZR in the air to glide during manual jumps or ordinary
  falls. **Glide Item**, directly below the toggle, selects **Cucco** (default) or
  an original textured **Glider** with a wooden frame and leather grips. Both use
  native Cucco glide movement; landing puts the selected item away. The Glider
  hides and silences only its internally summoned Cucco, leaving world Cuccos alone.
  With **Stamina Bar** enabled, either Glide Item consumes 5 stamina per second
  while gliding. Empty stamina ends the glide; exhaustion blocks redeployment
  until stamina recovers to 50%. Disabling Stamina Bar removes the cost.
- **Custom glider texture:** edit a copy of
  [dawnlight-glider.png](art/glider/dawnlight-glider.png), then save it as
  `texture_replacements/tex1_256x256_500b43cdfd40fa52_4.png` inside Dusklight's
  user data folder. Enable **Texture Replacements** in Dusklight and restart
  the game (or toggle that setting off/on to rescan). Preserve the atlas layout:
  the upper 208 rows contain the canopy, the bottom 48 the wood/leather.
  This replaces the texture without rebuilding Dawnlight; it does not change
  the model. The filename matches this build's texture and can change when the
  bundled artwork changes. See [glider texture details](art/glider/README.md).
- Optional **Revali's Gale**: ZR immediately performs a normal jump, including
  while running or sprinting. Keep ZR held through landing to stop and crouch;
  stay crouched for at least one second, then release to launch with the saved
  pre-jump speed/direction and the native Gale
  Boomerang tornado. **Gale Height**, below the toggle, adds 100–1000% of the
  original jump height (default 500%) to **Jump Height**. For example, 200% Jump
  Height plus 500% Gale Height gives 700% total height.
  Releasing before landing or before the full second in crouch cancels Gale
  without spending a charge. Time spent in the initial jump does not count.
  After one second, a flattened Gale tornado loops at Link's feet with wind
  audio until release. Cancelling the charge also stops this readiness cue.
  Gale also enables the initial ZR jump when R Jump is off; Glide is independent.
  Both new abilities default to disabled.
- Gale uses three charges by default. **Gale Charges** adjusts capacity (1–12),
  and **Gale Recovery Time** adjusts seconds per recovered charge (1–3600,
  default 120). Charges recover one at a time; another use never restarts a
  pending recharge. Recovery uses elapsed real time, including menus, cutscenes
  and scene transitions. The state lasts for the session, without save-file data.
- **Gale Counter** toggles only the display: native Epona sprint icons beneath
  the Fierce Deity bar, with full and spent charges aligned at the bars' left
  anchor. **Custom Gale Counter** in
  the HUD Editor adjusts X/Y offsets and scale relative to that default anchor.
  Empty charges block Gale, while the initial normal ZR jump remains available.
- Intro Skip new-save mode, enemy HP scaling, and optional NG+ HP scaling.
- Optional [Enemy Hard Mode](ENEMY_HARD_MODE.md) with shorter combat intervals,
  faster tracking, and selected profile-specific mechanics.
- Boss Rush Game Mode with the Garden of Twilight hub, individual boss portals,
  a complete-run portal, Cave of Ordeals access, hardmode arena hazards, save
  and resume support, and a Return to Hub Midna option.
- HUD Layout Editor for supported HUD elements, item/text/ammo offsets, button
  backing, D-Pad arrows and shadows, single-row hearts, round X/Y buttons, and
  HUD import/export.
- External custom model overlays for Link's outfits, Wolf Link, Sumo Link, the
  horse, items, animations, and shields, plus shield visibility and eye movement
  controls.
- Save compatibility and item integrity repairs.

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

## Arrow Modes

Added in Dawnlight 3.1.0. Open
`Mod Manager -> Dawnlight -> Open Dawnlight Settings -> Gameplay -> Combat`
and use the `Arrow Modes` toggle. It is enabled by default and works independently
of Hard Mode.

While actively aiming the Bow, tap **ZR** to cycle
**Normal -> Fire -> Triple Shot -> Normal**. Merely holding the Bow does not
change modes or reserve ZR. Arrow Modes support Vanilla, 3rd Person, and Cinema
aiming.

| Mode | Arrows consumed per shot | Effect |
| --- | --- | --- |
| Normal | 1 | Standard arrow behavior and damage. |
| Fire | 2 | 50% more damage; ignites lantern-compatible torches, wood, and spiderwebs. |
| Triple Shot | 3 | Three normal arrows in a horizontal spread. |

A brief arrow, flame, or spread icon above Link shows the selected mode; dots
indicate its ammunition cost. In first-person aim the indicator appears near
the center of the screen. If ammunition is insufficient, the indicator turns
red and the shot is blocked.

Fire arrows show the original Bulblin flame while aiming and in flight, including
in rooms without Bulblins. The effect is loaded from your game disc, so no extra
asset files are required. Fire arrows extinguish in water. Bomb arrows keep
their normal behavior, and Hawkeye zoom remains available through the native
item-action button.

Switching `Arrow Modes` off takes effect without restarting, returns the Bow to
normal arrows, and releases ZR for other actions. Arrows already fired retain
their effects. See [COMPATIBILITY.md](COMPATIBILITY.md#arrow-modes-and-zr-input)
for input considerations.

## Hard Mode

Open `Mod Manager -> Dawnlight -> Open Dawnlight Settings -> Hard Mode` to
configure Dawnlight's combat difficulty options. The three Hard Mode switches
are independent and disabled by default:

- `Enemy Hard Mode` makes supported regular enemies more aggressive without
  changing their health or damage. Selected attack, waiting, and recovery
  timers lose five timer points every three frames, shortening those intervals
  by approximately 40%. Supported enemies also track and approach Link more
  quickly, while their animation speed and exact hit, sound, and projectile
  event frames remain unchanged.
- `Boss Hard Mode` enables faster or expanded behavior for Ook, Diababa,
  Dangoro, Fyrus, and Death Sword. It also enables the additional Ganondorf
  arena projectiles in Boss Rush.
- `No Normal-Hit Invulnerability` removes Link's post-hit invulnerability after
  normal damage. Knockdowns, launches, wall impacts, and landing reactions keep
  their normal invulnerability window.

Enemy Hard Mode also adds selected enemy-specific mechanics: every second
Bulblin bow shot becomes a three-arrow spread, every Fire Toadpoli shot and
every second Water Toadpoli shot become three-ball spreads, Chilfos lead thrown
spears, selected Stalhounds can immediately follow up a pounce, and Bokoblins,
Lizalfos, and Dynalfos only suffer knockdown from every second normal combo
finisher.

Enemy HP scaling, NG+ HP scaling, and enemy damage scaling remain separate
settings and can be combined freely with these options. See
[ENEMY_HARD_MODE.md](ENEMY_HARD_MODE.md) for the complete enemy and boss profile
list, exact multipliers, exclusions, and implementation details.

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

## Developer Notes

- [Enemy Spawner: confirmed scene-ownership fix and adding new enemies](docs/darknut-arena-spawner.md)
- [Hero's Shade arena encounter](docs/heroes-shade-arena.md)

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
