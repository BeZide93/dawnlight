# Survivors Roundtable hub

The Survivors hub and its title scene use an original, simplified interpretation
of the Roundtable Hold: a sixteen-sided stone hall, round walnut table, nine
high-backed chairs, carved arches, drum columns, a ribbed ceiling, burgundy
banners, stylized memorials and a west-wall fireplace. This is new geometry and
an original generated material atlas, not extracted Elden Ring assets.

![Offline render of the exported room geometry](../images/survivors-roundtable.jpg)

This preview renders the shipped mesh and texture. It is **not an in-game
screenshot**: Link, Zelda, the existing ceremonial bowl, its shadow, the
equipment chest and native fire particles are added by the game. Fireplace fire
and point lighting are native effects; candle flames and coals are simple mesh
details. The hall uses baked vertex lighting rather than dynamic room shadows.

## Layout and integration

- Main hall radius: 1,800 game units; floor: Y = -400. The south approach reaches
  the existing doorway at Z = 3,900 / Y = -250 via a gentle ramp.
- Rest pose, title camera, Zelda and brazier anchors stay unchanged. The round
  table sits just behind the fire/rest area, centered at Z = -730, with the front
  seating bay open. Zelda's sword gift, talents and exit warning remain intact.
- The equipment chest still probes the east wall. Collision tests cover its
  approach, Zelda's position, the rest position and the route to the exit.
- Only completed room-51 BG actors created for the Survivors hub/title acquire
  the replacement. Their actor IDs retain ownership until deletion; switching
  game modes cannot convert outgoing rooms or normal combat rooms.
- Native stage props are filtered for this hub; the original door actors and
  scene infrastructure stay. Standalone hub decorations are unaffected.
- Original archive data and native room models remain untouched. Native room
  creation completes first; a private native DZB replaces its registered
  collision atomically. A custom J3D packet renders the hall. Missing resources,
  allocation failure or a full collision registry leave the original room.
- Deletion restores native pointers/registration before native cleanup. Disabling
  the mod also restores the original room; no custom collider vtable or particle
  callback remains attached. Draw packets are removed before their storage dies.

The normal **Darknut Arena combat map** keeps its original geometry. No touch UI,
save format, item reward or title input timing is changed.

## Reproduction

```sh
python3 tools/generate_roundtable.py --texture tools/assets/roundtable_materials.png
python3 tests/survivors_roundtable_test.py
python3 tests/survivors_roundtable_runtime_test.py
CXX=clang++ python3 tests/survivors_roundtable_runtime_test.py
python3 tools/preview_roundtable.py /tmp/roundtable.png
```

The generator emits a little-endian vertex stream, big-endian native DZB and a
256×256 tiled RGB565 atlas. Geometry generation and tests need only Python's
standard library; texture conversion needs Pillow and the optional preview
needs Pillow plus NumPy. Assets are checked in, so builds never need generation
tools or network image generation. There are 12,061 visual triangles and 3,702
collision triangles, with a balanced collision tree. Resources total about 1 MiB
and are cached until mod shutdown; each live room uses a private 2 MiB heap.

The runtime fixture executes the production implementation with a fake host,
ASan/UBSan and both compilers: partial/failed creation, 30 room lifetimes,
selection changes, repeated retained draws, native particle/light cleanup,
deletion and mod shutdown. Geometry tests verify reproducibility, native table
bounds/tree ancestry, winding, surface heights and interaction paths.

In-game visual and native-door validation still requires a playable installation:
load a new/existing save, finish Zelda's gift, open talents/equipment, start each
run map and return, repeatedly switch Play/Survivors, then check both the moving
title camera and the 0.7-second save arrival. The offline preview and fake-host
tests cannot establish the exact appearance or door-event timing on a device.

## Material source

Image generation mode: new opaque image. The checked-in source is a 256×256
downsample of the generated atlas; `materials.rgb565` is its mechanical encoding.
Prompt:

> Create an original game environment material texture atlas, square 1024x1024
> image, opaque. Exactly four equally sized square quadrants, no gaps or frames.
> Top-left: warm grey-brown worn medieval limestone ashlar blocks with subtle
> fine cracks, joints, rough pores. Top-right: dark aged walnut wood grain,
> boards, restrained scratches, no furniture. Bottom-left: muted deep burgundy
> woven cloth, slightly faded, evenly spread weave, no folds. Bottom-right:
> aged warm bronze metal surface, delicate engraved concentric scrolls, gentle
> patina. Each quadrant perfectly flat orthographic material scan, no
> perspective, no cast shadows, no directional highlights, diffuse albedo only.
> Consistent low contrast, natural realistic painted game texture, muted warm
> palette, no letters, no text, no objects, no logos. These textures will be
> tiled over a newly authored low-poly medieval fantasy hall.
