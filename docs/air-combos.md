# Air Combos

The optional `air-combos` boolean defaults to false and is exposed under
Gameplay / Combat. `r-jump` remains the prerequisite. This first implementation
reuses human sword animations and combat data from the game.

## Ownership and transitions

`jump_hooks.cpp` tracks the manual ZR jump separately from native ledge jumps,
backflips, side hops and the Bow Bullet Time owner. `air_combos.inc` owns one
four-hit string per manual jump. It intercepts both the mod's explicit R+B route
and native `checkCutJumpInFly`, so releasing ZR cannot select Jump Attack instead.

`checkCutAction` initializes each native normal/finish swing. Native animation
playback, hit windows, collider placement, damage, voices and sword sounds remain
in charge. `checkNextAction` is intercepted only for the owned air string: its
cancel checks consume native combo reservations or a fresh attack press; an
unbuffered animation end returns to falling. The tracked strike count also
survives the native finisher's reset before its cancel check. Held B does not
charge a spin during the string.

`commonProcInit` still runs. Its post-hook restores `MODE_JUMP` after native
initialization clears flags, so the global automatic-fall check does not abort
the sword animation. It does not call `setJumpMode` again or reset fall height.
Native upper-body aiming toward the locked target is retained.

While locked on with a current `mTargetedActor`, the combo pursues that actor's
live eye position. It aims to place Link's sword (80 units above his feet) at the
target's height, with an eight-unit vertical tolerance and an 85-unit horizontal
melee gap. Proportional approach slows near that gap. Combined horizontal and
vertical speed is capped at 12 units per tick, allowing ascent above ordinary
jump height and descent to lower targets. Gravity is zero during pursuit;
`mNormalSpeed`, movement yaw and vertical speed feed native `posMove`. No actor
pointer is retained across calls. Target changes take effect immediately; losing
lock-on or the target drops upward/forward pursuit and restores free-air physics.

Without a target, positive vertical velocity keeps ordinary gravity. Once
descending, gravity is -0.08 and terminal fall speed is -0.8 units per tick;
horizontal sword lunge speed is capped at six units per tick. The native
position, wall, floor and roof collision pipeline still runs. There is no
position teleport or ground-hit spoof; native movement also records any higher
peak for subsequent fall damage. Repeated hook calls assign the same requested
velocity rather than adding movement several times in one frame.

The whole string has a 90-execution budget (three seconds at 30 Hz), shared across
all its swings, target changes and ascent. Follow-ups cannot reset this budget.
Early landing or an unbuffered combo end can end it sooner.

Normal/finish actions retain ownership. Native recoil retains its animation and
knockback, with normal gravity and no further buffered attacks. Damage, demo,
transformation and unrelated actions discard ownership through the existing
common-procedure hook. Landing, disabling either option, heavy equipment, water
or an item change ends the active string. Cleanup clears attack collision and
buffered inputs and restores native gravity. Any target-driven ascent/forward
movement is stopped before handing control back; native damage/recoil initializers
still set their own velocity. `procFallInit(0, ...)` preserves the remaining
velocity; `checkLandAction(0)` retains the game's landing/fall-damage decisions.
A spent string remains marked through falling until landing, preventing B spam
from starting another string or native Jump Attack in that same manual jump.
Mod shutdown restores gravity and drops ownership without dereferencing a stale
player pointer.

## Input compatibility

With the feature on, manual ZR jump admission allows lock-on while retaining the
existing world-interaction, wall, chain, event and grounded-state checks. ZR's
new-press jump takes priority over manual shielding; continued grounded shield
holds still use the old handling. The manual shield hook ignores ZR during the
owned airborne session, allowing native B combo reservation to work under lock-on.
Without the feature, the original target/shield exclusion and ZR+B Jump Attack
remain intact.

Beginning an air string clears the separate Bow Bullet Time session before
changing gravity. No global timescale, enemy AI, sword damage multiplier or
stamina rule is changed. New airborne animation assets and enemy launch/juggle
behavior are outside this initial implementation.

## Validation

`python tests/air_combos_test.py` compiles the actual production air-combo hooks,
manual jump entry/common-procedure handling and shield predicate against an
instrumented native API. It exercises both attack entry routes, ZR release,
feature-off fallback, four strikes and the finisher, buffered input, finite
airtime across reinitialization, unchanged untargeted fall height/ascent,
higher/lower/overhead and moving targets, bounded pursuit, melee stopping distance,
repeated-call stability, target loss/switching, landing, toggles,
item changes, heavy/water rejection, damage/demo/transformation, recoil,
interaction precedence and airborne manual-shield suppression. Temporary C++
files and executables are removed after the run.

The fixture models native initializer contracts; it does not render animations
or simulate the full game. Visual blending, enemy contact, lock-on feel and
platform input should still be checked in-game.
