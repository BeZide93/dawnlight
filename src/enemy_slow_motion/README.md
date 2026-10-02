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
| Adult Goron (`NPC_GRA`) | Ordinary NPC body/expression animation, movement, turning, carried-object height curve | Conversations and scripted events; Dangoro uses the separate boss profile below |
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
| Water Toadpoli (`E_TK`) | Animation, swimming movement, turning, timers, and one water-ball creation/release per attack | Native water-surface anchoring and projectile movement path |
| Fire Toadpoli (`E_TK2`) | Animation, turning, timers, one fireball creation/release per attack | Lava anchoring remains native; fireball actor still uses its existing movement path |
| Stalfos (`E_SF`) | Body animation, movement, gravity, steering, head motion, timers and single sword-swing sound per animation frame | Scripted demos and seated intro |
| Bulblin, Lizalfos, Dodongo, Dynalfos, Skulltula, Baba Serpent, Big Baba, Deku Baba | Existing actor-specific animation, movement, timers and combat profiles | Actor-specific demos, attachments and ownership transitions; see each eligibility predicate |
| Helmasaur, Helmasaurus (`E_MM`, attached `E_MM_MT`) | Both sizes, animation, movement, gravity, steering, timers, frame sounds; attached shell follows the live parent joints | Detached/carried shell |
| Kargarok (`E_KR`) | Ordinary flight/combat, live model/collider movement, hover phase, steering, gravity, timers and voice events | Paths, escort/bomb/mounted variants, death and Ending Blow |
| Guay (`E_GE`) | Flight, orbit, dive, returning/perched movement, damage spin, animation and timers | Active boomerang attachment |
| Armos (`E_AI`) | Animation/BRK, jumps, turning, damage rotation, squash/paralysis phase, timers and single impact events | None beyond the shared combat-slow gate |
| Chus (`E_SM2`) | All colors, body animation/deformation, split pieces, movement, gravity, turning and merge/timer cadence | Ceiling placement and bottle-catch ownership |

The user tested the core with Darknut, Bokoblin and Mini Freezard before commit
`ad4536d`. Profiles added after that commit are compile-tested only and still
need in-game tests. This is not yet the entire requested enemy list.

## Boss and Miniboss Coverage

Every entry in `kBossRushEntries` now has a continuous profile (Darknut and
Aeralfos were already covered). Additional profiles cover ground King Bulblin,
Phantom Zant, the human-form Skull Kid encounter, and Dawnlight's Hero's Shade.
The implementation extends the existing actor-scoped slow motion: it does not
change the game's global clock or Link's update frequency.

| Profiles | Special handling |
| --- | --- |
| Ook | Jump impulse and boomerang-release event guards; airborne translation before model/collider updates |
| Ook's boomerang (`E_MK_BO`) | Outbound/return flight, room-four orbit, spin, sound cadence, hit cooldowns and pillar bounces; the Boss Hard Mode speed bonus is scaled once before distance checks; hidden/deleting actors and item/cutscene sequences retain the previous path |
| Diababa and its heads | Separate animation, movement and timer ownership |
| Dangoro | Gravity and shifted rolling collision origin; player grabs and the shared platform-slam action retain tick skipping |
| Fyrus | Body, eye/material, attack-effect and chain-anchor animation clocks |
| Deku Toad and Toado | Body/tongue timers and separate egg/minion animation clocks |
| Morpheel | Body/fin controllers, slowed swimming before matrices/colliders, 512-sample simulation history retained across display frames |
| Death Sword | Visible combat phases and both magic projectiles; their spawn origins stay native |
| Stallord | Body, head and bullet movement; conditional timers require complete hook symbols (see below) |
| Darkhammer and iron ball | Body, ball flight, gravity and ball timers; attached ball follows its parent |
| Blizzeta and ice blocks | Rotation, pursuit, material clocks and early-return deletion timers |
| Armogohma, Baby Gohma and Young Gohma | Body/beam/material animation, final eye/minion movement and separate timers |
| Argorok | Body and projectile movement; conditional timers require complete hook symbols |
| Zant, magic and mobile actor | Phase-specific homing/spin, portals and separate projectile lifetimes |
| Puppet Zelda | Reflected ball velocity preserved; triangle events fire once on simulation ticks, with smooth visual frames |
| Beast Ganon | Body and portal controllers; player-owned/wolf interactions retain the previous path |
| Ganondorf | Duel movement/gravity and animation; player clash and scripted ending retain the previous path |
| Ground King Bulblin | Animation, movement, gravity, recoil and timers |
| Phantom Zant | Body, portals and summoned projectile variants |
| Skull Kid | Human-form second encounter only |
| Hero's Shade | Dawnlight encounter only; custom attack, cooldown and hazard clocks follow the actor clock; tutorials, trials and cinematics keep their previous behavior |

