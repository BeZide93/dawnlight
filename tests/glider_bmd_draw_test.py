"""Run the production BMD draw function against instrumented J3D/heap objects."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
s = (ROOT / 'src/glider_bmd.cpp').read_text()
start = s.index('bool draw_glider_bmd(')
draw = s[start:s.index('\n}', start) + 2]
fixture = r'''
#include <cassert>
#include <cstring>
using Mtx=float[3][4];
void MTXConcat(const Mtx a,const Mtx b,Mtx out) {
 Mtx result{};
 for(int i=0;i<3;++i) for(int j=0;j<4;++j) {
  for(int k=0;k<3;++k) result[i][j]+=a[i][k]*b[k][j];
  if(j==3) result[i][j]+=a[i][3];
 }
 std::memcpy(out,result,sizeof(Mtx));
}
int heap=1;void* s_heap=nullptr;
struct CurrentHeap {int previous=heap;explicit CurrentHeap(void*){heap=2;}~CurrentHeap(){heap=previous;}};
struct J3DDrawBuffer {
 int count=0,draws=0;void frameInit(){count=0;}void setZMtx(Mtx){}
 void draw(){assert(count==1);++draws;}
} worldOpa,worldXlu,localOpa,localXlu;
J3DDrawBuffer* s_opaque=&localOpa;J3DDrawBuffer* s_translucent=&localXlu;
struct J3DSys {
 Mtx mViewMtx{{1,0,0,10},{0,1,0,20},{0,0,1,30}};
 J3DDrawBuffer* lists[2]{&worldOpa,&worldXlu};int marker=17;
 void setDrawBuffer(J3DDrawBuffer* p,int i){lists[i]=p;}
 auto getViewMtx(){return mViewMtx;}
 void reinitGX(){}void setDrawModeOpaTexEdge(){marker=1;}void setDrawModeXlu(){marker=2;}
} j3dSys;
Mtx expected;
struct Model {
 Mtx base{{1,0,0,0},{0,1,0,0},{0,0,1,0}};
 auto getBaseTRMtx(){return base;}
 void unlock(){}void lock(){}
 void update(){
  assert(heap==2&&j3dSys.lists[0]==s_opaque&&j3dSys.lists[1]==s_translucent);
  assert(!s_opaque->count&&!s_translucent->count);
  ++s_opaque->count;++s_translucent->count;j3dSys.marker=42;
 }
 void viewCalc(){assert(std::memcmp(j3dSys.mViewMtx,expected,sizeof(Mtx))==0);}
} model;
Model* s_model=&model;
struct dKy_tevstr_c{} light;
struct {int calls=0;void setLightTevColorType_MAJI(Model* m,dKy_tevstr_c* p){assert(m==s_model&&p==&light);++calls;}} g_env_light;
// PRODUCTION
int main(){
 const J3DSys before=j3dSys;worldOpa.count=7;worldXlu.count=9;
 Mtx pose{{0,0,.45f,100},{0,.45f,0,200},{-.45f,0,0,300}};
 for(int i=0;i<3;++i){
  pose[0][3]+=10;MTXConcat(before.mViewMtx,pose,expected);
  assert(draw_glider_bmd(pose,&light));
  assert(heap==1&&std::memcmp(&j3dSys,&before,sizeof(J3DSys))==0);
  assert(worldOpa.count==7&&worldXlu.count==9&&!worldOpa.draws&&!worldXlu.draws);
 }
 assert(localOpa.draws==3&&localXlu.draws==3&&g_env_light.calls==3);
 s_model=nullptr;assert(!draw_glider_bmd(pose,&light));
 assert(heap==1&&std::memcmp(&j3dSys,&before,sizeof(J3DSys))==0);
}
'''.replace('// PRODUCTION', draw)
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Glider BMD draw: presented pose, repeated private lists, world state/heap restoration and fallback passed')
