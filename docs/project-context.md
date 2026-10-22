# Dawnlight project context

This handoff preserves useful project context for future coding tasks. It is not
an automatic sync of ChatGPT conversations. Read [AGENTS.md](../AGENTS.md) for
working rules; current source, configuration and live PR state override dated notes.

## Identity and baseline

- Repository: [BeZide93/dawnlight](https://github.com/BeZide93/dawnlight).
- Dawnlight extends Twilight Princess with movement, combat, difficulty, HUD and
  Boss Rush features through the [Dusklight](https://github.com/TwilitRealm/dusklight)
  mod API. This repository is the mod, not the Dusklight host implementation.
- Documentation baseline: 2026-09-28, main commit
  `67f41565d89ebd95081aaef29a8d769dae0d163a`. `mod.json` then declares version 3.7.1
  and stable ID `dev.bezide.dawnlight`; the package is `dawnlight_mod.dusk`.
- `CMakeLists.txt` currently pins Dusklight to
  `35cdedced6fb77c128be115429907cf0990e1cc7`. Read the actual pin before ABI work;
  old conversations mention earlier revisions and presets that are no longer current.
- Primary maintainer test targets include Windows and Android. CI also builds
  Linux, macOS and iOS variants. Do not confuse a source-inspected compatibility
  combination with an in-game-tested one.

## Find the implementation

| Area | Main entry points | Reference |
| --- | --- | --- |
| Initialization and settings | `src/mod.cpp`, `src/config.cpp`, `src/ui.cpp` | [General settings](features/general-settings.md) |
| Mode overrides and save lifecycle | `src/general_modes.hpp`, `src/progression.cpp`, `src/save_state.cpp`, `src/new_save_modes.cpp` | [General settings](features/general-settings.md) |
| Jump, sprint, Gale, Glide | `src/jump_hooks.cpp`, `src/jump_abilities.inc`, `src/gale_counter.cpp`, `src/glider_visual.cpp`, `src/glider_bmd.cpp` | [Movement](features/movement-and-abilities.md) |
| Stamina and combat meters | `src/stamina.cpp`, `src/combat_meter.cpp`, `src/fierce_deity.cpp` | [Stamina](features/stamina.md), [combat](features/combat-and-aiming.md) |
| Aiming, Bullet Time, arrows | `src/aim_hooks.cpp`, `src/bullet_time.cpp`, `src/bow_modes.cpp`, `src/bow_fire_resource.cpp` | [Combat](features/combat-and-aiming.md), [arrows](features/arrow-modes.md) |
| Dual Wield and Collection | `src/dual_wield.cpp`, `src/collection_dual_wield.cpp` | [Dual Wield](features/dual-wield.md) |
| Difficulty and enemies | `src/enemy_hard_mode.cpp`, `src/boss_hard_mode.cpp`, `src/player_hard_mode.cpp`, `src/enemy_scaling.cpp`, `src/enemy_spawner.cpp` | [Hard Mode profiles](../ENEMY_HARD_MODE.md) |
| Boss Rush and Hero's Shade | `src/mod.cpp`, `src/boss_portal_mirrors.cpp`, `src/boss_portal_symbols.cpp`, `src/heroes_shade_encounter.cpp`, `src/heroes_shade_*.inc` | [Boss Rush](features/boss-rush.md), [Shade](heroes-shade-arena.md) |
| HUD, touch and item slots | `src/hud_fade.cpp`, `src/touch_buttons.cpp`, `src/item_slot_hooks.cpp` | [HUD](features/controls-and-hud.md), [compatibility](../COMPATIBILITY.md) |
| Models and eyes | `src/model_overlays.cpp`, `src/eye_movement.cpp` | [HUD/custom models](features/controls-and-hud.md) |
| Packaging and source assets | `.github/workflows/build.yml`, `tools/`, `art/`, `res/`, `src/generated/` | [Glider assets](../art/glider/README.md) |

These are navigation entry points, not isolated modules: check hook registration,
config persistence and cleanup as well as the file implementing the visible action.

## Behavior and design constraints

**Modes and persistence.** Dawnlight Mode and Progression System default to Off.
Mode-controlled effective values must not silently overwrite the user's saved
manual settings. The explicit warning/confirmation path for editing a controlled
setting is different from simply switching the mode Off. Use the current
[General settings guide](features/general-settings.md) for preset values rather
than copying historical chat values.

Progression follows the loaded save's story flags. Boss Rush makes controlled
abilities available without changing those flags; Gale capacity still follows
maximum heart containers. Changing saves must not carry transient meter values,
spent charges or unlock state into another slot.

**Dual Wield.** The feature is integrated in main at this baseline. Enabling the
setting makes a Collection option available; equipping it is a separate action.
The chosen second sword type is global, while equipped state is saved per slot
with separate normal/Boss-Rush choices. Preserve crossed-sword guarding, alternating
blade hit/trail ownership, draw/stow placement and native shield/Hidden Skill
behavior. Hand orientation, body twisting, sword overlap and clipping require
animation QA; old PR #53 feedback is history, not proof of a current defect.

**Boss Rush and Hero's Shade.** The current hub is the Garden of Twilight. Follow
the [hub guide](features/boss-rush.md) and [Shade arena guide](heroes-shade-arena.md),
not older hub experiments. Boss actor lifetime, pedestal initialization, scene
transitions, hook cleanup and cutscene state need explicit attention. Prior crash
reports included starting Shade through the sword pedestal. Reproduce against the
current revision before claiming the old failure still exists or is fixed.

**HUD and compatibility.** Shared hooks must preserve other mods' behavior, including
their menu entries, layout edits, item-slot ownership and hook ordering. Runtime
changes to private host layouts have caused startup crashes. Read
[COMPATIBILITY.md](../COMPATIBILITY.md) for exact revisions, named ConfigVar access,
touch editor ownership and restart requirements. Desktop Linux excludes private
touch UI headers; Linux success alone cannot verify the Android path.

**Glider assets.** The bundled BMD and the emergency generated mesh have different
texture paths. Use [art/glider/README.md](../art/glider/README.md) before changing
textures; editing the old atlas can leave the actual BMD wood/leather unchanged.
[DawnlightCustomGlider](https://github.com/BeZide93/DawnlightCustomGlider) is the
separate replacement-mod example. Keep model, texture and icon changes scoped to
the requested asset and validate which resource the game actually loads.

**Related Ichigo mod.** [dusklight-ichigo-mod](https://github.com/BeZide93/dusklight-ichigo-mod)
owns its own body/face/head/hand overlays and settings. Its eye hook sets both eyes
to an absolute 40% of native cached translation after Dawnlight's adjustment;
do not compound the two reductions. Its hair reduction addressed a reported Dark
Link crash, which should not automatically be attributed to Dawnlight code.

## Dated open work: 2026-09-28

The following PRs were open when this overview was written. They are pointers for
checking live status, not an instruction to implement, merge or rebase them:

| PR | Branch | Scope |
| --- | --- | --- |
| [#70](https://github.com/BeZide93/dawnlight/pull/70) | `test/flurry-direct-hit-damage` | Test direct Flurry damage, native recovery and disabled edge rendering |
| [#36](https://github.com/BeZide93/dawnlight/pull/36) | `air-combos` | Optional Air Combos for manual ZR jumps |
| [#34](https://github.com/BeZide93/dawnlight/pull/34) | `codex/fix-interaction-hud-layout` | Interaction prompt positions with custom HUD layouts |

Recheck PR state, changed files, user test reports and CI before continuing any
branch. Keep active work separate unless the user requests integration. Refresh
affected context when behavior changes; keep detailed feature numbers in the
existing feature guides and avoid copying private chats or binary payloads here.
