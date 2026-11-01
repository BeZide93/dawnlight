# Survivors Performance Mode

Enable **Performance Mode** in Survivors settings to reduce Dawnlight-owned visual
load. It defaults to off, persists in the mod configuration, and can be changed
during a run. Visual reservations are reconciled on the next player simulation
update. Damage, status durations, collision radii, projectile counts, enemy counts,
RNG, difficulty and score eligibility are unchanged.

## Visual budgets

These are simultaneous full-detail effect groups, not per-frame spawn limits.
Each group reserves its emitter slots until stopped. All categories also share
one emitter-slot budget; individual maxima cannot all be reached together.

| Visual | Normal | Pressure tier 1 | Pressure tier 2 |
| --- | ---: | ---: | ---: |
| Burning enemies (2 emitters each) | 32 | 24 | 16 |
| Sword/shock waves (up to 6 emitters each) | 8 | 6 | 4 |
| Lightning groups (2 emitters each) | 8 | 6 | 4 |
| Explosions (9 emitters each) | 4 | 3 | 2 |
| Boomerang trails (2 emitters each) | 8 | 6 | 4 |
| Shared custom emitter slots | 128 | 96 | 64 |
| Detailed chain links per ball | 32 | 24 | 16 |
| Detailed chain links across all balls | 128 | 96 | 64 |

Fire-arrow trails, Fire Edge and Jump Strike decorations also consume the shared
budget. Burns prefer nearby visible enemies. Reduced waves/explosions retain a
simple radius marker; lightning retains a connecting line; iron balls retain a
continuous chain strand and their ball model. Boomerangs retain their base model.
A reusable packet draws simple markers without consuming native debug-packet slots.
Switching mode or pressure tier stops old owned emitters before admitting replacements.

## Monitoring and adaptive reduction

**Performance Monitor**, separately off by default, adds a HUD line during runs
and logs counters once per second while Performance Mode is active. It reports
vertex usage/capacity, run peak, index/uniform/storage usage, pressure tier and
enemy/projectile/effect counts. Emitter counts are reserved custom slots, not the
number of particles or native emitters.

The mod optionally resolves the existing `aurora_get_stats` symbol on Dusklight
2.0.0–2.0.3. A render-worker callback copies the published statistics and reads
buffer capacities from the supplied graphics context. The game thread never
reads Aurora's mutable statistics directly. Run epochs discard stale queued
samples after a reset or mode change. Missing symbols or unrecognized host
versions leave fixed budgets active and show measurement unavailable.

The highest measured utilization among the four arenas selects tier 1 at 80%
and tier 2 at 90%. Recovery requires 120 distinct samples below 65% per tier.
Measurements describe completed recording work and arrive asynchronously; they
are not a live allocation guard. A sudden spike can exceed an engine buffer
before a sample reaches the mod. Native actors/particles, other mods and other
engine allocations are outside these budgets. This patch neither grows Aurora's
buffers nor catches its fatal overflow.

## Indexed hub geometry

The hub always draws the existing packed hall attributes through 16-bit GX
indices, including the title screen and with Performance Mode off. This is a
scene safety fix, independent of optional effect limits. Triangle order,
positions, colors, UVs, textures and collision are unchanged. Arrays remain
immutable for asynchronous FIFO processing.

For the bundled 192,534 vertices, two room submissions (including a fog replay)
require 3,080,544 vertex bytes rather than 6,931,224. The deduplicated attribute
arrays add at most 1,765,464 storage bytes across those two submissions. These
are room-only calculations, not a measurement of an entire in-game frame.

The Android v2.0.3 title-screen crash in `dusklight-20261101-005724.log` was
symbolicated against the official APK with the matching build ID
`e5fcdd6af182b596ebf50dcaf4fd11356529f87a`. The call at `draw_prim+0xc8`
(`0x16e6870`) enters `push_verts`; the stack then reaches `ByteBuffer::append`
and its fixed-capacity overflow abort. Deferred Fog was disabled in that log.
It does not expose the exact allocation size, drawing actor or number of room
submissions. The previous direct path consumed 3,465,612 vertex bytes per room
submission, leaving little space for the rest of the title scene. Indexed
drawing reduces that contribution to 1,540,272 bytes (56% less) in both modes.
This does not increase the engine's arena or guarantee that arbitrary other
mods cannot exhaust it.

## Validation

Tests exercise lifetime/shared caps, adaptive recovery, optional symbol lookup,
worker-only sampling, stale samples, shutdown, live settings, identical lightning
and burn damage, and repeated hub creation/draw/cleanup. The mesh test reconstructs
every indexed attribute and compares it with the original packed data. Existing
combat/resource/lifecycle tests continue to execute production functions.
The production room draw fixture also checks both toggle states, a live toggle
change and two submissions with 2 MiB reserved for other scene geometry, within
the same 5 MiB vertex arena. It validates indexed attribute formats and emitted
positions, colors and both UV sets, plus cleanup after repeated room creation.

Before release, verify the Ordon Sword event with deferred fog and late-game runs
on Steam Deck, Windows and Android. Automated boundary tests and a successful mod
build do not replace those renderer/device checks.


Performance Logs can additionally record buffer thresholds, peaks, effect counts,
scene/item-event context and sampling latency even while Performance Mode is off.
See [Buffer-crash diagnostics](../diagnostics/survivors-freezes.md#buffer-crash-diagnostics)
for field definitions, package-detection limits and the unknown source-frame ID.
