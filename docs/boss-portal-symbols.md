# Boss portal symbols

Each of the 18 boss and miniboss destinations in the Boss Rush hub is a complete
Mirror of Twilight from the Mirror Chamber. It replaces that boss's floor portal.
An original abstract emblem sits on the mirror face: blue means undefeated, red
means defeated, using the existing persistent save flags. The center Boss Rush
and Cave of Ordeals floor portals remain available.

![Original emblem artwork](../art/boss-portal-symbols.svg)

## Artwork and provenance

`art/boss-portal-symbols.svg` is the editable source, newly drawn for Dawnlight.
Curved paths, rounded strokes and a shared broken-ring frame give the symbols a
consistent appearance. The motifs refer to weapons, elements or broad creature
shapes; they contain no extracted game textures, model renders or traced game
art. The artwork, generator and renderer were written for this change. No code
or assets were taken from Twilit Essentials or PR #23.

The SVG's 18 symbols follow `kBossRushEntries` in `src/new_save_modes.cpp`:
boomerang, plant, rock, flame/chains, toad, tentacles, spectral sword, skull,
flail, ice crystal, armor, spider, winged sword, dragon, mask, marionette,
boar and crowned sword. The generator checks the names and ordering.

## Regenerate

Normal builds use the checked-in generated header and require no graphics tools.
To edit the artwork and regenerate the embedded mask:

```sh
python -m pip install CairoSVG==2.9.1 Pillow==12.3.0
python tools/generate_boss_symbols.py --preview /tmp/boss-symbols.png
```

The generator rasterizes each vector at 1024×1024 and downsamples to 256×256
with Lanczos filtering. The atlas contains only alpha, so both state colors use
the exact same shape. A bounded RLE decoder reads the embedded atlas. Base64
strings are split into small chunks for MSVC compatibility. The runtime creates
five mip levels and uses linear filtering for smooth edges at different distances.
Only this original artwork is added to the packaged mod.

## Mirrors and rendering

A dedicated `DLBMir` ActorService profile loads the complete `u_mr_mirror` model
from the player's `MR-Table` archive at runtime. No Nintendo model or texture is
included in the mod. The actor uses the engine's shared archive references and
per-actor model heaps. It does not run Mirror Chamber switches, stairs, effects,
cutscenes, or the native reflection actor's singleton logic.

All 18 mirrors face the hub center. Their 440-unit diameter fits the existing
18-place ring. The renderer measures the loaded rigid mesh, compensates its
bind-pose root/pivot and mounts the model and emblem in the same world frame.
The symbol face is about 361 units across, compared with the previous 240-unit
billboard. A nearly opaque dark circular face and slightly strengthened strokes
improve distance contrast. The image stays fixed to the inward-facing front. Looking from behind hides the
symbol instead of moving it onto the back. The original stone rim remains visible.

The original model is rotated 180 degrees around its up axis so its authored front
faces the hub center. Its baked-in tilt is preserved. The symbol plane is fitted
to the loaded mesh's broad front surface using its vertex positions and authored
normals, rather than the axis-aligned bounding box. Geometric plane candidates
from inner vertices also handle smoothed bevel normals. Raised rim vertices are
downweighted and rear planes are excluded. All four symbol corners use this same
plane with a fixed 0.4-unit normal offset to avoid z-fighting. No camera-dependent
rotation, flipping or front/back relocation is applied.

The existing GfxService renderer draws the emblem faces with scene depth testing
and depth writes. Transparent corners are discarded; later translucent effects
cannot paint through the solid mirror face. Pipelines match MSAA, reversed depth
and deferred render layouts. It never edits the model's materials or GX state.
GPU resources initialize lazily and are released on shutdown. ActorService drains
the mirror actors before removing their profile.

Hub spawning retains live and asynchronously loading IDs, retries missing slots
individually and attempts at most one destination per tick. Midna's existing
Fight / Yes / No prompt only becomes available once its mirror is ready. Hub
supplies, boss entry logic, saved progress and both center portals retain their
existing behavior.

## Validation

- Local Linux release-with-debug-info build and `.dusk` packaging passed.
- C++ atlas decoding was compared byte for byte with the SVG generator output.
- All 18 artwork names and portal indices match.
- The pinned Dawn library's validation backend accepted the actual shader,
  texture upload, mip chain, bindings and 12 pipeline combinations: forward and
  reversed depth, 1×/4× MSAA, and one/two/three color attachments.

- Geometry tests cover all 18 facing directions with each possible local disc
  axis, offset pivots and invalid bounds. Additional tilted-disc tests cover
  all 18 directions, the authored front, raised rims, plane alignment at all
  four corners and fixed inward-facing visibility.
- An extracted spawn-function harness covers delayed loads, single-slot retries,
  missing actors and keeping exactly 18 mirrors plus two center floor portals.

Run the geometry test with:

```sh
c++ -std=c++20 tests/boss_mirror_geometry_test.cpp -o /tmp/mirror-test
/tmp/mirror-test
```

The validation backend does not load game archives or render a game scene.
In-game verification is still needed for the original model's fit, visibility
from a distance, first hub entry, Midna prompts, defeat/reload colors and returns.
