#!/usr/bin/env python3
"""Author a longer Hero's Shade gait using fixed-length, two-bone leg IK.

Usage: python3 tools/generate_shade_stride.py /path/to/KN_a.arc
Developer dependencies: NumPy, SciPy. Original assets are extracted only into a
temporary directory; only the new root and leg tracks enter the generated header.
The 30-frame cycle has 60% planted stance, 40% smooth swing and 120 units of
planted travel. At 1.2x playback this matches 8 world units per simulation tick.
"""
from pathlib import Path
import struct
import numpy as np


def u16(b,o):return struct.unpack_from('>H',b,o)[0]
def s16(b,o):return struct.unpack_from('>h',b,o)[0]
def u32(b,o):return struct.unpack_from('>I',b,o)[0]
def f32(b,o):return struct.unpack_from('>f',b,o)[0]
def yaz(b):
 if b[:4]!=b'Yaz0':return b
 n=u32(b,4);out=bytearray();p=16
 while len(out)<n:
  flag=b[p];p+=1
  for bit in range(7,-1,-1):
   if len(out)>=n:break
   if flag&(1<<bit):out.append(b[p]);p+=1
   else:
    a,c=b[p:p+2];p+=2;dist=((a&15)<<8)|c;length=a>>4
    if not length:length=b[p]+18;p+=1
    else:length+=2
    for k in range(length):out.append(out[-dist-1])
 return bytes(out)
def extract(archive):
 b=yaz(Path(archive).read_bytes());assert b[:4]==b'RARC'
 data=32+u32(b,12);num=u32(b,40);entries=32+u32(b,44);strings=32+u32(b,52)
 for i in range(num):
  p=entries+i*20;nameoff=u16(b,p+6);start=strings+nameoff;name=b[start:b.index(b'\0',start)].decode()
  if b[p+4]&2:continue
  x=b[data+u32(b,p+8):data+u32(b,p+8)+u32(b,p+12)]
  if Path(name).name!=name or '\\' in name:raise ValueError('Invalid archive filename')
  (ROOT/name).write_bytes(x)

def sections(b):
 p=32;out={}
 for i in range(u32(b,12)):
  size=u32(b,p+4);out[b[p:p+4].decode()]=b[p:p+size];p+=size
 return out

def names(b,o):
 return [b[o+u16(b,o+4+i*4+2):].split(b'\0')[0].decode() for i in range(u16(b,o))]
def skeleton():
 b=sections((ROOT/'kn_a.bmd').read_bytes());j=b['JNT1'];n=u16(j,8);off=u32(j,12);ns=names(j,u32(j,20));joints=[]
 for i in range(n):
  idx=u16(j,u32(j,16)+2*i);p=off+idx*64
  joints.append({'name':ns[i],'scale':struct.unpack_from('>3f',j,p+4),'rotation':struct.unpack_from('>3h',j,p+16),'translation':struct.unpack_from('>3f',j,p+24),'parent':None})
 inf=b['INF1'];p=u32(inf,20);stack=[];cur=None
 while True:
  typ,idx=struct.unpack_from('>HH',inf,p);p+=4
  if typ==0:break
  if typ==1:stack.append(cur)
  elif typ==2:cur=stack.pop()
  elif typ==16:joints[idx]['parent']=stack[-1] if stack else None;cur=idx
 return joints
