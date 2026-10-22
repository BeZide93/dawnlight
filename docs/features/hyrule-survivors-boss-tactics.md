# Survivors boss tactics (PR #148)

The combat director applies to run-owned bosses and promoted elites on every
map, including Endless. It changes movement, attack selection, readiness,
projectile patterns and recovery. A subsequent balance change doubles the HP of
all four timed bosses, including final bosses and Endless respawns, after the
existing map/time/difficulty calculation (native cap: 32,767). Stationary guards
and ordinary enemies keep their previous HP; damage is unchanged. Native
animations, effects and collision remain in use. Dormant map guards stay dormant
and awakened guards use single-action versions of the ordinary elite combos.

## Encounter flow and recovery

- Ordinary wave spawning stops while any scheduled boss is alive or still being
  created, or while an awakened stationary elite is alive. Existing enemies stay.
  Dormant guards do not stop waves. Boss summons remain part of their attack;
  Phantom Zant has a separate summon cap. Waves resume after the last active boss
  is credited as defeated. The existing schedule, including Endless, is retained.
- After 300 active ticks (10 seconds) without receiving damage, an active boss
  is relocated within Link's ordinary 600–1,049-unit spawn ring. Any actual HP
  loss restarts this clock. Menus, paused play and dormant guards do not advance it.
  This is deliberately an inactivity guard, not a proof that the actor is stuck:
  it also relocates a healthy boss the player has ignored for ten seconds.
- Relocation tries twelve positions with a walkable path, valid dry arena floor
  and 350-unit spacing from another boss. If none is safe, it retries after one
  active second. It never heals, awards loot, or teleports onto an invalid floor.
- Current/old/home positions, arena anchors and structural children move together.
  Active projectile children retire; unknown pending children are cancelled and
  structural creation guards retry attachments. Thrown swords/orbs reset and the boss enters
  a safe native idle/chase state with one second of outgoing-damage grace.
- Heavy-hit stagger cannot be refreshed for 45 active actor ticks after an
  accepted stagger. Damage continues to work during this interval.
- Sampled horizontal lead normally uses 0.4 seconds, capped at 240 units; jump
  height is not predicted and teleport-sized motion does not become lead.
- Accepted contacts from one boss/body/child share a minimum 0.5-second interval.
  Existing per-attack budgets additionally limit sustained colliders and volleys;
  a blocked/rejected contact does not start the interval. King Bulblin's spin
  retains its explicitly renewable hit budget.
- Major attacks reserve finite slots against other live bosses' major attacks.
  This staggers their starts, without suspending every melee attack or projectile
  already in flight. Two-hit melee sequences end with 0.8 seconds of recovery.

## Main bosses

