# Survivors Roundtable hub

The Survivors hub and its title scene use an original, simplified interpretation
of the Roundtable Hold: a sixteen-sided stone hall, round walnut table, twelve
high-backed chairs, carved arches, drum columns, a ribbed ceiling, stylized
memorials and a west-wall fireplace. Ceiling banners and their rods are removed. This is new geometry and
an original generated material atlas, not extracted Elden Ring assets.

![Offline render of the exported room geometry](../images/survivors-roundtable.jpg)

This preview renders the shipped mesh and texture. It is **not an in-game
screenshot**: Link, Zelda, the equipment chest and the floating golden flame
are added by the game. The former central ceremonial bowl has been removed. Fireplace fire
and point lighting are native effects; coals are simple mesh details. The handmade
table candle holders have been removed, leaving the floating centre flame. The hall combines baked vertex lighting with a static soft-shadow lightmap
cast from the central flame and weaker fireplace onto the floor, furniture and
architecture. These shadows follow the fixed lights, not individual moving particles.

## Layout and integration

- Main hall radius: 1,800 game units (the compact build-1295 layout); floor:
  Y = -400. The exit now sits directly at the south hall edge,
  Z = 1815.4135 / Y = -400. The long rising corridor is removed.
- The table is centered at X/Z = 0, with twelve chairs, including the front row.
  Link rests at Z = 760, outside the chair ring; Zelda stands diagonally in front of the west fireplace at X/Z = -1170/430
  and faces the centre. The title camera follows the relocated rest position.
  Zelda's sword gift, talents and exit warning remain intact.
- A tall golden native flame with sparks floats above the tabletop at Y = -220.
  Its warm point light is centered at Y = -50. Five light samples bake soft
  shadows from the actual visible geometry into a 1024×1024 I8 lightmap;
  collision-only furniture proxies do not cast artificial box shadows.
  Ambient light remains visible within shadows. The fireplace is a second,
  local source with a maximum 22% contribution before normalization; its
  weaker shadows fade with distance and either light can fill the other's shadow.
- The fireplace has a stepped hearth, fluted jambs and corbels, a carved mantel
  frieze and cornice, recessed chimney panel, soot-darkened masonry firebox,
  an iron grate, crossed faceted logs and small ember clusters. Five overlapping
  native flames with varied heights replace the two isolated tiny fires.
  Its point light has an 850-unit range versus the table flame's 1,800.

- The table is 100.5 units high and 361.8 units in radius; chair seats are
  72.05 units high (including cushions) and the full chair height is 187. Table settings sit on the tabletop. Memorial plinths extend to the floor.
- Wall bays alternate between empty walnut shelves, three-tier arched stone
  niches and layered panels with projecting piers, corbels and relief bands.
  Books, vessels and stacks have been removed from all shelves and niches.
  Perimeter clutter uses a fixed random seed, unequal cluster sizes, different
  mixtures and independently rotated/scaled papers, splinters and stone chips.
  Interaction anchors remain clear; clutter is non-solid.
- The floor follows the reference's smaller rectangular stone flags: staggered
  courses 55–82 units deep, unequal widths 65–135 units, independently chipped
  corners and fine gray-brown mortar. The oversized polygon slabs and circular
  inlay are removed. Four stone surfaces retain independent crops/rotations.
- Room ambient shading is substantially darker, with cool gray-brown stone and
  localized amber pools around the central flame and weaker hearth. Bare floor
  and shaft limestone is balanced against the darker masonry albedo, instead
  of leaving chalk-white columns beside brown walls. This changes only the
  authored room's vertex colours; native actor illumination and collision stay
  intact. Flush paving receives shadows without casting onto its own mortar.
- Low fireplace undersides no longer enter collision. Native Acch checks roofs
  after gravity and before the ground ray: the thin hearth slabs previously
  became false ceilings beneath Link and could hide the supporting floor.
  The stepped tops and side walls remain solid. New shelf/cabinet bases also
  omit their bottom collider faces.
- Floor brightness is explicitly 255 (inherit native room lighting), rather
  than 0, which multiplies actor ambient light to zero. This fixes the black
  Link silhouette away from point lights without changing global lighting.
  Low ambient lighting keeps recesses dark while local firelight defines the room;
  one planar UV projection per irregular slab avoids diagonal texture seams.
