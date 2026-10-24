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
assert mesh[:4]==b'RTH1'
assert mesh==b'RTH1'+struct.pack('<I',len(hall.vertices))+b''.join(struct.pack('<5f4B',*v) for v in hall.vertices)
data=(g.OUT/'hall.dzb').read_bytes()
assert data==g.dzb(hall), 'Regenerate collision from the same authoring source'
assert len((g.OUT/'materials.rgb565').read_bytes())==256*256*2
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
chest_x=g.FRONT+25-150
for x,z in [(0,0),(0,260),(-320,0),(chest_x,0)]:
    assert abs(ground(x,z)+400)<.01, 'Rest/fire/Zelda/equipment anchor must remain clear'
assert abs(ground(0,-650)-(-299.5))<.01, 'Tabletop is 100.5 units above the floor'
assert abs(ground(0,-1110)-(-327.95))<.01, 'Chair collision reaches the cushion at 72.05 units'
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
routes=[((0,260),(-320,0)),((0,260),(chest_x,0)),((0,260),(0,3900))]
for start,end in routes:
    last=-400
    for step in range(101):
        t=step/100;x=start[0]+t*(end[0]-start[0]);z=start[1]+t*(end[1]-start[1])
        y=ground(x,z,0);assert y is not None and abs(y-last)<10,(x,z,y,last)
        last=y
assert abs(ground(0,3900,0)+250)<.01
assert ground(3000,0) is None
# Native Link wall checks use +30.01 and +129.99, not a complete body capsule.
# The old tabletop/seat surfaces missed the low circle and allowed penetration.
for i in range(48):
    angle=(i+.25)*math.tau/48
    def table_point(radius):
        return radius*math.sin(angle),g.FLOOR+30.01,-650+radius*math.cos(angle)
    assert segment_hits(table_point(410),table_point(340),True), 'Solid table skirt from every side'
for i in range(2,11):
    angle=i*math.tau/12
    cx=460*math.sin(angle);cz=-650+460*math.cos(angle)
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
    assert segment_hits(wall_point(0,-80),wall_point(0,10),True), 'Panel collision at its visible front'
    pillar_x=(2*g.HALF_DOOR-175)/2+38
    assert segment_hits(wall_point(pillar_x,-80),wall_point(pillar_x,0),True), 'Arch pilaster must be solid'
# Door wings cover the full corridor width, but leave native interaction and
# the opening/closing path unblocked. The back wall closes the previous void.
for x in (-g.HALF_DOOR+2,-230,230,g.HALF_DOOR-2):
    assert segment_hits((x,-200,3800),(x,-200,3950),True)
for x in (-120,0,120):
    assert not segment_hits((x,-120,3600),(x,-120,4140),True), 'Door event route must remain clear'
assert segment_hits((0,150,3800),(0,150,3950),True), 'Door lintel closes overhead gap'
assert segment_hits((0,-120,4140),(0,-120,4250),True), 'Visible back wall has collision'
# Atlas repeats must remain within one quadrant, including triangles split at
# UV repeat boundaries. The GPU uses clamp and cannot repeat a whole atlas.
for start in range(0,len(hall.vertices),3):
    face=hall.vertices[start:start+3]
    tiles={(int(v[3]*2),int(v[4]*2)) for v in face}
    assert len(tiles)==1, 'UV triangle crosses into another material'
    for v in face:
        if v[3:5] != (1.5/256,1.5/256):
            assert all(.02-1.e-6<=coord%.5<=.48+1.e-6 for coord in v[3:5]), 'Avoid emissive texel bleed'
# A tall column must contain complete vertical repeats of the stone tile at
# the same world scale as a broad wall, not one tile stretched over the shaft.
column=g.Hall();column.column(0,0,0,78,830,n=12)
wall=g.Hall();wall.quad((0,0,0),(600,0,0),(600,830,0),(0,830,0))
for surface in (column,wall):
    for start in range(0,len(surface.vertices),3):
        face=surface.vertices[start:start+3]
        assert max(v[1] for v in face)-min(v[1] for v in face)<=g.TEXEL_SPAN+.01
        for a in face:
            for b in face:
                if abs(a[0]-b[0])+abs(a[2]-b[2])<.001 and abs(a[1]-b[1])>1:
                    span=abs(a[1]-b[1])/(abs(a[4]-b[4])*2/.92)
                    assert abs(span-g.TEXEL_SPAN)<.01, 'Unequal vertical texture density'
assert len(hall.vertices)<=100000, 'Stay within renderer resource bounds'

print(f'Roundtable: {tn} native collision triangles, valid tree/room ancestry, clear hub routes')
