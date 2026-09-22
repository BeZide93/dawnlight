# Original pedestal stone

`charcoal-stone.png` is original artwork created for Dawnlight with the built-in
OpenAI image-generation tool on 2026-09-22, at the user's request. No game
texture, screenshot, third-party image or Twilit Essentials asset was supplied
to the generator. It is deliberately a quiet warm-charcoal stone grain, not a
copy of the arena floor. The full generated source is preserved here.

The ornaments are original octagonal mesh bands in `src/pedestal_mesh.hpp`:
thin warm inlays and shadow lines around the base, shaft and top. Keeping these
separate from the stone image aligns them to the actual pedestal edges, avoids
stretched ornament on bevels and leaves the sword's insertion area quiet.

## Runtime asset

Run `python tools/generate_pedestal_texture.py` (Pillow required) after changing
the PNG, and commit `src/generated/pedestal_stone.hpp`. The converter only
resizes and encodes the generated art. It stores big-endian tiled GX RGB565
at 256, 128, 64, 32, 16, 8 and 4 pixels square: 174,752 bytes, embedded in the
mod binary and therefore included in every `.dusk` bundle. Mipmaps and
trilinear filtering reduce distant shimmer. No runtime file decoding or
external image tools are needed, and no copyrighted game texture is bundled.

## Generation prompt

Create a high-quality ORIGINAL seamless tileable stone albedo texture for a small octagonal fantasy sword pedestal. Square 1024x1024, flat orthographic material scan filling the whole canvas edge to edge. Warm dark charcoal grey limestone with a very subtle muted olive-brown undertone, finely grained natural mineral flecks, restrained thin irregular smoky veins and soft weathering. The overall value should be medium-dark, not black, with modest low contrast, so it complements warm beige stone tiles while remaining clearly darker. Fine detail that reads beautifully at 256x256. No tile grid, no seams, no individual blocks, no emblems, no carvings, no symbols, no ornamental borders, no text, no objects, no perspective, no lighting gradients, no cast shadows, no highlights baked in. This is ONLY the raw stone surface material; carefully aligned architectural edge moldings will be provided by the 3D mesh. Restrained elegant ancient stone, matte, physically plausible, no glossy marble swirls. Seamless edges on all four sides.
