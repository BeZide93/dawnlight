# Dawnlight glider

Original Dawnlight mesh. The canopy artwork comes from the project logo supplied
by the user (`1001012593.png`), with only the large Dawnlight title removed using
the built-in image editor. The sun crest, dark background and ornament remain.
No model or texture was extracted from another game or mod.

- `dawnlight-glider.obj`: 1,712 triangles, Y up, Z forward, origin at the midpoint
  of the carrying hands. Canvas wingspan 200 units; peak height 56 units.
  Two separate bows connect the sail's front and rear. Leather grips run along
  Z = -11 to +11 at X = +/-22, rising from Y = -2 to +2 (~10 degrees).
- `dawnlight-glider.mtl`: material referencing the PNG atlas.
- `dawnlight-canopy-source.png`: supplied logo/background with the title removed;
  the complete edited image is retained as the editable texture source.
- `dawnlight-glider.png`: 256x256 runtime atlas: full artwork in the canvas region,
  separate wood/leather strips below it. No colored panel or substitute symbol
  is drawn over the supplied artwork. The canvas UVs turn the artwork 180 degrees
  and compensate for the wide sail so the central crest retains its proportions.
  Canvas is two-sided at runtime.

Image-edit prompt: remove only the large gold word “Dawnlight”; reconstruct the
stone/ornament behind the letters; preserve the crest, its position/size, colors,
background, runes and frame. Keep an opaque square image without new elements.

`python3 tools/generate_glider.py` regenerates the editable assets and
`src/generated/glider_art.hpp`. This developer tool needs Pillow. Normal builds
need neither Pillow nor a model converter. The checked-in header embeds the
same mesh and texture (GX RGB565, seven mip levels) directly in the mod binary;
no room archive or external installation is required.

The runtime uses a native J3D draw packet/GX textured mesh, as the custom Shade
pedestal does, rather than requiring a BMD exporter. The pose follows Link's
hand midpoint and facing; native carrying animation and glide physics remain
responsible for movement. The GLIDE-owned Cucco still exists internally and is
hidden/muted only for the Glider selection. No world Cucco is replaced.

In-game QA: check deployment during a manual jump and an ordinary fall; camera
above/below; both turn directions; switching Glide Item while airborne; landing,
damage, disabling Glide, cutscenes and room changes; ordinary carried Cuccos.
The mesh preview and automated tests do not replace this runtime visual check.