Cutscenes, scripted placement and player-owned interactions are deliberately
excluded by each profile's eligibility predicate. They still use the existing
fallback where the combat-slow gate permits it. Mounted King Bulblin/Bullbo and
unregistered child actors (including Morpheel's tentacles)
also retain the previous path; this is not a continuous conversion of every
actor involved in every boss sequence.

Stallord, Argorok and Hero's Shade decrement some timers conditionally inside
actions. Their profiles enable only when the host exports non-inlined
`cLib_calcTimer<int>` and `cLib_calcTimer<unsigned char>` symbols. If either is
missing/inlined, or installing either hook fails, they retain the previous path.
No timer is pre-incremented speculatively. File-local boss hooks use symgen's
translation-unit aliases instead of platform-specific mangled names.

`python3 tests/boss_enemy_slow_test.py` executes production callbacks with
asset-free fixtures: direct movement versus nested `posMoveF`, owned/foreign
chase parameters, conditional timer calls, ice deletion, projectile spawn
origins, triangle event edges, and Morpheel's history with nested actors and
index wrap. It also checks Ook's boomerang with the production Boss Hard Mode
speed bonus, return flight, impact impulses, timer/sound cadence, room-four
turning, pillar bounces, and inactive/foreign-actor guards. CI runs it alongside
the existing Cave and shared timing tests.
These tests verify the callbacks, not the host's runtime hook resolution.

Before merging, test each Boss Rush entry with Bullet Time and Flurry Rush,
including slow-mode entry/exit, damage, defeat and phase changes. Repeat with
Boss Hard Mode; test mirrors, a full rush and Cave randomizer returns. Check
Morpheel's body length, reflected Zelda balls, Ook's outgoing/returning
boomerang and pillar bounces (normal/hard), Dangoro's platform, Blizzeta's
ice deletion, Zant's room changes and Hero's Shade clones/hazards. Verify the
three conditional-timer profiles on both Windows and Linux hosts. No game
runtime or Windows build has been exercised by the asset-free tests.

## Remaining Requested Profiles

Chu Worm, Imp Poe, Phantom Rider, Poe, Zant Mask and Zant's Hand still use
the previous slow-motion path.
Goron children/elders/shopkeepers use other NPC actors. Chu Worm is distinct
from the Chus (`E_SM2`) covered above.

Known audit points for continuing: Chu Worm's separate core and bubble bodies;
Zant Mask's procedural hover and Zant's Hand's orb attachment. Do not register
these with a generic timer list alone.

## Cave Enemy Validation

Armos also installs a death-flash safety hook independently of active slow motion.
It requests the normal scene effect `0x81ED` and checks the returned emitter before
making it immortal. If the resource is unavailable or the emitter pool is full,
only the flash is omitted; the native 56-tick explosion countdown, defeated switch
and actor deletion still run. This covers ordinary, spawner and randomized Armos.
The flash-start block runs at death-function entry so native code can continue
without reaching its unchecked emitter dereference.

`python3 tests/armos_death_fallback_test.py --dusklight-dir dusklight` runs the
production guard with the pinned host's actual `e_ai_damage` implementation and
asset-free services. It checks successful/failed particle creation, countdown
cadence, native movement, switches, deletion and independent actors. CI runs it
after CMake has fetched the host sources. In-game Android/Windows testing is still
needed, including a room with the effect and a spawner room without it.

`python3 tests/cave_enemy_slow_test.py` executes the production Helmasaur,
Kargarok, Guay, Armos and Chu callbacks with asset-free native-shaped fixtures.
It checks jump impulses, collision correction order, event repetition, orbit/dive
motion, shell ownership, merge cooldowns and split/reset guards.
`python3 tests/cave_enemy_spawner_test.py` executes the production spawn selector
for both Helmasaur sizes, Kargarok, Guay, Armos and the five Cave Chu colors.
Existing saved selection indices are preserved by appending the ten entries.

These tests and the native SDK build do not replace an in-game check: spawn each
entry with slow motion off/on, test damage/death and slow-mode transitions, pull
a Helmasaur shell, catch/release a Guay with the boomerang, and split/merge Chus.
Also check a randomized Cave run and coexistence with unsupported enemies.

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
Scope tests cover slow motion off, incomplete contexts, nested inactive actors,
nesting overflow and reset. Hooks must not receive an empty execute placeholder
as an active profile (the cause of the 2026-09-09 Cave of Ordeals entry crash).
