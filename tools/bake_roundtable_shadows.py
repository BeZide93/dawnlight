#!/usr/bin/env python3
"""Bake soft static visibility from the floating table flame into a GX I8 atlas."""
import hashlib,json,os,struct,subprocess,tempfile
from pathlib import Path
import numpy as np
import generate_roundtable as g
ROOT=g.ROOT

def main():
    hall=g.Hall().build();size=g.LIGHTMAP_SIZE
    samples=[]
    for face in hall.receivers:
        yy,xx=np.mgrid[0:face['h'],0:face['w']]
        points=np.array(face['origin'])+(xx[...,None]-1)*g.LIGHTMAP_STEP*np.array(face['u'])+(yy[...,None]-1)*g.LIGHTMAP_STEP*np.array(face['v'])
        polygon=np.array(face['points']);closest=None;best=None
        for j in range(1,len(polygon)-1):
            a,b,c=polygon[[0,j,j+1]];e1=b-a;e2=c-a;delta=points-a
            d00=e1@e1;d01=e1@e2;d11=e2@e2;den=d00*d11-d01*d01
            if abs(den)<1.e-8:continue # a cone quad can have a repeated tip
            v=(d11*(delta@e1)-d01*(delta@e2))/den;w=(d00*(delta@e2)-d01*(delta@e1))/den
            bary=np.maximum(np.stack((1-v-w,v,w),-1),0);bary/=bary.sum(-1)[...,None]
            candidate=bary@np.array((a,b,c));distance=((candidate-points)**2).sum(-1)
            if closest is None:closest=candidate;best=distance
            else:
                select=distance<best;closest[select]=candidate[select];best=np.minimum(best,distance)
        assert closest is not None and np.isfinite(closest).all(), "Invalid shadow receiver"
        points=closest # extend polygon edge lighting into chart padding
        block=np.empty(xx.size,dtype=[('pixel','<u4'),('point','<f4',3)])
        block['pixel']=((yy+face['y'])*size+xx+face['x']).ravel();block['point']=points.reshape(-1,3)
        samples.append(block)
    targets=np.concatenate(samples)
    with tempfile.TemporaryDirectory() as temp:
        temp=Path(temp);binary=temp/'baker';source=ROOT/'tools/bake_roundtable_shadows.cpp'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O2','-Wall','-Wextra',str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary),'--self-test'],check=True)
        data=temp/'scene.bin';raw=temp/'shadow.bin'
        data.write_bytes(struct.pack('<3I6f',size,len(hall.casters),len(targets),*g.LIGHT_POSITION,*g.HEARTH_POSITION)+np.array(hall.casters,dtype='<f4').tobytes()+targets.tobytes())
        subprocess.run([str(binary),str(data),str(raw)],check=True)
        pixels=np.frombuffer(raw.read_bytes(),dtype=np.uint8).reshape(size,size)
        # Native GX I8 stores 8x4 texel blocks, not linear scanlines.
        tiled=pixels.reshape(size//4,4,size//8,8).transpose(0,2,1,3).tobytes()
        (g.OUT/'shadows.i8').write_bytes(tiled)
        metadata={'light':g.LIGHT_POSITION,'hearth':g.HEARTH_POSITION,'hearth_weight':.22,'size':size,'step':g.LIGHTMAP_STEP,'min':int(pixels.min()),'max':int(pixels.max())}
        metadata['sources']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in
            ('res/survivors_roundtable/hall.mesh','tools/generate_roundtable.py','tools/bake_roundtable_shadows.cpp','tools/bake_roundtable_shadows.py','res/survivors_roundtable/shadows.i8')}
        (g.OUT/'shadows.json').write_text(json.dumps(metadata,indent=2)+'\n')
        print('Saved soft shadows:',metadata['min'],metadata['max'])
if __name__=='__main__':main()
