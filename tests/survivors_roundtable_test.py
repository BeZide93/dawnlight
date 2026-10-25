"""Check shipped geometry, native DZB tree, surface winding and playable routes."""
from pathlib import Path
import importlib.util
import math
import struct

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('hall',ROOT/'tools/generate_roundtable.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
hall=g.Hall().build()
mesh=(g.OUT/'hall.mesh').read_bytes()
assert mesh[:4]==b'RTH2'
assert mesh==b'RTH2'+struct.pack('<I',len(hall.vertices))+b''.join(struct.pack('<7f4B',*v) for v in hall.vertices)
data=(g.OUT/'hall.dzb').read_bytes()
assert data==g.dzb(hall), 'Regenerate collision from the same authoring source'
assert len((g.OUT/'materials.rgb565').read_bytes())==1024*1024*2
vn,vo,tn,to,bn,bo,nn,no,gn,go,inf,io,flags=struct.unpack_from('>13I',data)
assert flags==0 and gn==3 and inf==1
vertices=[struct.unpack_from('>3f',data,vo+i*12) for i in range(vn)]
triangles=[struct.unpack_from('>5H',data,to+i*10) for i in range(tn)]
blocks=[struct.unpack_from('>H',data,bo+i*2)[0] for i in range(bn)]+[tn]
nodes=[struct.unpack_from('>10H',data,no+i*20) for i in range(nn)]
seen=set();leaf_blocks=set()
def visit(i,parent=65535):
    assert i not in seen;seen.add(i)
    flag,p,*children=nodes[i];assert p==parent
    if flag&1:
        block=children[0];assert block<bn and block not in leaf_blocks
        leaf_blocks.add(block);assert 0<blocks[block+1]-blocks[block]<=24
    else:
        for child in children:
            if child!=65535:visit(child,i)
visit(0)
assert len(seen)==nn and len(leaf_blocks)==bn
assert blocks[0]==0 and blocks[-1]==tn and blocks==sorted(blocks)
for i in range(3):
    name,sx,sy,sz,rx,ry,rz,tx,ty,tz,parent,sibling,child,room,first,tree,info=struct.unpack_from('>I3f3h2x3f6HI',data,go+i*52)
    assert data[name:name+10]==b'roundtable'
    assert parent==(i-1 if i else 65535) and child==(i+1 if i<2 else 65535)
    assert room==51 and tree==(0 if i==2 else 65535)
    assert (sx,sy,sz)==(1,1,1) and (tx,ty,tz)==(0,0,0)
for a,b,c,material,group in triangles:
    assert max(a,b,c)<vn and material==0 and group==2
    assert g.length(g.cross(g.sub(vertices[b],vertices[a]),g.sub(vertices[c],vertices[a])))>.001
info0,info1,info2,passes=struct.unpack_from('>4I',data,io)
assert info0&63==63 and info1==255 and passes==0, 'Solid normal ground, no exit/link'
assert (info0>>6)&255==255, 'Inherit room ambient light; zero turns grounded actors black'
assert info0>>14==0, 'No unintended special surface flags'
assert info2&255==0, 'Static room, no moving-background camera'
assert all((info2>>shift)&255==255 for shift in (8,16,24)), 'No dungeon camera/path references'

def ground(x,z,ceiling=1000):
    heights=[]
    for a,b,c,_,_ in triangles:
        a,b,c=vertices[a],vertices[b],vertices[c]
        n=g.cross(g.sub(b,a),g.sub(c,a))
        if n[1]<=.01:continue
        det=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
        if abs(det)<1.e-5:continue
        u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/det
        v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/det
        if min(u,v,1-u-v)<-1.e-5:continue
        y=u*a[1]+v*b[1]+(1-u-v)*c[1]
        if y<=ceiling:heights.append(y)
    return max(heights) if heights else None

assert g.RADIUS==1800, 'Keep the compact build-1295 footprint'
chest_x=g.CHEST_X
for x,z in [(0,g.REST_Z),(g.ZELDA_X,g.ZELDA_Z),(chest_x,0)]:
    assert abs(ground(x,z)+400)<.01, 'Rest/Zelda/equipment anchor must remain clear'
assert abs(ground(0,0)-(-299.5))<.01, 'Tabletop is 100.5 units above the floor'
assert abs(ground(0,-460)-(-327.95))<.01, 'Chair collision reaches the cushion at 72.05 units'
chair=g.Hall();chair.chair(0,0,0)
assert min(v[1] for v in chair.vertices)==g.FLOOR
assert abs(max(v[1] for v in chair.vertices)-g.FLOOR-187)<.01
# A horizontal ray just above the floor must hit each memorial plinth. The
# old floating box at +150 passed a top-down ground-height check.
def segment_hits(start,end,front_only=False):
    direction=g.sub(end,start)
    dot=lambda a,b:sum(x*y for x,y in zip(a,b))
    for a,b,c,_,_ in triangles:
        a,b,c=vertices[a],vertices[b],vertices[c]
        edge1=g.sub(b,a);edge2=g.sub(c,a);h=g.cross(direction,edge2)
        det=dot(edge1,h)
        if abs(det)<1.e-6 or (front_only and det<0):continue
        offset=g.sub(start,a);u=dot(offset,h)/det
        if u<0 or u>1:continue
        q=g.cross(offset,edge1);v=dot(direction,q)/det
        if v<0 or u+v>1:continue
        if 0<=dot(edge2,q)/det<=1:return True
    return False
for angle in (math.pi*.75,math.pi,math.pi*1.25):
    def point(inset):
        radius=g.FRONT-200+inset
        return radius*math.sin(angle),g.FLOOR+1,radius*math.cos(angle)
    assert segment_hits(point(-200),point(0)), 'Memorial must reach the floor'

# Walkable links between all interaction points, including the authored door.
routes=[((0,g.REST_Z),(-720,650)),((-720,650),(g.ZELDA_X,g.ZELDA_Z)),
        ((0,g.REST_Z),(chest_x,0)),((0,g.REST_Z),(0,g.EXIT_Z))]
for start,end in routes:
    last=-400
    for step in range(101):
        t=step/100;x=start[0]+t*(end[0]-start[0]);z=start[1]+t*(end[1]-start[1])
        y=ground(x,z,0);assert y is not None and abs(y-last)<10,(x,z,y,last)
        last=y
assert abs(ground(0,g.EXIT_Z,0)-g.FLOOR)<.01
assert ground(0,3000) is None, "Remove the former long corridor"
assert ground(3000,0) is None
# Native Link wall checks use +30.01 and +129.99, not a complete body capsule.
# The old tabletop/seat surfaces missed the low circle and allowed penetration.
for i in range(48):
    angle=(i+.25)*math.tau/48
    def table_point(radius):
        return radius*math.sin(angle),g.FLOOR+30.01,radius*math.cos(angle)
    assert segment_hits(table_point(410),table_point(340),True), 'Solid table skirt from every side'
for i in range(12):
    angle=i*math.tau/12
    cx=460*math.sin(angle);cz=460*math.cos(angle)
    for approach in range(8):
        direction=approach*math.tau/8
        start=(cx+65*math.sin(direction),g.FLOOR+30.01,cz+65*math.cos(direction))
        assert segment_hits(start,(cx,g.FLOOR+30.01,cz),True), 'Chair seat footprint must stop walking'
# Rays from the walkable side must hit the visible panel/pilaster, rather than
# only the rear shell. Test along each bay at native body-check height.
for i in range(1,g.SIDES):
    angle=i*math.tau/g.SIDES
    def wall_point(x,z):
        return (g.FRONT*math.sin(angle)+x*math.cos(angle)+z*math.sin(angle),
                g.FLOOR+129.99,
                g.FRONT*math.cos(angle)-x*math.sin(angle)+z*math.cos(angle))
    assert segment_hits(wall_point(0,-230),wall_point(0,10),True), 'Panel collision at its visible front'
    pillar_x=(2*g.HALF_DOOR-175)/2+38
    assert segment_hits(wall_point(pillar_x,-80),wall_point(pillar_x,0),True), 'Arch pilaster must be solid'
# Solid plinths block Link's low wall circle; actual shelf slabs and side posts
# stay solid. Compartments must remain hollow for native Carry physics.
assert len(g.NICHE_BAYS)==6 and len(g.SHELF_BAYS)==2
for bays in (g.NICHE_BAYS,g.SHELF_BAYS):
    assert set(bays)=={(-i)%g.SIDES for i in bays}, 'Mirror bay types about the entrance axis'
assert not set(g.NICHE_BAYS)&set(g.SHELF_BAYS)
for i in g.NICHE_BAYS:
    angle=i*math.tau/g.SIDES
    def niche_point(x,y,z):
        return (g.FRONT*math.sin(angle)+x*math.cos(angle)+z*math.sin(angle),
                g.FLOOR+y,g.FRONT*math.cos(angle)-x*math.sin(angle)+z*math.cos(angle))
    for x in (-185,-120,-60,0,60,120,185):
        for y in (1,30.01,64):
            assert segment_hits(niche_point(x,y,-230),niche_point(x,y,-120),True), 'Solid niche plinth/sill stops walking below the first shelf'
    for side in (-1,1):
        for y in (30.01,129.99,400):
            assert segment_hits(niche_point(side*300,y,-125),niche_point(side*180,y,-125),True), 'Closed niche side approach'
    assert not segment_hits(niche_point(0,135,-230),niche_point(0,135,-30),True), 'Native props can leave the open recess'
    for x in (-185,0,185):
        q=niche_point(x,0,-155)
        assert abs(ground(q[0],q[2],g.FLOOR+50)-g.FLOOR)<.01, 'Grounded niche approach'

# The doorway sits directly at the hall edge, at the same floor height.
# Its solid surround must leave native +/-240-unit event goals unobstructed.
assert g.EXIT_Z-g.FRONT<=60 and g.EXIT_BACK_Z-g.EXIT_Z>=900
for x in (-g.HALF_DOOR+2,-230,230,g.HALF_DOOR-2):
    assert segment_hits((x,g.FLOOR+130,g.FRONT-150),(x,g.FLOOR+130,g.EXIT_Z+100),True)
for x in (-120,0,120):
    assert not segment_hits((x,g.FLOOR+130,g.EXIT_Z-240),(x,g.FLOOR+130,g.EXIT_Z+240),True), 'Clear door event path'
    assert abs(ground(x,g.EXIT_Z+240,g.FLOOR+100)-g.FLOOR)<.01
assert segment_hits((0,g.FLOOR+400,g.FRONT-150),(0,g.FLOOR+400,g.EXIT_Z+100),True)
assert segment_hits((0,g.FLOOR+130,g.EXIT_BACK_Z-60),(0,g.FLOOR+130,g.EXIT_BACK_Z+40),True)
# Extended interior is supported, unobstructed and uniformly black at depth.
for z in (g.EXIT_Z+400,g.EXIT_Z+700,g.EXIT_BACK_Z-50):
    assert abs(ground(0,z,g.FLOOR+100)-g.FLOOR)<.01
    assert not segment_hits((0,g.FLOOR+130,g.EXIT_Z),(0,g.FLOOR+130,z),True)
rear=[v for v in hall.vertices if abs(v[0])<=g.HALF_DOOR+1 and v[2]>=g.EXIT_Z+260]
assert rear and all(v[7:10]==(0,0,0) for v in rear), 'No lit rear masonry remains'
# Red cloth remains on chair cushions only, never on walls or ceiling.
cloth=g.MATERIAL_RECTS[2]
assert not any(cloth[0]/1024<v[3]<(cloth[0]+cloth[2])/1024 and
               cloth[1]/1024<v[4]<(cloth[1]+cloth[3])/1024 and v[1]>g.FLOOR+200 for v in hall.vertices)
# Each textured triangle remains inside one padded material rectangle.
for start in range(0,len(hall.vertices),3):
    face=hall.vertices[start:start+3]
    if all(v[3:5]==(1.5/1024,1.5/1024) for v in face):continue
    candidates=[]
    for material,(x,y,w,h) in g.MATERIAL_RECTS.items():
        if all(x+w*.04-1.e-4<=v[3]*1024<=x+w*.96+1.e-4 and
               y+h*.04-1.e-4<=v[4]*1024<=y+h*.96+1.e-4 for v in face):candidates.append(material)
    assert len(candidates)==1, 'Triangle crosses material border or padding'
# A tall column must contain complete vertical repeats of the stone tile at
# the same world scale as a broad wall, not one tile stretched over the shaft.
column=g.Hall();column.column(0,0,0,78,830,n=12)
wall=g.Hall();wall.quad((0,0,0),(600,0,0),(600,830,0),(0,830,0))
for surface in (column,wall):
    for start in range(0,len(surface.vertices),3):
        face=surface.vertices[start:start+3]
        assert max(v[1] for v in face)-min(v[1] for v in face)<=g.MASONRY_SPAN+.01
        for a in face:
            for b in face:
                if abs(a[0]-b[0])+abs(a[2]-b[2])<.001 and abs(a[1]-b[1])>1:
                    span=abs(a[1]-b[1])/(abs(a[4]-b[4])*2/.92)
                    assert abs(span-g.MASONRY_SPAN)<.01, 'Unequal vertical texture density'
assert all(0<=v[5]<1 and 0<=v[6]<1 for v in hall.vertices), 'Valid shadow atlas coordinates'
assert len(hall.vertices)<=195000, 'Stay within renderer resource bounds'

print(f'Roundtable: {tn} native collision triangles, valid tree/room ancestry, clear hub routes')

# The equipment probe starts at the authored perimeter, clear of table/chairs.
assert chest_x>1500 and abs(ground(chest_x,0)+400)<.01
assert g.ZELDA_X<-1000 and g.ZELDA_Z>300
assert abs(math.atan2(-g.ZELDA_X,-g.ZELDA_Z)-g.ZELDA_YAW*math.pi/32768)<.0001
# Recess is open above the log bed; jambs and hearth remain solid.
assert not segment_hits((g.HEARTH_POSITION[0]+300,-180,0),(g.HEARTH_POSITION[0]-100,-180,0),True)
assert segment_hits((g.HEARTH_POSITION[0]+300,-180,260),(g.HEARTH_POSITION[0]-100,-180,260),True)

# Removed table candelabra: nothing remains above the tabletop away from flame.
for x,z in ((-220,0),(220,0),(0,-240)):
 assert not any(abs(v[0]-x)<20 and abs(v[2]-z)<20 and -295<v[1]<-100 for v in hall.vertices)
assert all(len(face['points'])>=3 for face in hall.receivers)
assert any(len(face['points'])==4 for face in hall.receivers), 'Share quad shadow charts'

# Native source extraction must run only after CMake fetched the pinned SDK.
workflow=(ROOT/'.github/workflows/build.yml').read_text()
assert workflow.index('name: Configure CMake') < workflow.index('python3 tests/survivors_roundtable_entry_test.py')

# Native dressing must fit the authored supports and leave interaction paths.
props=g.prop_layout()
assert len(props)==36 and sum(p[2]==g.FLOOR for p in props)==12
assert {p[0] for p in props}=={0,1,2,3}
for i,p in enumerate(props):
 kind,x,y,z,yaw,w,h,d=p
 for q in props[i+1:]:
  if y==q[2]:
   assert math.hypot(x-q[1],z-q[3])>(math.hypot(w,d)+math.hypot(q[5],q[7]))/2, 'Dressing bounds overlap'
 if y==g.FLOOR:
  assert abs(ground(x,z,y+120)-g.FLOOR)<.01, 'No permanent prop box after moving or breaking Carry'
  assert min(math.hypot(x-a,z-b) for a,b in ((g.CHEST_X,0),(g.ZELDA_X,g.ZELDA_Z),(0,g.EXIT_Z),(0,g.REST_Z)))>220
 else:
  bay=round(math.atan2(x,z)*g.SIDES/math.tau)%g.SIDES
  assert bay in g.NICHE_BAYS+g.SHELF_BAYS
  a=bay*math.tau/g.SIDES
  xx=x*math.cos(a)-z*math.sin(a);zz=x*math.sin(a)+z*math.cos(a)-g.FRONT
  radius=w/2 if kind!=2 else w/2*(abs(math.sin(yaw-a))+abs(math.cos(yaw-a)))
  assert abs(xx)+radius<170 and zz-radius>-129.5 and zz+radius<-15, 'Native footprint extends beyond support'
  assert abs(ground(x,z,y+10)-y)<.01, 'Native Carry spawn must find its own support, not a hidden cabinet roof'
  assert not segment_hits((x,y+30,z),(x+math.sin(a)*-90,y+30,z+math.cos(a)*-90),True), 'No invisible front wall traps Carry objects'
  assert y-g.FLOOR in ((95,335,575) if bay in g.NICHE_BAYS else (77,262,447))
print('Native dressing: 36 separated placements, grounded supports and clear anchors')

# Dynamic objects must contribute neither baked occluders nor permanent DZB.
original_layout=g.prop_layout
g.prop_layout=lambda:[]
empty=g.Hall().build()
g.prop_layout=original_layout
assert hall.casters==empty.casters and hall.collision==empty.collision
print('Native Carry: real shelf support, open compartments, no permanent prop colliders/shadows')
