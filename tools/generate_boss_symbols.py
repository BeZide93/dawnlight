#!/usr/bin/env python3
"""Build the user-supplied transparent boss PNGs into an embedded alpha atlas.
Developer-only dependency: Pillow. No game files are read.
"""
from pathlib import Path
import base64
import math
import re
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
CELL, COLUMNS, ROWS = 256, 4, 5
source = ROOT / 'art/boss-icons'
# The atlas order is the saved defeat-bit order. Fail regeneration if it drifts.
entries = (ROOT/'src/new_save_modes.cpp').read_text().split(
    'constexpr BossRushEntry kBossRushEntries[] = {', 1)[1].split('};', 1)[0]
portal_names = re.findall(r'BossRushEntry::\w+, "([^"]+)"', entries)
assert len(portal_names) == 18 and len(set(portal_names)) == 18
filenames = [name.replace(' ', '_')+'.png' for name in portal_names]
assert {p.name for p in source.glob('*.png')} == set(filenames)
atlas = Image.new('L', (CELL*COLUMNS, CELL*ROWS))
for index, filename in enumerate(filenames):
    with Image.open(source/filename) as icon:
        assert icon.mode == 'RGBA', f'{filename}: expected transparent RGBA'
        alpha = icon.getchannel('A')
        bounds = alpha.getbbox()
        assert bounds and alpha.getextrema() == (0,255), f'{filename}: empty/opaque icon'
        mask = alpha.crop(bounds)
    # Keep the supplied silhouette and aspect ratio. Fit its actual opaque
    # pixels inside the circular face, including wide wings/weapons at corners.
    radius = max(math.hypot(x+.5-mask.width/2, y+.5-mask.height/2)
                 for y in range(mask.height) for x in range(mask.width)
                 if mask.getpixel((x,y)))
    scale = min(216/max(mask.size), 114/radius)
    size = tuple(max(1,round(n*scale)) for n in mask.size)
    mask = mask.resize(size, Image.Resampling.LANCZOS)
    atlas.paste(mask, ((index%COLUMNS)*CELL+(CELL-mask.width)//2,
                       (index//COLUMNS)*CELL+(CELL-mask.height)//2))
raw = atlas.tobytes()
rle = bytearray()
i = 0
while i < len(raw):
    count = 1
    while count < 255 and i+count < len(raw) and raw[i+count] == raw[i]:
        count += 1
    rle.extend((count, raw[i]))
    i += count
encoded = base64.b64encode(rle).decode()
header = '// Generated from art/boss-icons/*.png by tools/generate_boss_symbols.py.\n'
header += '// User-supplied boss silhouettes; only their alpha masks are embedded.\n#pragma once\n\n'
header += 'namespace dawnlight::portal_art {\n'
header += f'inline constexpr unsigned kCount = {len(portal_names)}, kCell = {CELL}, kColumns = {COLUMNS};\n'
header += f'inline constexpr unsigned kWidth = {atlas.width}, kHeight = {atlas.height};\n'
header += 'inline constexpr const char* kNames[] = {\n'
header += ''.join(f'    "{name}",\n' for name in portal_names) + '};\n'
header += 'inline constexpr const char* kMaskRleBase64[] = {\n'
# Keep each concatenated C++ string below MSVC's string-literal limit.
for start in range(0, len(encoded), 8000):
    chunk = encoded[start:start+8000]
    header += '\n'.join('    \"'+chunk[i:i+100]+'\"' for i in range(0,len(chunk),100)) + ',\n'
header += '};\n}\n'
(ROOT/'src/generated/boss_portal_art.hpp').write_text(header)
print(f'{len(portal_names)} emblems; atlas {atlas.width}x{atlas.height}; RLE {len(rle)} bytes; header {len(header)} bytes')
if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--preview',type=Path)
    args = parser.parse_args()
    if args.preview:
        # Preview both states over a checkerboard to expose the transparency.
        preview = Image.new('RGB', (CELL*6, (CELL+24)*6))
        draw = ImageDraw.Draw(preview)
        for index, name in enumerate(portal_names):
            mask = atlas.crop(((index%COLUMNS)*CELL, (index//COLUMNS)*CELL,
                               (index%COLUMNS+1)*CELL, (index//COLUMNS+1)*CELL))
            for state, color in enumerate(((140,214,255),(255,77,64))):
                x, y = (index%3*2+state)*CELL, index//3*(CELL+24)
                for cy in range(0,CELL,16):
                    for cx in range(0,CELL,16):
                        shade = 80 if (cx//16+cy//16)%2 else 120
                        draw.rectangle((x+cx,y+cy,x+cx+15,y+cy+15),fill=(shade,)*3)
                glass = Image.new('RGBA',(CELL,CELL))
                ImageDraw.Draw(glass).ellipse((0,0,CELL-1,CELL-1),fill=(*color,148))
                preview.paste(glass,(x,y),glass)
                preview.paste((0,0,0),(x,y,x+CELL,y+CELL),mask)
                draw.text((x+8,y+CELL+4),name+(' - defeated' if state else ''),fill='white')
        preview.save(args.preview)
