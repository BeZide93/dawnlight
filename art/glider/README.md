# Dawnlight glider

## Bundled default BMD

`res/DawnlightGlider.bmd` is an exact copy of
[`BeZide93/DawnlightCustomGlider`](https://github.com/BeZide93/DawnlightCustomGlider/tree/3d1cea0f4f6a8a66550fb8edd93f0f34976e0255)
commit `3d1cea0f4f6a8a66550fb8edd93f0f34976e0255`, root file `DawnlightGlider.bmd`
(927,520 bytes; SHA-256
`c03615be7649b8e1bc53ddbeecb0775e801da007f4be48452a7fc3d4bd984aa9`).
It includes the model, materials, and all seven mip levels of the Canvas, Wood,
and Leather textures. The matching OBJ/MTL, PNGs, and material/export settings
remain in that repository's `source/` directory; the Blender project is at its root.

Normal builds package the BMD through `RES_DIR res`; no converter or download
is required. Dawnlight loads an external DVD overlay first, then this private
bundled resource if the overlay is absent or cannot load. Both gliding and the
item-get animation use the selected BMD. The original generated GX mesh below
is retained only as an emergency fallback if neither BMD can load.

To update the default, copy the upstream BMD into `res/DawnlightGlider.bmd`,
update this source revision/checksum, and verify its embedded textures. Running
`tools/generate_glider.py` changes only the emergency mesh, not the default BMD.

## Original emergency mesh

Original Dawnlight mesh. The canopy artwork comes from the project logo supplied
by the user (`1001012593.png`), with only the large Dawnlight title removed using
the built-in image editor. The original image and runtime atlas are unchanged;
mesh UVs enlarge the crest and runtime lighting keeps the decoration readable.
No model or texture was extracted from another game or mod.

- `dawnlight-glider.obj`: 1,792 triangles, Y up, Z forward, origin at the midpoint
  of the carrying hands. Canvas wingspan 200 units; peak height 56 units.
  The swept leading bow is thicker; the cloth has an angular, scalloped trailing
  edge matching the inventory illustration. Two separate bows connect the sail's front and rear. Leather grips run along
  Z = -11 to +11 at X = +/-22, rising from Y = -2 to +2 (~10 degrees).
- `dawnlight-glider.mtl`: material referencing the PNG atlas.
- `dawnlight-canopy-source.png`: original supplied logo/background,
  without the title; the complete edited image is the editable texture source.
- `dawnlight-glider.png`: 256x256 runtime atlas: full artwork in the canvas region,
  separate wood/leather strips below it. No colored panel or substitute symbol
  is drawn over the supplied artwork. The canvas UVs turn the artwork 180 degrees
  and compensate for the wide sail so the central crest retains its proportions.
  UVs frame source Y=10.8–63.5%, making the crest cover about 95% of sail depth.
  The runtime ambient tint retains at least 160/255 brightness per channel.
  Canvas is two-sided at runtime.

Original image-edit prompt: remove only the large gold word “Dawnlight”;
reconstruct the stone/ornament behind the letters; preserve the crest, its
position/size, colors, background, runes and frame. Keep an opaque square image
without new elements. No further image editing is needed for the UV enlargement.

## Replacing the texture without rebuilding

The default BMD has three separate textures. Download the matching PNG from
[the pinned source directory](https://github.com/BeZide93/DawnlightCustomGlider/tree/3d1cea0f4f6a8a66550fb8edd93f0f34976e0255/source),
edit a copy, and use the filename below:

| Source PNG | Replacement filename |
| --- | --- |
| `Canvas.png` | `tex1_256x256_500b43cdfd40fa52_4.png` |
| `Wood.png` | `tex1_512x256_36018d9e1ea9b592_4.png` |
| `Leather.png` | `tex1_512x256_e11c67725590fd8d_4.png` |

Place the replacements in `<Dusklight user data>/texture_replacements/`, enable
**Texture Replacements**, and restart or toggle that setting off/on to rescan.
Preserve each image's layout and aspect ratio; higher-resolution replacements
keep the same filename. Wood and leather now use their own textures, so editing
the bottom strips of the old canopy atlas does not change the BMD's frame/grips.

These hashes are XXH64 (seed 0) of each GX-tiled RGB565 base mip. They are
specific to the bundled BMD; external packs may have different texture hashes.
The emergency mesh still uses the old `dawnlight-glider.png` atlas and its canopy
hash. An enabled mod replacing the same texture takes priority over this folder.

`python3 tools/generate_glider.py` regenerates the editable assets and
`src/generated/glider_art.hpp`. This developer tool needs Pillow. Normal builds
need neither Pillow nor a model converter. The checked-in header embeds the
same mesh and texture (GX RGB565, seven mip levels) directly in the mod binary;
no room archive or external installation is required.

The emergency model uses a native J3D draw packet/GX textured mesh, as the custom Shade
pedestal does, rather than requiring a BMD exporter. Each packet render samples Link's
presented sword/shield attachment matrices and root rotation so the canopy follows
the same interpolation as the player and camera. The grip midpoint uses the item
joints inside the palms, not the wrist joints. The host matrix lookup is resolved
through the symbol manifest, with a logged simulation-pose fallback if absent.
Link's actor draw only queues the packet during simulation updates; `fpcLf_Draw`
skips that actor on intermediate frames. Pose sampling therefore happens inside
the retained packet's `draw()`, when presentation replacements are active. The
packet keeps an actor ID and resolves it at render time, avoiding stale pointers.
While the custom glider is attached, a post-hook on `setDrawHand` selects Link's
native sword/shield grip shapes (materials 0/6). The next native hand draw restores
the normal shapes after landing or switching items. The overhead arm animation
and native glide physics still drive movement. The GLIDE-owned Cucco exists internally and is
hidden/muted only for the Glider selection. Its feather emitter is cleared after
native execution and before retirement, including in Ordon Village; ordinary
Cuccos and the Cucco selection keep their feather effects. No world Cucco is replaced.

In-game QA: check deployment during a manual jump and an ordinary fall; camera
above/below; both turn directions; switching Glide Item while airborne; landing,
damage, disabling Glide, cutscenes and room changes; ordinary carried Cuccos.
Also check interpolation off/on and turning at higher presentation frame rates,
plus closed hands while gliding and normal hands after landing or item switching.
The mesh preview and automated tests do not replace this runtime visual check.


## Glider acquisition icon

`glider-item-icon.png` is a separate 256x256 transparent, hand-painted-style
inventory illustration created with the built-in Imagegen tool. It is used only
for Dawnlight's Glider acquisition message, not for the 3D canopy texture.
`python tools/generate_glider_item_icon.py` downsamples it to 128x128 and writes
`res/glider-item-icon.rgba8` in GX RGBA8 tile order. The checked-in resource is
bundled by normal builds; Pillow is only needed to regenerate it. The native
J2D message keeps its usual 48px layout footprint, placement and fade animation.

Generation prompt:

> Use case: stylized-concept. Create one production-ready transparent 2D inventory item icon for a Zelda Twilight Princess game mod, square 1024x1024 PNG with genuine alpha transparency. Subject: a handheld fantasy paraglider, seen in three-quarter perspective from slightly above, entire glider isolated and centered with safe transparent margins. Wide slightly curved dark charcoal/brown cloth canopy, wooden rim and spars, TWO separate wooden/leather grips hanging below left and right, each running front-to-back. On the canopy a clear golden sunrise above a small horizon framed below by a blue crescent with golden edging; no writing. Twilight Princess inventory-icon look: hand-painted muted earthy colors, simplified strong readable silhouette, chunky dark outline and thin warm ivory outer edging like the bow item icon, crisp highlights and restrained shading. Must remain readable as a tiny 48 to 96 pixel icon. No Link, no hands, no character, no background scene, no UI window, no text, no bow, no arrows, no drop shadow beyond icon outline. Glider fills about 85 percent of width and 70 percent of height. Save the resulting PNG for use as a repository asset.


## Optional BMD model via a separate `.dusk`

Dawnlight loads **`/res/Object/DawnlightGlider.bmd`** on first Glider use.
A data-only overlay mod can supply this file without replacing Dawnlight or a
vanilla game actor. The BMD is used for both gliding and the item-get animation;
the latter automatically uses the same 45% presentation scale as the built-in
model. The acquisition icon remains the separate 2D illustration.

From the repository root:

```sh
python3 tools/package_glider_model.py MyGlider.bmd MyGlider.dusk
```

Install and enable the resulting `.dusk` alongside Dawnlight, then restart the
game. The packer uses only Python's standard library and preserves the BMD bytes.
Use `--id your.unique.mod_id --name "My Glider"` for a separately named pack.
The archive contains `mod.json` and
`overlay/res/Object/DawnlightGlider.bmd`; it contains no platform binary.
Dusklight's normal overlay priority applies if multiple packs supply this path.
After changing, enabling, or disabling a model pack, restart the game: the
model is cached for the Dawnlight session and is not hot-reloaded mid-draw.

Model requirements:

- Export a valid, uncompressed Twilight Princess **BMD3** (`J3D2bmd3`), up to
  8 MiB, with the standard eight sections. BDL, BMD1/2, archives, and compressed
  files are not accepted. Container checks catch wrong formats, truncation,
  missing sections, and empty geometry; they are not a full BMD validator.
- Embed the model's textures/materials in the BMD (TEX1). External BTIs, BCK/BRK
  animations, and replacement of Link's skeleton are not loaded. The model is
  rendered in its rest pose with its own materials and Link's room lighting.
- Use the editable `dawnlight-glider.obj` as the size/orientation reference:
  **Y up, Z forward**, origin at the midpoint of the hands, wingspan 200 units.
  The two grip centerlines are X = -22 / +22, from (Y=-2, Z=-11) to
  (Y=2, Z=11). Apply object transforms before export. Make the cloth two-sided
  in the exported material if it must remain visible from below.
- Keep the same coordinate system for the full-size model; Dawnlight supplies
  translation, yaw, and reward scale. There is no per-pack transform setting.

Without a usable overlay, the bundled DawnlightCustomGlider BMD is used. A failed
file read, rejected container, or failed model creation logs a warning and tries
the bundled BMD. Only a failure of that resource uses the emergency GX mesh.
The file and native J3D allocations stay in a dedicated heap across room changes
and are freed at mod shutdown. Opaque/translucent materials draw into private
lists; the world lists, camera, and J3D state are restored after each packet.

Runtime QA still needed with an exported replacement BMD: load a pack on each
supported host, glide and turn at interpolated frame rates, acquire the Glider,
change rooms, and restart after disabling the pack to verify the bundled BMD fallback.
Automated tests cover container rejection, exact packaging, render-state isolation,
and the existing attachment/presentation behavior; they do not run the game.

## Icon/model alignment update

The canopy source PNG, atlas PNG, embedded GX texture bytes, and complete original
3D grip assemblies remain unchanged. Only the cloth/frame geometry and the
separate inventory icon were edited. The icon's grips now run fore/aft in two
wooden U-shaped bows. The editable icon is `glider-item-icon.png`; the generated
128x128 GX resource is `res/glider-item-icon.rgba8` at the repository root.

Icon edit used the built-in Imagegen tool with the old icon as edit target and a
plain render of the original mesh as a grip-geometry reference. Final prompt:

> Use case: precise-object-edit. Edit target: image 1, the existing transparent Dawnlight glider inventory icon. Reference only: image 2, a 3D mesh preview showing the correct handle geometry. Change ONLY the two grips and their supports under the canopy in image 1. Replace its transverse bicycle-style handles with the pair of narrow longitudinal U-shaped wooden bows in image 2: each grip runs FRONT TO BACK in the depth direction of the glider, not sideways across its width; two independent parallel bows, with dark leather wrapped bottom segments, left and right, joined to the front and rear of the sail. The model preview is geometry reference only, retain the hand-painted Twilight Princess item-icon style of image 1, dark outlines and warm ivory edging. Keep image 1's exact wide arched wooden upper rim, angular scalloped dark cloth canopy, gold sun and blue crescent emblem, colors, perspective, framing and scale unchanged. No new ornaments. No text. No person. Genuine transparent PNG background including the holes through the two handle loops. Output one square inventory icon, entire object inside the frame. Do not use the plain canopy of image 2.
