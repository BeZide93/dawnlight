# Hero's Shade arena encounter

The Boss Rush side arena (`D_DLBR0`, room 51) contains a Master Sword in an
original beveled stone plinth at room coordinates X/Z = 0/0. Its Y position is
resolved against the loaded room collision. Face it in human form and press
**A / Pull** within 230 units to start the encounter. The sword stays in its
plinth: this does not run the story sword-pickup event or replace Link's weapon.
After victory, interact again to replay. Leaving the room discards the fight.

## Combat

Hero's Shade has eight health points. Each successful native lesson counter
removes one point; a missed opportunity changes the phase after 600 simulation
ticks without damaging him. Success gives 45 ticks of recovery, followed by the
next phase. The fight cycles until his health reaches zero, then uses his native
warp-out animation. This is a new boss controller, not the original lesson event.

| Phase | Shade's offensive move | Native counter retained for Link |
| --- | --- | --- |
| Ending Blow | Downward sword thrust and ordinary sword attacks | Knockdown followed by Ending Blow |
| Shield Attack | Shield strike and sword attacks | Shield Attack, then sword follow-up |
| Reflection | Native magic-ball throw | Reflect using Shield Attack |
| Back Slice | Sidestep, roll, slash | Back Slice |
| Helm Splitter | Jumping overhead attack | Shield Attack, Helm Splitter, follow-up |
| Mortal Draw | Drawing slash | Mortal Draw |
| Jump Strike | Charged jumping attack with two doubles | Jump Strike |
| Great Spin | Rotating sword attack with two doubles | Great Spin |

The demonstration animations are adapted into attacks with bounded movement and
sword collision windows. The native teacher normally uses these as demonstrations,
not autonomous boss attacks. Ordinary sword attacks still use the native attack
spheres. Sword and ball attack power is set to 2 instead of tutorial zero damage.
Special attacks wait 40 simulation ticks between attempts, or 24 for Back Slice
(previously 140). This delay also counts down during native movement/attacks,
but specials still wait for the ready stance so counter follow-ups can complete.
Doubles are staggered by 15 ticks per double. Interrupting a special grants a
60-tick special-attack delay. Ordinary sword-attack startup waits at most 30 ticks
instead of the teacher's randomized 120–210. Native approach movement is 6 units
per tick instead of 2; slash lunges use 8 instead of 5, and jumping attacks use 9
instead of 6 horizontally. Animation playback, damage windows, counter windows
and the 45-tick recovery after a successful counter are unchanged. These settings
apply only to the arena's tracked fighters, not story Hidden Skill lessons.
The ball remains `KN_BULLET`, including the engine's shield-reflection logic and
return trajectory. If the room has no lesson particle bank, an original glowing
orb packet makes the projectile visible without replacing the room's particles.

## Isolation and lifecycle

- Only explicitly tracked encounter process IDs use the hooks. Normal story
  encounters with the teacher retain their existing behavior.
- All combat animations are in the native `KN_a` archive. Creation uses the
  lesson-7 resource pattern; `mType` changes only during combat execution and is
  restored to 6 before drawing/deletion. Resource unload matches resource load.
- During native reset, a temporary double index suppresses the automatic Golden
  Wolf spawn; the no-path parameter remains `0xff`. Afterwards the intended
  main/double identity and visibility are restored.
- `evtProc`/`evtOrder` are intercepted for owned fighters. Native success events
  become boss damage; tutorial dialogue, lesson cutscenes, rewards, story event
  bits and training scene changes never run.
- Creation uses `create_standalone_actor` in `enemy_spawner.cpp`: the shared,
  previously verified player-layer ownership and per-process room-state isolation
  also apply to Shade and native child projectiles. See
  [the spawner fix](darknut-arena-spawner.md).
- Companions and balls are removed on phase changes, victory or departure.
  Pending creation requests are cancelled, and normal scene deletion owns all
  actors. Unloading the mod drains only this encounter's deletion tags before
  removing its callbacks.
- No boss-counter/save-format change, forced teleport, gate lock or new reward.
  The Cave/arena exit routes remain available during the fight.
- The Master Sword model/animations load from the user's `MstrSword` archive.
  The plinth and fallback orb are original procedural geometry. No game asset
  files or Twilit Essentials code were added to the mod package.

## Verification

Automated checks:

```sh
python tests/heroes_shade_battle_test.py
python tests/enemy_spawner_scene_test.py
python tests/bossrush_cave_routes_test.py
python tests/bossrush_warp_test.py
```

These cover phase success/failure, repeated-hit immunity, timeout/replay,
animation damage windows, scene ownership, routes and existing warp behavior.
A full Linux mod build also verifies compilation, linking and packaging.
These checks cannot substitute for a device playtest of native animation and
collision behavior.

Device checklist (still required):

1. Enter from the Cave, inspect the sword/plinth placement, press A once and
   confirm exactly one visible main Shade appears.
2. Take an unguarded sword and ball hit; block normally; reflect the ball using
   Shield Attack. Confirm the return hit damages Shade and does not open a lesson.
3. Test every counter above, particularly Ending Blow and the head-lock follow-up.
   Check the offensive jump/slash timing and both doubles in the final phases.
4. Win and replay; leave/warp during a projectile or double spawn; re-enter and
   reload a save. Confirm Link stays visible and no enemy survives in the hub.
5. Visit a normal Hidden Skill lesson on a story save; confirm it is unchanged.
