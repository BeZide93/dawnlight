"""Execute production native-arrow detail selection, draw fallback and lifecycle."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <vector>
#include "survivors_performance_policy.hpp"
namespace survivor_perf=dawnlight::survivor_perf;
namespace dawnlight::survivor_perf {
bool mode=true;unsigned pressure=0;
bool enabled(){return mode;}unsigned tier(){return pressure;}
}
using ActorId=unsigned;constexpr ActorId kNone=~0u;constexpr unsigned enemy_capacity=64;
constexpr int fpcNm_E_ARROW_e=741,ARROW_TYPE_NORMAL=0,ARROW_TYPE_FIRE=1,ARROW_TYPE_BOMB=2,ACTION_ARROW_SHOT=0;
struct cXyz {
 float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return {x+b.x,y+b.y,z+b.z};}
 cXyz operator-(cXyz b)const{return {x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float s)const{return {x*s,y*s,z*s};}
 float abs2()const{return x*x+y*y+z*z;}
};
struct GXColor{unsigned char r,g,b,a;};
struct Actor{ActorId id=kNone;int profile=fpcNm_E_ARROW_e;struct{cXyz pos;}current;};
struct e_arrow_class:Actor{int mArrowType=0,mAction=0,field_0xa14=0;void* mpModel=this;struct{short x=0,y=0;}shape_angle;};
struct Enemy{ActorId id=kNone;bool retiring=false,credited=false,elite=false;};
struct Child{ActorId id=kNone;bool retiring=false;ActorId parent=kNone;bool shell=false;unsigned damageFactor=1;short profile=fpcNm_E_ARROW_e;bool eliteOwner=false;};
std::array<Child,enemy_capacity*4>s_enemyArrows;
std::array<Enemy,enemy_capacity>s_enemies;
std::array<e_arrow_class,enemy_capacity*4>actors;
std::array<bool,enemy_capacity*4>present{},pending{};
Actor player;bool playerPresent=true,s_active=true,s_hub=false,arena=true,transition=false;
bool in_arena(){return s_active&&arena;}bool transitioning(){return transition;}
Actor* dComIfGp_getPlayer(int){return playerPresent?&player:nullptr;}
Actor* actor_by_id(ActorId id){return id<actors.size()&&present[id]?&actors[id]:nullptr;}
bool fpcM_IsCreating(ActorId id){return id<pending.size()&&pending[id];}
int fopAcM_GetName(Actor* a){return a->profile;}ActorId fopAcM_GetID(Actor* a){return a->id;}
Child* tracked_enemy_arrow(ActorId id){for(auto& c:s_enemyArrows)if(c.id==id)return &c;return nullptr;}
float cM_ssin(short a){return std::sin(float(a)*6.2831853f/65536);}
float cM_scos(short a){return std::cos(float(a)*6.2831853f/65536);}
struct Packet {Packet* next=nullptr;Packet* getNextPacket(){return next;}void setNextPacket(Packet* p){next=p;}};
using J3DPacket=Packet;using u32=unsigned;
struct PerformanceLines:Packet{struct Line{cXyz a,b;GXColor color;};std::vector<Line>lines;};
bool listAvailable=true;unsigned submissions=0;
struct List {
 Packet* entries[2]{};Packet** mpBuffer=entries;
 unsigned getEntryTableSize(){return 2;}
 int entryImm(Packet* p,unsigned i){if(!listAvailable)return 0;++submissions;p->setNextPacket(mpBuffer[i]);mpBuffer[i]=p;return 1;}
} drawList;
struct dDlst_list_c{enum{DB_XLU_LIST};};
struct {struct{List* mDrawBuffers[1]{&drawList};}drawlist;}g_dComIfG_gameInfo;
struct ModContext{};void* mod_ctx=nullptr;void* svc_hook=nullptr;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};constexpr int MOD_OK=0;
struct SurvivorsArrowFrame{};struct SurvivorsArrowDraw{};
unsigned registrations=0,removals=0,failAt=0,warnings=0;
namespace mods {
template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}
template<class H,class F>int hook_add_pre(void*,F){return ++registrations==failAt?1:MOD_OK;}
template<class H>void hook_uninstall(void*){++removals;}
}
struct Log{void warn(void*,const char*){++warnings;}}logService;auto* svc_log=&logService;
'''
# Compile the actual shared packet insertion/unlink helpers as well.
source=(root/'src/survivors_performance_visuals.inc').read_text()
fixture+=source[source.index('bool submit_line_packet('):source.index('void submit_performance_lines(')]
fixture+=source[source.index('void clear_line_packet('):source.index('void clear_performance_lines(')]
main=r'''
int main(){
 Packet unrelated;drawList.entries[0]=&unrelated;
 initialize_enemy_arrow_visuals();assert(survivor_perf::presentation.native.available);
 for(unsigned i=0;i<100;++i){s_enemyArrows[i].id=i;actors[i].id=i;present[i]=true;actors[i].current.pos.z=float(i*10);}
 s_enemies[0]={1,false,false,false};s_enemies[1]={2,false,false,true};
 auto draw=[&](unsigned i){std::any args[]{&actors[i]};int result=0;auto action=before_enemy_arrow_draw(nullptr,args,&result,nullptr);if(action==HOOK_SKIP_ORIGINAL)assert(result==1);return action;};
 for(unsigned tier=0;tier<3;++tier){
  survivor_perf::pressure=tier;before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);
  const unsigned limit=survivor_perf::native_arrow_detail_limit(tier);
  for(unsigned i=0;i<100;++i)assert(draw(i)==(i<limit?HOOK_CONTINUE:HOOK_SKIP_ORIGINAL));
  const auto& stats=survivor_perf::presentation.native;
  assert(stats.normalEnemies==1&&stats.eliteEnemies==1&&stats.projectiles==100&&stats.arrows==100);
  assert(stats.normalArrows==100&&stats.fireArrows==0&&stats.bombArrows==0&&stats.eliteArrows==0);
  assert(stats.fullDraws==limit&&stats.reducedDraws==100-limit);
  assert(s_enemyArrowLines.lines.size()==3*(100-limit));
  auto n=s_enemyArrowLines.lines.size();draw(99);assert(s_enemyArrowLines.lines.size()==n); // Re-entry never duplicates packet geometry.
 }
 const auto& line=s_enemyArrowLines.lines.back();assert(line.b.z==1030.f&&line.b.y==0);
 actors[0].mAction=1;before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);
 assert(draw(0)==HOOK_SKIP_ORIGINAL&&draw(16)==HOOK_CONTINUE); // Flying threats before stuck arrows.
 actors[0].mAction=0;
 for(bool* flag:{&survivor_perf::mode,&playerPresent,&arena}){
  *flag=false;before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);assert(draw(99)==HOOK_CONTINUE);*flag=true;
 }
 s_hub=true;before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);assert(draw(99)==HOOK_CONTINUE);s_hub=false;
 transition=true;before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);assert(draw(99)==HOOK_CONTINUE);transition=false;
 before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);
 survivor_perf::mode=false;assert(draw(99)==HOOK_CONTINUE);survivor_perf::mode=true;
 // Native type/hidden state is rechecked after selection, without mutation.
 actors[99].mArrowType=ARROW_TYPE_BOMB;assert(draw(99)==HOOK_CONTINUE);
 actors[99].mArrowType=ARROW_TYPE_FIRE;assert(draw(99)==HOOK_CONTINUE);
 actors[99].mArrowType=0;actors[99].field_0xa14=1;assert(draw(99)==HOOK_CONTINUE);
 actors[99].field_0xa14=0;actors[99].mpModel=nullptr;assert(draw(99)==HOOK_CONTINUE);actors[99].mpModel=&actors[99];
 s_enemyArrows[99].eliteOwner=true;assert(draw(99)==HOOK_CONTINUE); // Boss provenance survives owner death.
 s_enemyArrows[99].eliteOwner=false;
 listAvailable=false;assert(draw(99)==HOOK_CONTINUE&&s_enemyArrowLines.lines.empty());listAvailable=true;
 assert(draw(99)==HOOK_SKIP_ORIGINAL&&s_enemyArrowLines.lines.size()==3);
 // A new ID in a reused slot cannot inherit a previous selection.
 actors[99].id=200;s_enemyArrows[99].id=200;assert(draw(99)==HOOK_CONTINUE);
 actors[99].id=99;s_enemyArrows[99].id=99;
 actors[90].mArrowType=ARROW_TYPE_FIRE;actors[91].mArrowType=ARROW_TYPE_BOMB;s_enemyArrows[92].eliteOwner=true;
 before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);
 assert(survivor_perf::presentation.native.fireArrows==1&&survivor_perf::presentation.native.bombArrows==1&&survivor_perf::presentation.native.eliteArrows==1);
 assert(draw(90)==HOOK_CONTINUE&&draw(91)==HOOK_CONTINUE&&draw(92)==HOOK_CONTINUE);
 actors[90].mArrowType=actors[91].mArrowType=0;s_enemyArrows[92].eliteOwner=false;
 pending[99]=true;s_enemyArrows[98].shell=true;s_enemyArrows[98].profile=468;
 s_enemyArrows[97].retiring=true;
 before_enemy_arrow_frame(nullptr,nullptr,nullptr,nullptr);
 assert(survivor_perf::presentation.native.pending==1&&survivor_perf::presentation.native.attached==1&&survivor_perf::presentation.native.retiring==1);
 assert(draw(99)==HOOK_CONTINUE&&draw(98)==HOOK_CONTINUE&&draw(97)==HOOK_CONTINUE);
 shutdown_enemy_arrow_visuals();assert(drawList.entries[0]==&unrelated&&unrelated.getNextPacket()==nullptr);assert(!s_enemyArrowFrameReady&&s_enemyArrowLines.lines.empty()&&removals==2);
 registrations=0;failAt=2;initialize_enemy_arrow_visuals();
 assert(!survivor_perf::presentation.native.available&&removals==4&&warnings==1);
 assert(draw(96)==HOOK_CONTINUE);
 for(unsigned i=0;i<100;++i)assert(actors[i].current.pos.z==float(i*10)&&actors[i].mArrowType==0); // No simulation edits.
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_enemy_arrow_visuals.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors native arrow visuals: tiers, visible fallback, ownership, lifecycle and native simulation passed')
