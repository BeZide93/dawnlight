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
been accepted. The camera eases toward him with letterboxing; Link's native CoWarp material
and arrival particles materialize Shade from the ground upward. Two original captions refer to his
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

Message startup is asynchronous: `fopMsgM_messageSetDemo` accepts a request via
`setMessageIndexDemo(..., false)` while native status is still 1 (idle). The
controller must observe that specific message opening before treating a return
to status 1 as completion. Previously it released the cinematic camera on the
still-pending title, allowing the banner to appear after the cutscene. The
regression fixture now covers delayed opening, visible hold and fade-out for
both dialogue and title. Cancellation of an owned pending request schedules
native status 19 (`deleteProc`) as well as the kill flag, clearing its queued
text before gameplay resumes; replacement messages are never touched.

After the last successful counter, doubles and projectiles are removed. Shade
finishes any airborne fall and uses his native get-up animation when needed.
He praises Link as a true hero and asks him to carry their legacy into the world.
Link's CoWarp dissolve removes him from the top downward, with the native
departure particles and a brief afterglow;
only then is the actor deleted and gameplay restored. The captions have German
and English variants (English fallback for the other supported languages).
Each page automatically advances after 120 simulation ticks, and the controller
waits for the actual native message to finish instead of cutting it off at an
assumed time. Appearance takes 145 ticks (45-tick CoWarp arrival hold plus
100-tick material reveal); departure takes 100 ticks plus a 30-tick afterglow. Recovery and dialogue have bounded failure timeouts.

The accepted intro starts Darknut's native cinematic stream (`0x2000037`).
When the introduction ends (including the boss-name banner), the encounter switches
to `Z2BGM_TN_MBOSS`, the Temple of Time Darknut battle music, on the native
sub-sequence channel. It continues through all four intermissions. Starting the
victory scene stops that sub-sequence and immediately hands back to the room music;
it does not wait for Darknut's usual 510-tick post-battle delay. The underlying
room sequence is preserved, and a replaced room stream is remembered and restored.

`heroes_shade_music.inc` owns only this encounter's music. Death, departure,
unexpected fighter/pedestal deletion and mod shutdown stop its matching cues.
Restoration never restarts an old stream over game-over or destination music.
The cinema fixture exercises intro-to-battle timing, room sequence/stream
restoration, cancellation, replay and foreign-cue ownership. Listen on device
for cue transitions, volume and native stream duration during long dialogue.
All audio comes from the player's game; no music files are bundled.

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
- `heroes_shade_warp.inc` reuses the game's `Always/warp_tex` material setup
  (`loaderBasicBmd(BMWE)` and `setWarpSRT`), CoWarp particles `0x9F3`/`0x9F4`
  and `Z2SE_AL_WARP_OUT`/`Z2SE_AL_WARP_IN_TATE`. Texture scroll is 0.15 per
  simulation tick; the 100-tick dissolve edge is scaled to Shade's height.
  Native skeleton poses and the ghost/opaque body passes are preserved. The old
  squash animation and procedural appearance ribbons are removed.
- Visibility is enforced in the main fighter's `Draw` hook, not just through
  `mNoDraw`: native `daNpc_Kn_c::twilight()` can clear that flag later in execute.
  The intro event-request period submits neither body, so the complete actor
  cannot appear before the warp sound/particles start. The 45-tick arrival hold
  also submits no geometry; the moving warp material then reveals both private
  bodies. Departure's final frame, afterglow and pending deletion stay hidden
  even if the native flag is cleared. Victory recovery, dialogue, combat,
  doubles and unrelated Shade actors keep their normal draw path. The draw hook
  is removed with the other encounter hooks on shutdown.
- Warp models have separate materials, texture matrices, display lists and raw
  archive bytes. A fresh `/res/Object/KN_a.arc` is read into a dedicated heap:
  re-parsing the already loaded archive would endian-swap live vertex data again.
  The BMWE material-capacity and joint-count checks precede use. Link's player
  procedure, model and teleport destination are never changed. No game asset is
  bundled, and shared KN_a material data is never edited.
- Models are prepared once on first appearance and reused for replay. The heap
  reserves up to 8 MiB during initialization, then shrinks to actual usage.
  Particles use event movement so they remain visible during the cinematic;
  cutscene cancellation clears them. Fighter deletion stops the effect, while
  pedestal deletion and mod shutdown unlink the private archive and free its heap.
  Missing/incompatible resources log a warning once and cannot strand the
  encounter. This fallback only shows particles during arrival and reveals the
  original body when the arrival shot ends; it does **not** provide a progressive
  dissolve. Departure hides the fallback body halfway through the effect.
- `python tests/heroes_shade_warp_test.py` compiles the actual runtime against
  instrumented APIs: fresh archive loading, distinct model data, pose transfer,
  unchanged source materials, bidirectional reveal timing, native particle/sound
  IDs, repeat draws without simulation, replay reuse and failure/cleanup paths.
  It also runs the actual draw hook after deliberately clearing `mNoDraw`,
  covering event request, arrival hold/reveal, departure, afterglow, deletion,
  resource failure and isolation from doubles/story actors. These stubs verify
  draw submission and timing; they cannot validate the game's rendered shader.
  The cinematic test covers the longer shots and cancellation at every shot.


