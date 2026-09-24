# Dawnlight glider

Original mesh and texture authored for Dawnlight; no model, texture or symbols
were extracted from Zelda, another game, or another mod. Covered by this
repository's license.

- `dawnlight-glider.obj`: 1,572 triangles, Y up, Z forward, origin at the midpoint
  of the carrying hands. Wingspan 250 game units; grip bar spans X = -38 to +38.
- `dawnlight-glider.mtl`: material referencing the original PNG atlas.
- `dawnlight-glider.png`: 128x128 woven canvas, original wing/sun motif, timber
  and leather atlas. Canvas is two-sided at runtime.

`python3 tools/generate_glider.py` regenerates the editable assets and
`src/generated/glider_art.hpp`. This developer tool needs Pillow. Normal builds
need neither Pillow nor a model converter. The checked-in header embeds the
same mesh and texture (GX RGB565, six mip levels) directly in the mod binary;
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
