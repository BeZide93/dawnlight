"""Production dry-floor and reachable-spawn checks, with native collision boundaries."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "survivors_reachable.hpp"
using namespace dawnlight::survivors;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}float absXZ()const{return std::hypot(x,z);}};
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct dBgS_WtrChk{cXyz bottom;float top=0,height=0;void Set(cXyz p,float y){bottom=p;top=y;}float GetHeight(){return height;}};
struct dBgS_LinChk{cXyz a,b;bool link=false;void SetLink(){link=true;}void Set(cXyz* x,cXyz* y,void*){a=*x;b=*y;}};
struct Arena{int room=6;}data;const Arena& arena(){return data;}
unsigned groundChecks=0,waterChecks=0,wallChecks=0;
float stairs(float x){return x<0?0:x<60?30:x<120?60:x<180?90:120;}
int terrain=0,code=0,room=6;bool wet=false,wall=false,barrier=false;float waterY=50;
struct Bg{
 float GroundCross(dBgS_ObjGndChk* q){
  ++groundChecks;
  if(terrain==7 && !((q->pos.x>=-2400&&q->pos.x<=600&&q->pos.z>=0&&q->pos.z<=200)||
    (q->pos.x>=400&&q->pos.x<=600&&q->pos.z>=0&&q->pos.z<=1600)||
    (q->pos.x>=400&&q->pos.x<=2600&&q->pos.z>=1400&&q->pos.z<=1600)))return -1.e9f;
  if(std::abs(q->pos.x)>3000 || std::abs(q->pos.z)>3000)return -1.e9f;
  if(terrain==10)return stairs(q->pos.x);
  if(terrain==8&&q->pos.x>0)return 20;
  if(terrain==9&&q->pos.x>0)return 100;
  if(terrain==1&&q->pos.x>400&&q->pos.x<600)return -1.e9f;
  if(terrain==2&&q->pos.x>400)return 100; // Reachable by sight, not by walking.
  if(terrain==3)return q->pos.x*0.2f; // Continuous gentle slope.
  if(terrain==4&&q->pos.z>20)return -100; // Unsafe footprint edge.
  return 0;
 }
 int GetRoomId(dBgS_ObjGndChk&){return room;}int GetGroundCode(dBgS_ObjGndChk&){return code;}
 bool WaterChk(dBgS_WtrChk* q){++waterChecks;q->height=waterY;return wet&&waterY>=q->bottom.y&&waterY<=q->top;}
 bool LineCross(dBgS_LinChk* q){
  ++wallChecks;
  // Simulate player-only native collision: an object/default ray would miss it.
  if(q->link && (terrain==5||terrain==6) && ((q->a.x<700)!=(q->b.x<700)) &&
     (terrain==6 || std::min(q->a.z,q->b.z)<1000))return true;
  // Native wall rays also collide with vertical stair risers.
  if(terrain==10)for(float x:{0.f,60.f,120.f,180.f}) {
   if((q->a.x<x)!=(q->b.x<x)) {
    const float t=(x-q->a.x)/(q->b.x-q->a.x);
    const float y=q->a.y+t*(q->b.y-q->a.y);
    if(y<stairs(x)&&y>=stairs(x-1))return true;
   }
  }
  return wall;
 }
}bg;Bg& dComIfG_Bgsp(){return bg;}
bool arena_path_clear(cXyz,cXyz,float){return !barrier;}
struct Floor{cXyz a,b,c;};std::vector<Floor>s_eliteFloors;float s_eliteFloorArea=0;
struct Random{unsigned state=123;unsigned next_random(){state=1664525u*state+1013904223u;return state;}}s_run;
void cell(float x,float z,float size=500){
 s_eliteFloors.push_back({{x,0,z},{x+size,0,z},{x,0,z+size}});
 s_eliteFloors.push_back({{x+size,0,z+size},{x,0,z+size},{x+size,0,z}});
 s_eliteFloorArea+=size*size;
}
void mesh(){s_eliteFloors.clear();s_eliteFloorArea=0;for(int x=-3000;x<3000;x+=500)for(int z=-3000;z<3000;z+=500)cell(float(x),float(z));}
unsigned s_map=1;bool s_arenaMeshReady=true,s_arenaSpawn=true;cXyz s_arenaSpawnPosition;
'''
main=r'''
int main(){
 cXyz from(0,0,0),to(1000,0,0);
 assert(walkable_spawn_path(from,to,40)&&to.y==0);
 wet=true;assert(!walkable_spawn_path(from,to,40)); // Actual water volume over ordinary ground.
 waterY=-100;assert(walkable_spawn_path(from,to,40)); // Bridge above water.
 wet=false;
 for(int bad:{1,2,4}){terrain=bad;to={1000,0,0};assert(!walkable_spawn_path(from,to,40));}
 terrain=3;to={1000,0,0};assert(walkable_spawn_path(from,to,40)&&std::abs(to.y-200)<1);
 terrain=0;room=7;assert(!walkable_spawn_path(from,to,40));room=6;
 for(int bad:{4,5,10}){code=bad;assert(!walkable_spawn_path(from,to,40));}code=0;
 wall=true;assert(!walkable_spawn_path(from,to,40));wall=false;
 barrier=true;assert(!walkable_spawn_path(from,to,40));
 barrier=false;
 mesh();
 const auto grow=[] {
  unsigned frame=0;
  do {
   groundChecks=waterChecks=wallChecks=0;update_reachable_floor();
   assert(groundChecks+waterChecks+wallChecks<=194); // 64 samples plus the initial seed probe.
   assert(++frame<10000);
  }while(!s_reachableFloor.ready());
 };
 terrain=2;clear_reachable_floor();grow();
 assert(!reachable_arena_floor({1500,100,0})); // Large flat but inaccessible plateau.
 terrain=1;clear_reachable_floor();grow();
 assert(!reachable_arena_floor({1500,0,0})); // Dry bank across an unbridgeable gap.
 terrain=5;clear_reachable_floor();grow();
 to={1500,0,0};assert(!walkable_spawn_path(from,to,40));
 assert(reachable_arena_floor({1500,0,0})); // Valid route around the wall, beyond direct sight.
 terrain=6;s_map=4;clear_reachable_floor();grow();
 assert(!reachable_arena_floor({1500,0,0})); // Throne's player-only light wall.
 assert(reachable_arena_floor({300,0,0}));
 clear_reachable_floor();assert(!reachable_arena_floor({300,0,0}));
 s_arenaSpawn=false;update_reachable_floor();assert(s_reachableFloor.empty());s_arenaSpawn=true;
 s_arenaMeshReady=false;update_reachable_floor();assert(s_reachableFloor.empty());s_arenaMeshReady=true;
 terrain=0;clear_reachable_floor();update_reachable_floor();
 cXyz sampled;assert(!arena_distribution_ready()&&!sample_reachable_arena_floor(sampled));
 grow();assert(arena_distribution_ready()&&std::abs(arena_distribution_area()-36000000)<1);
 unsigned near=0,far=0;
 for(unsigned i=0;i<100;++i){
  unsigned frames=0;
  while(!sample_reachable_arena_floor(sampled)){
   groundChecks=waterChecks=wallChecks=0;update_reachable_floor();
   assert(groundChecks+waterChecks+wallChecks<=192);assert(++frames<1000);
  }
  near+=sampled.x < -1000;far+=sampled.x>1000;
 }
 assert(near>=20&&far>=20); // Uniform finished component, not the earliest near-start sites.
 // A 200-unit-wide, bent corridor whose turn falls between the old 512-unit grid lines.
 terrain=7;s_arenaSpawnPosition={-2300,0,100};s_eliteFloors.clear();s_eliteFloorArea=0;
 for(int x=-2400;x<600;x+=200)cell(float(x),0,200);
 for(int z=200;z<1600;z+=200)cell(400,float(z),200);
 for(int x=600;x<2600;x+=200)cell(float(x),1400,200);
 clear_reachable_floor();grow();assert(reachable_arena_floor({2500,0,1500}));
 // Shared XZ edges connect small stair risers, never a tall plateau.
 for(int kind:{8,9}){
  terrain=kind;mesh();const float height=kind==8?20.f:100.f;
  for(auto& tile:s_eliteFloors)if((tile.a.x+tile.b.x+tile.c.x)>0)tile.a.y=tile.b.y=tile.c.y=height;
  s_arenaSpawnPosition={-1000,0,0};clear_reachable_floor();grow();
  assert(reachable_arena_floor({1500,height,0})==(kind==8));
 }
 // Descend from the entrance landing over four 30-unit risers. Narrow
 // treads are valid transit surfaces even though a chest cannot fit on them.
 terrain=10;s_map=0;
 for(bool splitEdges:{false,true}) {
 s_eliteFloors.clear();s_eliteFloorArea=0;
 const float cuts[]{-1000,0,60,120,180,1000};
 for(unsigned i=0;i<5;++i){
  const float a=cuts[i],b=cuts[i+1],y=stairs((a+b)*.5f);
  // Alternate one long boundary with two shorter boundaries (T-junctions).
  // Neither shorter edge has both endpoints in common with the long edge.
  const unsigned pieces=splitEdges&&i%2?2:1;
  for(unsigned j=0;j<pieces;++j){
   const float z0=-500+1000.f*j/pieces,z1=-500+1000.f*(j+1)/pieces;
   s_eliteFloors.push_back({{a,y,z0},{b,y,z0},{a,y,z1}});
   s_eliteFloors.push_back({{b,y,z1},{a,y,z1},{b,y,z0}});
  }
  s_eliteFloorArea+=(b-a)*1000;
 }
 for(bool descending:{true,false}) {
  s_arenaSpawnPosition=descending?cXyz(800,120,0):cXyz(-800,0,0);
  clear_reachable_floor();grow();
  assert(reachable_arena_floor({-800,0,0})&&reachable_arena_floor({800,120,0}));
  unsigned lower=0,upper=0;
  for(unsigned i=0;i<80;++i){
   unsigned frames=0;while(!sample_reachable_arena_floor(sampled)){update_reachable_floor();assert(++frames<1000);}
   lower+=sampled.x<-100;upper+=sampled.x>280;
  }
  assert(lower>=15&&upper>=15);
 }
 }
 // Matching also handles diagonal/reversed partial edges, without joining
 // disjoint collinear intervals or an overlapping floor 100 units above it.
 {
  std::vector<Floor> floors{
   {{0,0,0},{120,0,120},{-120,0,120}},
   {{120,0,120},{60,0,60},{180,0,0}},
   {{240,0,240},{300,0,300},{360,0,240}},
   {{120,100,120},{60,100,60},{180,100,0}}};
  ReachableFloor<cXyz> graph;graph.seed({-20,0,100},unsigned(floors.size()));
  graph.build(floors,0);assert(!graph.ready());
  unsigned ticks=0;
  while(!graph.ready()){
   graph.build(floors,1);graph.expand([](cXyz,cXyz){return WalkResult::Clear;});
   assert(++ticks<1000);
  }
  assert(ticks>floors.size()); // One operation per build call resumes mid-edge.
  assert(graph.valid(graph.containing({120,0,60})));
  assert(!graph.valid(graph.containing({300,0,260})));
  assert(!graph.valid(graph.containing({120,100,60})));
  graph.clear();assert(graph.empty()&&!graph.ready());
 }
 // Actual spawn footprints remain strict near a stair edge.
 to={10,30,0};assert(!walkable_spawn_path({-800,0,0},to,40));
 to={800,120,0};assert(walkable_spawn_path({-800,0,0},to,40));
 s_arenaSpawnPosition={0,0,0};
 // Distant segments really suspend at the shared sample budget.
 terrain=0;ArenaWalk path;path.begin({-2000,0,0},{2000,0,0},40);
 unsigned budget=4;assert(path.advance(budget)==WalkResult::Pending&&budget==0&&path.active);
 unsigned frames=1;while(path.active){budget=4;path.advance(budget);++frames;}assert(frames>20);
 // Darknut/Palace now wait for connected-floor preparation as well; no isolated platforms.
 for(unsigned map:{0u,3u}){
  s_map=map;s_arenaSpawnPosition={0,0,0};terrain=2;mesh();clear_reachable_floor();
  assert(needs_reachable_floor()&&!arena_distribution_ready());grow();
  assert(arena_distribution_ready()&&!reachable_arena_floor({1500,100,0}));
  assert(reachable_arena_floor({300,0,0}));
 }
 // Darknut uses the finalized post-entry checkpoint on the arena side of
 // the sealed door, rather than the early loading checkpoint outside it.
 s_map=0;terrain=6;mesh();s_arenaSpawnPosition={1500,0,0};clear_reachable_floor();
 grow();assert(reachable_arena_floor({1500,0,0})&&!reachable_arena_floor({300,0,0}));
 for(unsigned n=0;n<30;++n){
  unsigned frames=0;while(!sample_reachable_arena_floor(sampled)){update_reachable_floor();assert(++frames<1000);}
  assert(sampled.x>700);
 }
 // Invalid checkpoint ground must not publish any spawn component.
 s_arenaSpawnPosition={4000,0,0};clear_reachable_floor();update_reachable_floor();assert(!arena_distribution_ready());
 std::cout<<"Survivors ground: water, bridges, ledges, gaps, slopes, footprints and barriers passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_ground.inc').read_text()+(root/'src/survivors_reachable.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