| Boss | Changes |
|---|---|
| Darknut | Melee readiness timer ×0.7, attack animation speed ×1.2, short unarmored lunge and two-attack sequence. Repeated sword throw retains its 3-second cooldown and uses bounded sampled lead. |
| King Bulblin | Normal swing wind-up ×0.75. Spin preparation includes a short advance, then limited steering while spinning. Existing 6-second spin cooldown and 0.5-second accepted-hit interval remain. |
| Death Sword | Distant chase switches to ranged after 2.5 seconds instead of 4. Two native orbs travel at 65 rather than 50 units/tick; first aims directly and second uses sampled lead. |
| Stallord | Three fireballs separated by nine ticks (0.3 seconds), speed 125 rather than 100. Every third volley forms a wider fan; the others cover the sampled target and two nearby lanes. Aim from the actual mouth position toward Link, including height, before the first flight step. Fan angles use that direction rather than the animated head pose. The volley keeps its sampled target even if Link moves or the boss changes phase; projectiles do not home. Native breath tell and recovery remain. |
| Twilit Bloat | Two electric charges, with a 0.7-second lateral reposition before the second native wind-up and 1.5-second recovery after the pair. Terrain line checks constrain lateral movement. |
| Twilit Carrier | Rider fan precedes a warned, straight talon pass. Rider arrows stop during the pass; braking and catch poses provide its tell. The carrier commits to a sampled line and returns to flight afterward. Waiting for the rider is bounded to three seconds if an arrow cannot spawn. |
| Phantom Zant | Repeating projectile / projectile / summon sequence. Alternating narrow/wide five-shot fans with projectile speed ×1.3. Summon casts allow up to six Shadow Keese or three Shadow Beasts, with at most six of this boss's summoned enemies alive. |
| Darkhammer | Minimum spin 1.5 seconds, extra throw wait 0.6 seconds, post-throw wait capped at 1.5 seconds. Ball launch speed ×1.25; native harmless retrieval stays harmless. The circling ball uses its actual position for damage collision, including spin startup, and can hit at most once every 15 active ticks (0.5 seconds), shared across Link’s colliders and retained across spin restarts. Rejected contacts do not consume the interval; separate Darkhammers have independent clocks. Outbound/return throws retain one hit per throw. |
| Puppet Zelda | Orb/sword/triangle selection 40%/30%/30%. Triangle alternates current and predicted position, then stays fixed after appearing. Idle waits between attacks are halved again (4× native timer rate); attack/charge/recovery animations and clocks retain their existing 2× rate. Light-ball casts alternate a simultaneous three-way fan (center and ±0x900 yaw) with three shots spaced nine active ticks (0.3 seconds) apart. The native center ball retains its return mechanics; two additional electric balls share that cast’s accepted-hit budget, use native light-ball effects/speed/radius, stop on contact or terrain, and expire after four active seconds. Queued shots pause with the boss and clear on death, relocation, restart or exit. |
| Ganondorf | Bounded charge lead and melee readiness ×0.8. Five ghost riders arrive as three plus two, with a 0.8-second delay and offset second flank; a rider wave delays charge entry. Existing 5-second charge and 8-second summon cooldowns remain. |

## Promoted ordinary bosses

| Boss | Changes |
|---|---|
| Lizalfos | Short forward hop/step during attack startup; alternating sequences can add a tail follow-up. At most two attacks per sequence. |
| Helmasaur | Two committed straight charges, re-aiming between them, followed by recovery. Wall impact keeps a readable recovery rather than immediately retrying. |
| Dynalfos | Blade/tail follow-up, attack animation speed ×1.2, bounded late turning. The native recursive tail/knife continuation is disabled. |
| Bow Bulblin | Alternating fan and two quick individual arrows, with projectile speed ×1.15. Each fan/pair shares an attack budget. Ordinary archers retain their single/single/triple normal-run cadence and existing Endless volleys. |
| Bokoblin | Faster gap closing at medium range, two swings, then 0.8-second recovery. Stationary guards get a smaller approach step and no forced second swing. |
| Kargarok | Two warned dives, repositioning toward a different side between them. Attack entry no longer requires the narrow native 300–400-unit distance window. The damaging pass follows a committed line; recovery follows the pair. |
| Stalhound | Sampled pounce direction and an alternating short second pounce; recovery after the sequence. Native alignment follows Link until the pounce tell starts (attack phase 2). Only then is aim locked, and general facing stops overriding it. |
| Stalfos | Doubled model/weapon size, matching body/weapon radii and approach spacing. Readiness runs at 4× native rate (previously 2×); added recovery is 12 ticks / 0.4 seconds (previously 24 / 0.8). Native attack animations alternate, with a forward step and 25% shorter late wind-up. The native exact-frame jump cue is preserved. |
| Shadow Beast | Short advance into a two-arm sequence, with the second attack selecting the other native animation and limited late turning. |

## Melee accuracy follow-up

Death Sword now stops in a 350–500-unit horizontal strike band before native
attack startup. Its old chase could enter the band at speed 100 and then brake
by only 3 each tick; the movement bonus carried it under or past Link before
the swing. Bound the final approach using the current Endless movement factor,
stop at attack entry and retreat if already closer than 350. If Link leaves the
band while braking, resume chase. Active sword animations, native sword hitboxes,
magic and story encounters are unchanged; no positional teleport is used.

