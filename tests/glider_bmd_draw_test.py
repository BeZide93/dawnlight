"""Exercise immediate and deferred glider draws with native-style matrix pointers."""
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
using u16=unsigned short;
using Mtx=float[3][4];
void MTXCopy(const Mtx from,Mtx to){std::memcpy(to,from,sizeof(Mtx));}
void MTXConcat(const Mtx a,const Mtx b,Mtx out) {
 Mtx result{};
 for(int i=0;i<3;++i) for(int j=0;j<4;++j) {
  for(int k=0;k<3;++k) result[i][j]+=a[i][k]*b[k][j];
  if(j==3) result[i][j]+=a[i][3];
 }
 std::memcpy(out,result,sizeof(Mtx));
}
int heap=1;void* s_heap=nullptr;
struct ShapePacket {
 Mtx* base=nullptr;
 void setBaseMtxPtr(Mtx* p){base=p;}
};
ShapePacket shapes[3];
Mtx expected;
bool defer=false;
int immediateDraws=0,pendingDraws=0;
void draw_shapes(){
 for(auto& shape:shapes){
  assert(shape.base&&std::memcmp(*shape.base,expected,sizeof(Mtx))==0);
 }
}
struct CurrentHeap {int previous=heap;explicit CurrentHeap(void*){heap=2;}~CurrentHeap(){heap=previous;}};
struct J3DDrawBuffer {
 int count=0,draws=0;void frameInit(){count=0;}void setZMtx(Mtx){}
 void draw(){
  assert(count==1);++draws;
  if(defer){++pendingDraws;return;}
  draw_shapes();++immediateDraws;
 }
} worldOpa,worldXlu,localOpa,localXlu;
J3DDrawBuffer* s_opaque=&localOpa;J3DDrawBuffer* s_translucent=&localXlu;
struct J3DSys {
 Mtx mViewMtx{{1,0,0,10},{0,1,0,20},{0,0,1,30}};
 J3DDrawBuffer* lists[2]{&worldOpa,&worldXlu};int marker=17;
 void setDrawBuffer(J3DDrawBuffer* p,int i){lists[i]=p;}
 auto getViewMtx(){return mViewMtx;}
 void reinitGX(){}void setDrawModeOpaTexEdge(){marker=1;}void setDrawModeXlu(){marker=2;}
} j3dSys;
struct ModelData {u16 getShapeNum(){return 3;}} data;
struct Model {
 Mtx base{{1,0,0,0},{0,1,0,0},{0,0,1,0}};
 auto getBaseTRMtx(){return base;}
 ModelData* getModelData(){return &data;}
 ShapePacket* getShapePacket(u16 i){assert(i<3);return &shapes[i];}
 void unlock(){}void lock(){}
 void update(){
  assert(heap==2&&j3dSys.lists[0]==s_opaque&&j3dSys.lists[1]==s_translucent);
  assert(!s_opaque->count&&!s_translucent->count);
  ++s_opaque->count;++s_translucent->count;j3dSys.marker=42;
 }
 void viewCalc(){
  assert(std::memcmp(j3dSys.mViewMtx,expected,sizeof(Mtx))==0);
  // Native prepareShapePackets() rebinds mode-0 packets to the global view
  // on EVERY viewCalc. A fix applied only during loading is insufficient.
  for(auto& shape:shapes)shape.setBaseMtxPtr(&j3dSys.mViewMtx);
 }
} model;
Model* s_model=&model;
struct dKy_tevstr_c{} light;
struct {int calls=0;void setLightTevColorType_MAJI(Model* m,dKy_tevstr_c* p){assert(m==s_model&&p==&light);++calls;}} g_env_light;
// PRODUCTION
int main(){
 const J3DSys before=j3dSys;worldOpa.count=7;worldXlu.count=9;
 Mtx pose{{0,0,.45f,100},{0,.45f,0,200},{-.45f,0,0,300}};
 for(int i=0;i<6;++i){
  defer=i>=3;
  // Gameplay size and the smaller item-get presentation, with rotation.
  const float scale=i%2==0?.45f:1.0f;
  pose[0][2]=scale;pose[1][1]=scale;pose[2][0]=-scale;
  pose[0][3]+=10;MTXConcat(before.mViewMtx,pose,expected);
  assert(draw_glider_bmd(pose,&light));
  assert(heap==1&&std::memcmp(&j3dSys,&before,sizeof(J3DSys))==0);
  assert(worldOpa.count==7&&worldXlu.count==9&&!worldOpa.draws&&!worldXlu.draws);
  if(defer){
   assert(pendingDraws==2);
   // Deferred Fog draws the retained material/shape packets after the outer
   // GliderPacket has returned and restored the camera. Another pass can
   // change that camera again before the retained packets are consumed.
   j3dSys.mViewMtx[0][3]=-900;
   draw_shapes();pendingDraws=0;j3dSys=before;
   // Its configuration-ID replay invokes the glider again in the same frame.
   assert(draw_glider_bmd(pose,&light));
   assert(std::memcmp(&j3dSys,&before,sizeof(J3DSys))==0);
   assert(pendingDraws==2);draw_shapes();pendingDraws=0;
  }
 }
 assert(immediateDraws==6&&localOpa.draws==9&&localXlu.draws==9&&g_env_light.calls==9);
 s_model=nullptr;assert(!draw_glider_bmd(pose,&light));
 assert(heap==1&&std::memcmp(&j3dSys,&before,sizeof(J3DSys))==0);
}
'''.replace('// PRODUCTION', draw)
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Glider BMD draw: immediate/deferred materials, camera changes, replay, '
      'presentation scale, world state/heap restoration and fallback passed')
