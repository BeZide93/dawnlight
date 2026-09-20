#!/usr/bin/env python3
"""Build Dawnlight's own SVG emblems into an embedded, antialiased alpha atlas.
Developer-only dependencies: CairoSVG and Pillow. No game files are read.
"""
from pathlib import Path
import base64
import io
import re
import xml.etree.ElementTree as ET
import cairosvg
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
NS = '{http://www.w3.org/2000/svg}'
CELL, COLUMNS, ROWS = 256, 4, 5
source = ROOT / 'art/boss-portal-symbols.svg'
svg = ET.parse(source).getroot()
symbols = svg.findall(f'{NS}defs/{NS}symbol')
assert len(symbols) == 18
# The atlas order is the saved defeat-bit order. Fail regeneration if it drifts.
entries = (ROOT/'src/new_save_modes.cpp').read_text().split(
    'constexpr BossRushEntry kBossRushEntries[] = {', 1)[1].split('};', 1)[0]
portal_names = re.findall(r'BossRushEntry::\w+, "([^"]+)"', entries)
assert portal_names == [symbol.attrib['data-name'] for symbol in symbols]
atlas = Image.new('L', (CELL*COLUMNS, CELL*ROWS))
for index, symbol in enumerate(symbols):
    assert symbol.attrib['id'] == f'boss-{index}'
    content = ''.join(ET.tostring(child, encoding='unicode') for child in symbol)
    standalone = f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256" color="white">{content}</svg>'
    png = cairosvg.svg2png(bytestring=standalone.encode(), output_width=CELL*4, output_height=CELL*4)
    mask = Image.open(io.BytesIO(png)).getchannel('A').resize((CELL,CELL), Image.Resampling.LANCZOS)
    atlas.paste(mask, ((index%COLUMNS)*CELL, (index//COLUMNS)*CELL))
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
header = '// Generated from art/boss-portal-symbols.svg by tools/generate_boss_symbols.py.\n'
header += '// Original Dawnlight artwork; contains no extracted game assets.\n#pragma once\n\n'
header += 'namespace dawnlight::portal_art {\n'
header += f'inline constexpr unsigned kCount = {len(symbols)}, kCell = {CELL}, kColumns = {COLUMNS};\n'
header += f'inline constexpr unsigned kWidth = {atlas.width}, kHeight = {atlas.height};\n'
header += 'inline constexpr const char* kNames[] = {\n'
header += ''.join(f'    "{s.attrib["data-name"]}",\n' for s in symbols) + '};\n'
header += 'inline constexpr const char* kMaskRleBase64[] = {\n'
# Keep each concatenated C++ string below MSVC's string-literal limit.
for start in range(0, len(encoded), 8000):
    chunk = encoded[start:start+8000]
    header += '\n'.join('    \"'+chunk[i:i+100]+'\"' for i in range(0,len(chunk),100)) + ',\n'
header += '};\n}\n'
(ROOT/'src/generated/boss_portal_art.hpp').write_text(header)
print(f'{len(symbols)} emblems; atlas {atlas.width}x{atlas.height}; RLE {len(rle)} bytes; header {len(header)} bytes')
if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--preview',type=Path)
    args = parser.parse_args()
    if args.preview:
        cairosvg.svg2png(url=str(source),write_to=str(args.preview))
