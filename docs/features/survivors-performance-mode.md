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

## Enemy HUD and normal-enemy shadows

Performance Mode also budgets Dawnlight's own enemy HUD. Local display toggles
still decide whether a feature is enabled; enabling Performance Mode does not
turn on hidden labels or rewrite any saved preferences.

| Display | Mode off | Mode on | Tier 1 | Tier 2 |
| --- | ---: | ---: | ---: | ---: |
| Enemy labels / bars | 64 | 32 | 16 | 8 |
| Exact HP labels | 64 | 8 | 4 | 0 |
| Floating damage numbers | 96 | 24 | 12 | 0 |

Visible nearby enemies are preferred, with the native lock-on target first.
Limits also apply at draw time so a live tier change cannot retain excessive
fading labels. Damage within the first six simulation ticks of one enemy's
popup is combined without extending its lifetime. Excess popups are discarded,
not queued for a later burst. Text uses one outline pass plus its foreground
instead of four outline passes. Mode off restores the original rendering.
The separate boss HUD, stamina display and all combat rules are unchanged.

Twilit Essentials retains ownership of its enabled HUD features. Dawnlight does
not disable its settings or constrain its draws. Disable TE's corresponding
features and enable the Survivors HUD alternatives to use these budgets.
The diagnostics expose active TE provider features instead of implying that
Dawnlight's counters include them.

Normal tracked enemies submit native `dComIfGd_setShadow` shadows only within
1,200 world units of Link in the base tier and 600 in tier 1; tier 2 suppresses
those shadow submissions entirely. Elites/bosses, Link, untracked actors, hub,
transitions and non-running phases keep native behavior. This never skips the
actor Draw/Execute methods or changes enemy population. A nested actor draw has
its own scope; no model/actor pointer is retained after returning. Zero is the
native no-shadow key, and the host rebuilds shadow submissions every frame.

The optional hooks roll back to native shadows if registration fails and are
removed on shutdown. Their signatures/symbols were checked against the pinned
SDK and the official Android 2.0.3 ELF. The native real-shadow pool already holds
at most eight shadows, so this is additional headroom, not a replacement for
reducing the main enemy geometry. Other shadow submission paths are unaffected.

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

The version check accepts `2.0.0` through `2.0.3` and their four-component
zero-revision forms (`2.0.0.0` through `2.0.3.0`), with an optional leading `v`.
The official Android v2.0.3 HostService reports `2.0.3.0`; rejecting that spelling
previously disabled sampling, buffer warnings and adaptive pressure tiers while
leaving fixed effect budgets active. Nonzero revisions, suffixes and newer
releases remain unsupported until their statistics ABI has been checked.

The highest measured utilization among the four arenas selects tier 1 at 70%
and tier 2 at 80%. Recovery requires 120 distinct samples below 65% per tier.
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

Tests exercise HUD draw limits, lock-on priority, damage aggregation, TE ownership,
nested shadow scopes, native fallback, hook cleanup, lifetime/shared caps, adaptive recovery, optional symbol lookup,
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
