# Hero's Shade arena encounter

The Boss Rush side arena (`D_DLBR0`, room 51) contains a Master Sword in an
original beveled stone plinth at room coordinates X/Z = 0/0. Its Y position is
resolved against the loaded room collision. Face it in human form and press
**A / Pull** within 230 units to start the encounter. The sword stays in its
plinth: this does not run the story sword-pickup event or replace Link's weapon.
After victory, interact again to replay. Leaving the room discards the fight.

The plinth uses Dawnlight's original warm-charcoal stone texture, bundled in
all builds as an embedded GX RGB565 image with seven mip levels. It no longer
selects, samples or borrows room textures. See [asset provenance and regeneration](../art/pedestal/README.md).
The octagonal cap and ledge use planar UVs; side faces and bevels use tangent UVs
at the same grain scale. Narrow warm-colored inlays and dark recessed-looking
bands follow the actual octagonal edges, with a matching inset outline around
the cap. They are mesh bands, not ornaments stretched over unrelated surfaces.
The central cap remains undecorated around the sword. Pedestal dimensions,
collision and interaction are unchanged.

The immutable image has mod lifetime and 32-byte alignment; each persistent
`PlinthPacket` owns its GX texture object. No archive image, shared material or
actor heap buffer is borrowed or rewritten. The checked-in generated header
keeps Python/Pillow out of platform builds. Regenerate it only after changing
the source PNG with `python tools/generate_pedestal_texture.py`.

## Intro and victory scenes

The sword now starts a short original boss introduction. Shade stays hidden
through asynchronous creation and until the native potential demo event has
been accepted. The camera eases toward him with letterboxing; blue spectral
ribbons precede his native materialization. Two original captions refer to his
unfinished teaching and Link's inherited courage, followed by a sword-ready
pose before combat begins. During the final 30% of the pointing/ready gesture
(`KN_DEMO_KAMAE`, sequence 24 step 0), the native boss-name banner displays
**Hero's Shade** using `MESSAGE_BOX_BOSS_NAME` and the game's original
`zelda_boss_name.blo` screen and fades. The registered title is the same in every
supported language and does not replace any stock boss text. It auto-advances
after 90 ticks; camera/event ownership and combat suspension remain in place
until the message has closed. The reveal uses the body's current frame divided
by the clip's end frame, and starts on the cue tick while the gesture continues.
It no longer waits for the subsequent idle step. The 70% cue gives the native
fade-in a head start during the sword's withdrawal; its exact visual alignment
still needs in-game verification. Stale frames from the preceding clip are
ignored until the ready animation is installed. A 120-tick fallback prevents a stuck animation from
trapping the cutscene. A bounded 300-tick wait handles failed/busy message
creation, and normal cancellation closes only our owned title. Replaying the
encounter shows it again; the victory scene has no boss-name banner.

After the last successful counter, doubles and projectiles are removed. Shade
finishes any airborne fall and uses his native get-up animation when needed.
He praises Link as a true hero and asks him to carry their legacy into the world.
His native warp-out is accompanied by gold light ribbons and a brief afterglow;
only then is the actor deleted and gameplay restored. The captions have German
and English variants (English fallback for the other supported languages).
Each page automatically advances after 120 simulation ticks, and the controller
waits for the actual native message to finish instead of cutting it off at an
assumed time. Appearance takes 54 ticks including anticipation; departure plus
afterglow takes 75. Recovery and dialogue have bounded failure timeouts.

Implementation and lifecycle details:

- `heroes_shade_cinema.hpp` owns the shot sequence; only the main actor's native
  execute hook advances it. Drawing cannot advance dialogue or battle state.
- The scene orders its own potential demo event, then uses the native camera
  and Link's original-demo mode. It never runs the teaching event scripts or
  writes lesson/save flags. Battle phase timers, offensive actions, body targets
  and blade colliders are suspended throughout the scene.
- The camera dolly checks room geometry. Restoration records process IDs for
  the camera and player so room teardown cannot alter replacement instances.
  Stage departure, player death, unexpected actor deletion and mod shutdown
  release acquired control. A failed async spawn can be retried at the sword.
