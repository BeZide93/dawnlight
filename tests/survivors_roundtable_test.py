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
assert abs(ground(0,-1110)-(-333.45))<.01, 'Rear chair seat is 66.55 units high'
chair=g.Hall();chair.chair(0,0,0)
assert min(v[1] for v in chair.vertices)==g.FLOOR
assert abs(max(v[1] for v in chair.vertices)-g.FLOOR-187)<.01
# A horizontal ray just above the floor must hit each memorial plinth. The
# old floating box at +150 passed a top-down ground-height check.
def segment_hits(start,end):
    direction=g.sub(end,start)
    dot=lambda a,b:sum(x*y for x,y in zip(a,b))
    for a,b,c,_,_ in triangles:
        a,b,c=vertices[a],vertices[b],vertices[c]
        edge1=g.sub(b,a);edge2=g.sub(c,a);h=g.cross(direction,edge2)
        det=dot(edge1,h)
        if abs(det)<1.e-6:continue
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
assert abs(ground(0,3900)+250)<.01
assert ground(3000,0) is None
print(f'Roundtable: {tn} native collision triangles, valid tree/room ancestry, clear hub routes')