- A detailed stone portal frames the exit with grounded bases, fluted jambs,
  joint bands, capitals, two segmented archivolts, a keystone and a carved
  cornice with bronze insets. Solid wings and a lintel enclose the shutter.
  A flat 300-unit pocket behind the door supports the native event goals;
  there is no corridor to walk down before reaching the door. The stage-actor
  hook relocates only the captured south-door spawn before native construction,
  so its matrices, collision and cached event positions use the new anchor.
  Door parameters, yaw, interactions and normal combat doors are preserved.
- Recessed wall panels and arch pilasters collide at their visible front faces.
  The table has a continuous collision skirt to the floor, and chairs have
  full seat/body and back volumes. These cover Link's native +30.01-unit wall
  check, which previously passed below the tabletops/seats between thin legs.
  The upper collision surfaces match the tabletop and chair cushion heights.
- Masonry uses a coarser 320-unit repeat for legible block courses; bare stone,
  wood and other materials retain a 200-unit reference scale. Shaft UVs wrap
  around the circumference and shift independently between stone drums. Repeats are split into triangles before export, with atlas
  padding to prevent sampling adjacent materials or reserved emissive texels.
  The collider retains unsplit surfaces and does not grow with texture repeats.
- The equipment chest uses the authored east-wall anchor at X = 1640.4135.
  It no longer traces from inside the central table; its ground probe rejects
  anything more than 30 units above/below the room floor. Collision tests cover
  its approach, Zelda's position, the rest position and the route to the exit.
- The room packet is queued in the background opaque pass, before native
  shadow projection. The previous actor-opaque queue ran after shadows and
  overpainted Link's and Zelda's silhouettes. Native moving actor shadows
  remain responsible for their pose, equipment and lighting direction.
- Only completed room-51 BG actors created for the Survivors hub/title acquire
  the replacement. Their actor IDs retain ownership until deletion; switching
  game modes cannot convert outgoing rooms or normal combat rooms.
- Native stage props are filtered for this hub; the original door actors and
  scene infrastructure stay. Standalone hub decorations are unaffected.
- New collision surfaces explicitly carry no native room-camera/path references,
  preventing them from selecting camera or path record zero of the dungeon.
- Original archive data and native room models remain untouched. Native room
  creation completes first; a private native DZB replaces its registered
  collision atomically. A custom J3D packet renders the hall. Missing resources,
  allocation failure or a full collision registry leave the original room.
- Deletion restores native pointers/registration before native cleanup. Disabling
  the mod also restores the original room; no custom collider vtable or particle
  callback remains attached. Draw packets are removed before their storage dies.

![Offline fireplace geometry; animated flames are supplied by the game](../images/survivors-fireplace.jpg)

The normal **Darknut Arena combat map** keeps its original geometry. No touch UI,
save format, item reward or title input timing is changed.

![Offline close-up of empty shelves and floor clutter](../images/survivors-wall-bays.jpg)

![Offline view of smaller staggered rectangular flags](../images/survivors-floor.jpg)

![Offline portal; the native shutter is supplied by the game](../images/survivors-entry.jpg)

## Reproduction

```sh
python3 tools/generate_roundtable.py --texture tools/assets/roundtable_materials.png
python3 tools/bake_roundtable_shadows.py
python3 tests/survivors_roundtable_entry_test.py
CXX=clang++ python3 tests/survivors_roundtable_entry_test.py
python3 tests/survivors_hearth_floor_test.py
python3 tests/survivors_roundtable_test.py
python3 tests/survivors_roundtable_shadows_test.py
python3 tests/survivors_roundtable_runtime_test.py
CXX=clang++ python3 tests/survivors_roundtable_runtime_test.py
python3 tools/preview_roundtable.py /tmp/roundtable.png
```

The generator emits a little-endian vertex stream, big-endian native DZB and a
1024×1024 tiled RGB565 atlas. The shadow baker needs NumPy and a C++20 compiler;
its output is a GX-tiled I8 lightmap with a second UV set in the RTH2 mesh. Geometry generation and tests need only Python's
standard library; texture conversion needs Pillow and the optional preview
needs Pillow plus NumPy. Assets are checked in, so builds never need generation
tools or network image generation. There are 44,677 visual triangles and 5,772
collision triangles, with a balanced collision tree. Resources total about 7.2 MiB
and are cached until mod shutdown; each live room uses a private 2 MiB heap.

