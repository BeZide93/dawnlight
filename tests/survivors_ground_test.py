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
#include "survivors_reachable.hpp"
using namespace dawnlight::survivors;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}float absXZ()const{return std::hypot(x,z);}};
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct dBgS_WtrChk{cXyz bottom;float top=0,height=0;void Set(cXyz p,float y){bottom=p;top=y;}float GetHeight(){return height;}};
struct dBgS_LinChk{cXyz a,b;bool link=false;void SetLink(){link=true;}void Set(cXyz* x,cXyz* y,void*){a=*x;b=*y;}};
struct Arena{int room=6;}data;const Arena& arena(){return data;}
int terrain=0,code=0,room=6;bool wet=false,wall=false,barrier=false;float waterY=50;
struct Bg{
 float GroundCross(dBgS_ObjGndChk* q){
  if(std::abs(q->pos.x)>3000 || std::abs(q->pos.z)>3000)return -1.e9f;
  if(terrain==1&&q->pos.x>400&&q->pos.x<600)return -1.e9f;
  if(terrain==2&&q->pos.x>400)return 100; // Reachable by sight, not by walking.
  if(terrain==3)return q->pos.x*0.2f; // Continuous gentle slope.
  if(terrain==4&&q->pos.z>20)return -100; // Unsafe footprint edge.
  return 0;
 }
 int GetRoomId(dBgS_ObjGndChk&){return room;}int GetGroundCode(dBgS_ObjGndChk&){return code;}
 bool WaterChk(dBgS_WtrChk* q){q->height=waterY;return wet&&waterY>=q->bottom.y&&waterY<=q->top;}
 bool LineCross(dBgS_LinChk* q){
  // Simulate player-only native collision: an object/default ray would miss it.
  if(q->link && terrain>=5 && ((q->a.x<700)!=(q->b.x<700)) &&
     (terrain==6 || std::min(q->a.z,q->b.z)<1000))return true;
  return wall;
 }
}bg;Bg& dComIfG_Bgsp(){return bg;}
bool arena_path_clear(cXyz,cXyz,float){return !barrier;}
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
 const auto grow=[] {for(unsigned i=0;i<600;++i)update_reachable_floor();};
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
 terrain=0;grow();s_reachableQueries=0;assert(!reachable_arena_floor({300,0,0}));
 update_reachable_floor();assert(s_reachableQueries==8&&reachable_arena_floor({300,0,0}));
 s_map=3;clear_reachable_floor();assert(reachable_arena_floor({300,0,0})); // Authored Palace landings unchanged.
 ReachableFloor<cXyz> bounded;bounded.seed({0,0,0});unsigned probes=0;
 bounded.expand(16,[&](cXyz,cXyz&){++probes;return true;});assert(probes<=16);
 std::cout<<"Survivors ground: water, bridges, ledges, gaps, slopes, footprints and barriers passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_ground.inc').read_text()+(root/'src/survivors_reachable.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
