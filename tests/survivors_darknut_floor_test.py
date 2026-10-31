"""Replay the captured Darknut entrance treads and main-floor boundary."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
#include "survivors_reachable.hpp"
using namespace dawnlight::survivors;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}float absXZ()const{return std::hypot(x,z);}};
struct Floor{cXyz a,b,c;};
// Minimal connected entrance strip from the 2026-10-24 runtime capture.
// Preserve KCL rounding, 50-unit risers and the floor under the stair landing.
std::vector<Floor>s_eliteFloors{
 {{0.000f,-400.000f,0.000f},{-680.009f,-400.000f,2540.344f},{0.000f,-400.000f,2617.437f}}, // captured triangle 78
 {{0.000f,-400.000f,0.000f},{0.000f,-400.000f,2617.437f},{680.009f,-400.000f,2540.344f}}, // captured triangle 79
 {{-572.000f,-300.000f,3155.000f},{0.000f,-300.000f,3355.002f},{0.000f,-300.000f,3155.000f}}, // captured triangle 168
 {{-572.000f,-300.000f,3155.000f},{-572.000f,-300.000f,3355.000f},{-0.007f,-300.000f,3355.000f}}, // captured triangle 169
 {{-572.000f,-250.000f,3355.000f},{0.000f,-250.000f,3560.000f},{0.000f,-250.000f,3355.000f}}, // captured triangle 171
 {{-572.000f,-250.000f,3355.000f},{-572.000f,-250.000f,3560.000f},{-0.002f,-250.000f,3560.000f}}, // captured triangle 172
 {{-150.000f,-250.000f,3735.000f},{0.000f,-250.000f,3735.000f},{0.000f,-250.000f,3559.999f}}, // captured triangle 173
 {{-150.000f,-250.000f,3735.000f},{-0.001f,-250.000f,3560.000f},{-571.999f,-250.000f,3560.000f}}, // captured triangle 178
 {{0.000f,-350.000f,2880.000f},{0.000f,-350.000f,2419.998f},{-506.005f,-355.000f,2421.769f}}, // captured triangle 174
 {{-512.000f,-350.000f,2880.000f},{0.003f,-350.000f,2880.000f},{-506.001f,-355.000f,2421.772f}}, // captured triangle 175
 {{0.000f,-350.000f,2880.000f},{-572.004f,-350.000f,3155.000f},{0.000f,-350.000f,3155.000f}}, // captured triangle 176
 {{-512.000f,-350.000f,2880.000f},{-572.001f,-350.000f,3155.001f},{0.005f,-350.000f,2880.000f}}, // captured triangle 177
 {{0.000f,-350.000f,2040.000f},{0.000f,-350.000f,2420.002f},{167.400f,-350.000f,2040.000f}}, // captured triangle 198
 {{0.000f,-350.000f,2040.000f},{-167.400f,-350.000f,2040.000f},{0.000f,-350.000f,2420.002f}}, // captured triangle 199
 {{0.000f,-375.000f,2040.000f},{167.400f,-375.000f,2040.000f},{225.180f,-375.000f,1900.000f}}, // captured triangle 206
 {{0.000f,-375.000f,2040.000f},{225.180f,-375.000f,1900.000f},{0.000f,-375.000f,1900.000f}}, // captured triangle 207
 {{0.000f,-375.000f,2040.000f},{0.000f,-375.000f,1900.000f},{-225.180f,-375.000f,1900.000f}}, // captured triangle 215
 {{0.000f,-375.000f,2040.000f},{-225.180f,-375.000f,1900.000f},{-167.400f,-375.000f,2040.000f}}, // captured triangle 216
 {{572.000f,-300.000f,3155.000f},{0.007f,-300.000f,3355.000f},{572.000f,-300.000f,3355.000f}}, // captured triangle 182
 {{572.000f,-300.000f,3155.000f},{0.000f,-300.000f,3155.000f},{0.000f,-300.000f,3355.002f}}, // captured triangle 183
 {{572.000f,-250.000f,3355.000f},{0.002f,-250.000f,3560.000f},{572.000f,-250.000f,3560.000f}}, // captured triangle 185
 {{572.000f,-250.000f,3355.000f},{0.000f,-250.000f,3355.000f},{0.000f,-250.000f,3560.000f}}, // captured triangle 186
 {{0.000f,-250.000f,3735.000f},{149.999f,-250.000f,3735.000f},{0.000f,-250.000f,3560.000f}}, // captured triangle 187
 {{150.000f,-250.000f,3735.000f},{571.999f,-250.000f,3560.000f},{0.001f,-250.000f,3560.000f}}, // captured triangle 193
 {{0.000f,-350.000f,2880.000f},{506.005f,-355.000f,2421.769f},{0.000f,-350.000f,2419.998f}}, // captured triangle 188
 {{0.000f,-350.000f,2880.000f},{512.002f,-350.000f,2880.000f},{506.003f,-355.000f,2421.773f}}, // captured triangle 189
 {{0.000f,-350.000f,2880.000f},{0.000f,-350.000f,3155.000f},{572.004f,-350.000f,3155.000f}}, // captured triangle 190
 {{0.000f,-350.000f,2880.000f},{571.998f,-350.000f,3154.997f},{511.998f,-350.000f,2880.000f}}, // captured triangle 191
};
unsigned s_map=0;bool s_arenaMeshReady=true,s_arenaSpawn=true;
cXyz s_arenaSpawnPosition{0,-250,3565.17f};
struct{int room=51;}data;const auto& arena(){return data;}
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct dBgS_WtrChk{void Set(cXyz,float){}float GetHeight(){return 0;}};
struct dBgS_LinChk{cXyz a,b;void SetLink(){}void Set(cXyz* x,cXyz* y,void*){a=*x;b=*y;}};
unsigned probes=0;bool sealed=false;
struct BG{
 float GroundCross(dBgS_ObjGndChk* q){
  ++probes;float best=-1.e9f;
  for(const auto& f:s_eliteFloors){
   const auto u=f.b-f.a,v=f.c-f.a,p=q->pos-f.a;const float d=u.x*v.z-u.z*v.x;
   if(std::abs(d)<.001f)continue;
   const float a=(p.x*v.z-p.z*v.x)/d,b=(u.x*p.z-u.z*p.x)/d;
   if(a<-.0001f||b<-.0001f||a+b>1.0001f)continue;
   const float y=f.a.y+a*u.y+b*v.y;if(y<=q->pos.y&&y>best)best=y;
  }
  return best;
 }
 int GetRoomId(dBgS_ObjGndChk&){return 51;}int GetGroundCode(dBgS_ObjGndChk&){return 0;}
 bool WaterChk(dBgS_WtrChk*){++probes;return false;}
 bool LineCross(dBgS_LinChk* q){
  ++probes;
  // A real wall is still blocked with the larger Darknut stair limit.
  if(sealed&&((q->a.z<3300)!=(q->b.z<3300)))return true;
  // Vertical riser faces: test the ray's height at the crossing.
  for(float z:{3155.f,3355.f})if((q->a.z<z)!=(q->b.z<z)){
   const float t=(z-q->a.z)/(q->b.z-q->a.z),y=q->a.y+t*(q->b.y-q->a.y);
   const float upper=z==3355?-250.f:-300.f;
   if(y<upper&&y>=upper-50)return true;
  }
  return false;
 }
}bg;auto& dComIfG_Bgsp(){return bg;}
bool arena_path_clear(cXyz,cXyz,float){return true;}
struct{unsigned state=123;unsigned next_random(){state=1664525u*state+1013904223u;return state;}}s_run;
float s_eliteFloorArea=1;
'''
main=r'''
int main(){
 const auto grow=[](){unsigned frames=0;do{probes=0;update_reachable_floor();assert(probes<=194&&++frames<5000);}while(!s_reachableFloor.ready());};
 // Same native stairs stay over-limit on the other maps; no global relaxation.
 for(unsigned map:{1u,2u,3u,4u}){
  s_map=map;clear_reachable_floor();grow();
  assert(!reachable_arena_floor({200,-400,1000}));
 }
 s_map=0;clear_reachable_floor();grow();
 assert(reachable_arena_floor({200,-400,1000}));
 assert(reachable_arena_floor({0,-300,3250}));
 unsigned lower=0,accepted=0;
 for(unsigned i=0;i<200;++i){
  cXyz p;unsigned frames=0;while(!sample_reachable_arena_floor(p)){probes=0;update_reachable_floor();assert(probes<=192&&++frames<5000);}
  if(elite_floor_position(p)){++accepted;lower+=p.y<-390;}
 }
 assert(accepted>100&&lower>70); // Includes the actual final spawn footprint/projection.
 // Return route works too; initialization cannot merely jump down a cliff.
 s_arenaSpawnPosition={200,-400,1000};clear_reachable_floor();grow();
 assert(reachable_arena_floor({0,-250,3500}));
 sealed=true;s_arenaSpawnPosition={0,-250,3565.17f};clear_reachable_floor();grow();
 assert(!reachable_arena_floor({200,-400,1000}));
 std::cout<<"Darknut captured floors: 50-unit entrance risers, midpoint tolerance, lower-floor spawns, return route, walls and map isolation passed\n";
}
'''
# Exercise the same final footprint check used by map chests, pots and portals.
source=(root/'src/survivors_elites.inc').read_text()
start=source.index('bool elite_floor_position(');brace=source.index('{',start);depth=0
for end in range(brace,len(source)):
 depth+=(source[end]=='{')-(source[end]=='}')
 if not depth:break
footprint=source[start:end+1]
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_ground.inc').read_text()+(root/'src/survivors_reachable.inc').read_text()+footprint+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
