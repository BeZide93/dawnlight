#!/usr/bin/env python3
"""Author the Survivors hall. Original geometry; no Elden Ring game assets.

One source produces the visible mesh and native DZB collision. Coordinates are
Twilight Princess units, Y up; the existing hub rest/fire anchor stays (0,-400,0).
Run from any directory. Pillow is needed only for --texture / --preview.
"""
from pathlib import Path
import argparse
import math
import random
import struct

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'res/survivors_roundtable'
FLOOR = -400.0
# Compact layout preferred in build 1295; the exit sits at the hall edge.
RADIUS = 1800.0
SIDES = 16
EAVES_HEIGHT = 1660.0
CROWN_HEIGHT = 2250.0
TORCH_PILLARS = (0,3,4,7,8,15) # Leave the two west fireplace piers bare.
# Mirror paired bays about the entrance/centre axis (index -> -index mod 16).
NICHE_BAYS = (1,3,7,9,13,15)
SHELF_BAYS = (5,11)
FRONT = RADIUS * math.cos(math.pi/SIDES)
HALF_DOOR = RADIUS * math.sin(math.pi/SIDES)
TAU = math.tau
# One atlas quadrant covers this many world units, on both texture axes.
# UV repeats are split in the authoring step so GX_CLAMP cannot bleed into
# neighbouring atlas materials. Collision keeps the unsplit source triangles.
TEXEL_SPAN = 200.0
MASONRY_SPAN = 320.0
EXIT_Z = FRONT+50.0
EXIT_FLOOR = FLOOR
EXIT_BACK_Z = EXIT_Z+900.0
EXIT_HALF_WIDTH = 190.0
EXIT_HEIGHT = 350.0
REST_Z = 760.0
ZELDA_X = -1170.0
ZELDA_Z = 430.0
ZELDA_YAW = round(math.atan2(-ZELDA_X,-ZELDA_Z)*32768/math.pi)
HEARTH_POSITION = (-FRONT+250.0,FLOOR+200.0,0.0)
CHEST_X = FRONT+25-150
LIGHT_POSITION = (0.0, FLOOR+350.0, 0.0)
LIGHTMAP_SIZE = 1024
LIGHTMAP_STEP = 24.0
TEXTURE_SIZE = 1024
# Shared dressing palette: limestone, walnut, faded upholstery and aged bronze.
# Explicit local tints still supply recess dirt and firelight-facing highlights.
ROOM_TINTS = {0:(212,196,165), 1:(194,166,128),
              2:(180,146,121), 3:(154,125,81)}

def torch_layout():
    # Inward faces of six paired piers; flame above Link's head.
    return tuple(((RADIUS*.965-148)*math.sin((i+.5)*TAU/SIDES), FLOOR+435,
                  (RADIUS*.965-148)*math.cos((i+.5)*TAU/SIDES)) for i in TORCH_PILLARS)

def prop_layout():
    """Irregular native Carry placement; bounds reserve their unscaled footprints.
    kind: small pot, large pot, wooden crate, barrel. Dimensions are full extents.
    """
    rng=random.Random(90813);items=[]
    def add_prop(kind,x,y,z,yaw,width,height,depth):
        items.append((kind,x,y,z,yaw,width,height,depth))
    # Unequal groups; leave Zelda, equipment, doorway and the chair ring clear.
    for bay,locals_ in ((2,(-130,40)),(6,(-170,0,170)),(8,(-120,75)),
                       (10,(-135,40)),(14,(-170,0,170))):
        a=bay*TAU/SIDES
        for j,x in enumerate(locals_):
            kind=(bay+j)%4;r=1420+rng.uniform(-50,55)
            if bay==6 and j==1:r-=160 # Keep the native barrel clear of the memorial plinth.
            width,height,depth=((60,100,60),(100,120,100),(75,100,75),(100,120,100))[kind]
            add_prop(kind,r*math.sin(a)+x*math.cos(a),FLOOR,
                     r*math.cos(a)-x*math.sin(a),rng.uniform(-math.pi,math.pi),width,height,depth)
    # A few pieces on unevenly selected tiers, with clear sill and arch margins.
    for bay in NICHE_BAYS+SHELF_BAYS:
        a=bay*TAU/SIDES;niche=bay in NICHE_BAYS
        levels=(95,335,575) if niche else (77,262,447)
        rows=[rng.randrange(3) for _ in range(3)]
        if len(set(rows))==1:rows[2]=(rows[0]+1)%3
        occupied={}
        for item,row in enumerate(rows):
            y=levels[row];kind=(bay+item)%4
            width,height,depth=((60,100,60),(100,120,100),(75,100,75),(100,120,100))[kind]
            slot=occupied.get(row,0);occupied[row]=slot+1
            x=(rng.uniform(-110,-65) if rows.count(row)>1 and slot==0 else
               rng.uniform(65,110) if rows.count(row)>1 else rng.uniform(-95,95))
            z=-69+rng.uniform(-1,1)
            add_prop(kind,FRONT*math.sin(a)+x*math.cos(a)+z*math.sin(a),FLOOR+y,
                     FRONT*math.cos(a)-x*math.sin(a)+z*math.cos(a),a+rng.uniform(-.55,.55),width,height,depth)
    return items

# Keep masonry at its original resolution; wood, cloth and bronze share the
# upper-right area. Four bare-stone variants fill the lower half of the atlas.
MATERIAL_RECTS = {0:(0,0,512,512), 1:(512,0,512,256),
                  2:(512,256,256,256), 3:(768,256,256,256),
                  6:(0,512,512,256), 7:(512,512,512,256),
                  8:(0,768,512,256), 9:(512,768,512,256)}