Stalhound previously locked aim at phase 2 but still used a direction sampled
before the native alignment wait. Resample once when the visible pounce tell
actually starts, using six ticks of capped horizontal lead. Align both movement
and model heading so the mouth collider follows the same line. Keep that heading
through the tell, flight and landing; a follow-up samples its own target. There
is no airborne homing, and native bite timing/radius, jump impulse and shield
rebound remain intact. General facing still leaves the committed tell alone.

Native chase/braking and Stalhound attack fixtures plus director tests cover
close/far approaches, high entry speed, Endless factors, moving targets during
alignment, fixed flight heading, subsequent pounces and pause behavior. Actual
blade and bite contact frequency still needs device verification.

## Implementation and validation

`survivors_boss_tactics.hpp` contains the damage watchdog and stagger clock;
`survivors_boss_tactics_state.inc` owns actor-ID state and encounter predicates.
`survivors_boss_tactics.inc` implements movement, combo boundaries, queued shots,
contact safety and relocation. Existing boss adapters remain responsible for
native entry/animation setup. State is cleared on actor deletion, missing-child
cleanup, run reset and restart. Skipped Execute calls do not advance post-Execute
tactics or initialize projectiles.

Child-spawn post hooks register ownership first, attack budgets second, and boss
aim/volley setup last. The dispatcher runs equal-priority callbacks in registration
order. Running boss setup before ownership skipped it entirely, leaving Stallord's
fireballs with a horizontal default heading. The Stallord regression now dispatches
the callbacks in the order extracted from production registration, starting with
an untracked child, before checking all three shots across directions and heights.

The deterministic native fixtures cover the inactivity clock, pause/damage reset,
failed geometry retries, spawn suppression/resumption, stagger immunity, accepted
contact timing, major-attack reservations, two-hit/guard boundaries, committed
flight, Bloat's pair, delayed shots, Zelda's selection and summon caps. They also
retain the existing native-roster, projectile ownership, special-attack, hook
registration and process-lifetime regressions. CI runs the new director fixture
with GCC and Clang.

The Stalhound regression also executes the SDK's actual `e_sh_attack` with the
production facing and boss-aim hooks. Previously the ten-tick aim lock returned
a cached heading while general facing kept resetting the body toward live Link;
native phase 1 could never satisfy its yaw-alignment check. The test reproduces
that stall before the fix and verifies preparation, tell, launch, bite and landing
after it, plus ordinary/unowned actor and stagger boundaries.

In-game validation is still required on Windows, Android and Switch: animation
cues and weapon reach, all maps' cliffs/slopes/ceilings, rider-before-dive timing,
teleport/structural-child visuals, simultaneous bosses in Endless, and late-wave
frame cost. No claim of device-tested balance or platform loader success is made
by the native fixture tests. Tune timings only after these checks; preserve the
documented HP budgets, unchanged damage, readable tells, safe relocation and finite summon budgets.

## Stalchild balance follow-up

Run-owned Stalchildren move at twice their previous horizontal locomotion speed,
composed with difficulty and Endless. Native attack readiness and close-range
idle waits take one quarter of their previous duration (round up to a whole tick).
Keep native attack animation/hit windows, random attack selection, emergence,
stagger and vertical movement. Story actors are excluded. Device checks: confirm
larger Stalfos sword contact at the new spacing and Stalchild pursuit near walls.

## Pending Stalhound device diagnosis

The user still reports zero damage while standing still after the aim/spacing
patches. Existing fixtures establish native attack state progression, not contact
with the device's actual animated skeleton and Link cylinders. Treat the issue as
**unresolved** until a device log identifies the failing stage.

Temporary passive geometry/contact diagnostics identified the height mismatch
below. They have now been removed at the user's request, including counters,
per-frame/contact callbacks and the instrumentation-only test/CI entries.
The actual bite-height fix and log-derived geometry regression remain.

