# Hyrule Survivors

[← Dawnlight overview](../../README.md)

Hyrule Survivors is a separate game mode included in `dawnlight_mod.dusk`.
Select it in Dusklight's game-mode menu and create or load a Survivors save.
It uses `gczelda2-survivors`, separate from the story and Boss Rush save files.
The title screen, save selection, Link/Epona names and optional TV settings finish
before the arena loads. Selecting the mode alone cannot start a run. Returning
to the title screen disarms the run until another save is selected and started.

## First arena

The first version is a ten-minute run in Dawnlight's isolated Darknut arena.
Start with three hearts and automatic normal sword strikes. Sword Wave starts
locked, like Whirling Blade and Light Aura. Move, dodge and collect blue experience
crystals while weapons attack automatically. Manual Jump remains available with
your configured binding; manual sword attacks and arena exits are disabled.
Normal strikes aim at nearby enemies and use Link's native sword animation and
collision. They wait for the sword to be drawn and do not interrupt jumps, dodges,
damage reactions or item actions.
Automatic strikes advance through the native four-hit combo, including its
directional/lock-on variants and finisher. Follow-up strikes respect the native
hit and cancel windows; interruptions or two seconds without a new strike reset
the sequence. Normal swings, finishers and spin attacks all retain stick-controlled
movement through the native ground and wall collision handling.

Leveling up pauses Link and the encounter and offers three upgrades. Controller,
keyboard and touch navigation use Dusklight's native mod window controls. Closing
the choice window does not resume combat or reroll its offers. Excess experience
is retained and multiple earned levels are offered consecutively.

| Upgrade | Effect |
| --- | --- |
| Sword Wave | Unlock aimed, piercing projectiles released during a normal sword swing; additional waves at levels 3 and 5 |
| Whirling Blade | Unlock or improve a native Spin Attack with freely steerable movement |
| Light Aura | Unlock or improve a smaller, more frequent area attack |
| Hero's Bow | Chest unlock; stronger/faster arrows, spread shots at ranks 3 and 5 |
| Boomerang | Chest unlock; Gale wind effect and one extra target at rank 1; each upgrade adds another hop and improves damage/size/cooldown |
| Bombs | Chest unlock; aimed ballistic throws with larger, stronger blasts and shorter cooldowns |
| Ball and Chain | Chest unlock; launch and retract a heavy ball, stronger/larger/faster with upgrades |
| Power | Increase automatic weapon damage |
| Haste | Reduce automatic weapon cooldowns |
| Reach | Increase weapon size |
| Magnet | Attract experience from farther away |
| Heart Container | Add one maximum heart without healing |
| Rupees | Receive 20 rupees without healing |

Equip all three base weapons, all four chest weapons and up to four different
passive upgrades. Chest weapons do not consume passive slots.
Weapons and passives have five levels. Rupees remain available after the
other upgrades reach their caps. Native enemy invulnerability, defense and death
reactions still apply; the mod replaces the native nonlinear damage table only
for its own weapons and automatic sword strikes. Sword Wave uses the two smaller
normal-spin particle effects, following the moving-emitter approach of Great Spin
Projectile. It does not use the large Great Spin effect or a debug sphere.

Each confirmed ordinary enemy kill has a 5% chance to drop a native heart, a 40%
chance to drop a green rupee and a 5% chance to drop a blue rupee. The remaining
50% drop no item. Experience crystals are still awarded independently. Native
random drops are suppressed to prevent duplicate rewards; hearts and rupees use
the game's normal pickup behavior. Level-ups never heal Link.

Bokoblins and Keese appear first; Rats join after 30 seconds. Spawn frequency,
population and Bokoblin health increase during the run. At ten minutes, ordinary
spawning stops and an Elite Bokoblin appears. Defeat it to win. Death opens a
result screen with time, kills, level and weapon levels. **New run** reloads the
arena and resets all run upgrades.

## Treasure chests

