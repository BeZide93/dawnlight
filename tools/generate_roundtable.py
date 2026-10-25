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


def add(a, b): return tuple(x+y for x, y in zip(a, b))
def sub(a, b): return tuple(x-y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def length(a): return math.sqrt(sum(x*x for x in a))


class Hall:
    def __init__(self):
        self.vertices = []
        self.collision = []
        self.transform = lambda p: p
        self.receivers = []
        self.casters = []
        self.light_faces = []

    def triangle(self, points, material=0, tint=(255, 255, 255), solid=True, visible=True, uv=None, shadow_face=None):
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
            self.casters.append(tuple(p))
            face_id=self.receiver(p) if shadow_face is None else shadow_face
        if uv is None:
            uv = ((.03, .03), (.97, .03), (.97, .97))
        # Indirect room lighting plus warm local contributions from both fires.
        # Native actors receive the corresponding runtime point lights.
        shade = .72 + .16*max(0, normal[1]/n) + .10*max(0, -normal[0]/n)
        def emit(points, coords):
            for pos, tex in zip(points, coords):
                distance = math.hypot(pos[0], pos[2])
                warm = max(0, 1-distance/1400)*.14
                hearth_distance=length(sub(pos,HEARTH_POSITION))
                hearth_warm=.22*max(0,1-hearth_distance/900)
                light = (shade+warm+hearth_warm, shade+warm*.6+hearth_warm*.45,
                         shade+warm*.15+hearth_warm*.10)
                dirt=1.0
                if FLOOR<=pos[1]<=FLOOR+1 and normal[1]>0:
                    radius=math.hypot(pos[0],pos[2])
                    dirt=1-.24*max(0,1-abs(radius-1450)/380)*(0.55+.45*math.sin(pos[0]*.011+pos[2]*.007)**2)
                color = tuple(min(255, round(t*c*dirt)) for t, c in zip(tint, light))
                tile = min(material, 3)
                # Keep bilinear samples inside their atlas quadrant and clear
                # of the reserved 4x4 emissive texels in the stone corner.
                u = (tile % 2 + .04 + tex[0]*.92)*.5
                v = (tile // 2 + .04 + tex[1]*.92)*.5
                if material >= 4:
                    u=v=1.5/TEXTURE_SIZE
                    if material == 4:color=tint
                self.vertices.append((*pos, u, v, 0.0, 0.0, *color, 255))
                self.light_faces.append(face_id)
        if material >= 4:
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
        w=max(2,math.ceil((max(xs)-min(xs))/LIGHTMAP_STEP)+1)+2
        h=max(2,math.ceil((max(ys)-min(ys))/LIGHTMAP_STEP)+1)+2
        result=len(self.receivers)
        self.receivers.append({'points':p,'origin':origin,'u':u,'v':v,'w':w,'h':h})
        return result

    def quad(self, a, b, c, d, uv=None, **kw):
        if uv is None:
            # Measure after the object transform: scaled furniture must not
            # change the apparent size of wood grain or stone courses.
            pa,pb,pc,pd=[self.transform(p) for p in (a,b,c,d)]
            width=max(length(sub(pb,pa)),length(sub(pc,pd)))/TEXEL_SPAN
            height=max(length(sub(pd,pa)),length(sub(pc,pb)))/TEXEL_SPAN
            uv=((0,0),(width,0),(width,height),(0,height))
        points=[self.transform(p) for p in (a,b,c,d)]
        if kw.get('visible',True) and kw.get('material',0)!=4 and length(cross(sub(points[1],points[0]),sub(points[2],points[0])))>.001:
            kw['shadow_face']=self.receiver(points)
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

    def vessel(self,x,y,z,r=19,h=55,material=0):
        self.column(x,y,z,r*.65,h*.18,top=r,n=6,material=material,solid=False,tint=(171,153,125))
        self.column(x,y+h*.18,z,r,h*.55,top=r*.72,n=6,material=material,solid=False,tint=(171,153,125))
        self.column(x,y+h*.73,z,r*.72,h*.27,top=r*.48,n=6,material=material,solid=False,tint=(186,166,128))

    def book(self,x,y,z,w,h,seed):
        # Unequal bindings, recessed page edges and narrow bronze spine bands.
        colors=((110,57,43),(67,83,70),(97,76,42),(84,58,69))
        self.box(x,y,z,w,h,45,material=5,tint=(173,158,122),solid=False)
        for side in (-1,1):self.box(x+side*w/2,y,z,3,h+3,51,material=5,tint=colors[seed%4],solid=False)
        self.box(x,y,z-25,w+3,h+3,4,material=5,tint=colors[seed%4],solid=False)
        for yy in (y+12,y+h-12):
            self.quad((x-w/2,yy,z-27.2),(x+w/2,yy,z-27.2),
                (x+w/2,yy+3,z-27.2),(x-w/2,yy+3,z-27.2),material=3,solid=False)

    def wall_bay(self,index,width):
        # Structural backing stays behind the recesses. Each bay projects into
        # the room by at most 140 units, away from every interaction route.
        shelving=index in (1,5,9,13)
        niches=index in (3,7,11,15)
        self.box(0,0,12,width-130,760,12,material=0 if niches else 1,
            bottom_solid=False,tint=(145,137,116) if niches else (168,151,126))
        if shelving:
            self.box(0,0,-52,460,638,138,visible=False,bottom_solid=False)
            for x in (-216,216):self.box(x,0,-50,28,652,126,material=1,solid=False)
            for y in (60,245,430,615):
                self.box(0,y,-57,458,17,145,material=1,solid=False)
                self.box(0,y+2,-132,476,15,12,material=3,tint=(154,125,81),solid=False)
            for row,y in enumerate((77,262,447)):
                for j in range(6):
                    self.book(-165+j*43,y,-64,21+(j*7+index)%12,
                        76+(j*13+row*17)%52,j+row+index)
                self.vessel(143,y,-66,22,64,3 if row%2 else 0)
            self.box(0,652,-50,495,25,158,material=1,solid=False)
        elif niches:
            # Three deep alcoves with stone shelves and curved archivolts.
            for x in (-238,238):self.box(x,0,-45,85,775,140,bottom_solid=False)
            for row,y in enumerate((65,305,545)):
                self.box(0,y,-43,402,22,139,bottom_solid=False)
                self.box(0,y+22,4,390,175,8,tint=(78,72,59),solid=False)
                for j in range(8):
                    a=j*math.pi/8;b=(j+1)*math.pi/8
                    pt=lambda r,t,z:(r*math.cos(t),y+78+r*.7*math.sin(t),z)
                    self.quad(pt(177,a,-116),pt(177,b,-116),pt(199,b,-116),pt(199,a,-116),solid=False)
                    self.quad(pt(177,a,-116),pt(177,a,0),pt(177,b,0),pt(177,b,-116),solid=False,tint=(137,125,103))
                self.vessel(-72,y+22,-47,25,73)
                self.vessel(40,y+22,-44,18,52,3)
                for j in range(3):self.box(108,y+22+j*7,-43,51-j*5,6,35,material=5,tint=(138,117,81),solid=False)
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

    def floor_clutter(self):
        # Repeatable little clusters near wall bays. These thin decorations
        # never enter the DZB or make Link climb floating collision surfaces.
        for i in (1,2,3,5,6,7,8,9,10,14,15):
            angle=i*TAU/SIDES+.06;radius=RADIUS-390-(i%3)*45
            x=radius*math.sin(angle);z=radius*math.cos(angle)
            self.local(x,z,angle+.25)
            for j in range(3):
                px=-75+j*53;pz=(j*47+i*13)%95-45
                self.quad((px-18,1.1,pz-25),(px-16,1.1,pz+27),(px+21,1.1,pz+22),(px+17,1.1,pz-24),
                    material=5,tint=(149+j*9,133+j*7,96+j*5),solid=False)
                for line in range(4):
                    yy=1.2;zz=pz-12+line*6
                    self.quad((px-11,yy,zz),(px-11,yy,zz+1),(px+9-line,yy,zz+1),(px+9-line,yy,zz),
                        material=5,tint=(77,70,54),solid=False)
            for j in range(2):self.box(-48+j*105,1,62-j*105,84,4,16,material=1,tint=(143,121,90),solid=False)
            for j in range(4):
                self.column(-87+(j*59)%164,.7,-72+(j*37)%130,7+(j%3)*3,5+j*2,top=2,n=5,
                    tint=(141,134,119),solid=False)
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
                self.wall_bay(i,width)
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
        # Central table, with a complete circle of twelve chairs. The rest
        # pose and interactions remain outside this furniture footprint.
        self.local(0,0,0,scale=(.67,.5,.67))
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
        for i in range(12):
            angle=i*TAU/12
            self.chair(460*math.sin(angle),460*math.cos(angle),angle+math.pi)
        # Recessed masonry firebox, layered carved surround and a real log bed.
        self.fireplace()
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
        self.floor_clutter()
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
        # Enclose the full door-frame depth in a 600-unit stone reveal; keep the centre clear
        # for the native approach, opening animation and +/-240 goal positions.
        side_width=HALF_DOOR-EXIT_HALF_WIDTH
        for sign in (-1,1):
            self.box(sign*(EXIT_HALF_WIDTH+side_width/2),EXIT_FLOOR,EXIT_Z,
                side_width,660,600)
            self.box(sign*(EXIT_HALF_WIDTH+22),EXIT_FLOOR,EXIT_Z-310,
                44,EXIT_HEIGHT,24)
        self.box(0,EXIT_FLOOR+EXIT_HEIGHT,EXIT_Z,
            2*EXIT_HALF_WIDTH,660-EXIT_HEIGHT,600)
        self.box(0,EXIT_FLOOR+EXIT_HEIGHT,EXIT_Z-310,
            2*EXIT_HALF_WIDTH+88,40,24)
        # Keep floor behind the opening for the real door event, and close the
        # far end visibly as well as physically (rather than a black void).
        self.box(0,EXIT_FLOOR,4220,2*HALF_DOOR,660,40,tint=(140,132,120))
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
                uv=tuple((face[axis]+1.5+sum(a*b for a,b in zip(delta,face[basis]))/LIGHTMAP_STEP)/LIGHTMAP_SIZE
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
    print(f'{len(hall.vertices)//3} visible triangles; {len(hall.collision)} collision triangles; {len(mesh)} mesh bytes')
    return hall


def texture(path):
    from PIL import Image
    im=Image.open(path).convert('RGB').resize((TEXTURE_SIZE,TEXTURE_SIZE),Image.Resampling.LANCZOS)
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
