#!/usr/bin/env python3
"""Build Dawnlight's original glider, editable OBJ/MTL/PNG and embedded GX data.
Developer-only dependency: Pillow. Runtime/builds use the checked-in header.
Coordinates: Y up, Z forward; origin is the midpoint of Link's hands.
"""
from pathlib import Path
import math
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'art/glider'
SIZE, LAST_MIP = 128, 5


def texture():
    im = Image.new('RGB', (SIZE, SIZE))
    for y in range(SIZE):
        for x in range(SIZE):
            grain = ((x * 17 + y * 31 + x * y * 3) % 9) - 4
            base = (214, 199, 156) if y < 104 else (103, 64, 36) if y < 116 else (57, 39, 29)
            weave = (2 if x % 2 else -2) + (2 if y % 2 else -2)
            im.putpixel((x, y), tuple(max(0, min(255, c + grain + weave)) for c in base))
    d = ImageDraw.Draw(im)
    d.rectangle((0, 0, 127, 103), outline=(48, 85, 77), width=6)
    for x in (22, 43, 84, 105):
        d.line((x, 6, x, 97), fill=(155, 133, 96), width=1)
    # Original wing/sun motif; no borrowed game or other mod textures.
    d.polygon([(64, 22), (74, 42), (111, 28), (96, 55), (76, 62), (64, 84),
               (52, 62), (32, 55), (17, 28), (54, 42)], fill=(42, 83, 76))
    d.polygon([(64, 32), (70, 49), (91, 43), (76, 56), (64, 71),
               (52, 56), (37, 43), (58, 49)], fill=(211, 171, 91))
    d.ellipse((58, 45, 70, 57), fill=(240, 222, 170))
    for x in range(8, 121, 5):
        d.line((x, 7, x+1, 7), fill=(243, 225, 185))
        d.line((x, 96, x+1, 96), fill=(243, 225, 185))
    for y in range(108, 116, 3):
        d.line((0, y, 127, y), fill=(126, 83, 47))
    return im


def mesh():
    vertices, faces = [], []
    def vertex(p, uv):
        vertices.append((*p, *uv))
        return len(vertices)-1
    def triangle(a, b, c):
        faces.append((a,b,c))
    def canopy(x, t):
        return (x * 125, 54 + 24 * (1-x*x) + 5*math.sin(t*math.pi),
                (t-.5) * (110 - 34*abs(x)) + 14*abs(x))
    for i in range(16):
        for j in range(6):
            q = []
            for di,dj in ((0,0),(1,0),(1,1),(0,1)):
                u,t=(i+di)/16,(j+dj)/6
                q.append(vertex(canopy(u*2-1,t), (u, t*103/128)))
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
                ring.append(vertex(point, (.1+.8*end, (120 if leather else 107+k)/128)))
        for k in range(6):
            a0,a1,b0,b1=ring[k],ring[(k+1)%6],ring[6+k],ring[6+(k+1)%6]
            triangle(a0,b0,b1);triangle(a0,b1,a1)
        for k in range(1,5):
            triangle(ring[0],ring[k+1],ring[k]);triangle(ring[6],ring[6+k],ring[7+k])
    # Curved ribs below canvas, leading/trailing spars, suspension and grip bar.
    for x in (-1,-.5,0,.5,1):
        for j in range(6):
            a=list(canopy(x,j/6));b=list(canopy(x,(j+1)/6));a[1]-=2;b[1]-=2
            beam(a,b,1.4)
    for t in (0,1):
        for i in range(16):
            beam(canopy(i/8-1,t),canopy((i+1)/8-1,t),1.8)
    for side in (-1,1):
        for t in (0,1):
            beam((side*22,0,0),canopy(side*.56,t),1.6)
    beam((-38,0,0),(38,0,0),2.5)
    beam((-30,0,0),(-14,0,0),3.1,True);beam((14,0,0),(30,0,0),3.1,True)
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
