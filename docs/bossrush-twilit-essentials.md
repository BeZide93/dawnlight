# Twilit Essentials Boss Rush hub

The hub is now the Temple of Time Darknut chamber (`D_MN06B`, room 51),
using the animated gallery and floating name/location labels from
[BeZide93/dawnlight-twilit-essentials](https://github.com/BeZide93/dawnlight-twilit-essentials/tree/805fac000711135ee2d85e086ab6100d767717bd/src/boss_rush).
Source revision: `805fac000711135ee2d85e086ab6100d767717bd`.

Approach a miniature and press **A / Fight** to replay that encounter. Defeated
boss names turn red. The portal in the center retains Dawnlight's complete-run
confirmation and progression. The physical exit door goes to the Cave of
Ordeals; there is no separate Cave portal.

The oil and red-potion stations, fairy, bomb chest, arrow pot, Deku-seed pot,
custom hub music, and third Midna **Return to Hub** choice remain available.
Dawnlight's existing 18 encounter indices and save format are unchanged; the
full run still includes the native final sequence. This port does not add a
separate Horseback Ganon replay or import Essentials' other game modes/settings.

## Implementation

- `src/boss_rush/` adapts Essentials' model table, animated multipart models,
  projected labels, Ganondorf cape, and resource helpers. No extracted assets
  are added: models and animations come from the user's game archives.
- Teleports use Essentials' scene fade parameters and warp sound on departure
  and arrival. Hub entry uses an explicit restart position and camera reset.
- The Ganondorf replay uses `D_MN09B`, point 1, dungeon switch 1, stable actor
  initialization, native ground-duel state, fixed starting positions, barrier,
  camera release, and battle music. The old manually spawned `D_MN09C` boss and
  camera-92 intro setup are removed.
- Hub-only completion queries suppress the native Darknut. The same room remains
  a real fight when the GameMode state is Run/Replay. Native reward chests are
  removed before spawning Dawnlight's supplies. Hub-only door-bar queries start
  the exit unlocked and keep it unlocked, avoiding the native enemy-clear gate
  cutscene without setting persistent dungeon switches.
- Gallery allocations use a persistent heap. Archive references are acquired
  once, including pending loads, and released after all gallery models. Joint
  callbacks are restored before releasing shared model data. The existing enemy
  spawner guard now targets the new hub.
- GameModeService registration, save/resume, boss progress, rewards and Midna
  flow ownership remain in Dawnlight. The Dusklight host and SDK pin are unchanged.

## Running alongside Twilit Essentials

Use an Essentials build with the Boss Rush session-ownership fix. Older builds
activate their gallery just by entering `D_MN06B`, room 51, and draw a second set
of miniatures and labels over Dawnlight's gallery. Essentials must require its
own active Boss Rush session and must not adopt a session from the current room
when enabled or reloaded. Update both mods and restart the game after replacing
the packages.

## Validation

A Linux RelWithDebInfo build is performed for this change. GitHub Actions checks
all configured target platforms. Compilation does not validate in-game visuals
or timing; these checks require Dusklight and game data:

- Create a Boss Rush save and load an existing hub/run save; verify spawn,
  camera, supplies, all 18 miniatures, animations and floating labels.
- Start and finish a Darknut replay, return through Midna, die/retry, then repeat.
  Verify the room contains no live Darknut when used as the hub, no gate-opening
  cutscene plays on hub load/return, and the exit door is usable immediately.
- Repeat with both updated mods enabled; verify only Dawnlight's 18 miniatures
  and labels appear, its supply chest remains, and each A press starts one fight.
  With Essentials alone, enter its Boss Rush from the menu/portal and verify its
  gallery, Darknut replay and return. A normal Darknut fight must not show a
  gallery, including when enabling/reloading Essentials in that room.
- Start the central full run, save/continue at a boss, and complete its finale.
- Enter the Cave through the door, then use Midna's third option to return.
- Replay Ganondorf twice; verify no horse intro, visible barrier, player control,
  correct music, defeat detection and hub return.
- Repeat boss/hub transitions and return to the title screen while gallery
  archives are loading; check for memory growth or stale models.