Two treasure chests can be present at once. The first appears after 20–30 seconds;
subsequent spawns are attempted at random 25–45 second intervals. Positions need
flat ground, a clear footprint and an unobstructed line from Link. Failed/full
placements retry after three seconds. Closed chests remain until purchased or the
run ends; opened chests disappear immediately when their native reward sequence finishes.

Approach a chest and use the native **Open** action. Without enough rupees
it has no interaction prompt. Opening an affordable chest takes priority over
automatic attacks and other actions: press Open to interrupt them immediately.
Auto-aim/spin facing cannot hide the prompt; distance and line of sight still apply.
There is no purchase/confirmation dialog. The HUD
shows the next chest price on a separate line: **20 rupees**, then **40**, **60**,
and so on. Link receives the random reward through the native Get Item lift,
holding pose and item message while enemies and the run timer pause. The lid
opens during this sequence. Currency, reward and price are committed once the
sequence completes; failed loading or cancellation does not charge rupees.

Each purchase chooses equally among the eligible reward types: Hero's Bow,
Boomerang, Bombs, Ball and Chain, Master Sword and a bottle of Red Potion.
The four automatic item weapons unlock at rank 1; duplicates add one rank up to
5, and unlocked weapons also enter the normal level-up pool.

Master Sword is a one-time reward per run. Its Get Item presentation uses a
private instance of Link's sword model instead of the native rupee placeholder.
It equips automatically and adds 3
Survivors damage to normal sword strikes, Whirling Blade and Sword Wave.
A potion reward adds one new filled bottle while fewer than four bottles are
owned, even if an existing bottle is empty. Once all four slots are occupied,
it refills one empty bottle instead. Filled bottles are never overwritten. Red
Potion can be used normally from the item inventory; drinking it makes that
empty bottle eligible for a future refill. Four full bottles exclude potions
from the reward pool until a bottle is emptied. These two rewards are chest-only.

Maxed weapons and an already-owned Master Sword are excluded. New chests stop
spawning only while no useful reward is available. Each run resets the separate
Survivors inventory, Master Sword unlock, chest prices and weapon ranks, with
a giant wallet (at least 1000 capacity) and zero rupees.

All four weapons attack automatically alongside the sword without assigning
item slots or consuming ammunition. Bow and boomerang use Link's native upper-body
draw/shoot and throw animations while retaining movement. Link faces the target
and releases each projectile at its animation cue; higher fire rates accelerate
the animation. Sword, bow and boomerang animations take turns.
They aim at the nearest
live tracked enemy within range and line of sight. Arrows stop on impact;
boomerangs use the native Gale wind model/animation and particle trails. They
chain to one additional enemy at rank 1, then one more per rank (up to six
different targets at rank 5), before returning to Link. Each hop needs a live
target within range and line of sight, and cannot revisit an earlier target.
They deal Survivors damage on their outward/return paths. Bombs follow an
arc and explode at the target or a wall; the ball launches outward and retracts
toward Link. Projectiles cannot pass through walls. Power, Haste and Reach also
apply. Custom bomb/ball hits use the linear Survivors damage path to avoid
vanilla Bokoblin instant-death/health-correction special cases. Native item/chest models and bomb particles are reused; the chest lid
animates during the native reward presentation. These are run-local purchase
props. The demo item uses the native presentation-only flag; successful
completion explicitly grants Master Sword/bottles in the isolated Survivors
save. Story treasure flags remain unaffected.

## Records and saving

**Save records** on the result screen opens the game's native save prompt.
Complete that prompt before quitting to persist the records. The mode tracks
completed runs, wins, longest survival, most kills and highest level per save
slot. A new run keeps unsaved records in memory, but closing the application
without saving loses them. Failed runs still count as completed attempts.

Saving during combat does not suspend a run. Loading a Survivors save always
returns to a fresh start screen. Story and Boss Rush slots are not converted.

Dawnlight's combat presets, progression locks, HP scaling, Bullet Time, Flurry
Rush, Dark Link, Dual Wield, stamina, sprint, glide and Gale are overridden for
this mode without rewriting their saved configuration. Manual Jump uses normal
height. Other mods can still change gameplay; cross-mod balance is not enforced.

