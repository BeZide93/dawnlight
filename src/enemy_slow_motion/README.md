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

The original Darknut implementation was tested in game by the user. The
extracted core and newly added profiles still require in-game regression tests.
This is not yet coverage of every enemy or boss.

## Adding a Profile

1. Audit execute ordering, early returns, decremented timers, periodic counters,
   animation events, direct position writes, collision correction and child actors.
2. Add a named profile source and register its getter in the core and CMake.
3. Supply an eligibility predicate, a preparation callback, and optional movement,
   observation and reset callbacks. Use the shared execute-hook installer.
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