Run `python tests/heroes_shade_cinema_test.py` for the real controller's event,
caption, warp, pause, timeout and cleanup paths against native API stubs. The
existing native-hook fixture also verifies that scene swords cannot register
hits and scene events cannot consume boss health. On-device checks still needed:
first arrival (no one-frame pop), caption readability, victory after both fall
directions, camera near arena walls, replay, and leaving/unloading during a scene.

## Intermissions at 8 / 6 / 4 / 2 HP

The fight starts at **10 HP**. After each of the first four pairs of accepted
hits, one intermission begins. The energy ward, Beamos eyes and wind preserve normal
movement, sword attacks and combos; only the fire intermission pauses the combat controller. Repeated
success events cannot consume health during it. Resolving the intermission
costs no HP; after the wind, two further successful counters are needed for the
existing victory scene. Missed-skill timeouts do not trigger intermissions.

| HP remaining | Intermission | Resolution |
| --- | --- | --- |
| 8 | Blue energy ward (Shade keeps attacking) | Bomb explosion or Ball and Chain; other weapons are ignored. The shell expands and fades for 12 ticks when broken. |
| 6 | Fyrus fire wave | Five-second warning through a sword gesture and orange charge rings, then six seconds of outward travel. Reach a wall Clawshot target and hang above the fire. |
| 4 | Two wall-mounted Beamos heads/eyes | Each tracks Link with a laser after a two-second charge. One arrow removes each eye and its beam. |
| 2 | Outward wind | One second of visible wind without force, then a three-second smooth buildup. Wind continues until Link faces the central Master Sword and presses A within reach. Iron Boots prevent the applied force. |

Four native `Obj_HsTarget` actors use the `L7HsMato` variant and its original
hookable DZB. Each target's largest hookable face is rotated to face the arena
with a horizontal normal, using the actual wall normal rather than a guessed
model pitch. The same matrix is applied to the model and MoveBG collision.
This selects Link's native wall-wait path instead of the ceiling-hang path that
immediately detached near the wall. Targets sit 90 units in front of the wall
for hanging clearance; native release and subsequent Clawshot use remain available.
Wall rays position them on diagonal walls, normally 650 units above the arena
floor. Rays may shift within their wall sector or use height 550 to avoid a
corner/opening. Each target is placed independently and pending actor IDs are
retained: a missing wall or eye location cannot suppress the other targets or
create duplicate requests. The eyes use side walls at height 400. The sword
prompt waits until target creation, eye placement and effect loading complete. Targets remain until arena
teardown, so finishing the fire phase cannot delete the surface Link hangs on.
Creation/deletion uses the same player-layer ownership and pending-request
cancellation as the encounter and enemy-spawner fixes.

`heroes_shade_trials.hpp` holds health-independent intermission timing;
`heroes_shade_trials.inc` implements the private arena runtime included by
`heroes_shade_encounter.cpp`. The pedestal owns all models, archives, draw
packets and collision volumes. Only the main Shade execute hook advances time.
Doubles/projectiles are removed on entry. During the ward, eyes and wind, the normal
offensive controller and sword colliders remain active. A post-`setCollision` hook
disarms Shade's body damage target during every trial, preventing native hit
reactions from stranding the offensive controller in a lesson state. The separate
ward sphere accepts the breaking weapons and follows Shade's final position after
movement. Fire suspends ordinary combat until completion. Pause/menu and
unrelated events disarm volumes without advancing or resetting progress.
Death, warps, deletion and mod shutdown cancel effects. Intermissions do not
register or open dialogue: visual cues announce attacks without explaining
their solutions. Intro and victory captions remain unchanged.

The fire uses only `E_fm`'s two original attack-effect BMD/BCK/BTK resources and
the second effect's BRK; it does not create Fyrus or run his boss/room scripts.
Horizontal scale covers the farthest wall, including an off-center caster.
Both native motion clips are sampled across 180 ticks of travel, followed by
28 ticks of visual fade. The collider radius follows the sampled frame; its
height is limited to 300, below a hanging Link. Native fire material and attack
special `0xE` use the engine's `ChkAtNoGuard` path. The warning has no damage.

