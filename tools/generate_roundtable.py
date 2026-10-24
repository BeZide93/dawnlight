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
# Compact layout preferred in build 1295; the native door stays connected
# by the existing approach corridor rather than enlarging the main hall.
RADIUS = 1800.0
SIDES = 16
FRONT = RADIUS * math.cos(math.pi/SIDES)
HALF_DOOR = RADIUS * math.sin(math.pi/SIDES)
TAU = math.tau
# One atlas quadrant covers this many world units, on both texture axes.
# UV repeats are split in the authoring step so GX_CLAMP cannot bleed into
# neighbouring atlas materials. Collision keeps the unsplit source triangles.
TEXEL_SPAN = 200.0
EXIT_Z = 3900.0  # captured native BS_SHUTTER_START actor position
EXIT_FLOOR = -250.0
EXIT_HALF_WIDTH = 190.0
EXIT_HEIGHT = 350.0


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
        shade = .72 + .16*max(0, normal[1]/n) + .10*max(0, -normal[0]/n)
        def emit(points, coords):
            for pos, tex in zip(points, coords):
                distance = math.hypot(pos[0], pos[2])
                warm = max(0, 1-distance/1400)*.14
                light = (shade+warm, shade+warm*.6, shade+warm*.15)
                color = tuple(min(255, round(t*c)) for t, c in zip(tint, light))
                tile = min(material, 3)
                # Keep bilinear samples inside their atlas quadrant and clear
                # of the reserved 4x4 emissive texels in the stone corner.
                u = (tile % 2 + .04 + tex[0]*.92)*.5
                v = (tile // 2 + .04 + tex[1]*.92)*.5
                if material == 4:
                    u=v=1.5/256
                    color=tint
                self.vertices.append((*pos, u, v, *color, 255))
        if material == 4:
            emit(p, uv)
            return
        # Clip the textured triangle at integer UV boundaries. Each resulting
        # polygon retains the original winding and interpolated world position.
        # A full repeat therefore has the same scale on a wall and a short trim.
        for row in range(math.floor(min(t[1] for t in uv)), math.ceil(max(t[1] for t in uv))):
            for col in range(math.floor(min(t[0] for t in uv)), math.ceil(max(t[0] for t in uv))):
                polygon=[(*pos,*tex) for pos,tex in zip(p,uv)]
                for axis,edge,sign in ((3,col,1),(3,col+1,-1),(4,row,1),(4,row+1,-1)):
                    result=[]
                    for i,a in enumerate(polygon):
                        b=polygon[(i+1)%len(polygon)]
                        da=(a[axis]-edge)*sign;db=(b[axis]-edge)*sign
                        if da>=0:result.append(a)
                        if (da<0)!=(db<0):
                            t=da/(da-db)
                            result.append(tuple(x+t*(y-x) for x,y in zip(a,b)))
                    polygon=result
                    if not polygon:break
                for i in range(1,len(polygon)-1):
                    face=(polygon[0],polygon[i],polygon[i+1])
                    points=[v[:3] for v in face]
                    if length(cross(sub(points[1],points[0]),sub(points[2],points[0])))<.001:continue
                    emit(points,[(v[3]-col,v[4]-row) for v in face])

    def quad(self, a, b, c, d, uv=None, **kw):
        if uv is None:
            # Measure after the object transform: scaled furniture must not
            # change the apparent size of wood grain or stone courses.
            pa,pb,pc,pd=[self.transform(p) for p in (a,b,c,d)]
            width=max(length(sub(pb,pa)),length(sub(pc,pd)))/TEXEL_SPAN
            height=max(length(sub(pd,pa)),length(sub(pc,pb)))/TEXEL_SPAN
            uv=((0,0),(width,0),(width,height),(0,height))
        self.triangle((a,b,c), uv=uv[:3], **kw)
        self.triangle((a,c,d), uv=(uv[0],uv[2],uv[3]), **kw)

    def box(self, x, y, z, w, h, d, **kw):
        a=(x-w/2,y,z-d/2); b=(x+w/2,y,z-d/2)
        c=(x+w/2,y,z+d/2); e=(x-w/2,y,z+d/2)
        up=lambda p:add(p,(0,h,0))
        self.quad(b,a,up(a),up(b),**kw)
        self.quad(c,b,up(b),up(c),**kw)
        self.quad(e,c,up(c),up(e),**kw)
        self.quad(a,e,up(e),up(a),**kw)
        self.quad(up(a),up(e),up(c),up(b),**kw)
        self.quad(a,b,c,e,**kw)

    def column(self,x,y,z,r,h,top=None,n=12,**kw):
        top=r if top is None else top
        for i in range(n):
            a=i*TAU/n; b=(i+1)*TAU/n
            p=(x+r*math.sin(a),y,z+r*math.cos(a))
            q=(x+r*math.sin(b),y,z+r*math.cos(b))
            u=(x+top*math.sin(a),y+h,z+top*math.cos(a))
            v=(x+top*math.sin(b),y+h,z+top*math.cos(b))
            # Unwrap around the circumference instead of stretching a complete
            # stone tile over every narrow, 830-unit-tall shaft face.
            scale_r=length(sub(self.transform((x+r,y,z)),self.transform((x,y,z))))/r
            arc=TAU*max(r,top)*scale_r/TEXEL_SPAN
            height=length(sub(self.transform((x,y+h,z)),self.transform((x,y,z))))/TEXEL_SPAN
            self.quad(p,q,v,u,uv=((i*arc/n,0),((i+1)*arc/n,0),
                ((i+1)*arc/n,height),(i*arc/n,height)),**kw)
            self.triangle(((x,y+h,z),u,v),uv=((.5,.5),(.5+.46*math.sin(a),.5+.46*math.cos(a)),(.5+.46*math.sin(b),.5+.46*math.cos(b))),**kw)

    def local(self,x,z,angle,scale=(1,1,1)):
        sine=math.sin(angle); cosine=math.cos(angle)
        self.transform=lambda p:(x+p[0]*scale[0]*cosine+p[2]*scale[2]*sine,
                                 p[1]*scale[1]+FLOOR,
                                 z-p[0]*scale[0]*sine+p[2]*scale[2]*cosine)

    def reset(self): self.transform=lambda p:p

    def arch(self,width,bottom,height,depth=90):
        radius=width/2
        for i in range(12):
            a=i*math.pi/12; b=(i+1)*math.pi/12
            point=lambda r,t,z:(r*math.cos(t),bottom+r*math.sin(t),z)
            self.quad(point(radius,a,0),point(radius,b,0),point(radius+75,b,0),point(radius+75,a,0),solid=True)
            self.quad(point(radius,a,0),point(radius,a,depth),point(radius,b,depth),point(radius,b,0),solid=True,tint=(195,185,165))
        for side in (-1,1):
            self.box(side*(radius+38),0,40,76,bottom,depth,solid=True)
            self.box(side*(radius+38),bottom-24,20,110,40,140,solid=True)
        self.box(0,height,50,width+250,65,180,solid=True)

    def chair(self,x,z,angle):
        self.local(x,z,angle,scale=(.55,.55,.55))
        for px in (-48,48):
            for pz in (-45,45): self.box(px,0,pz,18,110,18,material=1,solid=False)
        self.box(0,96,0,130,25,120,material=1,solid=False)
        self.box(0,121,0,109,10,98,material=2,solid=False)
        self.box(0,120,-49,130,174,24,material=1,solid=False)
        self.box(0,145,-34,88,116,5,material=2,solid=False)
        for px in (-66,66):
            self.box(px,110,-45,18,205,20,material=1,solid=False)
            self.column(px,315,-45,17,25,top=0,material=3,solid=False,n=6)
        self.box(0,282,-49,155,22,32,material=3,solid=False)
        # Continuous seat/body and back volumes: Link's lowest wall circle
        # is 30 units high, so separate thin legs leave a walk-through gap.
        self.box(0,0,0,130,131,120,visible=False)
        self.box(0,131,-45,150,184,40,visible=False)
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
                    face=(polygon[0],polygon[j+1],polygon[j])
                    # Shared planar UVs prevent diagonal seams on clipped tiles.
                    self.triangle(tuple((p[0],FLOOR+.5,p[1]) for p in face),
                        uv=tuple(((p[0]-x)/200,(p[1]-z)/200) for p in face),
                        solid=False,tint=(shade,shade,shade))
        # Sixteen-sided hall. The south bay is open to the original door.
        for i in range(SIDES):
            angle=i*TAU/SIDES
            width=2*HALF_DOOR
            self.local(FRONT*math.sin(angle),FRONT*math.cos(angle),angle)
            if i:
                self.quad((width/2,0,25),(-width/2,0,25),(-width/2,1200,25),(width/2,1200,25),tint=(180,175,160))
                # Dark recessed panel visible through a carved arch.
                self.box(0,0,12,width-130,760,12,material=1,solid=True,tint=(210,195,175))
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
        self.local(0,-650,0,scale=(.67,.5,.67))
        self.column(0,0,0,300,55,material=1,solid=False,n=32)
        self.column(0,55,0,175,100,top=240,material=1,solid=False,n=24)
        self.column(0,155,0,540,34,material=1,solid=False,n=48)
        self.column(0,189,0,520,9,material=3,n=48,solid=False)
        self.column(0,198,0,495,3,material=1,solid=False,n=48)
        self.column(0,201,0,185,2,material=3,n=32,solid=False)
        # The native wall solver samples at +30/+130 units, straddling
        # the thin tabletop. A perimeter down to the floor prevents Link's torso
        # entering the overhang while preserving a flat, walkable top.
        self.column(0,0,0,540,201,material=1,n=48,visible=False)
        # Place settings rest on the tabletop, not one unit above its surface.
        for i in range(12):
            angle=i*TAU/12
            x=400*math.sin(angle);z=400*math.cos(angle)
            self.column(x,201,z,35,3,material=3,solid=False)
            self.column(x,204,z,25,2,material=1,solid=False)
        self.reset()
        for i in range(2,11):
            angle=i*TAU/12
            self.chair(460*math.sin(angle),-650+460*math.cos(angle),angle+math.pi)
        # Smaller candle holders sit directly on the 100.5-unit-high tabletop.
        # Their proportions are independent of the broad table's horizontal scale.
        for x,z in ((-134,-650),(134,-650),(0,-810.8)):
            self.local(x,z,0,scale=(.4,.4,.4))
            base=100.5/.4
            self.column(0,base,0,37,15,material=3,solid=False)
            self.column(0,base+15,0,10,105,material=3,solid=False)
            self.box(0,base+108,0,140,12,12,material=3,solid=False)
            for dx in (-60,0,60):
                self.column(dx,base+120,0,8,45,material=0,tint=(255,226,164),solid=False,n=6)
                self.column(dx,base+165,0,9,30,top=0,material=4,tint=(255,212,103),solid=False,n=6)
            self.reset()
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
            # Grounded plinth, narrower shaft and cap support the entire figure.
            self.box(0,0,0,280,35,220)
            self.box(0,35,0,220,290,170)
            self.box(0,325,0,260,35,200)
            self.column(0,360,0,95,260,top=45,solid=False,n=8)
            self.column(0,620,0,48,85,top=18,solid=False,n=8)
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
        # The native shutter stays at its captured position. Build solid wings
        # and a lintel around it, tied into both corridor walls and the ceiling.
        # Recess the door behind a 40-unit stone reveal; keep the centre clear
        # for the native approach, opening animation and +/-240 goal positions.
        side_width=HALF_DOOR-EXIT_HALF_WIDTH
        for sign in (-1,1):
            self.box(sign*(EXIT_HALF_WIDTH+side_width/2),EXIT_FLOOR,EXIT_Z,
                side_width,660,80)
            self.box(sign*(EXIT_HALF_WIDTH+22),EXIT_FLOOR,EXIT_Z-48,
                44,EXIT_HEIGHT,24)
        self.box(0,EXIT_FLOOR+EXIT_HEIGHT,EXIT_Z,
            2*EXIT_HALF_WIDTH,660-EXIT_HEIGHT,80)
        self.box(0,EXIT_FLOOR+EXIT_HEIGHT,EXIT_Z-48,
            2*EXIT_HALF_WIDTH+88,40,24)
        # Keep floor behind the opening for the real door event, and close the
        # far end visibly as well as physically (rather than a black void).
        self.box(0,EXIT_FLOOR,4220,2*HALF_DOOR,660,40,tint=(140,132,120))
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
    # Bits 6..13 are the floor brightness (YukaCol). Zero makes Link
    # black: native lighting multiplies ambient by YukaCol / 100. Use 255
    # to inherit the room lighting, including when standing on furniture.
    ti=append(struct.pack('>4I',0x3f | (0xff << 6),0xff,0xffffff00,0))
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
