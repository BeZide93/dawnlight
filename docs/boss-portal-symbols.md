# Boss portal symbols

Each of the 18 boss and miniboss destinations in the Boss Rush hub is a complete
Mirror of Twilight from the Mirror Chamber. It replaces that boss's floor portal.
A supplied black boss icon sits over translucent colored glass on the mirror
face: blue means undefeated, red means defeated, using the existing
persistent save flags. The icon itself stays black in both states. The center Boss Rush
and Cave of Ordeals floor portals remain available.

## Artwork and provenance

`art/boss-icons/` contains the 18 transparent PNGs supplied by the user in
`Twilight_Princess_Boss_Symbole_Schwarz_PNG.zip`, preserved byte for byte.
These replace the previous hand-drawn SVG emblems. The generator matches each
filename to the boss name in `kBossRushEntries` in `src/new_save_modes.cpp`,
including spaces converted to underscores; missing or extra PNGs fail generation.
No game archive is read by the generator. The mod embeds only the supplied icons'
alpha masks; the game's mirror models/textures are loaded at runtime. No code
or assets were taken from Twilit Essentials or PR #23.

## Regenerate

Normal builds use the checked-in generated header and require no graphics tools.
To replace the source PNGs and regenerate the embedded mask:

```sh
python -m pip install Pillow==12.3.0
python tools/generate_boss_symbols.py --preview /tmp/boss-symbols.png
```

The generator crops transparent margins and fits each silhouette inside the
circular face, preserving aspect ratio and all supplied details. Lanczos resizing
produces antialiased 256×256 masks. The atlas contains only alpha; the shader
renders it as black ink in both states. A bounded RLE decoder reads the embedded
atlas. Base64 strings are split into small chunks for MSVC compatibility. The
runtime creates five mip levels and uses linear filtering for smooth edges at
different distances. The optional preview shows both states over a checkerboard.

## Mirrors and rendering

A dedicated `DLBMir` ActorService profile loads the complete `u_mr_mirror` disc
and its `u_mr_table` frame/stand from the player's `MR-Table` archive at runtime.
No Nintendo model or texture is included in the mod. Both models use native
scale `(1, 1, 1)`. The stand is sampled at the final frame of `u_mr_table_up`,
matching its raised Mirror Chamber pose. The mirror attaches to the stand's
`MIRROR` joint with its own root transform preserved, as in the original game.
The actor uses shared archive references and per-actor model heaps, without
running chamber switches, stairs, effects, cutscenes or reflection singleton logic.

The former fixed 440-unit diameter and 240-unit center height are removed.
The entire posed assembly is placed with only yaw and translation, facing the
hub center. The frame model also contains the chamber's stone platform, so its
lowest vertex is not a suitable floor anchor. Placement instead uses the native
standing-panel height (Y = 4613.6299, documented in the SDK mirror-table actor)
and buries that surface 2 units below the hub floor. The platform underneath it
stays underground; the frame, attached mirror and fitted symbol move together.
Neither the disc nor the stand is resized to fit the artwork. Instead, the symbol
size follows the loaded vanilla disc dimensions, inset to leave the rim visible.
The animation remains frozen in the raised pose and its shared joint calculator
is restored after each evaluation/draw.

A circular blue overlay at 10% opacity lets the original mirror show
through; defeated bosses use a red overlay at the same opacity. The supplied
black icon is composited over it with its own alpha, so solid icon pixels remain
opaque black while gaps reveal the translucent background. The image stays fixed to the inward-facing front; looking from behind hides the
symbol instead of moving it onto the back. The native attachment, tilt and
relative position between the mirror and its frame are preserved. The symbol plane is fitted
to the loaded mesh's broad front surface using its vertex positions and authored
normals, rather than the axis-aligned bounding box. Geometric plane candidates
from inner vertices also handle smoothed bevel normals. Raised rim vertices are
downweighted and rear planes are excluded. All four symbol corners use this same
plane with a fixed 0.4-unit normal offset to avoid z-fighting. No camera-dependent
rotation, flipping or front/back relocation is applied.

The GfxService renderer prepares each view's vertices after opaque geometry,
then submits its single overlay batch after `dComIfGd_drawXluListDark`, when the
normal and dark translucent material lists have finished. The pinned SDK has no
post-translucent world-camera stage, so a post-hook supplies this insertion point
before post-processing and the HUD. GfxService flushes pending GX commands before
inserting the overlay. No extra opaque background is drawn.

Scene depth testing remains enabled, but depth writes are disabled: a 10%-opaque
circle must not act as an opaque depth barrier for the native mirror materials.
Transparent corners are discarded. Consumed batches cannot draw twice, and a
before-HUD cleanup discards any frame-local batch whose translucent pass was
skipped. Shutdown removes the post-hook and both stage hooks before releasing
GPU resources. Pipelines match MSAA, reversed depth and deferred render layouts;
no shared model materials or GX state are edited. ActorService drains mirror
actors before removing their profile.

Hub spawning retains live and asynchronously loading IDs, retries missing slots
individually and attempts at most one destination per tick. Midna's existing
Fight / Yes / No prompt only becomes available once its mirror is ready. Its
150-unit interaction radius is centered 300 units in front of the mirror toward
the hub center, at floor height, so Link can interact before reaching the pedestal.
The two central floor portals keep their original trigger positions. Hub
supplies, boss entry logic, saved progress and both center portals retain their
existing behavior.

## Validation

- Local Linux release-with-debug-info build and `.dusk` packaging passed.
- C++ atlas decoding was compared byte for byte with the PNG generator output.
- All 18 source PNGs match the supplied ZIP byte for byte; silhouettes fit inside
  the circular face without stretching or clipping.
- All 18 artwork names and portal indices match.
- The pinned Dawn library's validation backend accepted the actual shader,
  texture upload, mip chain, bindings and 12 pipeline combinations: forward and
  reversed depth, 1×/4× MSAA, and one/two/three color attachments.

- Pipeline checks also require depth writes to be disabled and source-alpha
  blending to be enabled. Extracted dispatch checks cover one submission per view
  and discarding stale batches when a translucent pass is skipped.
- Geometry tests cover all 18 facing directions with each possible local disc
  axis, native attachment offsets, unchanged scale/distances, buried platform
  tops and foundations with the native mirror height preserved,
  artwork sizing from the original dimensions and invalid bounds. Additional tilted-disc tests cover
  all 18 directions, the authored front, raised rims, plane alignment at all
  four corners and fixed inward-facing visibility.
- Extracted prompt-function checks cover all 18 approach positions, separated
  trigger areas, mirror readiness, vertical bounds and both center floor portals.
- An extracted spawn-function harness covers delayed loads, single-slot retries,
  missing actors and keeping exactly 18 mirrors plus two center floor portals.

Run the geometry test with:

```sh
c++ -std=c++20 tests/boss_mirror_geometry_test.cpp -o /tmp/mirror-test
/tmp/mirror-test
```

The validation backend does not load game archives or render a game scene.
In-game verification is still needed for the vanilla stand/disc fit and ground
placement, distance visibility, first hub entry, Midna prompts, saved colors and returns.