Beamos eyes use `E_bm6`'s named `bm6_eye` material, fully raised pose and active
eye color, without drawing the tower or installing native Beamos gameplay logic.
Each model has its own draw packets. Eye submission does not change shared
shape visibility and does not assume that the head joint itself owns a mesh. A native Beamos spawned alongside the fight can install archive
joint callbacks that expect a real Beamos actor: the eye-only model suppresses
those callbacks only around its synchronous matrix calculation and immediately
restores them. Lasers use the original `EF_BIMOL6` BMD/BCK and scrolling/startup
BTKs from `E_bm6`, with per-eye animation state. Collision follows the native
startup reach and room-clipped endpoints. Before firing, each eye selects a
floor spot about 600 units away from Link; several wall-clipped candidates are
compared to keep the initial beam path away from him. The spots stay fixed
during the two-second charge, then move toward his floor position at at most
10 units per tick. The two beams alternate their capsule registration so they
cannot hit twice in the same simulation step. A hit disarms both for 90 ticks
(three seconds) and resets their aim to safe floor spots, then tracking resumes.
The beams remain visible during this recovery and Link's invincibility timer
suppresses only collision, never rendering. Rewinding a completed startup BTK
also restores its playback speed: setting only its frame to zero left it stopped
and could make subsequent beams disappear. Eye targets remain vulnerable and
accept arrows only.
Wind rings appear immediately, with no external force during the first 30
simulation ticks (one second). Over the next 90 ticks (three seconds), a smoothstep curve raises
the force from weak to the existing maximum of 55. The active gust has no timeout.
Facing the Master Sword within the existing 230-unit interaction range and pressing
A requests wind completion; the next Shade execute tick stops applying force and
finishes the trial. This interaction consumes A and preserves the existing fighter,
HP, skill progression and lava rim. It cannot spawn another Shade or restart the
intro, and pause, menus, death, wolf form, events and transitions suppress it.
Wind uses Link's native external-force API, retaining wall collision. Both the equipment check and the native heavy-aware
force flag let Iron Boots resist it. Ordinary walking cannot cancel that force.

At the end of the fire phase, an 80-unit-wide molten strip appears along the
arena walls and remains until victory, death, departure or replay. It follows
64 room-wall samples, excluding rays into the exit corridor. Original dark-red
edges and moving orange seams mark its extent. A native fire capsule along the
nearest strip segment deals one heart on contact, with a 45-tick repeat-hit
pause; the height is low enough for hanging Link to remain safe. Only one rim
capsule is registered per tick, avoiding collision-table pressure and duplicate
hits at corners. Intermission completion preserves the rim; full encounter
cancellation removes it, and pause/menus suspend its drawing and collision.

Doubles retain their native formation and defeat animations. The offensive
controller now waits for action 15 (Jump Strike double) or 21 (Great Spin double),
not formation actions 14/20. Once ready, they use the existing sword/sword/special
combos and staggered cooldowns, including normal one-heart sword attacks.

No game asset files or code from Twilit Essentials are added to the mod. The
native resources are loaded from the player's game at runtime; the ward,
telegraph and wind rings are original procedural effects.

Validation: `python tests/heroes_shade_trials_test.py` runs the actual runtime
methods with instrumented collision APIs, checking weapon filters, separate
eye removal, warning/damage boundaries, shared beam endpoints, pause/resume,
cleanup, safe beam startup and repeated recovery, persistent rim contact, and
the wind warning, smooth buildup, restart, Iron Boots, indefinite duration and
stopping force after sword interaction. `python tests/heroes_shade_pedestal_test.py`
executes the actual pedestal method, checking normal startup, range/facing and
state guards, A consumption, no duplicate spawn and the two remaining hits after
wind resolution. The native-hook test verifies active blades during ward/eyes/wind,
fire-only suspension and protected boss health/body targets. The battle test checks
all ten hits, all four thresholds exactly once, ignored hits during trials,
phase timeouts and replay. `python tests/heroes_shade_wall_targets_test.py`
checks the actual alignment method for ceiling, reversed and tilted DZB faces
on all four wall orientations, including matching model/collision transforms.
Existing native-hook and cinematic tests also run.

On-device verification is still required for the four wall attachment points,
Clawshot reach/hanging clearance, Beamos head alignment and visibility, fire
appearance across the room, attacks during ward/eyes/wind, laser recovery, wind
strength and sword interaction under pressure, replay, and
leaving/dying/unloading during each phase. These tests cannot render game
archives or exercise actual Link physics.

### Missing sword / targets regression

The initial intermission patch put both effect archives and the Beamos model
checks inside the pedestal's creation transaction. A failed effect initialization
therefore deleted the entire pedestal before its execute method could create
any wall targets. The Beamos path also incorrectly required exactly six joints
and a mesh attached directly to `BM6_JNT_HEAD`, and requested a differed display
list even when the BMDE resource had no shared list. These assumptions are not
requirements of the native Beamos renderer.

The pedestal now uses its original sword-only heap and becomes visible as soon
as `MstrSword` loads. Effects load later in an independent solid heap owned by
the pedestal, restoring the previous current heap before returning. Success
shrinks the allocation; failure frees it once and logs the failing step instead
of destroying/recreating the sword every frame. Teardown frees the effect heap
before releasing its archives. Target placement runs independently of effect
loading. Combat still waits for the complete arena to prevent invisible hazards.

The eye model follows the native BMDE allocation convention: flags 0 without
a shared display list, shared mode for locked shared data, differed mode only
for unlocked shared data. It looks up `bm6_eye` by name and submits that material's
own joint/shape packet. Callback backup storage follows the actual joint count.

`python tests/heroes_shade_initialization_test.py` exercises these real methods
with no head-mesh API, a seven-joint model, all shared/locked list combinations,
asynchronous resource loading, allocation failure/current-heap restoration,
and partial wall/eye availability without duplicate target actors. The existing
trial, battle, native-hook and cinematic regression tests remain applicable.

## Combat

Hero's Shade has ten health points. Each successful native lesson counter
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
