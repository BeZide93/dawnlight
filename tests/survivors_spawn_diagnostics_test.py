"""Bounded development-only Darknut diagnostics; no gameplay or collision changes."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "survivors_reachable.hpp"
using namespace dawnlight::survivors;
constexpr bool debug_enabled=DAWNLIGHT_SURVIVORS_DEBUG!=0;
constexpr unsigned kNone=~0u;unsigned s_map=0;
struct cXyz{float x=0,y=0,z=0;cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}cXyz operator*(float n)const{return{x*n,y*n,z*n};}};
struct Floor{cXyz a,b,c;};std::vector<Floor>s_eliteFloors;
ReachableFloor<cXyz>s_reachableFloor;cXyz s_arenaSpawnPosition{10,0,10};
struct daAlink_c{struct{cXyz pos;}current;struct{float GetGroundH()const{return 0;}bool ChkGroundHit()const{return true;}}mLinkAcch;}player;
struct Chest{bool active;cXyz position;};std::vector<Chest>s_chests{{true,{1,20,2}},{false,{1,500,2}}};
struct Pot{unsigned id;bool retiring;cXyz position;};std::vector<Pot>s_pots{{1,false,{1,30,2}},{2,true,{1,600,2}}};
void*mod_ctx=nullptr;std::vector<std::string>messages;
struct Log{void info(void*,const char* text){messages.emplace_back(text);}}logger;auto*svc_log=&logger;
'''
main=r'''
int main(){
 for(unsigned i=0;i<20;++i)s_eliteFloors.push_back({{0,0,0},{100,0,0},{0,0,100}});
 s_reachableFloor.seed(s_arenaSpawnPosition,unsigned(s_eliteFloors.size()));
 while(!s_reachableFloor.ready()){
  s_reachableFloor.build(s_eliteFloors,1024);
  s_reachableFloor.expand([](cXyz,cXyz){return WalkResult::Clear;});
 }
 s_map=1;update_spawn_diagnostics(&player);record_arena_spawn_sample({1,2,3},{1,2,3},true);assert(messages.empty());
 s_map=0;update_spawn_diagnostics(nullptr);assert(messages.empty());
 update_spawn_diagnostics(&player);
 if(!debug_enabled){record_arena_spawn_sample({1,2,3},{1,2,3},true);assert(messages.empty());return 0;}
 assert(messages.size()==9); // One summary and at most eight triangles per tick.
 assert(messages[0].find("chests=1 y=[20.00,20.00]")!=std::string::npos);
 assert(messages[0].find("pots=1 y=[30.00,30.00]")!=std::string::npos);
 assert(messages[1].find("triangle=0 reached=1")!=std::string::npos);
 for(unsigned i=0;i<40;++i)record_arena_spawn_sample({1,-100,3},{1,-90,3},i%2);
 assert(messages.size()==25&&s_spawnDiagnostics.accepted==20&&s_spawnDiagnostics.rejected==20);
 for(unsigned i=1;i<1800;++i){
  const auto before=messages.size();update_spawn_diagnostics(&player);assert(messages.size()-before<=10);
 }
 assert(s_spawnDiagnostics.frames==1800&&s_spawnDiagnostics.meshDone);
 const auto before=messages.size();
 for(unsigned i=0;i<200;++i){update_spawn_diagnostics(&player);record_arena_spawn_sample({},{},true);}
 assert(messages.size()==before&&s_spawnDiagnostics.accepted==20);
 // Neither capture nor timeouts change the sampled component or live objects.
 assert(s_reachableFloor.reached_count()==20&&s_chests[0].position.y==20&&s_pots[0].position.y==30);
 s_reachableFloor.clear();update_spawn_diagnostics(&player);
 assert(s_spawnDiagnostics.frames==1&&s_spawnDiagnostics.accepted==0&&messages.size()==before+1);
 // A missing/unfinished scan is diagnosed too, with no premature triangle dump.
 assert(messages.back().find("ready=0")!=std::string::npos);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_spawn_diagnostics.inc').read_text()+main)
 for enabled in (0,1):
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Survivors spawn diagnostics: scope, frame/log limits, scan generations, disabled build and gameplay isolation passed')
