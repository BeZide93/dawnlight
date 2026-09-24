"""Pack the authored transparent icon into GX RGBA8 tiles for its J2D picture.
Run after replacing art/glider/glider-item-icon.png. Requires Pillow.
"""
from pathlib import Path
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
SIZE = 128

def encode(image):
    rgba = image.convert('RGBA').resize((SIZE, SIZE), Image.Resampling.LANCZOS)
    out = bytearray()
    for by in range(0, SIZE, 4):
        for bx in range(0, SIZE, 4):
            tile = [rgba.getpixel((x, y)) for y in range(by, by+4) for x in range(bx, bx+4)]
            for r, g, b, a in tile:
                out.extend((a, r))
            for r, g, b, a in tile:
                out.extend((g, b))
    return bytes(out)

if __name__ == '__main__':
    with Image.open(ROOT/'art/glider/glider-item-icon.png') as image:
        pixels = encode(image)
    (ROOT/'res/glider-item-icon.rgba8').write_bytes(pixels)
    print(f'Glider item icon: {SIZE}x{SIZE}, {len(pixels)} GX RGBA8 bytes')