def add(a, b): return tuple(x+y for x, y in zip(a, b))
def sub(a, b): return tuple(x-y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def length(a): return math.sqrt(sum(x*x for x in a))


class Hall:
    def __init__(self):
        self.palette = ROOM_TINTS
        self.vertices = []
        self.collision = []
        self.transform = lambda p: p
        self.receivers = []
        self.casters = []
        self.light_faces = []
        self.texel_scale = 1.0
        self.sculpture = False

    def triangle(self, points, material=0, tint=None, solid=True, visible=True, uv=None, shadow_face=None):
        if tint is None:tint=self.palette.get(material,(255,255,255))
        p = [self.transform(v) for v in points]
        normal = cross(sub(p[1], p[0]), sub(p[2], p[0]))
        n = length(normal)
        if n < .001:
            return
        if solid:
            self.collision.append(tuple(p))
        if not visible:
            return
        face_id = -1
        if material != 4:
            # Flush paving receives furniture shadows but must not cast onto
            # its own mortar/base plane half a unit below it.
            if not (normal[1]>0 and min(q[1] for q in p)>=FLOOR-.01 and max(q[1] for q in p)<=FLOOR+1):
                self.casters.append(tuple(p))
            face_id=self.receiver(p) if shadow_face is None else shadow_face
        if uv is None:
            uv = ((.03, .03), (.97, .03), (.97, .97))
        # Indirect room lighting plus warm local contributions from both fires.
        # Native actors receive the corresponding runtime point lights.
        shade = .42 + .08*max(0, normal[1]/n) + .04*max(0, -normal[0]/n)
        if self.sculpture:
            center=tuple(sum(q[k] for q in p)/3 for k in range(3))
            direction=sub(LIGHT_POSITION,center);distance=max(1,length(direction))
            shade=.29+.25*max(0,sum(normal[k]*direction[k] for k in range(3))/(n*distance))
        def emit(points, coords):
            for pos, tex in zip(points, coords):
                distance = length(sub(pos,LIGHT_POSITION))
                warm = .65*max(0, 1-distance/1800)**2
                hearth_distance=length(sub(pos,HEARTH_POSITION))
                hearth_warm=.30*max(0,1-hearth_distance/900)**2
                torch_warm=max((.32*max(0,1-length(sub(pos,t))/550)**2 for t in torch_layout()),default=0)
                # Local amber pools on the piers, subordinate to the central flame.
                light = (shade*.94+warm+hearth_warm+torch_warm,
                         shade*.97+warm*.68+hearth_warm*.55+torch_warm*.55,
                         shade+warm*.32+hearth_warm*.20+torch_warm*.18)
                # Bare limestone has a brighter source albedo than masonry.
                # Match both materials before applying the shared room lighting.
                grade=(.70,.71,.72) if material>=6 else (1,1,1)
                light=tuple(a*b for a,b in zip(light,grade))
                dirt=1.0
                if FLOOR<=pos[1]<=FLOOR+5 and normal[1]>0:
                    radius=math.hypot(pos[0],pos[2])
                    dirt=1-.24*max(0,1-abs(radius-1450)/380)*(0.55+.45*math.sin(pos[0]*.011+pos[2]*.007)**2)
                # Light dies out behind the shutter; keep the visible hall untouched.
                portal_dark=1.0
                if abs(pos[0])<=HALF_DOOR+1 and pos[2]>EXIT_Z:
                    portal_dark=max(0.0,1-(pos[2]-EXIT_Z)/240.0)**2
                color = tuple(min(255, round(t*c*dirt*portal_dark)) for t, c in zip(tint, light))
                # Keep bilinear samples inside their atlas rectangle and clear
                # of the reserved 4x4 emissive texels in the stone corner.
                rx,ry,rw,rh=MATERIAL_RECTS.get(material,MATERIAL_RECTS[0])
                u = (rx+(.04+tex[0]*.92)*rw)/TEXTURE_SIZE
                v = (ry+(.04+tex[1]*.92)*rh)/TEXTURE_SIZE
                if material in (4,5):
                    u=v=1.5/TEXTURE_SIZE
                    if material == 4:color=tint
                self.vertices.append((*pos, u, v, 0.0, 0.0, *color, 255))
                self.light_faces.append(face_id)
        if material in (4,5):
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

    def receiver(self,p):
        # Both triangles of a quad share one chart, preserving shadow density
        # while avoiding a second rectangular allocation for the same surface.
        edge=sub(p[1],p[0]);normal=cross(edge,sub(p[2],p[0]))
        u=tuple(x/length(edge) for x in edge)
        v=cross(tuple(x/length(normal) for x in normal),u)
        dot=lambda a,b:sum(x*y for x,y in zip(a,b))
        xs=[dot(sub(q,p[0]),u) for q in p];ys=[dot(sub(q,p[0]),v) for q in p]
        origin=add(p[0],tuple(x*min(xs)+y*min(ys) for x,y in zip(u,v)))
        # Distant upper architecture needs fewer lightmap texels than the floor.
        step=48.0 if min(q[1] for q in p)>FLOOR+1000 else (LIGHTMAP_STEP if max(q[1] for q in p)<=FLOOR+1 else 28.0)
        w=max(2,math.ceil((max(xs)-min(xs))/step)+1)+2
        h=max(2,math.ceil((max(ys)-min(ys))/step)+1)+2
        result=len(self.receivers)
        self.receivers.append({'points':p,'origin':origin,'u':u,'v':v,'w':w,'h':h,'step':step})
        return result

    def quad(self, a, b, c, d, uv=None, **kw):
        if uv is None:
            # Measure after the object transform: scaled furniture must not
            # change the apparent size of wood grain or stone courses.
            pa,pb,pc,pd=[self.transform(p) for p in (a,b,c,d)]
            span=MASONRY_SPAN if kw.get("material",0)==0 else TEXEL_SPAN
            span*=self.texel_scale
            width=max(length(sub(pb,pa)),length(sub(pc,pd)))/span
            height=max(length(sub(pd,pa)),length(sub(pc,pb)))/span
            uv=((0,0),(width,0),(width,height),(0,height))
        points=[self.transform(p) for p in (a,b,c,d)]
        if kw.get('visible',True) and kw.get('material',0)!=4 and length(cross(sub(points[1],points[0]),sub(points[2],points[0])))>.001:
            kw['shadow_face']=self.receiver(points)
        # Bilinear quad UV grids need only two triangles per tile. Clipping two
        # large diagonal triangles independently adds redundant sliver triangles.
        # Preserve the original two collision faces and one lightmap chart.
        rectangular=(
            uv[0]==(0,0) and uv[1][1]==0 and uv[3][0]==0 and
            uv[2]==(uv[1][0],uv[3][1]))
        if rectangular and (uv[1][0]>1 or uv[3][1]>1):
            if kw.get('solid',True):
                self.triangle((a,b,c),visible=False)
                self.triangle((a,c,d),visible=False)
            if not kw.get('visible',True):return
            width,height=uv[1][0],uv[3][1]
            point=lambda u,v:tuple(a[k]*(1-u/width)*(1-v/height)+b[k]*(u/width)*(1-v/height)+c[k]*(u/width)*(v/height)+d[k]*(1-u/width)*(v/height) for k in range(3))
            for row in range(math.ceil(height)):
                for col in range(math.ceil(width)):
                    u1=min(col+1,width);v1=min(row+1,height)
                    pts=(point(col,row),point(u1,row),point(u1,v1),point(col,v1))
                    tex=((0,0),(u1-col,0),(u1-col,v1-row),(0,v1-row))
                    self.triangle(pts[:3],uv=tex[:3],**dict(kw,solid=False))
                    self.triangle((pts[0],pts[2],pts[3]),uv=(tex[0],tex[2],tex[3]),**dict(kw,solid=False))
            return
        self.triangle((a,b,c), uv=uv[:3], **kw)
        self.triangle((a,c,d), uv=(uv[0],uv[2],uv[3]), **kw)

    def box(self, x, y, z, w, h, d, bottom_solid=True, **kw):
        a=(x-w/2,y,z-d/2); b=(x+w/2,y,z-d/2)
        c=(x+w/2,y,z+d/2); e=(x-w/2,y,z+d/2)
        up=lambda p:add(p,(0,h,0))
        self.quad(b,a,up(a),up(b),**kw)
        self.quad(c,b,up(b),up(c),**kw)
        self.quad(e,c,up(c),up(e),**kw)
        self.quad(a,e,up(e),up(a),**kw)
        self.quad(up(a),up(e),up(c),up(b),**kw)
        bottom=dict(kw)
        if not bottom_solid:bottom["solid"]=False
        self.quad(a,b,c,e,**bottom)

    def column(self,x,y,z,r,h,top=None,n=12,uv_offset=(0,0),cap=True,**kw):
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
            span=MASONRY_SPAN if kw.get("material",0)==0 else TEXEL_SPAN
            arc=TAU*max(r,top)*scale_r/span
            height=length(sub(self.transform((x,y+h,z)),self.transform((x,y,z))))/span
            self.quad(p,q,v,u,uv=tuple((u+uv_offset[0],v+uv_offset[1]) for u,v in
                ((i*arc/n,0),((i+1)*arc/n,0),((i+1)*arc/n,height),(i*arc/n,height))),**kw)
            if cap:self.triangle(((x,y+h,z),u,v),uv=((.5,.5),(.5+.46*math.sin(a),.5+.46*math.cos(a)),(.5+.46*math.sin(b),.5+.46*math.cos(b))),**kw)

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
            self.quad(point(radius,a,0),point(radius,a,depth),point(radius,b,depth),point(radius,b,0),solid=True,tint=(194,175,144))
        for side in (-1,1):
            self.box(side*(radius+38),0,40,76,bottom,depth,solid=True)
            self.box(side*(radius+38),bottom-24,20,110,40,140,solid=True)
        self.box(0,height,50,width+250,65,180,solid=True)

    def ring(self,inner,y0,outer,y1,n=48,material=1,tint=None):
        # Open annular moulding: no hidden full disks behind the tabletop.
        for i in range(n):
            a=i*TAU/n;b=(i+1)*TAU/n
            point=lambda r,y,t:(r*math.sin(t),y,r*math.cos(t))
            self.quad(point(inner,y0,a),point(outer,y1,a),
                      point(outer,y1,b),point(inner,y0,b),
                      material=material,tint=tint,solid=False)

    def chair(self,x,z,angle):
        self.local(x,z,angle,scale=(.55,.55,.55))
        for px in (-48,48):
            for pz in (-45,45):
                # Tapered legs with block feet and a turned shoulder.
                self.column(px,0,pz,10,79,top=7,n=4,material=1,solid=False,cap=False)
                self.box(px,0,pz,19,13,19,material=1,solid=False)
                self.box(px,79,pz,18,31,18,material=1,solid=False)
            self.box(px,34,0,9,12,90,material=1,solid=False)
        self.box(0,38,-45,96,10,9,material=1,solid=False)
        self.box(0,96,0,130,25,120,material=1,solid=False)
        self.box(0,121,0,109,10,98,material=2,solid=False)
        self.box(0,120,-49,130,174,24,material=1,solid=False)
        self.box(0,145,-34,88,116,5,material=2,solid=False)
        for px in (-66,66):
            self.box(px,110,-45,18,205,20,material=1,solid=False)
            self.column(px,315,-45,17,25,top=0,material=3,solid=False,n=6)
        # Front and rear framed panels, with a small carved lozenge on the back.
        for depth in (-66,-28):
            for px in (-48,48):self.box(px,139,depth,9,129,8,material=1,solid=False)
            for y in (136,265):self.box(0,y,depth,105,9,8,material=1,solid=False)
        self.box(0,150,-63,77,103,5,material=1,tint=(125,99,68),solid=False)
        for sign in (-1,1):
            for side in (-1,1):
                a=(0,200+sign*39,-67);b=(side*25,200,-67)
                self.quad(a,b,(b[0]-side*5,b[1],b[2]),(0,a[1]-sign*7,a[2]),material=3,solid=False)
        # A curved, bevelled crest replaces the flat rectangular top rail.
        for i in range(8):
            a=-math.pi/2+i*math.pi/8;b=-math.pi/2+(i+1)*math.pi/8
            point=lambda t,y,z:(76*math.sin(t),284+34*math.cos(t)+y,z)
            self.quad(point(a,0,-68),point(b,0,-68),point(b,13,-64),point(a,13,-64),material=1,solid=False)
            self.quad(point(b,0,-28),point(a,0,-28),point(a,13,-32),point(b,13,-32),material=1,solid=False)
            self.quad(point(a,13,-64),point(b,13,-64),point(b,13,-32),point(a,13,-32),material=3,solid=False)
        self.box(0,96,57,116,14,8,material=1,tint=(145,116,81),solid=False)
        for px in (-48,0,48):self.box(px,100,62,6,6,3,material=3,solid=False)
        # Continuous seat/body and back volumes: Link's lowest wall circle
        # is 30 units high, so separate thin legs leave a walk-through gap.
        self.box(0,0,0,130,131,120,visible=False)
        self.box(0,131,-45,150,184,40,visible=False)
        self.reset()

    def log(self, a, b, radius):
        # A faceted timber along an arbitrary axis, with visible cut end-grain.
        direction=sub(b,a);span=length(direction);axis=tuple(v/span for v in direction)
        u=cross(axis,(0,1,0));u=tuple(v/length(u) for v in u);v=cross(axis,u)
        def rim(p,i,r=radius):
            angle=i*TAU/8
            return add(p,tuple(r*(x*math.cos(angle)+y*math.sin(angle)) for x,y in zip(u,v)))
        for i in range(8):
            self.quad(rim(a,i),rim(a,i+1),rim(b,i+1),rim(b,i),
                material=1,solid=False,tint=(92,68,47))
            for end,reverse in ((a,True),(b,False)):
                ring=(end,rim(end,i),rim(end,i+1))
                self.triangle(tuple(reversed(ring)) if reverse else ring,
                    material=1,solid=False,tint=(150,104,60))

    def fireplace(self):
        # Preserve the approved hearth colours, including its untinted highlights.
        palette=self.palette;self.palette={}
        first_collision=len(self.collision)
        self.local(-FRONT+110,0,-math.pi/2)
        # Shallow, grounded hearth with a chamfered-looking stepped apron.
        self.box(0,0,-55,680,14,340,tint=(167,153,128))
        self.box(0,14,-45,644,14,300,tint=(195,180,151))
        self.box(0,28,-20,602,14,250,tint=(143,130,110))
        # The inner walls are stone, not a flat black wooden rectangle.
        self.box(0,42,82,410,350,28,tint=(88,75,61))
        for side in (-1,1):self.box(side*204,42,0,32,350,165,tint=(106,89,70))
        for row in range(7):
            for col in range(5):
                x=-158+col*79+(18 if row%2 else 0)
                self.box(x,44+row*47,65,75,43,7,solid=False,
                    tint=(95+row*3,79+row*2,62+row*2))
        # Heavy jambs with layered bases, fluted fronts and capital mouldings.
        for side in (-1,1):
            x=side*263
            self.box(x,42,0,112,330,190)
            for y,w,h,d in ((28,145,28,225),(56,124,22,207),(330,126,18,215),
                            (348,145,18,232),(366,165,22,248)):
                self.box(x,y,-12,w,h,d,tint=(212,196,165))
            for dx in (-29,0,29):
                self.column(x+dx,80,-101,10,245,n=6,solid=False,tint=(199,180,146))
            # Projecting corbel blocks under the mantel's outer ends.
            for step in range(4):self.box(x,282+step*20,-106-step*9,82+step*12,20,38+step*8)
        # A broad carved frieze, dentils and projecting mantel shelf.
        self.box(0,388,0,674,66,238,tint=(194,175,144))
        self.box(0,402,-123,604,37,12,tint=(123,107,84),solid=False)
        for x in range(-280,281,40):
            self.box(x,411,-135,17,17,12,material=3,solid=False,tint=(175,144,89))
            self.box(x,456,-130,20,20,28,tint=(212,194,160),solid=False)
        for y,w,h,d in ((454,706,12,270),(476,730,18,305),(494,700,18,280)):
            self.box(0,y,-8,w,h,d,tint=(213,197,169))
        # Chimney breast with recessed panel and matching upper cornice.
        self.box(0,512,25,570,350,150,tint=(172,158,134))
        self.box(0,548,-55,420,247,10,tint=(133,119,96),solid=False)
        for x in (-221,221):self.box(x,535,-62,22,270,25,solid=False)
        for y in (535,797):self.box(0,y,-62,464,17,25,solid=False)
        for y,w,h,d in ((850,614,20,195),(870,650,24,225),(894,620,22,205)):
            self.box(0,y,18,w,h,d,tint=(209,193,163))
        # Low iron andirons and grate; logs lie across it at different angles.
        for x in (-137,137):
            self.box(x,42,-38,13,22,175,material=3,tint=(65,60,51),solid=False)
            self.column(x,42,-117,10,98,top=7,n=6,material=3,tint=(79,69,53),solid=False)
            self.column(x,140,-117,15,13,n=8,material=3,tint=(118,91,55),solid=False)
        for z in (-90,-40,10):self.box(0,63,z,288,9,9,material=3,tint=(63,56,46),solid=False)
        for a,b,r in (((-147,84,-92),(138,83,-60),18),((-130,87,-6),(145,89,20),19),
                      ((-112,113,-115),(96,116,12),17),((130,113,-109),(-70,116,17),18)):
            self.log(a,b,r)
        # Small irregular ember clusters instead of large orange disks.
        for i in range(23):
            x=((i*67)%283)-141;z=((i*41)%129)-100
            self.column(x,43,z,5+(i%4)*2,4+(i%3),top=3,n=5,material=4,
                tint=((220,76,12) if i%3 else (255,151,34)),solid=False)
        # Low decorative undersides are not ceilings. Native Acch clamps its
        # ground ray against RoofChk; stacked 14-unit slabs otherwise present
        # a false roof below Link's body and can drive him beneath the floor.
        self.collision[first_collision:]=[p for p in self.collision[first_collision:]
            if not (cross(sub(p[1],p[0]),sub(p[2],p[0]))[1]<-.001 and max(v[1] for v in p)<FLOOR+180)]
        self.reset()
        self.palette=palette

    def wall_bay(self,index,width):
        # Structural backing stays behind the recesses. Each bay projects into
        # the room by at most 140 units, away from every interaction route.
        shelving=index in SHELF_BAYS
        niches=index in NICHE_BAYS
        self.box(0,0,12,width-130,760,12,material=0 if niches else 1,
            bottom_solid=False,tint=(145,137,116) if niches else ROOM_TINTS[1])
        if shelving:
            # Walnut joinery echoes the mantel: recessed panels, moulded rails,
            # stepped cornice and small corbels. A solid low plinth blocks Link,
            # while open compartments let native Carry objects rest and move.
            self.box(0,0,-52,460,60,138,material=1,bottom_solid=False,tint=(194,166,128))
            wood=dict(material=1,solid=False,tint=(194,166,128))
            dark=dict(material=1,solid=False,tint=(115,92,66))
            for x in (-216,216):
                self.box(x,0,-50,28,652,126,**dict(wood,solid=True),bottom_solid=False)
                self.box(x,34,-117,42,594,18,**wood)
                self.box(x,80,-128,19,488,8,**dark)
                for dx in (-13,13):self.box(x+dx,74,-134,5,501,9,**wood)
                for y,w,h,d in ((0,62,26,156),(26,48,18,143),
                                  (610,49,15,144),(625,62,22,158)):
                    self.box(x,y,-51,w,h,d,**wood)
            for y in (60,245,430,615):
                # Separate recessed back panels give the grain a smaller scale.
                if y<615:
                    for x in (-128,0,128):
                        self.box(x,y+31,-3,116,147,10,**dark)
                        for dx in (-58,58):self.box(x+dx,y+24,-13,7,160,13,**wood)
                    for py in (y+24,y+177):self.box(0,py,-13,390,7,13,**wood)
                self.box(0,y,-57,458,17,145,**dict(wood,solid=True),bottom_solid=False)
                for py,w,h,d in ((y-5,470,8,15),(y+3,476,8,19),(y+11,465,9,12)):
                    self.box(0,py,-132,w,h,d,**wood)
                for x in (-182,182):
                    for step in range(3):
                        self.box(x,y-36+step*10,-104-step*8,24+step*4,10,20+step*10,**wood)
            for y,w,h,d in ((647,480,12,153),(659,508,16,178),(675,492,12,164)):
                self.box(0,y,-50,w,h,d,**wood)
            for x in range(-180,181,45):self.box(x,635,-128,15,15,17,**wood)
        elif niches:
            # A real stone plinth stops Link's low wall circle below the first
            # sill. Keep the recesses hollow for native Carry physics; a tall
            # invisible box would push pots out or onto the top of the cabinet.
            self.box(0,0,-49,430,65,160,bottom_solid=False,tint=(194,175,144))
            # Warm carved limestone matches the fireplace rather than the cool
            # smooth shaft atlas. Layered archivolts have real stone joints.
            stone=dict(solid=False,tint=(212,196,165))
            for x in (-238,238):
                self.box(x,0,-49,85,775,160,bottom_solid=False,tint=(194,175,144))
                for y,w,h,d in ((0,110,26,159),(26,96,18,150),(746,105,18,158),(764,120,18,171)):
                    self.box(x,y,-49,w,h,d,**stone)
                for y in (116,356,596):
                    self.box(x,y,-117,55,112,12,tint=(133,119,96),solid=False)
                    for dx in (-23,23):self.box(x+dx,y-4,-126,9,120,14,**stone)
            for row,y in enumerate((65,305,545)):
                self.box(0,y,-43,402,30,139,bottom_solid=False,tint=(194,175,144))
                self.box(0,y+22,4,390,175,8,tint=(88,75,61),solid=False)
                for py,w,h,d in ((y-12,432,12,160),(y,418,10,151),(y+22,408,8,147)):
                    self.box(0,py,-49,w,h,d,**dict(stone,solid=True),bottom_solid=False)
                for side in (-1,1):
                    self.box(side*187,y+30,-57,27,48,120,**stone)
                    self.box(side*187,y+69,-63,45,13,134,**stone)
                for j in range(11):
                    a=j*math.pi/11+.006;b=(j+1)*math.pi/11-.006
                    pt=lambda r,t,z:(r*math.cos(t),y+78+r*.65*math.sin(t),z)
                    for inner,outer,front in ((173,204,-119),(204,214,-126)):
                        tone=202+(j*7+row*5)%17
                        kw=dict(solid=False,tint=(tone,tone-17,tone-44))
                        self.quad(pt(inner,a,front),pt(inner,b,front),pt(outer,b,front),pt(outer,a,front),**kw)
                        self.quad(pt(inner,a,front),pt(inner,a,0),pt(inner,b,0),pt(inner,b,front),solid=False,tint=(143,125,99))
                        # Joint reveals and outer returns make each voussoir solid.
                        self.quad(pt(inner,a,0),pt(inner,a,front),pt(outer,a,front),pt(outer,a,0),**kw)
                        self.quad(pt(outer,b,0),pt(outer,b,front),pt(inner,b,front),pt(inner,b,0),**kw)
                self.box(0,y+185,-130,33,45,24,**stone)
                self.box(0,y+225,-130,45,8,29,**stone)
        elif index not in (0,4,12):
            # Layered wall panels and projecting stone buttresses break up the
            # remaining bays without blocking memorials or the equipment chest.
            for x in (-185,185):self.box(x,0,-25,36,700,60,bottom_solid=False)
            for y in (120,400,680):self.box(0,y,-16,350,18,40,material=3,tint=(143,119,78),solid=False)
        if index not in (0,12):
            # Upper stone course, alternating corbels and inset relief panels.
            self.box(0,790,-18,width-145,32,65,solid=False)
            for x in (-190,0,190):
                self.box(x,824,-8,140,104,25,tint=(147,135,111),solid=False)
                self.box(x,812,-26,154,14,55,solid=False)
                self.box(x,928,-26,154,14,55,solid=False)
                self.box(x,970,-25,40,30,78,solid=False)

    @staticmethod
    def clip_polygon(polygon,nx,nz,limit):
        result=[]
        for i,p in enumerate(polygon):
            q=polygon[(i+1)%len(polygon)]
            a=nx*p[0]+nz*p[1]-limit;b=nx*q[0]+nz*q[1]-limit
            if a<=0:result.append(p)
            if (a<0)!=(b<0):
                t=a/(a-b);result.append((p[0]+t*(q[0]-p[0]),p[1]+t*(q[1]-p[1])))
        return result

    def floor_stone(self,polygon,rng,y=FLOOR+.5,tint=None,tilt=(0,0),origin=(0,0),edges=False,material=None,uv_polygon=None):
        if len(polygon)<3:return
        # One chart/UV plane per convex slab prevents diagonal texture seams.
        if sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(polygon,polygon[1:]+polygon[:1]))>0:
            polygon=polygon[::-1]
        points=[(x,y+tilt[0]*(x-origin[0])+tilt[1]*(z-origin[1]),z) for x,z in polygon]
        if length(cross(sub(points[1],points[0]),sub(points[2],points[0])))<.001:return
        face=self.receiver(points)
        angle=rng.uniform(0,TAU);co=math.cos(angle);si=math.sin(angle)
        coords=[(x*co-z*si,x*si+z*co) for x,z in polygon]
        bounds=[(x*co-z*si,x*si+z*co) for x,z in (uv_polygon or polygon)]
        xmin=min(u for u,v in bounds);zmin=min(v for u,v in bounds)
        span=max(max(u for u,v in bounds)-xmin,max(v for u,v in bounds)-zmin,1)
        crop=rng.uniform(.65,.93);u0=rng.uniform(0,1-crop);v0=rng.uniform(0,1-crop)
        uv=[(u0+(u-xmin)/span*crop,v0+(v-zmin)/span*crop) for u,v in coords]
        # Warm, restrained limestone tones: no bright/cool checkerboard tiles.
        shade=190+round(4*math.sin(origin[0]*.003)+3*math.cos(origin[1]*.004))+rng.randrange(-4,5)
        tint=tint or (shade,shade-21,shade-44)
        material=material if material is not None else rng.choice((6,6,7,7,8,9))
        for j in range(1,len(points)-1):
            self.triangle((points[0],points[j],points[j+1]),uv=(uv[0],uv[j],uv[j+1]),
                material=material,tint=tint,solid=False,shadow_face=face)
        if edges:
            # Closed thin stone edges reach the backing floor; no floating slabs
            # or undersides in native collision. Dark fresh fractures read at a
            # grazing angle even below the static lightmap's sample spacing.
            edge_tint=tuple(round(v*.67) for v in tint)
            for a,b in zip(points,points[1:]+points[:1]):
                self.quad(a,(a[0],FLOOR+.05,a[2]),(b[0],FLOOR+.05,b[2]),b,
                          material=material,tint=edge_tint,solid=False)

    def paving(self,boundary):
        # Worn rectangular flags in staggered, unequal courses, like the reference
        # hall. No large polygon cells or decorative circular floor inlay.
        # This is visual paving over the unchanged continuous native collider.
        rng=random.Random(0x524F554E44)
        wear=random.Random(0x57454152)
        anchors=[(ZELDA_X,ZELDA_Z,200),(CHEST_X,0,220),(0,REST_Z,170),
                 (0,EXIT_Z-100,280),(-FRONT+200,0,430)]
        # Keep the supply alcoves level, independently of live/broken Carry
        # objects. Their removal must never change baked room geometry/shadows.
        anchors.extend((1420*math.sin(bay*TAU/SIDES),1420*math.cos(bay*TAU/SIDES),280)
                       for bay in (2,6,8,10,14))
        z=-RADIUS-160
        while z<RADIUS:
            depth=rng.uniform(55,82)
            x=-RADIUS-rng.uniform(60,230)
            while x<RADIUS:
                width=rng.uniform(65,135)
                gap=rng.uniform(.7,1.0)
                x0=x+gap;x1=x+width-gap;z0=z+gap;z1=z+depth-gap
                # Each corner has its own small chip, not repeated rectangular
                # outlines. Thin gray-brown mortar stays below the stone tops.
                cx=(x0+x1)/2;cz=(z0+z1)/2
                clear=math.hypot(cx,cz)>650 and all(math.hypot(cx-a,cz-b)>r for a,b,r in anchors)
                damage=wear.random() if clear else 1.0
                broken=damage<.055;raised=.055<=damage<.12
                # A few chipped corners per intact plate; larger irregular
                # losses on the damaged plates rather than the same eight edges.
                corners=[(x0,z0),(x1,z0),(x1,z1),(x0,z1)]
                chipped=wear.sample(range(4),4 if broken or raised else (1 if wear.random()<.6 else 2))
                poly=[]
                for i,c in enumerate(corners):
                    if i not in chipped:poly.append(c);continue
                    amount=wear.uniform(4,13) if broken else wear.uniform(1,4)
                    for neighbour in (corners[(i-1)%4],corners[(i+1)%4]):
                        dx=neighbour[0]-c[0];dz=neighbour[1]-c[1];dist=math.hypot(dx,dz)
                        poly.append((c[0]+dx/dist*amount,c[1]+dz/dist*amount))
                for i,a in enumerate(boundary):
                    b=boundary[(i+1)%len(boundary)]
                    nx=a[2]-b[2];nz=b[0]-a[0]
                    poly=self.clip_polygon(poly,nx,nz,nx*a[0]+nz*a[2])
                    if not poly:break
                if poly:
                    if broken:
                        angle=wear.uniform(.25,2.8);nx=math.cos(angle);nz=math.sin(angle)
                        line=nx*cx+nz*cz
                        pieces=(self.clip_polygon(poly,nx,nz,line-1.1),
                                self.clip_polygon(poly,-nx,-nz,-line-1.1))
                    else:pieces=(poly,)
                    material=wear.choice((6,6,7,7,8,9))
                    stone_seed=wear.randrange(1<<30)
                    for part in pieces:
                        slope=wear.uniform(0,TAU)
                        # Less than five units of relief: visible unevenness,
                        # while the continuous support stays safely at FLOOR.
                        tilt=(.025*math.cos(slope),.025*math.sin(slope)) if broken or raised else (0,0)
                        self.floor_stone(part,random.Random(stone_seed),y=FLOOR+(2.3 if broken or raised else .5),
                            tilt=tilt,origin=(cx,cz),edges=broken or raised,material=material,uv_polygon=poly)
                x+=width
            z+=depth

    def floor_clutter(self):
        # Unequal, asymmetric scatter patches, with independently rotated pieces.
        # Keep travel/interaction anchors free. No decorative piece enters DZB.
        rng=random.Random(0xC1A77E2)
        anchors=((ZELDA_X,ZELDA_Z),(CHEST_X,0),(0,REST_Z),(-1515,0))
        for cluster in range(19):
            angle=rng.uniform(.35,TAU-.35);radius=rng.uniform(1120,1490)
            cx=radius*math.sin(angle);cz=radius*math.cos(angle)
            spread=rng.uniform(35,135);kind=rng.randrange(3)
            for j in range(rng.randrange(2,11)):
                x=cx+rng.gauss(0,spread);z=cz+rng.gauss(0,spread*.6)
                if math.hypot(x,z)>1550 or any(math.hypot(x-a,z-b)<240 for a,b in anchors):continue
                self.local(x,z,rng.uniform(0,TAU))
                item=kind if rng.random()<.65 else rng.randrange(3)
                if item==0:
                    w=rng.uniform(19,45);d=rng.uniform(24,57);shade=rng.randrange(125,184)
                    poly=[(-w*.5,-d*.5),(-w*.5,d*.38),(-w*.25,d*.5),(w*.48,d*.43),(w*.5,-d*.46)]
                    for k in range(1,len(poly)-1):
                        self.triangle(tuple((a,1.1,b) for a,b in (poly[0],poly[k],poly[k+1])),
                            material=5,tint=(round(shade*.76),round((shade-14)*.76),round((shade-42)*.76)),solid=False)
                    for line in range(rng.randrange(2,6)):
                        zz=-d*.25+line*4
                        self.quad((-w*.3,1.2,zz),(-w*.3,1.2,zz+1),
                            (w*rng.uniform(.05,.3),1.2,zz+1),(w*.25,1.2,zz),
                            material=5,tint=(75,69,55),solid=False)
                elif item==1:
                    w=rng.uniform(35,110);d=rng.uniform(7,19)
                    self.box(0,1,0,w,rng.uniform(2,5),d,material=1,tint=(144,126,97),solid=False)
                else:
                    self.column(0,.7,0,rng.uniform(4,15),rng.uniform(3,10),
                        top=rng.uniform(1,4),n=rng.randrange(4,7),material=rng.randrange(6,10),
                        tint=(165,150,126),solid=False)
                self.reset()

    def ellipsoid(self,center,radii,segments=12,rings=6,**kw):
        # Sculpted volumes, with deliberate seams along the back rather than
        # stacked cones. All statue detail is visual; the plinth owns collision.
        def pt(j,i):
            v=-math.pi/2+math.pi*j/rings;u=TAU*i/segments
            return (center[0]+radii[0]*math.cos(v)*math.sin(u),
                    center[1]+radii[1]*math.sin(v),center[2]+radii[2]*math.cos(v)*math.cos(u))
        for j in range(rings):
            for i in range(segments):
                self.quad(pt(j,i),pt(j,i+1),pt(j+1,i+1),pt(j+1,i),
                    uv=((i/segments,j/rings),((i+1)/segments,j/rings),
                        ((i+1)/segments,(j+1)/rings),(i/segments,(j+1)/rings)),solid=False,**kw)

    def limb(self,points,radii,**kw):
        # Tube along a bent arm/sleeve, avoiding straight rectangular appendages.
        rings=[];sides=8
        for i,(p,r) in enumerate(zip(points,radii)):
            tangent=sub(points[min(i+1,len(points)-1)],points[max(i-1,0)])
            tangent=tuple(v/length(tangent) for v in tangent)
            u=cross(tangent,(0,0,1));u=tuple(v/length(u) for v in u);v=cross(tangent,u)
            rings.append([tuple(p[k]+r*(math.cos(j*TAU/sides)*u[k]+math.sin(j*TAU/sides)*v[k]) for k in range(3)) for j in range(sides)])
        for a,b in zip(rings,rings[1:]):
            for j in range(sides):self.quad(a[j],a[(j+1)%sides],b[(j+1)%sides],b[j],solid=False,**kw)

    def statue(self):
        stone=dict(material=7,tint=(236,222,195))
        # Existing grounded plinth footprint remains solid and walk-blocking.
        self.box(0,0,0,280,35,220)
        self.box(0,35,0,220,290,170)
        for y,w,h,d in ((35,236,18,186),(302,232,23,186),(325,260,35,200)):
            self.box(0,y,0,w,h,d,solid=False,tint=(194,175,144))
        for x in (-91,91):self.box(x,68,-89,13,205,11,solid=False)
        self.box(0,86,-88,130,170,8,solid=False,tint=(145,127,99))
        for x in (-74,74):self.box(x,78,-94,8,188,12,solid=False)
        for y in (78,258):self.box(0,y,-94,156,8,12,solid=False)
        # Broad hem -> hips -> shoulders. Alternating pleats continue down the
        # robe, with a draped asymmetric fold instead of a pointed cone body.
        self.sculpture=True
        levels=((360,87,54),(386,83,50),(455,73,42),(535,58,33),
                (610,47,30),(674,54,32),(719,53,28),(746,33,23))
        count=24
        def point(level,j):
            y,w,d=level;a=j*TAU/count+.07*math.sin((y-360)/120)
            fold=(14 if j%3==0 else -5)*(1-(y-360)/650)
            sway=7*math.sin((y-360)/480*math.pi)
            return (sway+(w+fold)*math.sin(a),y+(3*math.sin(a*3) if y==360 else 0),(d+fold)*math.cos(a))
        for row in range(len(levels)-1):
            for j in range(count):
                self.quad(point(levels[row],j),point(levels[row],(j+1)%count),
                          point(levels[row+1],(j+1)%count),point(levels[row+1],j),solid=False,**stone)
        # Front mantle sweeping from shoulders to the opposite hip.
        for side in (-1,1):
            self.limb([(side*45,724,-3),(side*64,674,-13),(side*61,633,-39)], [21,18,16],**stone)
            self.limb([(side*61,633,-39),(side*47,635,-61),(side*18,641,-70)], [17,14,10],**stone)
            self.ellipsoid((side*16,641,-70),(15,10,13),segments=6,rings=4,**stone)
            # Finger ridges and a deep edge on the cuffs.
            for k in range(3):self.limb([(side*(8+k*5),638,-82),(side*(8+k*5),646,-80)],[1.6,1.4],**stone)
        self.ellipsoid((0,752,0),(18,27,18),segments=10,rings=4,**stone)
        self.ellipsoid((0,790,-1),(29,44,26),segments=12,rings=6,**stone)
        # Hair/cowl behind the face, cheekbones, brows, nose and closed mouth.
        for side in (-1,1):
            self.ellipsoid((side*26,781,7),(9,41,22),segments=6,rings=5,**stone)
            self.ellipsoid((side*12,786,-22),(10,9,4),segments=6,rings=4,**stone)
            self.ellipsoid((side*11,803,-24),(10,2,3),segments=6,rings=3,**stone)
            self.ellipsoid((side*11,798,-25),(5,1.6,1.4),segments=6,rings=3,material=7,tint=(112,99,79))
        self.ellipsoid((0,786,-31),(4,11,7),segments=6,rings=4,**stone)
        self.ellipsoid((0,770,-25),(7,1.4,2),segments=6,rings=3,material=7,tint=(145,125,98))
        # Raised mantle seams sweep over the torso into the long robe folds.
        for side in (-1,1):
            points=[(side*44,724,-24),(side*30,704,-36),(side*10,676,-41),
                    (-side*14,622,-38),(-side*30,552,-42),(-side*41,455,-51)]
            for a,b in zip(points,points[1:]):
                self.quad(add(a,(-3,0,0)),add(b,(-3,0,0)),add(b,(3,0,-3)),add(a,(3,0,-3)),solid=False,**stone)
        self.sculpture=False
        # A carved votive bowl cradled in both hands, with a bronze leaf motif.
        self.column(0,649,-70,18,8,top=25,n=10,material=3,solid=False)
        for i in range(5):
            a=i*TAU/5
            self.triangle(((0,656,-70),(25*math.sin(a),671,-70+25*math.cos(a)),
                           (12*math.sin(a+.6),658,-70+12*math.cos(a+.6))),material=3,solid=False)

    def pillar_torches(self):
        for i in TORCH_PILLARS:
            a=(i+.5)*TAU/SIDES
            self.local(RADIUS*.965*math.sin(a),RADIUS*.965*math.cos(a),a)
            metal=dict(material=3,solid=False,tint=(122,96,62))
            self.box(0,275,-83,28,145,13,**metal)
            for y in (285,402):self.column(0,y,-92,9,8,n=6,cap=False,**metal)
            self.box(0,309,-116,15,15,66,**metal)
            self.column(0,307,-148,12,98,top=18,n=6,cap=False,material=1,solid=False,tint=(102,77,47))
            self.column(0,398,-148,18,31,top=32,n=6,cap=False,**metal)
            self.column(0,429,-148,33,6,n=6,cap=False,**metal)
            for side in (-1,1):
                self.box(side*27,422,-148,5,29,8,**metal)
            self.reset()

    def upper_hall(self,boundary):
        self.texel_scale=2.0
        # Raised clerestory: recessed walnut panels framed by carved stone.
        for i in range(SIDES):
            a=i*TAU/SIDES;self.local(FRONT*math.sin(a),FRONT*math.cos(a),a)
            width=2*HALF_DOOR
            for y,h,d in ((1100,26,90),(1450,24,100),(1510,32,130),(1595,35,160)):
                self.quad((-width/2,y,-15-d/2),(width/2,y,-15-d/2),(width/2,y+h,-15-d/2),(-width/2,y+h,-15-d/2),solid=False,tint=(194,175,144))
                self.quad((-width/2,y,-15-d/2),(-width/2,y,25),(width/2,y,25),(width/2,y,-15-d/2),solid=False)
            for x in (-155,155):
                self.quad((x-85,1140,-13),(x+85,1140,-13),(x+85,1422,-13),(x-85,1422,-13),material=1,solid=False,tint=(117,88,56))
                for dx in (-82,82):
                    l=x+dx-6.5;r=x+dx+6.5
                    self.quad((l,1135,-40),(r,1135,-40),(r,1375,-40),(l,1375,-40),solid=False)
                    for side in (l,r):self.quad((side,1135,-40),(side,1375,-40),(side,1375,-10),(side,1135,-10),solid=False)
                # Small pointed blind arches with a central carved leaf.
                for side in (-1,1):
                    for j in range(4):
                        t=j/4;u=(j+1)/4
                        p=(x+side*82*(1-t),1370+75*math.sin(t*math.pi/2),-41)
                        q=(x+side*82*(1-u),1370+75*math.sin(u*math.pi/2),-41)
                        self.quad(p,q,add(q,(0,11,0)),add(p,(0,11,0)),solid=False)
                self.quad((x,1190,-27),(x+20,1224,-27),(x,1260,-27),(x-20,1224,-27),material=3,solid=False)
            self.reset()
        # Piers continue above the existing capitals into ornate corbels.
        for x,_,z in boundary:
            x*=.965;z*=.965
            for y,r,h,top in ((1032,70,430,62),(1462,96,28,110),(1490,110,35,130),(1525,130,35,145)):
                self.column(x,FLOOR+y,z,r,h,top=top,n=8,cap=False,solid=False)
        # Curved stone ribs, three exposed faces, spring from each capital and
        # converge on a layered central rosette. Timber infill follows the vault.
        def roof(r):return EAVES_HEIGHT+(CROWN_HEIGHT-EAVES_HEIGHT)*math.sqrt(max(0,1-(r/RADIUS)**2))
        for i,a in enumerate(boundary):
            b=boundary[(i+1)%SIDES];radial=(a[0]/RADIUS,0,a[2]/RADIUS);side=(-radial[2],0,radial[0])
            for j in range(8):
                r0=RADIUS*j/8;r1=RADIUS*(j+1)/8
                p=(radial[0]*r0,FLOOR+roof(r0)-30,radial[2]*r0)
                q=(radial[0]*r1,FLOOR+roof(r1)-30,radial[2]*r1)
                edge=lambda v,w:tuple(v[k]+side[k]*w for k in range(3))
                if i%2==0:
                    for sideSign in (-1,1):
                        self.quad(edge(p,sideSign*34),edge(q,sideSign*34),
                                  add(edge(q,sideSign*34),(0,48,0)),add(edge(p,sideSign*34),(0,48,0)),solid=False)
                    self.quad(edge(p,-34),edge(q,-34),edge(q,34),edge(p,34),solid=False,tint=(212,196,165))
                pa=(a[0]*r0/RADIUS,FLOOR+roof(r0)+20,a[2]*r0/RADIUS)
                pb=(b[0]*r0/RADIUS,FLOOR+roof(r0)+20,b[2]*r0/RADIUS)
                qa=(a[0]*r1/RADIUS,FLOOR+roof(r1)+20,a[2]*r1/RADIUS)
                qb=(b[0]*r1/RADIUS,FLOOR+roof(r1)+20,b[2]*r1/RADIUS)
                self.quad(pa,pb,qb,qa,material=1,solid=False,tint=(142,112,79))
        for y,r,h,top in ((CROWN_HEIGHT-92,90,18,120),(CROWN_HEIGHT-74,120,24,150),(CROWN_HEIGHT-50,150,20,150)):
            self.column(0,FLOOR+y,0,r,h,top=top,n=16,material=0,solid=False,cap=False)
        self.column(0,FLOOR+CROWN_HEIGHT-95,0,90,10,n=16,material=3,solid=False)
        self.texel_scale=1.0

    def build(self):
        boundary=[(RADIUS*math.sin((i+.5)*TAU/SIDES),FLOOR,RADIUS*math.cos((i+.5)*TAU/SIDES)) for i in range(SIDES)]
        for i,a in enumerate(boundary):
            b=boundary[(i+1)%SIDES]
            self.triangle(((0,FLOOR,0),a,b),material=5,tint=(83,69,51))
        self.paving(boundary)
        support=[(x*(RADIUS+180)/RADIUS,FLOOR,z*(RADIUS+180)/RADIUS) for x,_,z in boundary]
        for i,a in enumerate(support):
            self.triangle(((0,FLOOR,0),a,support[(i+1)%SIDES]),visible=False)
        # Sixteen-sided hall. The south bay is open to the original door.
        for i in range(SIDES):
            angle=i*TAU/SIDES
            width=2*HALF_DOOR
            self.local(FRONT*math.sin(angle),FRONT*math.cos(angle),angle)
            if i:
                self.quad((width/2,0,25),(-width/2,0,25),(-width/2,1100,25),(width/2,1100,25),tint=(194,175,144))
                self.texel_scale=2.0
                self.quad((width/2,1100,25),(-width/2,1100,25),(-width/2,EAVES_HEIGHT,25),(width/2,EAVES_HEIGHT,25),tint=(194,175,144))
                self.texel_scale=1.0
                self.wall_bay(i,width)
            self.arch(width-175,460,920)
            self.box(0,1040,0,width+10,100,100,material=1,solid=False)
            self.box(0,1000,-8,width+10,30,140,material=3,solid=False)
            self.reset()
        # Carved limestone piers share the fireplace's material and warm tint.
        # Retain the existing simple collision envelope; small mouldings are visual.
        for x,_,z in boundary:
            x*=.965;z*=.965
            self.column(x,FLOOR,z,123,52,n=8,visible=False)
            self.column(x,FLOOR+52,z,103,50,n=12,visible=False)
            self.column(x,FLOOR+102,z,78,830,top=70,n=12,visible=False)
            self.column(x,FLOOR+932,z,98,45,n=12,visible=False)
            self.column(x,FLOOR+977,z,130,55,n=8,visible=False)
            stone=dict(material=0,solid=False,cap=False,tint=(212,196,165))
            for y,r,h,top,n in ((0,123,22,123,8),(22,123,13,112,8),
                    (35,112,17,112,12),(52,103,18,103,12),
                    (70,103,16,88,12),(86,88,16,88,12),
                    (912,78,20,90,12),(932,98,16,98,12),
                    (948,98,29,116,12),(977,130,18,130,8),
                    (995,130,12,120,8),(1007,120,25,120,8)):
                self.column(x,FLOOR+y,z,r,h,top=top,n=n,**stone)
            rng=random.Random(round(x*17+z*31))
            base=102
            for height in (108,140,140,140,140,142):
                radius0=78-(base-102)*8/830;radius1=78-(base+height-102)*8/830
                tone=rng.randrange(193,213)
                self.column(x,FLOOR+base,z,radius0,height-3,top=radius1,n=12,
                    material=0,solid=False,cap=False,uv_offset=(rng.random()*4,rng.random()*4),
                    tint=(tone,tone-17,tone-43))
                # Recessed drum seam, with a narrow bevel on each dressed edge.
                self.column(x,FLOOR+base+height-3,z,radius1-2,3,n=12,
                    solid=False,cap=False,tint=(123,107,84))
                base+=height
            # Clustered attached shafts and their feet give the broad pier relief.
            inward=math.atan2(-x,-z)
            for offset in (-.65,0,.65):
                a=inward+offset
                px=x+69*math.sin(a);pz=z+69*math.cos(a)
                self.column(px,FLOOR+126,pz,9,746,top=8,n=5,**stone)
                for y in (110,872):self.column(px,FLOOR+y,pz,14,16,n=5,**stone)
        self.upper_hall(boundary)
        # Central table, with a complete circle of twelve chairs. The rest
        # pose and interactions remain outside this furniture footprint.
        self.local(0,0,0,scale=(.67,.5,.67))
        self.column(0,0,0,300,24,material=1,solid=False,n=32,cap=False)
        self.ring(300,24,280,38,n=32)
        self.column(0,38,0,280,17,material=1,solid=False,n=32)
        self.column(0,55,0,175,100,top=240,material=1,solid=False,n=24,cap=False)
        for y,r in ((60,182),(135,228)):
            self.column(0,y,0,r,9,material=3,n=24,solid=False,cap=False)
        # Raised panels on the central pedestal, framed by narrow carved ribs.
        for i in range(12):
            a=i*TAU/12;b=(i+1)*TAU/12
            pt=lambda r,y,t:(r*math.sin(t),y,r*math.cos(t))
            self.quad(pt(190,74,a+.05),pt(190,74,b-.05),pt(224,128,b-.05),pt(224,128,a+.05),material=1,tint=(135,105,73),solid=False)
            self.quad(pt(189,74,a-.015),pt(189,74,a+.015),pt(226,132,a+.015),pt(226,132,a-.015),material=3,solid=False)
        self.column(0,155,0,540,23,material=1,solid=False,n=48,cap=False)
        self.ring(540,178,530,189)
        self.column(0,189,0,530,5,material=3,n=48,solid=False,cap=False)
        self.ring(530,194,515,201,material=1)
        self.ring(495,201,515,201,material=3)
        # Radial wood panels use world-scale grain, not one enlarged texture
        # across the whole table. Fine joints expose the darker layer below.
        self.ring(174,200,495,200,material=1,tint=(99,77,53))
        for i in range(32):
            a=i*TAU/32+.001;b=(i+1)*TAU/32-.001
            pt=lambda r,t:(r*math.sin(t),201,r*math.cos(t))
            self.quad(pt(174,a),pt(495,a),pt(495,b),pt(174,b),
                uv=((0,0),(1.075,0),(1.075,.325),(0,.115)),
                material=1,tint=(185+i%3*6,158+i%3*5,119+i%3*4),solid=False)
        self.column(0,200,0,174,3,material=1,n=32,solid=False)
        self.ring(160,203.1,174,203.1,n=32,material=3)
        # Interlaced angular inlay around the flame; a restrained carved motif.
        for i in range(16):
            a=i*TAU/16;b=(i+1)*TAU/16
            pt=lambda r,t:(r*math.sin(t),203.2,r*math.cos(t))
            self.quad(pt(80,a),pt(150,(a+b)/2),pt(143,(a+b)/2),pt(73,a),material=3,solid=False)
            self.quad(pt(150,(a+b)/2),pt(80,b),pt(73,b),pt(143,(a+b)/2),material=3,solid=False)
        # Repeated carved diamonds on the apron instead of a plain thick slab.
        for i in range(24):
            a=i*TAU/24;sn=math.sin(a);co=math.cos(a)
            pt=lambda x,y:(541*sn+x*co,y,541*co-x*sn)
            self.quad(pt(0,158),pt(20,167),pt(0,176),pt(-20,167),material=3,solid=False)
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
        for i in range(12):
            angle=i*TAU/12
            self.chair(460*math.sin(angle),460*math.cos(angle),angle+math.pi)
        # Recessed masonry firebox, layered carved surround and a real log bed.
        self.fireplace()
        for angle in (math.pi*.75,math.pi,math.pi*1.25):
            self.local((FRONT-200)*math.sin(angle),(FRONT-200)*math.cos(angle),angle)
            self.statue()
            self.reset()
        self.pillar_torches()
        self.floor_clutter()
        # The native shutter is authored directly into the south bay. Its
        # creation parameters use the same generated anchor as this surround.
        # Hidden passage behind the door supports native entrance/exit travel.
        # Segment the surfaces so vertex shading can fade them into black.
        cuts=[FRONT,EXIT_Z,EXIT_Z+60,EXIT_Z+120,EXIT_Z+180,EXIT_Z+240,EXIT_BACK_Z]
        for z0,z1 in zip(cuts,cuts[1:]):
            self.quad((-HALF_DOOR,FLOOR,z0),(-HALF_DOOR,FLOOR,z1),
                      (HALF_DOOR,FLOOR,z1),(HALF_DOOR,FLOOR,z0),material=6,tint=(204,201,192))
            for sign in (-1,1):
                points=[(sign*HALF_DOOR,FLOOR,z0),(sign*HALF_DOOR,FLOOR,z1),
                        (sign*HALF_DOOR,FLOOR+660,z1),(sign*HALF_DOOR,FLOOR+660,z0)]
                if sign<0:points.reverse()
                self.quad(*points,tint=(194,175,144))
            self.quad((-HALF_DOOR,FLOOR+660,z0),(HALF_DOOR,FLOOR+660,z0),
                      (HALF_DOOR,FLOOR+660,z1),(-HALF_DOOR,FLOOR+660,z1),solid=False,tint=(130,120,110))
        side_width=HALF_DOOR-EXIT_HALF_WIDTH
        for sign in (-1,1):
            self.box(sign*(EXIT_HALF_WIDTH+side_width/2),FLOOR,FRONT+60,
                     side_width,EAVES_HEIGHT,170,bottom_solid=False,tint=(194,175,144))
            # Layered jambs with grounded bases, recessed flutes and capitals.
            x=sign*(EXIT_HALF_WIDTH+26)
            self.box(x,FLOOR,FRONT-45,76,26,94,bottom_solid=False)
            self.box(x,FLOOR+26,FRONT-44,48,EXIT_HEIGHT-26,68,bottom_solid=False)
            for dx in (-13,13):self.box(x+dx,FLOOR+48,FRONT-81,5,EXIT_HEIGHT-74,6,solid=False,tint=(144,138,123))
            for y in (92,158,224,290):self.box(x,FLOOR+y,FRONT-48,54,6,80,solid=False)
            self.box(x,FLOOR+EXIT_HEIGHT-8,FRONT-48,82,30,90,solid=False)
        self.box(0,FLOOR+EXIT_HEIGHT,FRONT+60,2*EXIT_HALF_WIDTH,EAVES_HEIGHT-EXIT_HEIGHT,170,tint=(194,175,144))
        # A double archivolt and central keystone frame the recessed lunette
        # above the rectangular shutter, using the same stone as the hall.
        for r0,r1,z in ((197,220,FRONT-85),(224,250,FRONT-68)):
            for i in range(16):
                a=(i+.025)*math.pi/16;b=(i+.975)*math.pi/16
                pt=lambda r,t:(r*math.cos(t),FLOOR+EXIT_HEIGHT+r*math.sin(t),z)
                self.quad(pt(r0,a),pt(r0,b),pt(r1,b),pt(r1,a),solid=False)
                self.quad(pt(r0,a),add(pt(r0,a),(0,0,30)),add(pt(r0,b),(0,0,30)),pt(r0,b),solid=False,tint=(167,156,135))
        self.box(0,FLOOR+EXIT_HEIGHT+204,FRONT-86,38,65,38,solid=False)
        self.box(0,FLOOR+EXIT_HEIGHT+85,FRONT-32,86,86,16,material=3,tint=(139,114,75),solid=False)
        for y,width,height,depth in ((626,560,25,58),(657,590,18,82),(680,616,24,104)):
            self.box(0,FLOOR+y,FRONT-28,width,height,depth,solid=False)
        for x in (-220,-110,0,110,220):self.box(x,FLOOR+630,FRONT-61,48,18,9,material=3,tint=(143,119,78),solid=False)
        self.box(0,FLOOR,EXIT_BACK_Z+20,2*HALF_DOOR,660,40,bottom_solid=False,tint=(140,132,120))
        # Carry actors supply their own collision and transient projected
        # shadows. No decoration proxies remain in the DZB or lightmap bake.
        # Ground-level decorative bottoms are false Acch ceilings after gravity.
        # Keep sides/tops and real overhead arches; apply to every wall bay.
        self.collision=[p for p in self.collision if not
            (cross(sub(p[1],p[0]),sub(p[2],p[0]))[1]<-.001 and max(v[1] for v in p)<FLOOR+180)]
        # The closed stone shell hides exterior column backs and buried trim.
        # Keep the entrance bay open and retain all collision independently.
        kept=[];faces=[]
        normals=[(math.sin(i*TAU/SIDES),math.cos(i*TAU/SIDES)) for i in range(1,SIDES)]
        for start in range(0,len(self.vertices),3):
            tri=self.vertices[start:start+3]
            if max(v[1] for v in tri)<FLOOR-.01:continue
            portal=any(v[2]>FRONT and abs(v[0])<=HALF_DOOR+50 for v in tri)
            if not portal and any(min(v[0]*nx+v[2]*nz for v in tri)>FRONT+25.01 for nx,nz in normals):continue
            kept.extend(tri);faces.extend(self.light_faces[start:start+3])
        self.vertices=kept;self.light_faces=faces
        self.pack_lightmaps()
        return self

    def pack_lightmaps(self):
        # Deterministic shelf pack, ordered by height to limit atlas waste.
        x=1;y=1;row_height=0
        for face in sorted(self.receivers,key=lambda f:-f['h']):
            if x+face['w']>=LIGHTMAP_SIZE:x=1;y+=row_height;row_height=0
            assert y+face['h']<LIGHTMAP_SIZE, 'Lightmap atlas overflow'
            face['x']=x;face['y']=y;x+=face['w'];row_height=max(row_height,face['h'])
        for i,(vertex,face_id) in enumerate(zip(self.vertices,self.light_faces)):
            if face_id<0:uv=(.5/LIGHTMAP_SIZE,.5/LIGHTMAP_SIZE)
            else:
                face=self.receivers[face_id];delta=sub(vertex[:3],face['origin'])
                uv=tuple((face[axis]+1.5+sum(a*b for a,b in zip(delta,face[basis]))/face['step'])/LIGHTMAP_SIZE
                         for axis,basis in (('x','u'),('y','v')))
            self.vertices[i]=vertex[:5]+uv+vertex[7:]


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
    mesh=b'RTH2'+struct.pack('<I',len(hall.vertices))+b''.join(struct.pack('<7f4B',*v) for v in hall.vertices)
    (OUT/'hall.mesh').write_bytes(mesh)
    (OUT/'hall.dzb').write_bytes(dzb(hall))
    # Native effects and the hearth geometry must use the same source layout.
    (ROOT/'src/generated/roundtable_layout.hpp').write_text(
        '// Generated by tools/generate_roundtable.py.\n#pragma once\n'
        'namespace dawnlight::roundtable_layout {\n'
        f'inline constexpr float floorY = {FLOOR:.1f}f;\n'
        f'inline constexpr float exitZ = {EXIT_Z:.6f}f;\n'
        f'inline constexpr float exitHalfWidth = {HALF_DOOR:.6f}f;\n'
        f'inline constexpr float chestX = {CHEST_X:.6f}f;\n'
        f'inline constexpr float zeldaZ = {ZELDA_Z:.1f}f;\n'
        f'inline constexpr short zeldaYaw = {ZELDA_YAW};\n'
        f'inline constexpr float restZ = {REST_Z:.1f}f;\n'
        f'inline constexpr float zeldaX = {ZELDA_X:.1f}f;\n'
        f'inline constexpr float flameY = {FLOOR+180:.1f}f;\n'
        f'inline constexpr float flameLightY = {LIGHT_POSITION[1]:.1f}f;\n'
        f'inline constexpr float hearthFireX = {-FRONT+165:.6f}f;\n'
        f'inline constexpr float hearthFireY = {FLOOR+62:.1f}f;\n'
        f'inline constexpr float hearthLightY = {HEARTH_POSITION[1]:.1f}f;\n'
        f'inline constexpr float hearthLightX = {HEARTH_POSITION[0]:.6f}f;\n'
        '}\n')
    torches=','.join('{'+','.join(f'{v:.6f}f' for v in t)+'}' for t in torch_layout())
    layout=ROOT/'src/generated/roundtable_layout.hpp'
    layout.write_text(layout.read_text().rsplit('}',1)[0]+f'struct Torch {{ float x,y,z; }};\ninline constexpr Torch torches[]{{{torches}}};\n}}\n')
    rows=[]
    for kind,x,y,z,yaw,w,h,d in prop_layout():
        rows.append('    {'+str(kind)+','+','.join(f'{v:.6f}f' for v in (x,y,z,yaw,w,h,d))+'},')
    (ROOT/'src/generated/roundtable_props.hpp').write_text(
        '// Generated by tools/generate_roundtable.py.\n#pragma once\n'
        'namespace dawnlight::roundtable_layout {\n'
        'struct Prop { unsigned kind; float x,y,z,yaw,width,height,depth; };\n'
        'inline constexpr Prop props[]{\n'+'\n'.join(rows)+'\n};\n}\n')
    print(f'{len(hall.vertices)//3} visible triangles; {len(hall.collision)} collision triangles; {len(mesh)} mesh bytes')
    return hall


def texture(path):
    from PIL import Image
    source=Image.open(path).convert('RGB')
    stone=Image.open(ROOT/'tools/assets/roundtable_stone_variants.png').convert('RGB')
    im=Image.new('RGB',(TEXTURE_SIZE,TEXTURE_SIZE))
    for material,rect in MATERIAL_RECTS.items():
        src=stone if material>=6 else source;tile=material-6 if material>=6 else material
        w,h=src.size;qx=tile%2;qy=tile//2
        crop=src.crop((qx*w//2,qy*h//2,(qx+1)*w//2,(qy+1)*h//2))
        rx,ry,rw,rh=rect;im.paste(crop.resize((rw,rh),Image.Resampling.LANCZOS),(rx,ry))
    for y in range(4):
        for x in range(4):im.putpixel((x,y),(255,255,255))
    data=bytearray()
    for by in range(0,TEXTURE_SIZE,4):
        for bx in range(0,TEXTURE_SIZE,4):
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
