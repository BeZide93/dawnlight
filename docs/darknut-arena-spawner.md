# Empty Darknut arena: spawner regression

The September 21 Android logs describe two different failures:

- At `9b03f77`, successive Bokoblin processes are created and discarded before
  their resources load. The room's forced cleared switch also reaches
  `fopAc_Create`, which rejects ordinary enemies before profile creation.
- The Goron bypasses that enemy-group check and reaches resource loading, then
  the renderer aborts. The earlier Bokoblin crashes have the same renderer stack.

The latter stack was resolved against the v2.0.1 Android binary matching build
ID `8431032be1f483cf884995a28bddd81556950b09` (Dusklight `422d7bb1`). It reaches
`ByteBuffer::append` through `gfx::push` and `GX_AURORA_DRAW_INDEXED`'s index upload.
This is the fixed index staging buffer's capacity abort. The log does **not**
identify the offending model, index count, or reason for excessive submissions.
The `152118` log reproduces it with vanilla Link and only Dawnlight enabled.

## Changes

- Restore the nested process tracking from `61b89c5`, including tracking before
  the engine assigns `actor_type` and paired pre/post callbacks.
- Let manually spawned processes query native switch/completion values. Keep
  forced cleared values for native room objects only. Leave room `-1` and
  invalid switch sentinels alone. No save flags are written.
- Restore the stage-name actor filter to keep authored supplies and event
  triggers out of the empty connector. Numeric ActorService creation bypasses
  that lookup.
- Check material and shape draw queues for cycles before drawing in `D_DLBR0`,
  room 51, in Boss Rush. A cycle would repeatedly submit geometry until the
  renderer aborts. Break only its closing link, preserving each distinct packet
  and the existing order; leave valid lists and other rooms untouched.

The draw-list change is defensive: a cycle is a candidate cause, **not proven
by the crash log**. If it runs, the log contains `Dawnlight arena: repaired cyclic
material draw queue` or `... cyclic shape draw queue` (up to four messages per
entry). An index-buffer crash without either message needs further renderer
investigation; it must not be reported as a confirmed cycle failure.

## Validation

- `python tests/bossrush_cave_routes_test.py`: production process hooks, nested
  creation, native-state passthrough, room isolation, draw-guard scope, Cave
  routes and both Continue answers.
- `python tests/bossrush_packet_chains_test.py`: null/acyclic queues, self-cycles,
  prefix-plus-cycle queues, long queues and repeated repair; unique packets and
  order preserved.
- `python tests/bossrush_warp_test.py`: existing warp lifecycle regression checks.
- C++ syntax checks against the pinned Dusklight SDK for both changed units.

Android/device validation is still required: spawn Bokoblin and Goron in the
empty arena, then exit/re-enter and repeat. Retain the complete log, including
any repair messages. This patch does not change renderer buffer sizes or import
Twilit Essentials code. PR23 is unchanged.


## Bomskit and Deku Baba follow-up

The later `Boomskit.log` and `Deku_baba.log` both contain material-cycle repair
messages from `0a66046`. Cyclic material lists are therefore now observed on the
device, rather than merely a candidate. Most other spawns work according to the
user's test.

Bomskit creates an `E_CR_EGG` child. Only the parent's ID was exempt from the
arena's cleared-room queries. The native room enemy gate consequently rejects
the egg before its profile initialization. Its deletion unconditionally calls
`Z2Creature::stopAnime`; that matches this log's SIGSEGV and deletion stack. Track
successful child creation IDs whenever the parent belongs to the spawner,
including further descendants, before their asynchronous base creation begins.
Native children remain unaffected; deletion removes each tracked ID separately.

The Deku Baba log ends after resource loading and a material-cycle repair, with
no crash stack. Its stalk uses a different intrusive list:
`mDoExt_3DlineMatSortPacket::setMat` prepends the material, and both interpolation
refresh and drawing walk `field_0x4` until null. Submitting an existing member
again forms a cycle outside the previous J3D packet guard. Reject duplicate line
submissions within the current list, repair existing line cycles, and allow the
material again after native list reset. Also repair material buckets before
`J3DMatPacket::entry`, since native sorting itself traverses those buckets.
These guards remain restricted to the Boss Rush connector.

New guard messages start with `Dawnlight arena: draw queue guard:` and identify
material, shape, line, or duplicate line submission. The device log does not
prove which additional traversal stalls Deku Baba; the patch covers both the
sort-time and stalk-list gaps. A new in-game test is still needed.

Regression coverage now includes asynchronous child admission before actor
initialization, grandchildren, failed/native child requests, independent child
cleanup, material sorting before draw, duplicate stalk submissions, line-list
reset and stage/save isolation. All three existing test scripts and both C++
syntax checks pass.