## Initial scope and device testing

This is the initial arena implementation. It retains the native camera and uses
code-drawn crystals, aura rings and chain lines, plus native chest/item models,
sword animations and particle effects. No additional textures, models or music
are required. Larger arenas, camera changes, weapon evolutions, persistent
unlock trees and mid-run suspend are not part of this version.

The budgets are 24 tracked enemies (including pending/deleting actors),
24 attack objects (eight base-weapon slots and four per chest weapon), two chest
props and 64 experience crystals. Model allocation is bounded by a 2 MiB heap,
trimmed after loading. Excess crystals merge their value.
These limits are starting budgets, not a measured Android performance guarantee.
Spawns require a valid floor and unobstructed line from Link, and are staggered.
A failed elite creation/despawn can be retried without counting as a victory.
Only confirmed deaths of initialized, tracked actors award experience.

Before merging/releasing, test on Android and desktop:

1. Create and reload a separate Survivors slot; verify story and Boss Rush saves.
2. Complete a run using all seven weapons; check native enemy death, elite victory
   and no HP scaling leakage. Verify jump, dodge, controller and touch controls.
3. Level up while surrounded, close/reopen the choice window, and earn several
   levels at once. Verify Link/enemies and the timer stay paused during selection.
4. Die, save the result, restart, reload the slot, and decline the save prompt.
5. Reset, change modes and disable/reload Dawnlight while enemies or projectiles
   are present. Confirm no stale actors, collisions or windows remain.
6. Stress the population and crystal caps and check frame time/memory on Android.
7. Start without Sword Wave, hit nearby enemies with automatic normal strikes,
   verify the full four-hit combo while moving, then unlock Wave and verify a
   swing releases the small spin projectile. Verify the combo restarts after a
   damage reaction or a gap between targets, and permits movement during finishers.
8. Steer in every direction during Whirling Blade, including into walls; verify
   native spin hits and that damage/dodges safely interrupt attacks.
9. Collect heart and rupee drops; verify leveling up (including Heart Container)
   does not heal and that restarting clears pickups and all weapon unlocks.
10. Approach chests with 19/20 rupees: only the affordable chest offers Open.
    Check range/walls, native Get Item animation and lid motion, then
    prices 20/40/60 in the HUD with no purchase dialog or duplicate charges.
    Verify enemies/timer pause but Link and the item/camera finish normally.
    Press Open during sword/spin attacks, bow draw, boomerang throw and damage
    reactions; the chest must interrupt immediately without a delayed projectile.
11. Obtain all four chest weapons, test nearest-target selection, arrow impact,
    bomb blasts, returning boomerangs/balls and walls. Verify rank upgrades and
    simultaneous movement/automatic sword combos. Check native bow draw/shoot,
    facing and boomerang throw poses while moving and locking on. Verify faster
    animation with Haste, Gale wind effects and 2/6 distinct targets at ranks 1/5.
12. Restart/change modes with closed/open chests and active item projectiles;
    verify prices, ranks, models, particles and collisions reset. Verify story
    treasure flags are unaffected and the next run restores Ordon Sword/no bottles.
13. Obtain Master Sword once and verify equipment and sword damage. Collect four
    red-potion bottles, drink one, then obtain a refill. Check new-bottle priority
    over empty bottles, exclusion with four full bottles, and instant chest removal.

Automated tests cover rules, caps, upgrade choice integrity, record encoding,
true-death accounting, deferred spawn cleanup, startup/reset routing and shared
Boss Rush arena admission, sword scheduling with the native combo selector,
movement during all three sword procedures, wave timing and
particle cleanup, drop gating and level-up rewards. Loot tests additionally cover
purchase pricing/caps, guarded transactions, placement, all four trajectories,
target selection, boomerang chains, native ranged-animation release timing,
interruption/priority, fairness, model-load failures and shared-animation/heap
cleanup. They do not replace the
in-game checks above.