- Captions are registered with MessageService. Cancellation only closes the
  matching owned message; it never deletes the shared `dMsgObject` actor.
- Native `ctrlWarp` state 3 materializes and state 1 disappears. The controller
  never advances departure into state 2, which would teleport/reappear again.
  The original translucent ribbons use their own stable packet in the pedestal
  actor. No shared model material, external mod code or game asset is packaged.

Run `python tests/heroes_shade_cinema_test.py` for the real controller's event,
caption, warp, pause, timeout and cleanup paths against native API stubs. The
existing native-hook fixture also verifies that scene swords cannot register
hits and scene events cannot consume boss health. On-device checks still needed:
first arrival (no one-frame pop), caption readability, victory after both fall
directions, camera near arena walls, replay, and leaving/unloading during a scene.

## Combat

Hero's Shade has eight health points. Each successful native lesson counter
removes one point; a missed opportunity changes the phase after 600 simulation
ticks without damaging him. Success gives 45 ticks of recovery, followed by the
next phase. The fight cycles until his health reaches zero, then starts the
victory scene above. This is a new boss controller, not the original lesson event.

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

### Double reactions and Ending Blow window

Jump Strike uses the native double hit reaction again: there is no execute-hook
instant hide/delete or defeated-slot mask. Native knockback, landing and the
existing disappearance flow run normally. The shared grounded-landing correction
below remains active for doubles, including Jump Strike and Spin Attack falls.

Once the main Shade is flat on the ground with the Ending Blow down flag active,
the native waiting timer advances by eight ticks per update instead of one. This
quarters the previously halved window: one eighth of the original duration, rounded
up to a full simulation tick. Flight,
landing, successful finishing-hit animations and story lessons keep their timing.
Native expiry still waits for an already-started Ending Blow to resolve.

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
  there is no delayed proximity hit. Two stable capsule colliders also trace
  those points between consecutive poses, so fast cuts cannot pass through Link
  between sphere samples. Back Slice activates its first cut pose immediately,
  without tracing the preceding non-damaging roll. Frozen frames, interrupted
  motions and large discontinuities cannot generate swept hits.
  A player hit or shield block consumes the strike. Jump Strike alone can rearm
  once, after both blade points remain clearly separated from all of Link's hurt
  cylinders for two samples. This permits its second blow without repeated hits
  from a blade resting on Link. Normal sword damage is 4 quarter-heart units
  (one heart); specials use 8 (two hearts). Native shield and
  invulnerability handling still decide whether each contact damages Link.
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
- Jump animations already contain vertical motion and forward root travel.
  Do not add a second physical jump: it puts the sword above Link at impact.
  A post-`setAttnPos` hook anchors the backbone's X/Z position to actor movement
  on both visible models while preserving authored height and orientation.
  It recalculates the pose before collision setup, without advancing animation
  or registering another draw entry. Helm Splitter commits to a landing 140
  units beyond Link, where its turning cut faces him; Jump Strike stops 110
  units before him. Targets are captured at takeoff, so Link can evade them.
  On natural completion, Helm Splitter keeps its authored ending pose, then
  transfers the final half-turn into actor yaw and installs native stance 6
  with zero blend before clearing the attack. Position stays at the landing
  point. This avoids one uncorrected frame of the old 594-unit root offset and
  avoids blending that displaced root back into the ready pose. Normal combat
  stance blending resumes on the next tick. If Link is behind this stance,
  native approach turns toward him in place: forward and X/Z speed stay zero
  until the remaining yaw error is at most `0x400` (5.625 degrees). This prevents
  the walking arc produced by turning and moving at the same time. Link already
  in front resumes the normal approach immediately. Real hit reactions, a new
  attack and phase reset clear the turn guard; vertical motion is unaffected.
  Target-relative movement uses `posMove`; a scoped `beforeMove` hook adds
  gravity for that path, since only `posMoveF` normally integrates it. Native
  forward movement still applies its own gravity once.

