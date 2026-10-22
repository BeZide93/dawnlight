# Survivors boss tactics (PR #148)

The combat director applies to run-owned bosses and promoted elites on every
map, including Endless. It changes movement, attack selection, readiness,
projectile patterns and recovery; it does not increase damage or HP. Native
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
| Stallord | Three fireballs separated by nine ticks (0.3 seconds), speed 125 rather than 100. Every third volley forms a wider fan; the others cover the sampled target and two nearby lanes. Native breath tell and recovery remain. |
| Twilit Bloat | Two electric charges, with a 0.7-second lateral reposition before the second native wind-up and 1.5-second recovery after the pair. Terrain line checks constrain lateral movement. |
| Twilit Carrier | Rider fan precedes a warned, straight talon pass. Rider arrows stop during the pass; braking and catch poses provide its tell. The carrier commits to a sampled line and returns to flight afterward. Waiting for the rider is bounded to three seconds if an arrow cannot spawn. |
| Phantom Zant | Repeating projectile / projectile / summon sequence. Alternating narrow/wide five-shot fans with projectile speed ×1.3. Summon casts allow up to six Shadow Keese or three Shadow Beasts, with at most six of this boss's summoned enemies alive. |
| Darkhammer | Minimum spin 1.5 seconds, extra throw wait 0.6 seconds, post-throw wait capped at 1.5 seconds. Ball launch speed ×1.25; native harmless retrieval stays harmless. |
| Puppet Zelda | Orb/sword/triangle selection 40%/30%/30%. Triangle alternates current and predicted position, then stays fixed after appearing. Existing 2× attack animation/readiness behavior remains. |
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
| Stalfos | Native attack animations alternate, with a forward step and 25% shorter late wind-up. The native exact-frame jump cue is preserved. |
| Shadow Beast | Short advance into a two-arm sequence, with the second attack selecting the other native animation and limited late turning. |

## Implementation and validation

`survivors_boss_tactics.hpp` contains the damage watchdog and stagger clock;
`survivors_boss_tactics_state.inc` owns actor-ID state and encounter predicates.
`survivors_boss_tactics.inc` implements movement, combo boundaries, queued shots,
contact safety and relocation. Existing boss adapters remain responsible for
native entry/animation setup. State is cleared on actor deletion, missing-child
cleanup, run reset and restart. Skipped Execute calls do not advance post-Execute
tactics or initialize projectiles.

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
unchanged HP/damage, readable tells, safe relocation and finite summon budgets.
