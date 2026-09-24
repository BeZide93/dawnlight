# Dawnlight glider

Original Dawnlight mesh. The canopy artwork comes from the project logo supplied
by the user (`1001012593.png`), with only the large Dawnlight title removed using
the built-in image editor. The original image and runtime atlas are unchanged;
mesh UVs enlarge the crest and runtime lighting keeps the decoration readable.
No model or texture was extracted from another game or mod.

- `dawnlight-glider.obj`: 1,712 triangles, Y up, Z forward, origin at the midpoint
  of the carrying hands. Canvas wingspan 200 units; peak height 56 units.
  Two separate bows connect the sail's front and rear. Leather grips run along
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

1. Download `dawnlight-glider.png` from this directory and edit a copy. Keep the
   square atlas layout: canopy rows 0–207, wood 208–231, leather 232–255.
   A higher-resolution square PNG works too if every region scales proportionally.
2. Name it **`tex1_256x256_500b43cdfd40fa52_4.png`**, even when using a larger
   image. The filename identifies the original 256×256 RGB565 texture.
3. Place it in **`<Dusklight user data>/texture_replacements/`**. This is the
   host's data folder, not Dawnlight's `.dusk` archive or the game ISO.
4. Enable Dusklight's **Texture Replacements** setting. Restart the game, or
   switch the setting off and on to reload the directory after editing files.

The mesh rotates the canopy art 180° and samples only source Y=10.8–63.5%; keep
the emblem inside that range. Left/right background panels use different UV
spacing to preserve the crest's proportions on the wide sail. Wood and leather
use the bottom atlas strips separately. Changes affect both sides of the canvas.

The hash above is XXH64 (seed 0) of the embedded, GX-tiled RGB565 **base mip**,
not of the PNG file or the whole mip chain. It is specific to this artwork.
Do not use a wildcard hash: it could replace unrelated 256×256 game textures.
An enabled mod that replaces the same texture takes priority over this folder.

`python3 tools/generate_glider.py` regenerates the editable assets and
`src/generated/glider_art.hpp`. This developer tool needs Pillow. Normal builds
need neither Pillow nor a model converter. The checked-in header embeds the
same mesh and texture (GX RGB565, seven mip levels) directly in the mod binary;
no room archive or external installation is required.

The runtime uses a native J3D draw packet/GX textured mesh, as the custom Shade
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
hidden/muted only for the Glider selection. No world Cucco is replaced.

In-game QA: check deployment during a manual jump and an ordinary fall; camera
above/below; both turn directions; switching Glide Item while airborne; landing,
damage, disabling Glide, cutscenes and room changes; ordinary carried Cuccos.
Also check interpolation off/on and turning at higher presentation frame rates,
plus closed hands while gliding and normal hands after landing or item switching.
The mesh preview and automated tests do not replace this runtime visual check.
