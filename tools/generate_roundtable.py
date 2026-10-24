#!/usr/bin/env python3
"""Author the Survivors hall. Original geometry; no Elden Ring game assets.

One source produces the visible mesh and native DZB collision. Coordinates are
Twilight Princess units, Y up; the existing hub rest/fire anchor stays (0,-400,0).
Run from any directory. Pillow is needed only for --texture / --preview.
"""
from pathlib import Path
import argparse
import math
import struct

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'res/survivors_roundtable'
FLOOR = -400.0
# Captured native main-floor radius is 2617.437 (see darknut_floor_test).
# Keep its footprint, with a small inset for the replacement wall/column trim.
RADIUS = 2600.0
SIDES = 16
FRONT = RADIUS * math.cos(math.pi/SIDES)
HALF_DOOR = RADIUS * math.sin(math.pi/SIDES)
TAU = math.tau


def add(a, b): return tuple(x+y for x, y in zip(a, b))
def sub(a, b): return tuple(x-y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def length(a): return math.sqrt(sum(x*x for x in a))


class Hall:
    def __init__(self):
        self.vertices = []
        self.collision = []
        self.transform = lambda p: p

    def triangle(self, points, material=0, tint=(255, 255, 255), solid=True, visible=True, uv=None):
        p = [self.transform(v) for v in points]
        normal = cross(sub(p[1], p[0]), sub(p[2], p[0]))
        n = length(normal)
        if n < .001:
            return
        if solid:
            self.collision.append(tuple(p))
        if not visible:
            return
        if uv is None:
            uv = ((.03, .03), (.97, .03), (.97, .97))
        # Soft baked directional/ambient lighting, plus fire warmth near the
        # original brazier. Native Link/Zelda still receive its live point light.
        shade = .62 + .22*max(0, normal[1]/n) + .12*max(0, -normal[0]/n)
        for pos, tex in zip(p, uv):
            distance = math.hypot(pos[0], pos[2])
            warm = max(0, 1-distance/1400)*.14
            light = (shade+warm, shade+warm*.6, shade+warm*.15)
            color = tuple(min(255, round(t*c)) for t, c in zip(tint, light))
            if material == 4:
                color = tint
            tile = min(material, 3)
            u = (tile % 2 + tex[0])*.5
            v = (tile // 2 + tex[1])*.5
            if material == 4:u=v=1.5/256 # reserved untextured/emissive texel
            self.vertices.append((*pos, u, v, *color, 255))

    def quad(self, a, b, c, d, **kw):
        self.triangle((a,b,c), uv=((.03,.97),(.97,.97),(.97,.03)), **kw)
        self.triangle((a,c,d), uv=((.03,.97),(.97,.03),(.03,.03)), **kw)

    def box(self, x, y, z, w, h, d, **kw):
        a=(x-w/2,y,z-d/2); b=(x+w/2,y,z-d/2)
        c=(x+w/2,y,z+d/2); e=(x-w/2,y,z+d/2)
        up=lambda p:add(p,(0,h,0))
        self.quad(b,a,up(a),up(b),**kw)
        self.quad(c,b,up(b),up(c),**kw)
        self.quad(e,c,up(c),up(e),**kw)
        self.quad(a,e,up(e),up(a),**kw)
        self.quad(up(a),up(e),up(c),up(b),**kw)

    def column(self,x,y,z,r,h,top=None,n=12,**kw):
        top=r if top is None else top
        for i in range(n):
            a=i*TAU/n; b=(i+1)*TAU/n
            p=(x+r*math.sin(a),y,z+r*math.cos(a))
            q=(x+r*math.sin(b),y,z+r*math.cos(b))
            u=(x+top*math.sin(a),y+h,z+top*math.cos(a))
            v=(x+top*math.sin(b),y+h,z+top*math.cos(b))
            self.quad(p,q,v,u,**kw)
            self.triangle(((x,y+h,z),u,v),uv=((.5,.5),(.5+.46*math.sin(a),.5+.46*math.cos(a)),(.5+.46*math.sin(b),.5+.46*math.cos(b))),**kw)

    def local(self,x,z,angle):
        sine=math.sin(angle); cosine=math.cos(angle)
        self.transform=lambda p:(x+p[0]*cosine+p[2]*sine,p[1]+FLOOR,z-p[0]*sine+p[2]*cosine)

    def reset(self): self.transform=lambda p:p

    def arch(self,width,bottom,height,depth=90):
        radius=width/2
        for i in range(12):
            a=i*math.pi/12; b=(i+1)*math.pi/12
            point=lambda r,t,z:(r*math.cos(t),bottom+r*math.sin(t),z)
            self.quad(point(radius,a,0),point(radius,b,0),point(radius+75,b,0),point(radius+75,a,0),solid=False)
            self.quad(point(radius,a,0),point(radius,a,depth),point(radius,b,depth),point(radius,b,0),solid=False,tint=(195,185,165))
        for side in (-1,1):
            self.box(side*(radius+38),0,40,76,bottom,depth,solid=False)
            self.box(side*(radius+38),bottom-24,20,110,40,140,solid=False)
        self.box(0,height,50,width+250,65,180,solid=False)

    def chair(self,x,z,angle):
        self.local(x,z,angle)
        for px in (-48,48):
            for pz in (-45,45): self.box(px,0,pz,18,110,18,material=1)
        self.box(0,96,0,130,25,120,material=1)
        self.box(0,121,0,109,10,98,material=2,solid=False)
        self.box(0,120,-49,130,174,24,material=1)
        self.box(0,145,-34,88,116,5,material=2,solid=False)
        for px in (-66,66):
            self.box(px,110,-45,18,205,20,material=1,solid=False)
            self.column(px,315,-45,17,25,top=0,material=3,solid=False,n=6)
        self.box(0,282,-49,155,22,32,material=3,solid=False)
        self.reset()

    def build(self):
        boundary=[(RADIUS*math.sin((i+.5)*TAU/SIDES),FLOOR,RADIUS*math.cos((i+.5)*TAU/SIDES)) for i in range(SIDES)]
        for i,a in enumerate(boundary):
            b=boundary[(i+1)%SIDES]
            self.triangle(((0,FLOOR,0),a,b),tint=(65,57,47))
        # Individually clipped, staggered flagstones, with narrow mortar joints.
        extent=math.ceil(RADIUS/200)+1
        for row in range(-extent,extent):
            for col in range(-extent,extent):
                x=col*200+(row%2)*100;z=row*200
                polygon=[(x+3,z+3),(x+197,z+3),(x+197,z+197),(x+3,z+197)]
                for i,a in enumerate(boundary):
                    b=boundary[(i+1)%SIDES]; result=[]
                    side=lambda p:(b[0]-a[0])*(p[1]-a[2])-(b[2]-a[2])*(p[0]-a[0])
                    for j,p in enumerate(polygon):
                        q=polygon[(j+1)%len(polygon)];sp=side(p);sq=side(q)
                        if sp<=0: result.append(p)
                        if (sp<0)!=(sq<0):
                            t=sp/(sp-sq);result.append((p[0]+t*(q[0]-p[0]),p[1]+t*(q[1]-p[1])))
                    polygon=result
                    if not polygon:break
                shade=210+(row*7+col*11)%35
                for j in range(1,len(polygon)-1):
                    self.triangle(tuple((p[0],FLOOR+.5,p[1]) for p in (polygon[0],polygon[j+1],polygon[j])),solid=False,tint=(shade,shade,shade))
        # Sixteen-sided hall. The south bay is open to the original door.
        for i in range(SIDES):
            angle=i*TAU/SIDES
            width=2*HALF_DOOR
            self.local(FRONT*math.sin(angle),FRONT*math.cos(angle),angle)
            if i:
                self.quad((width/2,0,25),(-width/2,0,25),(-width/2,1200,25),(width/2,1200,25),tint=(180,175,160))
                # Dark recessed panel visible through a carved arch.
                self.box(0,0,12,width-130,760,12,material=1,solid=False,tint=(85,77,69))
            self.arch(width-175,460,920)
            self.box(0,1040,0,width+10,100,100,material=1,solid=False)
            self.box(0,1000,-8,width+10,30,140,material=3,solid=False)
            self.reset()
        # Ring of substantial stone columns with drum joints and capitals.
        for x,_,z in boundary:
            x*=.965;z*=.965
            self.column(x,FLOOR,z,123,52,n=8)
            self.column(x,FLOOR+52,z,103,50,n=12)
            self.column(x,FLOOR+102,z,78,830,top=70,n=12)
            for y in range(210,900,140):self.column(x,FLOOR+y,z,80,12,solid=False)
            self.column(x,FLOOR+932,z,98,45,n=12)
            self.column(x,FLOOR+977,z,130,55,n=8)
        # Ceiling vault, oak radial ribs and central bronze boss.
        for i,a in enumerate(boundary):
            b=boundary[(i+1)%SIDES]
            self.triangle(((0,FLOOR+1510,0),(b[0],FLOOR+1180,b[2]),(a[0],FLOOR+1180,a[2])),solid=False,tint=(130,119,105))
            # Rib is a tapered strip, seen from below.
            for t in range(10):
                t0=t/10;t1=(t+1)/10
                p=(a[0]*t0,FLOOR+1500-330*t0,a[2]*t0)
                q=(a[0]*t1,FLOOR+1500-330*t1,a[2]*t1)
                normal=(-a[2]/RADIUS*20,0,a[0]/RADIUS*20)
                self.quad(add(p,normal),sub(p,normal),sub(q,normal),add(q,normal),material=1,solid=False)
        self.column(0,FLOOR+1460,0,100,35,material=3,solid=False)
        # Round table behind the rest/fire area. Front is left clear so the
        # existing camera, kneeling pose and Zelda dialogue remain unobstructed.
        self.column(0,FLOOR,-730,300,55,material=1,n=32)
        self.column(0,FLOOR+55,-730,175,100,top=240,material=1,n=24)
        self.column(0,FLOOR+155,-730,540,34,material=1,n=48)
        self.column(0,FLOOR+189,-730,520,9,material=3,n=48,solid=False)
        self.column(0,FLOOR+198,-730,495,3,material=1,n=48)
        self.column(0,FLOOR+201,-730,185,2,material=3,n=32,solid=False)
        # Radial engraved rings and twelve carved brass settings.
        for i in range(12):
            angle=i*TAU/12
            x=400*math.sin(angle);z=-730+400*math.cos(angle)
            self.column(x,FLOOR+202,z,35,3,material=3,solid=False)
            self.column(x,FLOOR+205,z,25,2,material=1,solid=False)
            if i not in (0,1,11):self.chair(680*math.sin(angle),-730+680*math.cos(angle),angle+math.pi)
        # Candelabra on the table, leaving the foreground seating bay open.
        for x,z in ((-200,-730),(200,-730),(0,-970)):
            self.column(x,FLOOR+202,z,37,15,material=3,solid=False)
            self.column(x,FLOOR+217,z,10,105,material=3,solid=False)
            self.box(x,FLOOR+310,z,140,12,12,material=3,solid=False)
            for dx in (-60,0,60):
                self.column(x+dx,FLOOR+322,z,8,45,material=0,tint=(255,226,164),solid=False,n=6)
                self.column(x+dx,FLOOR+367,z,9,30,top=0,material=4,tint=(255,212,103),solid=False,n=6)
        # Hearth on the west wall; raised stone seat/bench and hood.
        self.local(-FRONT+110,0,-math.pi/2)
        self.box(0,0,-40,620,35,270)
        self.box(0,35,80,450,380,30,material=1,tint=(40,30,25),solid=False)
        for x in (-260,260):self.box(x,35,15,100,420,200)
        self.box(0,455,0,690,65,270)
        self.box(0,520,40,600,370,160)
        self.box(0,890,25,670,55,220)
        for x in (-150,-75,0,75,150):
            self.box(x,40,-10,55,30,140,material=1,solid=False,tint=(90,65,40))
            # Coals only. The runtime owns native animated fire/sparks and light.
            self.column(x,70,-10,24,4,material=4,tint=(255,102,30),solid=False,n=7)
        self.reset()
        # Burgundy banners and small stylized robed memorials in rear bays.
        for angle in (math.pi*.75,math.pi,math.pi*1.25):
            self.local((FRONT-200)*math.sin(angle),(FRONT-200)*math.cos(angle),angle)
            self.box(0,150,0,260,360,200,solid=True)
            self.column(0,510,0,95,260,top=45,solid=False,n=8)
            self.column(0,770,0,48,85,top=18,solid=False,n=8)
            self.box(-180,470,50,135,420,5,material=2,solid=False)
            self.box(-180,890,45,175,12,20,material=3,solid=False)
            self.reset()
        # Native south doorway at z=3900: retain its event and collision
        # footprint. The approach ramp meets its authored floor y=-250.
        for i in range(12):
            z0=FRONT+i*(4200-FRONT)/12;z1=FRONT+(i+1)*(4200-FRONT)/12
            y0=FLOOR+min(150,max(0,(z0-2600)*150/1000))
            y1=FLOOR+min(150,max(0,(z1-2600)*150/1000))
            self.quad((-HALF_DOOR,y0,z0),(-HALF_DOOR,y1,z1),(HALF_DOOR,y1,z1),(HALF_DOOR,y0,z0))
            for sign in (-1,1):
                x=sign*HALF_DOOR
                points=[(x,y0,z0),(x,y1,z1),(x,y1+660,z1),(x,y0+660,z0)]
                if sign<0: points.reverse()
                self.quad(*points,tint=(200,190,170))
            self.quad((-HALF_DOOR,y0+660,z0),(HALF_DOOR,y0+660,z0),(HALF_DOOR,y1+660,z1),(-HALF_DOOR,y1+660,z1),solid=False,tint=(130,120,110))
        # Invisible end cap behind the native door prevents leaving collision
        # if an event is cancelled; opening the door still triggers its event.
        self.quad((HALF_DOOR,-250,4200),(-HALF_DOOR,-250,4200),(-HALF_DOOR,600,4200),(HALF_DOOR,600,4200),visible=False)
        return self


def dzb(hall):
    """A balanced native tree, and root/room/mesh ancestry for room queries."""
    vertices=[];lookup={};triangles=[];blocks=[];nodes=[]
    def vertex(p):
        key=tuple(round(v,4) for v in p)
        if key not in lookup:lookup[key]=len(vertices);vertices.append(key)
        return lookup[key]
    def tree(faces,parent=0xffff):
        idx=len(nodes);nodes.append(None)
        if len(faces)<=24:
            block=len(blocks);blocks.append(len(triangles))
            triangles.extend(tuple(vertex(p) for p in face)+(0,2) for face in faces)
            nodes[idx]=(1,parent,block,*([0xffff]*7))
        else:
            centers=[tuple(sum(p[k] for p in face)/3 for k in range(3)) for face in faces]
            axis=max(range(3),key=lambda k:max(c[k] for c in centers)-min(c[k] for c in centers))
            ordered=[face for _,face in sorted(zip(centers,faces),key=lambda v:v[0][axis])]
            mid=len(ordered)//2
            children=(tree(ordered[:mid],idx),tree(ordered[mid:],idx))
            nodes[idx]=(0,parent,*children,*([0xffff]*6))
        return idx
    tree(hall.collision)
    data=bytearray(52)
    def append(blob):
        while len(data)%4:data.append(0)
        offset=len(data);data.extend(blob);return offset
    vo=append(b''.join(struct.pack('>3f',*p) for p in vertices))
    to=append(b''.join(struct.pack('>5H',*p) for p in triangles))
    bo=append(b''.join(struct.pack('>H',p) for p in blocks))
    no=append(b''.join(struct.pack('>10H',*p) for p in nodes))
    name=append(b'roundtable\0')
    groups=[]
    for i in range(3):
        groups.append(struct.pack('>I3f3h2x3f6HI',name,1,1,1,0,0,0,0,0,0,
            i-1 if i else 0xffff,0xffff,i+1 if i<2 else 0xffff,
            51,0,0 if i==2 else 0xffff,0))
    go=append(b''.join(groups))
    # Native dBgW: exit=63, link=255, static camera motion=0; room camera,
    # path and path point=255 (none). Zero camera/path IDs would accidentally
    # select the original dungeon's camera/path record zero on our new floor.
    ti=append(struct.pack('>4I',0x3f,0xff,0xffffff00,0))
    struct.pack_into('>13I',data,0,len(vertices),vo,len(triangles),to,len(blocks),bo,len(nodes),no,3,go,1,ti,0)
    assert len(vertices)<65535 and len(triangles)<65535
    return data


def write_assets():
    hall=Hall().build();OUT.mkdir(parents=True,exist_ok=True)
    mesh=b'RTH1'+struct.pack('<I',len(hall.vertices))+b''.join(struct.pack('<5f4B',*v) for v in hall.vertices)
    (OUT/'hall.mesh').write_bytes(mesh)
    (OUT/'hall.dzb').write_bytes(dzb(hall))
    # Native effects and the hearth geometry must use the same source layout.
    (ROOT/'src/generated/roundtable_layout.hpp').write_text(
        '// Generated by tools/generate_roundtable.py.\n#pragma once\n'
        'namespace dawnlight::roundtable_layout {\n'
        f'inline constexpr float hearthFireX = {-FRONT+120:.6f}f;\n'
        f'inline constexpr float hearthLightX = {-FRONT+265:.6f}f;\n'
        '}\n')
    print(f'{len(hall.vertices)//3} visible triangles; {len(hall.collision)} collision triangles; {len(mesh)} mesh bytes')
    return hall


def texture(path):
    from PIL import Image
    im=Image.open(path).convert('RGB').resize((256,256),Image.Resampling.LANCZOS)
    for y in range(4):
        for x in range(4):im.putpixel((x,y),(255,255,255))
    data=bytearray()
    for by in range(0,256,4):
        for bx in range(0,256,4):
            for y in range(4):
                for x in range(4):
                    r,g,b=im.getpixel((bx+x,by+y))
                    data.extend(struct.pack('>H',((r>>3)<<11)|((g>>2)<<5)|(b>>3)))
    (OUT/'materials.rgb565').write_bytes(data)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--texture',type=Path)
    args=parser.parse_args()
    write_assets()
    if args.texture:texture(args.texture)
