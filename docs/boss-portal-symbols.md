# Boss portal symbols

Each of the 18 boss and miniboss portals in the main-branch Boss Rush hub has an
original abstract emblem. The emblems float above the live portals and face the
camera. Blue means undefeated; red means defeated, using the same persistent
save flags as the portals themselves.

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

## Rendering and lifecycle

The renderer registers a GfxService scene hook and creates its GPU resources
lazily when a live hub portal is visible. It does not modify game model materials
or GX state. One batch draws camera-facing quads with a subtle vertical bob,
normal scene depth testing and no depth writes. Transparent quads are sorted
back to front. Other stages, transitions and menus do not draw these symbols.

Pipelines match the current scene layout, including MSAA, forward/reversed depth
and additional render targets used by deferred rendering. Only scene color is
written. Game state and vertex generation stay on the game thread; the render
worker receives a small immutable batch and only calls WebGPU. Resources are
released during mod shutdown.

The center Boss Rush and Cave of Ordeals portals, portal prompts, hub supplies,
boss entry logic and save format retain their main-branch behavior.

## Validation

- Local Linux release-with-debug-info build and `.dusk` packaging passed.
- C++ atlas decoding was compared byte for byte with the SVG generator output.
- All 18 artwork names and portal indices match.
- The pinned Dawn library's validation backend accepted the actual shader,
  texture upload, mip chain, bindings and 12 pipeline combinations: forward and
  reversed depth, 1×/4× MSAA, and one/two/three color attachments.

The validation backend does not render a game scene. In-game verification is
still needed: first entry into a fresh Boss Rush save, defeat/reload color
persistence, camera occlusion, re-entry from a boss and graphics setting changes.
