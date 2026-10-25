#!/usr/bin/env python3
"""Render the actual exported hall.mesh; this is not an in-game screenshot."""
from pathlib import Path
import argparse
import struct
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]

def render(output,eye=(850,360,1100),target=(0,-100,0),width=1280,height=800):
    mesh=(ROOT/'res/survivors_roundtable/hall.mesh').read_bytes()
    count=struct.unpack_from('<I',mesh,4)[0]
    dtype=np.dtype([('position','<f4',3),('uv','<f4',2),('light_uv','<f4',2),('color','u1',4)])
    vertices=np.frombuffer(mesh,dtype=dtype,count=count,offset=8)
    texture=np.zeros((1024,1024,3),np.float32)
    raw=(ROOT/'res/survivors_roundtable/materials.rgb565').read_bytes()
    offset=0
    for by in range(0,1024,4):
        for bx in range(0,1024,4):
            for y in range(4):
                for x in range(4):
                    p=struct.unpack_from('>H',raw,offset)[0];offset+=2
                    texture[by+y,bx+x]=((p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31)
    lightmap=np.frombuffer((ROOT/'res/survivors_roundtable/shadows.i8').read_bytes(),dtype=np.uint8).reshape(256,128,4,8).transpose(0,2,1,3).reshape(1024,1024)/255
    eye=np.array(eye);forward=np.array(target)-eye;forward=forward/np.linalg.norm(forward)
    right=np.cross(forward,(0,1,0));right/=np.linalg.norm(right);up=np.cross(right,forward)
    points=vertices['position']-eye
    depth=points@forward
    focal=width/(2*np.tan(np.radians(76)/2))
    screen=np.stack((width/2+points@right*focal/depth,height/2-points@up*focal/depth),axis=1)
    frame=np.full((height,width,3),(17,14,12),np.float32)
    zbuffer=np.full((height,width),np.inf)
    for start in range(0,count,3):
        depths=depth[start:start+3]
        if np.any(depths<10):continue # preview only; the GPU clips the actual room
        p=screen[start:start+3]
        x0,y0=np.maximum(np.floor(p.min(0)).astype(int),(0,0));x1,y1=np.minimum(np.ceil(p.max(0)).astype(int),(width-1,height-1))
        if x1<x0 or y1<y0:continue
        yy,xx=np.mgrid[y0:y1+1,x0:x1+1];xx=xx+.5;yy=yy+.5
        a,b,c=p
        denominator=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(denominator)<1.e-7:continue
        u=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/denominator
        v=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/denominator
        w=1-u-v
        inside=(u>=0)&(v>=0)&(w>=0)
        weights=np.stack((u/depths[0],v/depths[1],w/depths[2]),axis=-1)
        inverse=weights.sum(-1)
        dist=1/np.maximum(inverse,1.e-9)
        mask=inside&(dist<zbuffer[y0:y1+1,x0:x1+1])
        if not mask.any():continue
        weights/=np.maximum(inverse[...,None],1.e-9)
        uv=weights@vertices['uv'][start:start+3]
        color=weights@vertices['color'][start:start+3,:3]/255
        tex=np.clip((uv*1024).astype(int),0,1023)
        luv=weights@vertices['light_uv'][start:start+3]
        lightxy=np.clip(luv*1024-.5,0,1023);lo=lightxy.astype(int);hi=np.minimum(lo+1,1023);fraction=lightxy-lo
        light=(lightmap[lo[...,1],lo[...,0]]*(1-fraction[...,0])+lightmap[lo[...,1],hi[...,0]]*fraction[...,0])*(1-fraction[...,1])+(lightmap[hi[...,1],lo[...,0]]*(1-fraction[...,0])+lightmap[hi[...,1],hi[...,0]]*fraction[...,0])*fraction[...,1]
        pixels=texture[tex[...,1],tex[...,0]]*color*light[...,None]
        frame[y0:y1+1,x0:x1+1][mask]=pixels[mask]
        zbuffer[y0:y1+1,x0:x1+1][mask]=dist[mask]
    Image.fromarray(np.clip(frame,0,255).astype('uint8')).save(output)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args();render(args.output)
