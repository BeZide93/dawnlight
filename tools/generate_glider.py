#!/usr/bin/env python3
"""Build Dawnlight's original glider, editable OBJ/MTL/PNG and embedded GX data.
Developer-only dependency: Pillow. Runtime/builds use the checked-in header.
Coordinates: Y up, Z forward; origin is the midpoint of Link's hands.
"""
from pathlib import Path
import math
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'art/glider'
SIZE, LAST_MIP = 256, 6
CANVAS_ROWS = 208


def texture():
    # Pack the user's Dawnlight artwork (only the title removed) in the canvas
    # region. Keep the full picture, background and frame; no new motif/overlay.
    with Image.open(ART / 'dawnlight-canopy-source.png') as source:
        canvas = source.convert('RGB').resize((SIZE, CANVAS_ROWS), Image.Resampling.LANCZOS)
    im = Image.new('RGB', (SIZE, SIZE))
    im.paste(canvas, (0, 0))
    # Separate wood/leather atlas strips must not paint over the supplied art.
    for y in range(CANVAS_ROWS, SIZE):
        for x in range(SIZE):
            grain = ((x * 17 + y * 31 + x * y * 3) % 9) - 4
            base = (118, 79, 42) if y < 232 else (67, 43, 27)
            im.putpixel((x, y), tuple(c + grain for c in base))
    return im


def mesh():
    vertices, faces = [], []
    def vertex(p, uv):
        vertices.append((*p, *uv))
        return len(vertices)-1
    def triangle(a, b, c):
        faces.append((a,b,c))
    def canopy(x, t):
        # 20% narrower, tapered rear edge; peak above the grips drops from
        # 83 to 56 units. Keep the grip positions fixed to Link's carrying pose.
        return (x * (82 + 18*t), 34 + 18 * (1-x*x) + 4*math.sin(t*math.pi),
                (t-.5) * (80 - 12*abs(x)) + 8*abs(x))
    def canopy_uv(u, t):
        # The sail is roughly 200 units wide but only 80 deep. Give the central
        # crest more texture space in X so its circular outline stays round on
        # the mesh; distribute the surrounding frame over the outer panels.
        if u < .375:
            artwork_u = .6*u
        elif u > .625:
            artwork_u = .775 + .6*(u-.625)
        else:
            artwork_u = .225 + 2.2*(u-.375)
        # Turn the artwork half a revolution on the sail. The atlas strips for
        # the wooden frame and leather grips are sampled separately below.
        artwork_u, artwork_t = 1-artwork_u, 1-t
        return ((.5+artwork_u*(SIZE-1))/SIZE,
                (.5+artwork_t*(CANVAS_ROWS-1))/SIZE)
    for i in range(16):
        for j in range(6):
            q = []
            for di,dj in ((0,0),(1,0),(1,1),(0,1)):
                u,t=(i+di)/16,(j+dj)/6
                q.append(vertex(canopy(u*2-1,t), canopy_uv(u,t)))
            triangle(q[0],q[2],q[1]); triangle(q[0],q[3],q[2])
    def beam(a,b,r,leather=False):
        v=[b[k]-a[k] for k in range(3)]; length=math.sqrt(sum(x*x for x in v)); v=[x/length for x in v]
        axis=(0,1,0) if abs(v[1])<.9 else (1,0,0)
        n=[v[1]*axis[2]-v[2]*axis[1],v[2]*axis[0]-v[0]*axis[2],v[0]*axis[1]-v[1]*axis[0]]
        mag=math.sqrt(sum(x*x for x in n));n=[x/mag for x in n]
        m=[v[1]*n[2]-v[2]*n[1],v[2]*n[0]-v[0]*n[2],v[0]*n[1]-v[1]*n[0]]
        ring=[]
        for end,p in enumerate((a,b)):
            for k in range(6):
                angle=k*math.tau/6
                point=tuple(p[h]+r*(n[h]*math.cos(angle)+m[h]*math.sin(angle)) for h in range(3))
                ring.append(vertex(point, (.1+.8*end, (240 if leather else 214+2*k)/SIZE)))
        for k in range(6):
            a0,a1,b0,b1=ring[k],ring[(k+1)%6],ring[6+k],ring[6+(k+1)%6]
            triangle(a0,b0,b1);triangle(a0,b1,a1)
        for k in range(1,5):
            triangle(ring[0],ring[k+1],ring[k]);triangle(ring[6],ring[6+k],ring[7+k])
    # Curved ribs below canvas and leading/trailing spars.
    for x in (-1,-.5,0,.5,1):
        for j in range(6):
            a=list(canopy(x,j/6));b=list(canopy(x,(j+1)/6));a[1]-=2;b[1]-=2
            beam(a,b,1.4)
    for t in (0,1):
        for i in range(16):
            beam(canopy(i/8-1,t),canopy((i+1)/8-1,t),1.8)
    # Longitudinal bows: each joins the REAR and FRONT of the sail, with
    # its own fore/aft leather grip. Keep the midpoint at the existing hand.
    # Z is forward; a 4-unit rise over 22 units gives a gentle ~10 degree rake.
    for side in (-1,1):
        rear = [canopy(side*.36,0), (side*28,19,-25),
                (side*23,5,-16), (side*22,-2,-11)]
        front = [(side*22,2,11), (side*23,8,19), (side*28,23,31),
                 canopy(side*.36,1)]
        for path in (rear, front):
            for a,b in zip(path,path[1:]):
                beam(a,b,2.0)
        beam((side*22,-2,-11),(side*22,2,11),2.7,True)
    return vertices,faces