Successful-counter recovery suppresses attacks for 45 ticks, but does not freeze
an airborne knockback. Forward/backward fall motions 18/14 retain velocity and
gravity. A scoped post-`afterMoved` hook checks actual floor contact after native
background collision, then changes them to their matching landing/lying sequences
19/15 and clears slide velocity. This also applies to Spin Attack knockdowns of
the doubles, with or without global recovery. It never forces an arbitrary model
tilt or height, restarts an existing landing, or changes the main Shade's native
Ending Blow action 3 and its down/finishing flags. Both visible models continue
through the engine's shared motion controller.

Native approach speed remains six units per tick. Ordinary sword swings use
attack power 4 (one heart). All special blade strikes, their swept volumes and
the light ball use power 8 (two hearts) per accepted hit. Native damage modifiers
and invulnerability still apply. Successful counters still grant 45 ticks of
recovery; only the Ending Blow waiting window is accelerated as described above. All tuning applies only to tracked arena fighters, leaving
story Hidden Skill lessons untouched. No shared animation tables are modified.

The ball remains `KN_BULLET`, including the engine's shield-reflection logic and
return trajectory. An accepted reflected-ball success (event 11) also starts
native sequence 29 (`KN_DAMAGE_S` followed by `KN_WAIT_A`) once, providing a short
visible flinch. The health decrease and 45-tick recovery remain unchanged; repeat
events during recovery cannot replay the flinch or charge another hit.
If the room has no lesson particle bank, an original glowing
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
- Swept blade colliders live in the fixed fighter array and share the native
  actor's collision status. Disable them and clear hit references on phase/attack
  changes, event/warp/death pauses and deletion before resetting the fighter.
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
contacts, Back Slice's first cut and non-damaging steps, sweep endpoints,
Jump Strike's two-contact limit, interrupted/recovering actors, and the actual
jump-pose hook preserving height/orientation on both models, the root-neutral
Helm Splitter handoff and turn-in-place recovery (including wrapped yaw),
the eight-times-faster Ending Blow countdown and normal/special collision power, and one flinch per accepted reflected-ball hit. The fixture also checks both knockback directions for main/doubles,
recovery gravity, grounded-only landing transitions, and Ending Blow isolation.

Device checklist (still required):

1. Enter from the Cave, inspect the sword/plinth placement, press A once and
   confirm exactly one visible main Shade appears.
2. Verify one heart from an ordinary sword hit and two from a special/ball hit
   with normal damage settings. Block normally; reflect the ball using
   Shield Attack. Confirm the return hit damages Shade, makes him briefly flinch
   exactly once and does not open a lesson.
3. Have Shade block four distinct sword attacks: the second and fourth should
   trigger a sword riposte followed by Back Slice. A successful Shield Attack or
   other Hidden Skill must instead retain its vulnerable follow-up.
4. Without damaging him, observe the opening combos: two sword attacks then
   Helm Splitter, followed by combos ending in Back Slice and Jump Strike.
   Check Back Slice from several directions, while moving and near a wall;
   he should circle Link and damage on the cut without a delayed follow-up.
   Helm Splitter should pass over Link and connect on its turning cut. Its final
   stance must remain visible, with no position snap when combat resumes.
   Stand behind him at the end of the stance: he must turn in place rather than
   glide in a circle. Repeat in front; normal approach should resume directly. Jump
   Strike should allow damage on both distinct blows when native invulnerability
   permits it. Dodge jumps after takeoff; no remote damage should occur.
5. Test every counter above, particularly Ending Blow and the head-lock follow-up.
   Check ordinary counter knockdowns and Spin Attack knockdowns of both doubles:
   the fall must finish in a flat ground pose rather than a tilted flight pose.
   The Ending Blow waiting pose and its prompt must still work.
   Check attack collisions and both doubles in the final phases. Jump Strike
   must play their native hit/fall/landing response before disappearance.
   Verify the main Shade's Ending Blow opportunity lasts one quarter of the previously halved
   waiting time, while an already-started Ending Blow still completes. During special
   attacks, check unguarded blade contact and a shield block followed by lowering
   the shield: no delayed second hit should appear. Let Mortal Draw run repeatedly
   and replay the encounter to exercise the previously crashing accessory path.
6. Win and replay; leave/warp during a projectile or double spawn; re-enter and
   reload a save. Confirm Link stays visible and no enemy survives in the hub.
7. Visit a normal Hidden Skill lesson on a story save; confirm it is unchanged.
