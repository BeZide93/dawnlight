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

The lesson phase determines which counter damages Shade; it no longer restricts
his offensive move selection. Outside the reflection phase, he uses three-hit
combos: two ordinary sword attacks followed by a rotating finisher. The rotation
starts with Helm Splitter, then Back Slice, then Jump Strike, then the current
lesson's offensive move. Both jumping attacks are therefore available even in
the opening phase. Reflection keeps the native ball throw uninterrupted.

| Phase | Native counter retained for Link |
| --- | --- |
| Ending Blow | Knockdown followed by Ending Blow |
| Shield Attack | Shield Attack, then sword follow-up |
| Reflection | Reflect using Shield Attack |
| Back Slice | Back Slice |
| Helm Splitter | Shield Attack, Helm Splitter, follow-up |
| Mortal Draw | Mortal Draw |
| Jump Strike | Jump Strike; two doubles participate |
| Great Spin | Great Spin; two doubles participate |

### Blocks and combos

- Each second **sword attack blocked by Shade** starts a sword riposte followed
  by Back Slice. Counting happens after native hit handling and requires a new
  sword-block animation from a Link sword collision. Damage and successful
  Hidden Skill openings do not count as blocks and retain native follow-ups.
- Ordinary sword animations play at 1.65 times normal speed. Their damage window
  remains animation frames 30–40; both visible models share the playback speed.
- Specials no longer share an arbitrary 40–75% animation window. After native
  `modelCalc`, `setCollisionSword` samples both blade points from joint 13 of
  the visible model (native local X offsets 60/120, radius 30). In the striking
  step, a moving blade enables those native hit volumes at their current pose;
  there is no delayed proximity hit. Idle poses, frozen frames, step/animation
  changes and large discontinuities do not generate a strike. A player hit or
  shield block consumes that attack's contact until the next attack starts.
  Back Slice's sidestep and roll remain non-damaging. Damage power is explicitly
  set when the volumes are registered, and Link's native shield/invulnerability
  handling still decides the result of contact.
- Gaps are six simulation ticks within a combo and 18 between combos. Doubles
  stagger their full-combo delay by 15 ticks per double. The old native random
  single-attack timer is disabled only for owned arena fighters, so it cannot
  interrupt or starve the combo controller.
- Native sequences 25 (sword), 27 (block), 24 (readying) and 31 (the opening
  lesson's hit reaction) retain their sequence number when they return to a
  ready/walking stance in step 1. Those return steps are now
  recognized; requiring sequence 9 alone previously suppressed later attacks.
- Back Slice uses animation progress to follow a semicircle around Link at a
  150-unit radius, with bounded movement through native collision. The cut then
  faces Link and closes to sword reach. Sidestep/roll frames never deal damage.
- Jumps launch once from their actual animation progress, aim at Link's position
  at takeoff, and finish after landing. Movement uses the chosen attack rather
  than the current lesson phase. Link can evade the committed target.
  Target-relative movement uses `posMove`; a scoped `beforeMove` hook adds
  gravity for that path, since only `posMoveF` normally integrates it. Native
  forward movement still applies its own gravity once, and interrupted jumps
  continue falling.

Native approach speed remains six units per tick. Sword and ball attack power
remain 2. Successful counters still grant 45 ticks of recovery; lesson follow-up
windows are unchanged. All tuning applies only to tracked arena fighters, leaving
story Hidden Skill lessons untouched. No shared animation tables are modified.

The ball remains `KN_BULLET`, including the engine's shield-reflection logic and
return trajectory. If the room has no lesson particle bank, an original glowing
orb packet makes the projectile visible without replacing the room's particles.

## Isolation and lifecycle

- Only explicitly tracked encounter process IDs use the hooks. Normal story
  encounters with the teacher retain their existing behavior.
- All combat animations are in the native `KN_a` archive. Creation uses the
  lesson-7 resource pattern; `mType` changes only during combat execution and is
  restored to 6 before drawing/deletion. Resource unload matches resource load.
- The lesson-7 heap has no sheath (`mpPodModel`) or sheath animation calculator.
  For tracked fighters without that prop, `afterSetMotionAnm` skips only the
  accessory update, clears accessory playback flags and returns success. Body
  motion was already installed by `setMotionAnm`. Do not allocate an accessory
  calculator outside actor heap creation merely to satisfy this callback.
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

### Crash diagnosis (2026-09-21)

The Android log `dusklight-20260921-230304.log` reports a null dereference in
`mDoExt_bckAnm::init`, called by `daNpc_Kn_c::afterSetMotionAnm`, `setMotionAnm`
and `ctrlMotion`. Mortal Draw selects a sheath animation whose native update
uses `init(..., modify=true)`. That path dereferences the calculator normally
allocated with `modify=false` only in the lesson-5 (`mType == 4`) heap. The arena
creates lesson 7 and changes combat types at runtime, so that calculator was
never created. The scoped accessory hook above prevents this specific path;
it does not skip body animation or change ordinary story actors.

## Verification

Automated checks:

```sh
python tests/heroes_shade_battle_test.py
python tests/heroes_shade_native_hooks_test.py
python tests/enemy_spawner_scene_test.py
python tests/bossrush_cave_routes_test.py
python tests/bossrush_warp_test.py
```

These cover phase success/failure, repeated-hit immunity, timeout/replay,
every-second-block ripostes, combo rotation (including early jumps), native ready
return steps, pose-based blade activation, bounded Back Slice trajectories across
multiple starting angles, scene ownership, routes and existing warp behavior.
A full Linux mod build also verifies compilation, linking and packaging.
These checks cannot substitute for a device playtest of native animation and
collision behavior.
The native-hook fixture compiles the actual accessory/collision hook functions
against a minimal engine API. It checks absent/present/unowned accessories,
early and late moving blade contact, stationary poses, consumed shield/hit
contacts, Back Slice's non-damaging steps and interrupted/recovering actors.

Device checklist (still required):

1. Enter from the Cave, inspect the sword/plinth placement, press A once and
   confirm exactly one visible main Shade appears.
2. Take an unguarded sword and ball hit; block normally; reflect the ball using
   Shield Attack. Confirm the return hit damages Shade and does not open a lesson.
3. Have Shade block four distinct sword attacks: the second and fourth should
   trigger a sword riposte followed by Back Slice. A successful Shield Attack or
   other Hidden Skill must instead retain its vulnerable follow-up.
4. Without damaging him, observe the opening combos: two sword attacks then
   Helm Splitter, followed by combos ending in Back Slice and Jump Strike.
   Check Back Slice from several directions, while moving and near a wall;
   he should circle Link and turn into the cut. Dodge jumps after takeoff.
5. Test every counter above, particularly Ending Blow and the head-lock follow-up.
   Check attack collisions and both doubles in the final phases. During special
   attacks, check unguarded blade contact and a shield block followed by lowering
   the shield: no delayed second hit should appear. Let Mortal Draw run repeatedly
   and replay the encounter to exercise the previously crashing accessory path.
6. Win and replay; leave/warp during a projectile or double spawn; re-enter and
   reload a save. Confirm Link stays visible and no enemy survives in the hub.
7. Visit a normal Hidden Skill lesson on a story save; confirm it is unchanged.