class Bck:
 def __init__(self,path):
  self.b=sections(Path(path).read_bytes())['ANK1'];b=self.b
  self.shift=b[9];self.frames=u16(b,10);self.count=u16(b,12)
  self.offsets=[u32(b,k) for k in (24,28,32)]
  table=u32(b,20);self.tracks=[]
  for j in range(self.count):
   axes=[]
   for ax in range(3):
    row=[]
    for typ in range(3):
     count,start,tan=struct.unpack_from('>HHH',b,table+j*54+ax*18+typ*6)
     size=2 if typ==1 else 4;read=s16 if typ==1 else f32
     vals=[read(b,self.offsets[typ]+size*(start+k)) for k in range(count*(3+tan) if count>1 else count)]
     row.append((count,tan,vals))
    axes.append(row)
   self.tracks.append(axes)
 def sample(self,j,t):
  ans=np.zeros((3,3))
  for ax in range(3):
   for typ in range(3):
    n,tan,v=self.tracks[j][ax][typ];stride=3+tan
    if n==1:val=v[0]
    elif t<=v[0]:val=v[1]
    elif t>=v[-stride]:val=v[-stride+1]
    else:
     for i in range(n-1):
      a=v[i*stride:(i+1)*stride];c=v[(i+1)*stride:(i+2)*stride]
      if a[0]<=t<c[0]:
       d=c[0]-a[0];x=(t-a[0])/d
       val=(2*x**3-3*x*x+1)*a[1]+(x**3-2*x*x+x)*d*a[-1]+(-2*x**3+3*x*x)*c[1]+(x**3-x*x)*d*c[2];break
    ans[typ,ax]=val*((2**self.shift)*np.pi/32768 if typ==1 else 1)
  return ans

def matrix(s,r,t):
 x,y,z=r;sx,cx=np.sin(x),np.cos(x);sy,cy=np.sin(y),np.cos(y);sz,cz=np.sin(z),np.cos(z)
 rx=np.array([[1,0,0],[0,cx,-sx],[0,sx,cx]]);ry=np.array([[cy,0,sy],[0,1,0],[-sy,0,cy]]);rz=np.array([[cz,-sz,0],[sz,cz,0],[0,0,1]])
 m=np.eye(4);m[:3,:3]=rz@ry@rx@np.diag(s);m[:3,3]=t;return m

def pose(anim,joints,t):
 mats=[]
 for i,j in enumerate(joints):
  m=matrix(*anim.sample(i,t));p=j['parent'];mats.append(m if p is None else mats[p]@m)
 return np.array(mats)

FRAMES=30
LEGS=((25,26,27,28,1,0),(29,30,31,32,-1,.5))

def rot_between(a,b):
 a=a/np.linalg.norm(a);b=b/np.linalg.norm(b);v=np.cross(a,b);d=np.dot(a,b)
 assert d>-.9999
 cross=np.array([[0,-v[2],v[1]],[v[2],0,-v[0]],[-v[1],v[0],0]])
 return np.eye(3)+cross+cross@cross/(1+d)

def goal(phase,side):
 p=phase%1;stance=.6;reach=60
 if p<stance:z=reach-2*reach*p/stance;y=17
 else:
  t=(p-stance)/(1-stance)
  # Hermite endpoints match the backwards planted-foot velocity.
  tangent=-2*reach/stance*(1-stance)
  z=(2*t**3-3*t*t+1)*-reach+(t**3-2*t*t+t)*tangent+(-2*t**3+3*t*t)*reach+(t**3-t*t)*tangent
  y=17+20*np.sin(np.pi*t)**2
 return np.array([side*26.,y,z])

# Keep native foot pitch, but point both feet along the direction of travel.
FOOT_ROT={}
def prepare_feet():
 global FOOT_ROT
 start=pose(BASE,JOINTS,0)
 for hip,knee,foot,toe,side,offset in LEGS:
  d=start[toe,:3,3]-start[foot,:3,3];yaw=-np.arctan2(d[0],d[2]);FOOT_ROT[foot]=Rotation.from_euler('y',yaw).as_matrix()@start[foot,:3,:3]

def custom(t):
 local=np.array([BASE.sample(i,t) for i in range(len(JOINTS))]);phase=t/FRAMES
 local[0,2]=[1.2+1.4*np.sin(2*np.pi*phase),126+2*np.cos(4*np.pi*phase),1.5]
 matrices=[]
 for i,j in enumerate(JOINTS):
  m=matrix(*local[i]);p=j['parent'];matrices.append(m if p is None else matrices[p]@m)
 for hip,knee,foot,toe,side,offset in LEGS:
  h=matrices[hip][:3,3];target=goal(phase+offset,side)
  l1=np.linalg.norm(local[knee,2]);l2=np.linalg.norm(local[foot,2]);delta=target-h;dist=np.linalg.norm(delta);d=delta/dist
  assert abs(l1-l2)<dist<l1+l2, (t,side,dist,l1+l2)
  along=(l1*l1-l2*l2+dist*dist)/(2*dist);height=np.sqrt(l1*l1-along*along)
  pole=np.array([side*.08,0.,1.]);pole-=d*np.dot(pole,d);pole/=np.linalg.norm(pole)
  k=h+along*d+height*pole
  orig=matrices[hip][:3,:3];world_hip=rot_between(orig@local[knee,2],k-h)@orig
  parent=matrices[JOINTS[hip]['parent']][:3,:3];local[hip,1]=Rotation.from_matrix(parent.T@world_hip).as_euler('xyz')
  orig=matrices[knee][:3,:3];world_knee=rot_between(orig@local[foot,2],target-k)@orig
  local[knee,1]=Rotation.from_matrix(world_hip.T@world_knee).as_euler('xyz')
  local[foot,1]=Rotation.from_matrix(world_knee.T@FOOT_ROT[foot]).as_euler('xyz')
 return local
