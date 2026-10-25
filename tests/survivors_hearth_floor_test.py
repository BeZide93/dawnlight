"""Check native RoofChk-before-GroundCross semantics on the walkable hearth."""
from pathlib import Path
import math,struct,sys
ROOT=Path(__file__).resolve().parents[1]
path=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'res/survivors_roundtable/hall.dzb'
data=path.read_bytes();vn,vo,tn,to=struct.unpack_from('>4I',data)
vertices=[struct.unpack_from('>3f',data,vo+i*12) for i in range(vn)]
faces=[]
for i in range(tn):
 indices=struct.unpack_from('>3H',data,to+i*10);a,b,c=[vertices[j] for j in indices]
 det=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
 if abs(det)<.001:continue
 # det is the negative Y normal for this X/Z barycentric convention.
 faces.append((a,b,c,det,min(a[0],b[0],c[0]),max(a[0],b[0],c[0]),min(a[2],b[2],c[2]),max(a[2],b[2],c[2])))
def hits(x,z):
 result=[]
 for a,b,c,det,x0,x1,z0,z1 in faces:
  if not x0-.001<=x<=x1+.001 or not z0-.001<=z<=z1+.001:continue
  u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/det
  v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/det
  if min(u,v,1-u-v)<-.00001:continue
  result.append((u*a[1]+v*b[1]+(1-u-v)*c[1],det<0))
 return result
origin=-1800*math.cos(math.pi/16)+110
steps=0
for lateral in (-160,-80,0,80,160):
 for reverse in (False,True):
  depths=list(range(-275,36,5));depths=depths[::-1] if reverse else depths
  foot=-358 if reverse else -400
  for depth in depths:
   x,z=origin-depth,lateral;surfaces=hits(x,z)
   current=foot-3 # gravity is integrated before the native correction pass
   roof=min((y for y,up in surfaces if not up and y>current),default=math.inf)
   assert roof-current>=180, ('False low ceiling above hearth',x,z,foot,roof)
   # Native Acch clamps its ground probe against the nearest roof first.
   probe=min(current+100,roof)
   floor=max((y for y,up in surfaces if up and y<probe),default=-math.inf)
   assert floor>=-400.01 and abs(floor-foot)<=30, ('Lost support or oversized step',x,z,foot,floor)
   foot=floor;steps+=1
print(f'Hearth floor: {steps} native-style approach/retreat probes, no false ceilings or lost support')
