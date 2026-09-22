# Enemy Hard Mode

This document describes the enemy changes implemented by
[PR #14](https://github.com/BeZide93/dawnlight/pull/14). It documents the
behavior that is actually present in the code, rather than the broader set of
ideas considered while designing the feature.

## Configuration

The `Enemy Hard Mode` toggle is in the **Hard Mode** settings tab and is
disabled by default. It does not change enemy health or damage.

The same tab also contains:

- `Boss Hard Mode`, the renamed `Arena Hazards` option. It controls the boss
  changes documented below as well as the additional hazards in the Ganondorf
  Boss Rush fight.
- `No Normal-Hit Invulnerability`, which removes Link's post-hit invulnerability
  after normal damage while preserving the full window for knockdowns.
- The existing enemy HP and damage scaling settings.

### No Normal-Hit Invulnerability

When this option is enabled, a newly assigned player damage timer is cleared
unless the hit put Link into a large-damage, wall-impact, launch, or landing
knockdown process. Knockdowns retain ownership of their timer until it expires,
including the remaining invulnerability after Link gets back up.

The option is independent from `Enemy Hard Mode` and is disabled by default.

## Boss Hard Mode

The following changes are enabled together by `Boss Hard Mode`. They do not
alter boss health or outgoing damage.

### Ook (`E_MK` / `E_MK_BO`)

- Ook's normal combat boomerang updates its outbound heading toward Link every
  frame. Throws aimed upward at a selected Deku Baba retain their original
  target so they can still knock it from the ceiling.
- Its outbound and return travel speed is increased from `40` to `60` units per
  frame (`1.5x`).
- Bridge, room-4, and other scripted boomerang sequences are excluded.

### Diababa (`B_BQ`)

- Two Water Toadpolis (`E_TK`) spawn once the active fight begins, positioned
  `1200` units to Diababa's left and right and `1000` units forward near the
  front edge of the poison water.
- The post-damage poison-attack interval is reduced from `80` to `53` frames,
  making the attack occur approximately `1.5x` as often.

### Dangoro (`E_GOB`)

- Both melee wind-up timers are halved from `70` to `35` frames.
- The initial roll start-up timer is reduced from `60` to `45` frames.
- Active rolling movement is multiplied by `1.3x` before collision correction.

### Fyrus (`E_FM`)

- Walking, combat-running, and non-knockdown eye-hit recovery movement is
  multiplied by `1.5x` before collision correction.
- The non-knockdown recovery timer after an eye hit is reduced to `75%` of its
  original duration.
- A fire wave is forced after that recovery and after Fyrus completes his
  stand-up sequence from a knockdown.
- Fire-wave expansion animation advances `1.25x` as quickly; its collision
  radius continues to use that same animation frame.

### Death Sword (`E_VT`)

- In the flying visible phase, the first two arrow hits no longer transition
  Death Sword into its ground chase and melee sequence.
- The third arrow hit performs the original transition and resets the counter.
- The randomized wait between ranged attacks in that phase is halved from
  `150-209` to `75-105` frames. Attack animations and projectile event frames
  retain their original speed.

### Ganondorf (`B_GND`)

- The existing Boss Rush behavior remains unchanged: three damaging arena
  projectiles spawn every ten seconds during the direct Ganondorf fight.

## Shared behavior

All supported profiles use a per-actor three-phase cadence clock. On two of
every three normal timer ticks, selected timers receive one additional
decrement. Native logic therefore removes five timer points in three frames,
making the affected intervals approximately 40% shorter.

The initial cadence phase is derived from the actor's process ID. Groups
therefore do not receive their extra timer decrement on the same frame.

For profiles that expose the corresponding native chase values:

- Normal movement/approach acceleration uses a `1.2x` scale.
- Normal turning toward Link uses a `1.25x` scale.
- More agile profiles can override either value with `1.4x`.
- The multiplier is applied only to native chase operations targeting the
  actor's base `speedF`, `current.angle.y`, or `shape_angle.y`.

Hard Mode does not accelerate animation playback. This preserves exact-frame
hit, sound, and projectile events and avoids duplicate attacks. The cadence
also composes with the enemy slow-motion system: Hard Mode is evaluated from
real enemy timer ticks, while slow motion retains its fractional integration
and duplicate-event protections.

### Native death and Wolf knockdown timing

Extra timer decrements stop at zero or negative enemy HP for every profile.
Bulblin (`E_RD`), Lizalfos (`E_DN`) and Dynalfos (`E_MF`) also keep native timers
throughout their shared knockdown/death action (`ACTION_DAMAGE = 21`), including
a living knockdown or Wolf takedown before a final hit. The separate five-point
recovery adjustment remains limited to surviving grounded enemies.

These actors reuse timer slot 0 for getting up and slot 1 for disappearing.
After landing while Link is a wolf, native code initializes them to 80 and 55.
Accelerating only slot 0 made get-up run after about 48 enemy ticks, before the
55-tick death branch. The actor had already begun its death coloration and
lost HP, but returned to combat. Human sword knockdowns normally use roughly
60/35 ticks, which explains why that path was much less prone to the race.
Dynalfos have the same two-timer structure as the two reported enemy types.
Bokoblins use a separate native death action instead of this competing get-up
branch, consistent with them disappearing correctly.

The fix preserves native death, drops, switches, Wolf takedown flags and actor
cleanup. It does not force deletion, restore health or replace enemy actions.
Ordinary living combat still uses the five-timer-points-in-three-ticks cadence.

Run `python3 tests/enemy_hard_mode_death_test.py` to reproduce the old race and
verify the actual pre-execute guard/timer adjustments against terminal
conditions extracted from the pinned native actor sources. Coverage includes
all three affected profiles, each cadence offset, normal and fractional slow
motion, lethal hits during an existing knockdown, human hits, completed Wolf
takedowns, and Hard Mode off. On-device validation should exercise Wolf bites,
lethal pounces, takedowns and normal drops for melee and bow Bulblins, Lizalfos,
Dynalfos and the Bokoblin control case.

## Enemy profiles

### Darknut (`B_TN`)

- Shortens `mTimer3`.
- Also shortens `mTimer1` during high/low attacks and high/low guard states.
- This lets the existing attack, guard, and follow-up logic become available
  sooner.
- Scripted room, opening, transformation, and ending demo states remain
  excluded.
- No new counter or combo transition is forced.
- The profile uses manual steering, so the generic chase-based Hard Mode turn
  and acceleration multipliers are not applied directly.

### Bokoblin (`E_OC`)

- Shortens `field_0x6c0`, `field_0x6c2`, and `field_0x6c4`.
- Reduces native waiting, attack, and reorientation intervals.
- Alternates normal combo finishers per Bokoblin: the first uses the ordinary
  damage reaction and every second finisher retains its native knockdown.
- Reduces the grounded recovery after a surviving knockdown from the native
  `45–55` timer points to the Dynalfos value of `5`.
- Actor-ID cadence offsets stagger groups.
- Does not enforce a two-hit combo or cap the number of active attackers.
- The profile uses manual steering, so it receives no additional generic
  chase-based acceleration or turn multiplier.

### Aeralfos (`B_GG`)

- Shortens `mTimers[0]` and `field_0x65a`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Reduces native flight, attack, and reorientation pauses.
- Does not separately rewrite Clawshot or stagger vulnerability windows.

### Chilfos (`E_KK`)

- Shortens `mTimer` and `field_0x672`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Leads thrown spears toward `Link position + Link velocity * 8`.
- Suppresses a new thrown spear while three Chilfos spear actors are already
  active.
- Retains the slow-motion protection that prevents the frame-23 spear event
  from spawning repeatedly.
- Does not force a direct transition from a throw into a new melee state.

### White Wolfos (`E_WW`)

- Shortens `field_0x728` and `field_0x734`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Reduces native circling, approach, and attack intervals.
- Does not force an additional jump attack.

### Bulblin (`E_RD`)

- Shortens `attack_timer`, `timer[0]`, and `timer[2]`.
- Also shortens `bow_shake_timer` when the actor has a bow animation.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Every second bow shot becomes a three-arrow spread: the center arrow plus
  two arrows at `-0x900` and `+0x900` yaw.
- Extra arrows preserve the native arrow type and targeting parameters.
- Retains the slow-motion arrow guard that prevents duplicate arrows.
- Mounted Bulblins remain excluded by the profile eligibility checks.
- Does not add projectile leading or force a new two-hit melee state.

### Lizalfos (`E_DN`)

- Shortens `timer[0]`, `timer[2]`, and `unk_timer_1`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Alternates normal combo finishers per Lizalfos: only every second one causes
  its native knockdown.
- Reduces the grounded recovery after a surviving knockdown from the native
  `60–70` timer points to the Dynalfos value of `5`.
- Makes native attacks, reactions, and reorientation available sooner.
- Does not change sidestep randomness or force longer combo chains.

### Dynalfos (`E_MF`)

- Shortens `field_0x6c0[0]`, `field_0x6c0[2]`, and
  `field_0x6c0[3]`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.4x` turning.
- Alternates normal combo finishers per Dynalfos so only every second one
  causes a knockdown; its native `5`-point grounded recovery is preserved.
- Retains the slow-motion protection for fight-run sidestep impulses.
- Does not force guard, counter, or jump-attack transitions.

### Stalfos (`E_SF`)

- Shortens `mTimers[0]` and `mTimers[2]`.
- Also shortens `mTimers[1]` while action `8` or `9` is active.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Reduces the selected native block, attack, and recovery/rebuild intervals.
- Does not add poise or replace the rebuild state machine.

### Mini Freezard (`E_FZ`)

- Shortens `field_0x710` and `field_0x711`.
- Reduces native movement, collision, and retry pauses.
- Actor-ID cadence offsets stagger groups.
- Blizzeta-controlled children, iron-ball-gated variants, and roll-move states
  remain excluded.
- The profile manually interpolates movement and turning, so the generic
  chase-based acceleration and turn multipliers are not applied directly.

### Keese / Fire Keese / Ice Keese (`E_BA`)

- Shortens `mTimer[0]` and `mTimer[1]`.
- Applies `1.4x` movement/approach acceleration.
- Applies `1.4x` turning.
- Reduces native circling, dive-attack, and retry intervals.
- Normal, fire, and ice variants share this profile.
- Elemental variants do not receive separate rear-attack logic.

### Tektite (`E_TT`)

- Shortens `mAttackTimer`.
- Also shortens `mGenericTimer` while action `0` is active or
  `mMode >= 4`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Reduces jump cooldown and selected landing/recovery intervals.
- Does not predict Link's movement or force a double-jump state.

### Gibdo (`E_GI`)

- Shortens `field_0x684`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Makes native follow-up and repositioning behavior available sooner.
- Scream timers are intentionally not shortened.
- Does not force a second sword strike.

### Staltroop (`E_ZS`)

- Shortens `field_0x670` and `field_0x671`.
- Applies `1.25x` turning.
- Does not receive a base `speedF` acceleration multiplier because the profile
  does not register it as an owned chase value.
- Actor-ID cadence offsets stagger groups.
- Stallord's central controller is unchanged.

### Freezard (`E_FB`)

- Always shortens `field_0x680`.
- During attack action `1`, also shortens `field_0x69c` once
  `mMoveMode >= 3`.
- Applies `1.25x` turning.
- Does not receive a base `speedF` acceleration multiplier.
- Does not accelerate the active middle portion of the breath animation.
- Retains the slow-motion protection against duplicate breath events.

### Stalchild (`E_BS`)

- Shortens `timers[0]`, `timers[1]`, and `timers[2]`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Reduces native attack, waiting, and recovery intervals.
- Actor-ID cadence offsets stagger groups.
- Does not add additional poise.

### Bubble / Fire Bubble / Ice Bubble (`E_BU`)

- Shortens `timers[0]` and `timers[1]`.
- Applies `1.4x` movement/approach acceleration.
- Applies `1.4x` turning.
- Reduces passive hovering and retry intervals.
- Normal, fire, and ice variants share this profile.
- Does not add separate height coordination or rear-attack logic.

### Rat (`E_MS`)

- Shortens `mActionTimer[0]` and `mActionTimer[2]`.
- Applies `1.4x` movement/approach acceleration.
- Applies `1.4x` turning.
- Reduces native jump, chase, and retry intervals.
- Actor-ID cadence offsets stagger groups.
- Does not add an explicit flanking state.

### Puppet (`E_FS`)

- Shortens `mTimer[0]` only while `ACT_WAIT` or `ACT_MOVE` is active.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Makes approach and attack availability occur sooner.
- Retains the swept-hitbox guard that prevents repeated hits on fractional
  slow-motion frames.
- Does not add a new combo state.

### Bomskit (`E_CR`)

- Shortens `timers[0]`, `timers[1]`, and `timers[3]`.
- Applies `1.4x` movement/escape acceleration.
- Applies `1.25x` turning.
- Reduces native bomb/egg action and retreat intervals.
- Retains the slow-motion lifetime handling that prevents duplicate egg
  spawns.
- Does not lead bomb or egg placement toward Link's future position.

### Stalhound (`E_SH`)

- Shortens the `field_0x698[0]` wind-up while attack action `3`, phase
  `1`, is active.
- Applies `1.4x` movement/approach acceleration.
- Applies `1.4x` turning.
- Stalhounds with an odd process ID can perform one immediate follow-up pounce.
- The follow-up flag is cleared after use, preventing an endless pounce loop.
- Process-ID selection and cadence offsets stagger packs.

### Fire Toadpoli (`E_TK2`)

- Shortens `mActionTimer[0]` while action `2` is active.
- Applies `1.25x` turning.
- Does not receive a base `speedF` acceleration multiplier.
- Alternates direct aim with a led shot toward
  `Link eye position - 20 Y + Link velocity * 10`.
- Every shot becomes a three-ball spread: the center projectile plus
  two projectiles at `-0x900` and `+0x900` yaw.
- The extra projectiles remain suspended until the native center projectile is
  released, then all three launch together from the Toadpoli's muzzle.
- Recursion and slow-motion duplicate guards prevent the extra projectiles from
  recursively producing more spreads or repeating on the same event frame.

### Water Toadpoli (`E_TK`)

- Shortens `mActionTimer[0]` while action `2` is active.
- Applies `1.25x` turning.
- Does not receive a base `speedF` acceleration multiplier.
- Every second shot becomes a three-ball spread: the center projectile plus
  two projectiles at `-0x900` and `+0x900` yaw.
- Non-spread shots retain the native aim and projectile initialization.
- Spread projectiles use the shared suspended-ball release fix and launch
  together from the Toadpoli's muzzle.

### Dodongo (`E_DD`)

- During attack action `4`, shortens `field_0x6aa[0]` in the pre-breath
  phase (`field_0x68c == 0`) and late/recovery phases
  (`field_0x68c >= 4`).
- Applies `1.2x` movement/approach acceleration.
- Applies `1.4x` turning.
- Active breath phases `1` through `3` are intentionally not shortened.
- Does not force a tail attack or add a wider breath sweep.
- The existing weak-point behavior remains unchanged.

### Skulltula (`E_ST`)

- Shortens `mTimers[0]` during actions `3`, `0x0B`, `0x0E`,
  `0x0F`, and `0x33`.
- Also shortens `mDefTimer` during ground-fight action `0x33`.
- Applies `1.2x` movement/approach acceleration.
- Applies `1.25x` turning.
- Retains the slow-motion protection against duplicate silk projectiles.
- Does not rewrite vulnerability hitboxes.

### Baba Serpent (`E_HB`)

- Shortens `timers[0]` and `timers[1]`.
- Applies `1.25x` turning.
- Does not receive a base `speedF` acceleration multiplier because the profile
  tracks position values instead.
- Reduces native bite, waiting, and recovery intervals.
- Does not force a separate double-bite transition.

### Big Baba (`E_GB`)

- Shortens `timer[0]` and `timer[1]`.
- Applies `1.4x` movement/approach acceleration.
- Applies `1.25x` turning.
- Makes native head movement, bite availability, and repositioning more
  aggressive.
- Does not add a low-health branch or explicitly release the Baba Serpent
  earlier.

### Deku Baba (`E_DB`)

- Shortens `timers[0]` and `timers[1]`.
- Applies `1.4x` movement/approach acceleration.
- Applies `1.25x` turning.
- Reduces native rest, bite, and recovery intervals.
- Does not force a separate double-bite transition.

## Adult Goron exclusion

Adult Goron (`NPC_GRA`) remains one of the enemy slow-motion integration
profiles but is deliberately excluded from Enemy Hard Mode. The profile covers
normal Goron NPCs rather than a regular enemy. Dangoro is a separate actor and
is not covered by PR #14.

## New profile-specific mechanics

Most profiles become harder by reaching their existing behavior sooner and by
tracking Link more aggressively. PR #14 adds entirely new combat behavior only
for:

- **Chilfos:** led spear aim and a limit of three active thrown spears.
- **Stalhound:** one bounded follow-up pounce for selected actors.
- **Fire Toadpoli:** alternating direct/led aim and a three-ball spread on every shot.
- **Water Toadpoli:** a three-ball spread on every second shot.
- **Bulblin:** a three-arrow spread on every second bow shot.

The remaining profiles do not receive newly forced combos, counters, flanking
formations, low-health phases, or replacement attack state machines.