The runtime fixture executes the production implementation with a fake host,
ASan/UBSan and both compilers: partial/failed creation, 30 room lifetimes,
selection changes, repeated retained draws, native particle/light cleanup,
deletion and mod shutdown. Geometry tests verify reproducibility, native table
bounds/tree ancestry, winding, inherited floor lighting, furniture heights,
grounded memorial plinths, front-facing wall/seat/table blocking at native
body-check heights, clear door-event paths and uniform wall/column UV density.
The entry fixture executes the production stage hook with GCC and Clang,
covering hub/title scoping, all supported door profiles, exact anchor matching,
unchanged actor metadata and replay safety. Geometry probes check both event
approaches, portal walls/backstop and the absence of old corridor ground and
high cloth banners.
The hearth regression follows 630 approach/retreat steps with gravity and the
native roof-before-ground query order; it fails on the previous exported collider.
The shadow baker shares one receiver chart across each convex stone slab or quad,
retaining a 24-unit sample spacing within the 1024-square atlas.
The runtime test checks that every vertex is submitted across multiple GX batches.
Shadow tests validate source/asset fingerprints, intensity range, the atlas format,
ray occlusion, double-sided blockers, self-shadow bias and the weaker secondary
light. The runtime fixture rejects scheduling room geometry in the actor pass.

In-game visual and native-door validation still requires a playable installation:
load a new/existing save, finish Zelda's gift, open talents/equipment, start each
run map and return, repeatedly switch Play/Survivors, then check both the moving
title camera and the 0.7-second save arrival. The offline preview and fake-host
tests cannot establish the exact appearance or door-event timing on a device.

## Material source

Image generation mode: built-in image generation, new opaque image. The original
output is preserved in `tools/assets/roundtable_materials.png` (1254×1254);
`materials.rgb565` packs its four materials together with four bare-stone variants
from `tools/assets/roundtable_stone_variants.png` into one 1024×1024 texture.
Masonry keeps its 512-square region; the other materials occupy padded rectangles.
No image service is needed to reproduce builds. Original material prompt:

> Use case: stylized-concept. Asset type: original high-detail diffuse game material texture atlas for a medieval circular hall inspired by a warm ruined castle library. Produce a square 1024x1024 bitmap, exactly FOUR equally sized square quadrants, touching at the exact centre, no gutters or labels. Top-left quadrant: weathered warm grey-brown limestone ashlar masonry, varied medium-small rectangular stone courses, crisp fine pits, chipped edges, aged mortar, subtle soot and hairline cracks, delicate carved incisions on a few stones. Top-right: old dark walnut planks, narrow grain, worn scratches, muted aged wood. Bottom-left: deep muted burgundy cloth with fine woven fabric, subtle faded patches. Bottom-right: dark aged bronze with fine understated engraved scroll motifs and patina. Each quadrant a perfectly FLAT orthographic seamlessly tileable material scan, no objects, no perspective, no directional lighting or cast shadows, evenly lit albedo, no text, no border, no watermark. Restrained natural palette, detailed surfaces readable at close range without high-contrast noise. This is a production texture atlas, NOT an image of a room.


Additional stone asset: built-in image generation, new opaque image, saved as
`tools/assets/roundtable_stone_variants.png`. Exact prompt:

> Use case: stylized-concept. Asset type: production diffuse stone texture atlas for a medieval game floor and carved columns. Create one square image with exactly four equal square quadrants, touching at the centre, no gutters. Each quadrant shows a DIFFERENT seamless close-up of a single continuous worn limestone surface, not paving: top-left warm grey limestone with subtle mineral clouds and tiny pores; top-right muted sandy taupe stone with faint thin veins and smooth foot-polished patches; bottom-left cooler grey-brown stone with gentle irregular mottling and a few fine hairline cracks; bottom-right warm ash-grey stone with scattered tiny chips and softened scratches. Keep all four close in brightness and restrained in colour, moderately light rather than black. Fine readable realistic game surface detail, broad natural variation without conspicuous repeated spots. Perfectly flat orthographic albedo scan, even lighting. NO bricks, NO tiles, NO mortar, NO grid, NO borders, NO floor scene, NO perspective, NO cast shadows, NO objects, NO text, NO watermark. Each quadrant itself must tile seamlessly. Original asset.
