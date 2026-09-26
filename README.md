# Dawnlight

Enhances Twilight Princess with new abilities, combat mechanics, customizable
controls, and a Boss Rush mode. Dawnlight is a mod for
[Dusklight](https://github.com/TwilitRealm/dusklight).

> [!IMPORTANT]
> Dawnlight does not include game assets. You need a legal Dusklight installation
> and your own dumped copy of Twilight Princess.

## Features and guides

| Feature | What it adds |
| --- | --- |
| [Dawnlight Mode & progression](docs/features/general-settings.md) | An optional gameplay preset and abilities unlocked through story progress. |
| [Movement & abilities](docs/features/movement-and-abilities.md) | Sprint, Wolf Sprint, manual jumping, Glide, Revali's Gale and shared stamina. |
| [Combat & aiming](docs/features/combat-and-aiming.md) | Bullet Time, Flurry Rush, Fierce Deity, Great Spin projectiles, manual shielding and aiming modes. |
| [Dual Wield](docs/features/dual-wield.md) | A second Ordon Sword, alternating attacks and a crossed-sword guard. |
| [Arrow Modes](docs/features/arrow-modes.md) | Normal arrows, fire arrows and triple shots, switched while aiming. |
| [Hard Mode](docs/features/hard-mode.md) | Enemy and boss behavior changes, HP/damage scaling and optional invulnerability changes. |
| [Boss Rush](docs/features/boss-rush.md) | Garden of Twilight hub, boss portals, complete runs, save/resume and custom hub music. |
| [Controls & HUD](docs/features/controls-and-hud.md) | Third item slot, Android touch buttons, HUD presets/editor and custom model support. |

Each guide covers the relevant settings, controls and details. Intro Skip and
save compatibility are covered in [General settings](docs/features/general-settings.md).

## Installation

1. Download `dawnlight_mod.dusk` from [Releases](https://github.com/BeZide93/dawnlight/releases).
2. Place it in your Dusklight mods directory:

| OS | Path |
| --- | --- |
| Windows | `%APPDATA%\TwilitRealm\Dusklight\mods` |
| Linux | `~/.local/share/TwilitRealm/Dusklight/mods` |
| macOS | `~/Library/Application Support/TwilitRealm/Dusklight/mods` |
| Android | `<active Dusklight data folder>/mods` |

3. Enable Dawnlight in the in-game **Mod Manager**.
4. Open **Mod Manager → Dawnlight → Open Dawnlight Settings** to configure it.

On Android, use the folder selected in Dusklight's **Change Data Folder** setting.
**Dawnlight Mode** and **Progression System** default to off; enable them under
**General** if desired.

## Compatibility and customization

- [Mod compatibility](COMPATIBILITY.md): supported configurations for Twilit Essentials,
  Twilight HD HUD and Lazy Tweaks, including Z-slot and Android touch settings.
- [Glider texture replacement](art/glider/README.md#replacing-the-texture-without-rebuilding):
  customize the bundled canopy, wood and leather textures.
- [DawnlightCustomGlider](https://github.com/BeZide93/DawnlightCustomGlider):
  example `.dusk` model replacement with editable source files.

## Developer notes

- [Enemy and boss Hard Mode profiles](ENEMY_HARD_MODE.md)
- [Enemy Spawner: scene ownership and adding enemies](docs/darknut-arena-spawner.md)
- [Hero's Shade encounter](docs/heroes-shade-arena.md)

## Credits and license

Dawnlight is maintained by BeZide93 and builds on the Dusklight mod API.

Special thanks to the [Dusklight](https://github.com/TwilitRealm/dusklight)
project, the TP decompilation team, the GC/Wii decompilation community, the
Aurora developers, the TP speedrunning community, and all contributors.

Dawnlight is released under CC0 1.0 Universal. See [LICENSE.md](LICENSE.md) for
the full license text.

Parts of the code and documentation were created or modified with LLM assistance
and may contain mistakes despite review and testing. Use Dawnlight at your own
risk and [report reproducible issues](https://github.com/BeZide93/dawnlight/issues).