class Custom:
 def sample(self,j,t):return custom(t)[j]

def sample_mats(local):
 mats=[]
 for i,j in enumerate(JOINTS):
  m=matrix(*local[i]);p=j['parent'];mats.append(m if p is None else mats[p]@m)
 return np.array(mats)


def write_header(output):
 samples=np.array([custom(t) for t in range(FRAMES)])
 legs=[25,26,27,29,30,31]
 angles=np.rint(samples[:,legs,1]*32768/np.pi).astype(int)
 angles=(angles+32768)%65536-32768
 rows=['// Generated by tools/generate_shade_stride.py from a user-supplied KN_a.arc.',
       '// Authored leg/root tracks only; native upper-body animation remains in the game.',
       '#pragma once', '#include <array>', '#include <cstdint>',
       'namespace dawnlight::shade::stride {',
       'inline constexpr int frames = 30;',
       'inline constexpr float speed = 8.0f;',
       'inline constexpr float playback = 1.2f;',
       'inline constexpr std::array<int, 6> joints = {25, 26, 27, 29, 30, 31};',
       'inline constexpr std::int16_t rotations[30][6][3] = {']
 for frame in angles:rows.append('    {'+', '.join('{'+', '.join(map(str,v))+'}' for v in frame)+'},')
 rows+=['};', 'inline constexpr float root[30][3] = {']
 for frame in samples[:,0,2]:rows.append('    {'+', '.join(f'{v:.5f}f' for v in frame)+'},')
 rows+=['};','} // namespace dawnlight::shade::stride','']
 Path(output).write_text('\n'.join(rows))
 return samples

def validate():
 # Measure the authored transforms, not just their intended target positions.
 max_error=0
 for t in np.linspace(0,FRAMES,301):
  local=custom(t);mats=sample_mats(local)
  for hip,knee,foot,toe,side,offset in LEGS:
   error=np.linalg.norm(mats[foot,:3,3]-goal(t/FRAMES+offset,side))
   max_error=max(max_error,error)
   assert error<1e-5
   assert mats[toe,1,3]>0
   for a,b in ((hip,knee),(knee,foot)):
    assert abs(np.linalg.norm(mats[b,:3,3]-mats[a,:3,3])-np.linalg.norm(local[b,2]))<1e-5
 assert np.linalg.norm(custom(0)-custom(FRAMES))<1e-5
 print(f'IK/loop validation passed: max ankle error {max_error:.8f}; fixed bone lengths and toe clearance.')

if __name__=='__main__':
 import argparse,tempfile
 from scipy.spatial.transform import Rotation
 cli=argparse.ArgumentParser(description=__doc__)
 cli.add_argument('archive',type=Path)
 cli.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'src/generated/shade_stride.hpp')
 args=cli.parse_args()
 with tempfile.TemporaryDirectory() as tmp:
  ROOT=Path(tmp);extract(args.archive);JOINTS=skeleton();BASE=Bck(ROOT/'kn_step.bck')
  assert BASE.frames==30 and BASE.count==37
  for idx,name in ((0,'center'),(25,'legL1'),(26,'legL2'),(27,'footL'),(29,'legR1'),(30,'legR2'),(31,'footR')):
   assert JOINTS[idx]['name']==name
  prepare_feet();validate();write_header(args.output)
