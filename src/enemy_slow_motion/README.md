# Continuous Enemy Slow Motion

The shared runtime is `../enemy_slow_motion.cpp`. Profiles contain typed actor
accessors and actor-specific exceptions; no raw memory offsets are scanned.
Only registered, eligible profiles bypass the old actor tick skipping and pose
interpolation. All other actors retain their previous behavior.

## Current Coverage

| Profile | Continuous path | Retained fallback |
| --- | --- | --- |
| Darknut (`B_TN`) | Combat, both morph controllers, movement, timers, detached equipment | Opening, room demo, armor-change demo, ending |
| Bokoblin (`E_OC`) | Ordinary combat/movement, morph controller, timers, interpolated attack translation table | Death/water death, scripted demos, fall-death, active camera-reset timer |
| Mini Freezard (`E_FZ`) | Standalone movement, independent visual rotation, timers | Iron-ball-gated variant, Blizzeta-controlled/orbiting variants |
| Keese, Fire Keese, Ice Keese (`E_BA`) | Flight, body animation, steering, knockback, timers, modulo decisions, released/dead movement | Boomerang attachment, held wolf bite |
| Tektite (`E_TT`) | Red/blue variants, jumps, landing, foot angles, water landing offset, animation and timers | None beyond the shared combat-slow gate |
| Gibdo (`E_GI`) | Body/sword animation, movement, steering, attack events and enemy timers | Player scream escape input and its ownership timers stay real-time |
| Adult Goron (`NPC_GRA`) | Ordinary NPC body/expression animation, movement, turning, carried-object height curve | Conversations and scripted events; Dangoro is a separate, pending actor |
| Staltroop (`E_ZS`) | Appearance/retreat, animation, turning, timers, once-per-frame-zero voice | Stallord orchestration/relocation remains native |
| Aeralfos (`B_GG`) | Flight and ground combat, animation, steering, movement, attack cooldowns | Demo/camera/death paths and detached helmet; clawshot attachment position stays native |
| Chilfos (`E_KK`) | Body and weapon animation, movement, timers, spear projectiles, single spear spawn and melee impulse per animation event | Dormant iron-ball-gated variant until unlocked |
| Freezard (`E_FB`) | Body animation, turning, attack delay, slowed breath-hitbox cadence, breath projectile motion/lifetime | Native shared breath ownership and event/room switches |
| Stalchild (`E_BS`) | Body/weapon animation, movement, gravity, knockback, timers, modulo decisions, spear sound edge | None beyond the shared combat-slow gate |
| Bubble, Fire Bubble, Ice Bubble (`E_BU`) | Flight, body/jaw animation, head bounces, knockback, timers, gravity and collision-origin offsets | Clawshot attachment |
| Rat (`E_MS`) | Animation, turning, jumps, swimming, gravity, timers and modulo decisions | Held wolf bite; native lifetime reseeding |
| White Wolfos (`E_WW`) | Animation, movement, jumps, steering, head/ground angles and timers | Invisible pack coordinator |
| Puppet (`E_FS`) | Animation, movement, turning, gravity, timers, single attack-start event and continuous swept hitbox | Demo and pre-appearance relocation |
| Bomskit (`E_CR`) | Animation, movement, bounce/gravity, head movement, timers and native egg cadence | None beyond the shared combat-slow gate |
| Stalhound (`E_SH`) | Animation, movement, gravity, knockback, head angles, timers and mouth effects | Disappearance relocation remains native |
| Fire Toadpoli (`E_TK2`) | Animation, turning, timers, one fireball creation/release per attack | Lava anchoring remains native; fireball actor still uses its existing movement path |
| Bulblin (`E_RD`) | On-foot combat, bow/horn animation, joint easing, gravity, timers, one arrow per shooting frame | Mounted/King Bulblin, scripted actor sets, water death and collision-pause states |
| Lizalfos (`E_DN`) | Animation, procedural joint easing, movement/gravity, jump arc, timers, once-only sidestep impulses and frame sounds | Event/demo, water and collision-pause states |
| Dynalfos (`E_MF`) | Animation, joint easing, movement/gravity, leap arc, tail turning, timers, once-only sidestep impulses and frame sounds | Event, inactive, water and collision-pause states |
| Dodongo (`E_DD`) | Body/material animation, wall/ground movement, neck angles, gravity, timers and breath hitbox sweep | Scripted events |
| Skulltula (`E_ST`) | Ground/wall movement, homing jump/return, animation, leg easing, gravity, timers and silk emission cadence | Water death and scripted events; player attachment remains native |
| Baba Serpent (`E_HB`) | Head/leaf animation, head motion, knockback, timers and live stem relaxation with matching colliders | Player grabs, deletion sentinel and scripted events |
| Deku Baba (`E_DB`) | Head/leaf animation, lunges, knockback, detached movement/gravity and stem, timers | Player grabs, water death, deletion sentinel and scripted events |
| Big Baba (`E_GB`) | Head/flower/material animation, head motion, lunges, falling attack, timers and live stem relaxation | Intro/end demos and scripted flower ending |

The user tested the core with Darknut, Bokoblin and Mini Freezard before commit
`ad4536d`. Profiles added after that commit are compile-tested only and still
need in-game tests. This is not yet the entire requested enemy list.

## Remaining Requested Profiles

Armos, Baby Gohma, Chu Worm, Guay, Imp Poe, Kargarok,
Phantom Rider, Poe, Stalfos,
Young Gohma, Zant Mask and Zant's Hand still use the previous
slow-motion path. Dangoro (`E_GOB`) also remains pending, independently of
normal adult Gorons. Goron children/elders/shopkeepers use other NPC actors.

Known audit points for continuing: Armos' squash/paralysis counters and
timer-equality events; Bulblin's mounted/shared state; Chu Worm's separate core
and bubble bodies; Gohma's shared core ownership; Zant Mask's procedural hover;
Zant's Hand's orb attachment; Dangoro's player grabs and shifted collision
origin. Do not register these with a generic timer list alone.

## Adding a Profile

1. Audit execute ordering, early returns, decremented timers, periodic counters,
   animation events, direct position writes, collision correction and child actors.
2. Add a named profile source and register its getter in the core and CMake.
3. Supply an eligibility predicate, a preparation callback, and optional movement,
   observation and reset callbacks. Use the shared execute-hook installer, or
   `processExecute` for actors with a file-local native execute function. The
   shared dispatcher matches the actual actor execute-method pointer, not
   create/delete/draw calls.
4. List only owned morph controllers. `checkPass` and animation advance must use
   the same rate; integer frame comparisons need an actor-specific audit.
5. Compensate only unconditional timer decrements that actually run. Never hold
   periodic counters blindly: equality/bitmask events may repeat while held.
6. Position corrections must precede background collision and model/collider
   updates. Do not apply whole-execute position interpolation to teleports.
7. Check normal and slow combat, interruption, damage, death, state transitions,
   multiple instances, reset, and coexistence with an unsupported enemy.

Shared math tests: compile and run `tests/enemy_slow_motion_timing.cpp` with a
C++17 or newer compiler, with assertions enabled. They cover timer cadence,
independent phases, and forward/reverse motion sampling, not game hooks.
They also cover modulo-counter event cadence across wraparound, periodic
motion-curve sampling and direct movement with a shifted collision origin.
Frame-event tests cover duplicate suppression, independent actors, before/after
animation advance, silent frames, loops, animation changes and reset.
Scope tests cover slow motion off, incomplete contexts, nested inactive actors,
nesting overflow and reset. Hooks must not receive an empty execute placeholder
as an active profile (the cause of the 2026-09-09 Cave of Ordeals entry crash).