def encode_texture(im):
    data=bytearray()
    for level in range(LAST_MIP+1):
        size=SIZE>>level;mip=im.resize((size,size),Image.Resampling.LANCZOS)
        for by in range(0,size,4):
            for bx in range(0,size,4):
                for y in range(by,by+4):
                    for x in range(bx,bx+4):
                        r,g,b=mip.getpixel((x,y));value=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
                        data.extend(value.to_bytes(2,'big'))
    return data


def generate():
    ART.mkdir(parents=True,exist_ok=True)
    im=texture();im.save(ART/'dawnlight-glider.png')
    vertices,faces=mesh()
    obj='# Original Dawnlight glider. Y up; origin at hand midpoint.\nmtllib dawnlight-glider.mtl\no DawnlightGlider\n'
    obj+=''.join('v %.6f %.6f %.6f\n'%v[:3] for v in vertices)
    # OBJ V is bottom-up; GX/image V is top-down.
    obj+=''.join('vt %.6f %.6f\n'%(v[3],1-v[4]) for v in vertices)
    obj+='usemtl CanvasAndWood\n'+''.join('f '+' '.join(f'{i+1}/{i+1}' for i in f)+'\n' for f in faces)
    (ART/'dawnlight-glider.obj').write_text(obj)
    (ART/'dawnlight-glider.mtl').write_text('newmtl CanvasAndWood\nKa 1 1 1\nKd 1 1 1\nKs 0 0 0\nd 1\nillum 1\nmap_Kd dawnlight-glider.png\n')
    header='// Generated by tools/generate_glider.py; original art: art/glider/README.md.\n#pragma once\nnamespace dawnlight::glider_art {\n'
    header+='struct Vertex { float x,y,z,u,v; };\ninline constexpr Vertex kVertices[] = {\n'
    header+=''.join('    {'+','.join(f'{n:.6f}f' for n in v)+'},\n' for v in vertices)+'};\n'
    header+='inline constexpr unsigned short kTriangles[][3] = {\n'
    header+=''.join('    {'+','.join(map(str,f))+'},\n' for f in faces)+'};\n'
    header+=f'inline constexpr unsigned kSize={SIZE}, kLastMip={LAST_MIP};\n'
    header+='alignas(32) inline constexpr unsigned char kPixels[] = {\n'
    data=encode_texture(im)
    header+=''.join('    '+','.join(f'0x{b:02x}' for b in data[i:i+24])+',\n' for i in range(0,len(data),24))+'};\n}\n'
    (ROOT/'src/generated/glider_art.hpp').write_text(header)
    print(f'Glider: {len(vertices)} vertices, {len(faces)} triangles, {len(data)} texture bytes')

if __name__=='__main__':
    generate()