## Device-confirmed Stalhound height and Stalfos guard follow-up

The 2026-10-22 15:37:58 device log confirms an enabled Stalhound bite but zero
geometric overlaps and zero contact attempts. One horizontally centered sample
places the bite at Y=405.6 with radius 70, against a cylinder ending at Y=229.1:
106.5 units of vertical separation. Four recorded near passes reproduce positive
gaps of 93–109 units in the regression fixture.

For enlarged Stalhound bosses, retain animated bite X/Z but remove the extra
model-scale contribution to bite height above the root. Reconstruct Y from native
joint 13, so repeated registration cannot compound. Preserve native activation,
parked sentinel, trajectory and damage budgets. Include the native 1.15 base size
in body/bite radii (bite 80.5 at scale 2). This moves the gameplay attack volume
below the oversized animated jaw; it does not retarget Link or force damage.
The recorded near passes now overlap, while lateral/elevated dodge cases remain
misses. Temporary probe v2 has been removed. Native animation and terrain
validation still require a device run.

Stalfos' native `player_way_check` tests Link's facing, not line of sight. Requiring
opposing headings prevented attacks in Survivors' free-movement combat. Owned
bosses replace that prerequisite with a world line-of-sight check while retaining
native range, pitch, readiness and attack selection. Existing doubled spacing and
halved waits remain.

The native automatic guard selector also interrupts active sword swings; its two
random branches use the same mask. Suppress that selector only during the owned
actor's action call, restoring its shield parameter before rendering/collision.
Instead, a frontal sword threat during chase gets one 20% guard roll per 60 active
actor ticks, including failed rolls. Attacks are never interrupted by this roll.
Limit the held guard phase to six ticks; ongoing slashes cannot hold/restart it
forever. Block actual frontal sword/shield/projectile contacts during the visible
raised-shield phases through the existing boss guard pipeline. Startup, release,
rear hits and ordinary attacks remain vulnerable. Clear guard's native global
body invulnerability so this directional rule is effective.

Validation uses actual SDK fight-run, facing, guard and automatic selector code;
it reproduces the original facing veto, attack cancellation and indefinitely held
guard. Log-geometry, real block-response, ownership/lifecycle and hook tests pass.
Debug remains enabled; final device confirmation is pending.

### Independent boss HP difficulty

Boss HP is configurable from x1 to x10 (default x1) for timed bosses and
stationary elites on every map, including Endless. Enemy HP now applies only
to ordinary enemies. The timed-boss fixed x2 budget remains in addition to the
selected boss multiplier; the native 32,767 HP ceiling still applies.
Settings are persisted for the next run and do not mutate current enemy HP.
Rules/native-roster/config/UI and record-migration tests cover the separation.

### Shadow Beast approach and attack alignment

Owned Shadow Beast bosses stop in a world-space melee band before native
movement, including when a generic distance query was inlined. An approach
crossing the 250 x actor-scale ring is clipped at its first intersection before
native ground/wall correction, after difficulty/Endless movement boosts. Native
wind-up and attack windows remain; the director no longer adds nine forward
steps during a swing. If already overlapping, back away up to 18 units per
active tick on a walkable path before starting; a blocked retreat does not stall
combat. Resume chase when Link leaves range during preparation.

Use the actor's actual `E_S2` animation archive (previously `E_s1`) and keep
`mAnm` and slow/fast hit-window metadata aligned for both combo swings. General
facing no longer overwrites native/committed attack steering. Body and hand radii
include the native 1.3 base size. Attack sphere centers/activation remain native;
no automatic damage or target-snapped collider.

Regression tests execute the SDK's chase/fight functions with near/far starts,
several headings, both swings, boosted steps crossing through Link, and scoped
noncombat/terrain cases. Director tests cover call ordering, archive selection,
no attack drift and ownership; collider tests preserve the inactive sentinel.
Device confirmation of actual animated hand contacts remains pending.
