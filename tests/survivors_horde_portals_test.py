"""Exercise native portal ownership, A activation, random respawns and run gates."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <cstring>
#include <limits>
#include <set>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;using s16=short;constexpr ActorId kNone=~0u;
struct ModContext{};ModContext* mod_ctx=nullptr;enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 float absXZ(cXyz p)const{return std::hypot(x-p.x,z-p.z);}cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};
struct fopAc_ac_c{ActorId id=kNone;struct{cXyz pos;}current;};
struct Animation{float speed=1;unsigned plays=0;void setPlaySpeed(float s){speed=s;}void play(){++plays;}};
using Mtx=float[3][4];
struct Model{Mtx matrix{};void setBaseTRMtx(Mtx m){std::memcpy(matrix,m,sizeof(Mtx));}};
struct Emitter{Mtx matrix{};void setGlobalRTMatrix(const Mtx m){std::memcpy(matrix,m,sizeof(Mtx));}};
struct daObjBossWarp_c:fopAc_ac_c{
 Model model;Model* mpModel=&model;Emitter effect;Emitter* mpParticle[4]{nullptr,&effect,nullptr,nullptr};
 cXyz scale{1,.15f,1};Animation brk,btk0,btk1;Animation* mpBrkAnm=&brk;Animation* mpBtkAnm[2]{&btk0,&btk1};
 unsigned appearances=0,matrices=0,disappearances=0;
 void set_appear(){++appearances;}void setBaseMtx(){++matrices;}void disappear(int){++disappearances;}
};
struct daAlink_c:fopAc_ac_c{bool wolf=false;bool checkWolf(){return wolf;}}player;
ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}
std::map<ActorId,daObjBossWarp_c> actors;std::set<ActorId> pending;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:&it->second;}
bool fpcM_IsCreating(ActorId id){return pending.count(id);}
struct Service{unsigned deletes=0;void delete_actor(ModContext*,ActorId id){++deletes;actors.erase(id);}}service;
auto* svc_actor=&service;
Run s_run;unsigned s_map=1,s_player=42,s_spawnCooldown=50;bool s_victory=false,s_arenaMeshReady=true;
bool active=true,paused=false,distributionReady=true,badFloor=false,nearGuard=false,failCreate=false,wallHit=false;
bool playable(){return active&&!paused;}
unsigned life=12;unsigned dComIfGs_getLife(){return life;}
struct Arena{int room=0;}arenaData;const Arena& arena(){return arenaData;}
bool arena_distribution_ready(){return distributionReady;}float arena_distribution_area(){return 16000000;}
cXyz nextPosition{2000,0,2000};unsigned samples=0,creates=0;
bool random_arena_floor(cXyz& p){++samples;p=nextPosition;return !badFloor;}
bool separated_guard(cXyz,float){return !nearGuard;}
struct Chest{bool active=false;cXyz position;};std::array<Chest,6>s_chests;
struct Pot{ActorId id=kNone;cXyz position;};std::array<Pot,20>s_pots;
struct ActorSpawnParams{unsigned parameters;int argument,room_num;cXyz position;struct{s16 x,y,z;}angle;cXyz scale;void* create_function;};
constexpr int MOD_OK=0,fpcNm_Obj_BossWarp_e=7;
int create_standalone_actor(int profile,const ActorSpawnParams& p,ActorId& id){
 assert(profile==fpcNm_Obj_BossWarp_e&&p.parameters==0x17ffffff&&p.argument==-1&&p.room_num==arena().room);
 assert(p.scale.x==1&&p.scale.y==1&&p.scale.z==1&&!p.create_function);
 if(failCreate)return 1;id=++creates;pending.insert(id);return MOD_OK;
}
struct dBgS_LinChk{void SetLink(){}void Set(cXyz*,cXyz*,daAlink_c*){}};
float slopeX=0,slopeZ=0,roughness=0;bool missingNormal=false,missingFloor=false;
unsigned groundQueries=0;
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct cM3dGPla{cXyz mNormal;};
struct Background{
 bool LineCross(dBgS_LinChk*){return wallHit;}
 float GroundCross(dBgS_ObjGndChk*){++groundQueries;return missingFloor?-1.e9f:nextPosition.y;}
 bool GetTriPla(dBgS_ObjGndChk&,cM3dGPla* p){p->mNormal={-slopeX,1,-slopeZ};return !missingNormal;}
}background;
bool dry_arena_floor(cXyz& p,float,float){++groundQueries;
 p.y=nextPosition.y+slopeX*(p.x-nextPosition.x)+slopeZ*(p.z-nextPosition.z)+roughness;return !missingFloor;
}
auto& dComIfG_Bgsp(){return background;}
constexpr unsigned PAD_1=0,PAD_BUTTON_A=1,BUTTON_STATUS_ENTER=7;
struct Pad{unsigned mPressedButtonFlags=0;}pad;unsigned prompts=0;
struct mDoCPd_c{static bool getTrigA(unsigned){return pad.mPressedButtonFlags&PAD_BUTTON_A;}
 static Pad& getCpadInfo(unsigned){return pad;}};
void dComIfGp_setDoStatusForce(unsigned status,unsigned){assert(status==BUTTON_STATUS_ENTER);++prompts;}
'''
fixture+=r'''
bool activate_fate(daAlink_c*,const cXyz&){s_run.apply_fate(FateEffect::Horde);s_spawnCooldown=0;return true;}
'''
main=r'''
void fresh(unsigned map=1){
 s_hordePortals={};s_hordePortalDelay=0;actors.clear();pending.clear();s_chests={};s_pots={};
 s_run.start(42);s_map=map;s_spawnCooldown=50;s_victory=false;player={};player.id=s_player;
 active=s_arenaMeshReady=distributionReady=true;paused=badFloor=nearGuard=failCreate=wallHit=false;
 slopeX=slopeZ=roughness=0;missingNormal=missingFloor=false;groundQueries=0;
 samples=creates=prompts=service.deletes=0;life=12;pad={};nextPosition={2000,0,2000};
}
void ready(unsigned slot){auto& p=s_hordePortals[slot];pending.erase(p.id);auto& a=actors[p.id];a.id=p.id;a.current.pos=p.position;
 std::any args[]={&a};int result=0;
 assert(before_horde_portal_execute(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
 assert(p.initialized&&a.scale.y==1&&a.appearances==1&&a.brk.speed==0&&a.btk1.speed==0);
 before_horde_portal_execute(nullptr,args,&result,nullptr);assert(a.appearances==1);
}
void interact(bool pressed){pad.mPressedButtonFlags=pressed?PAD_BUTTON_A:0;std::any args[]={&player};
 assert(before_horde_portal_action(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);}
int main(){
 for(unsigned map=0;map<5;++map){fresh(map);update_horde_portals(&player);assert(creates==unsigned(map==1||map==2));}
 for(unsigned reason=0;reason<7;++reason){fresh();
  if(reason==0)paused=true;if(reason==1)s_run.phase=Phase::Upgrade;if(reason==2)s_victory=true;
  if(reason==3)distributionReady=false;if(reason==4)s_arenaMeshReady=false;if(reason==5)active=false;
  if(reason==6)s_run.ticks=s_run.run_duration();
  update_horde_portals(&player);assert(!creates&&!samples);
 }
 fresh();s_run.ticks=s_run.run_duration();s_run.endless=true;update_horde_portals(&player);assert(creates==1);
 fresh();for(unsigned i=0;i<4;++i){nextPosition={2000.f+2000*i,0,2000};s_hordePortalDelay=0;
  update_horde_portals(&player);assert(creates==i+1&&!horde_portal_visible(s_hordePortals[i]));ready(i);
  assert(horde_portal_visible(s_hordePortals[i]));
 }
 for(unsigned i=0;i<30;++i)update_horde_portals(&player);assert(creates==4);
 const auto old=s_hordePortals[0].position;const auto id=s_hordePortals[0].id;
 player.current.pos=old;interact(false);assert(prompts==1&&!s_run.horde_remaining);
 // All gameplay/height/wall gates leave the action and portal untouched.
 for(unsigned reason=0;reason<8;++reason){
  paused=reason==0;s_run.phase=reason==1?Phase::TreasureChoice:Phase::Running;s_victory=reason==2;
  player.wolf=reason==3;life=reason==4?0:12;wallHit=reason==5;player.current.pos=old;
  if(reason==6)player.current.pos.y+=100;if(reason==7)player.current.pos.x+=200;
  interact(true);assert(!s_run.horde_remaining&&horde_portal_visible(s_hordePortals[0])&&pad.mPressedButtonFlags==PAD_BUTTON_A);
 }
 paused=s_victory=player.wolf=wallHit=false;s_run.phase=Phase::Running;life=12;player.current.pos=old;
 s_run.ticks=100;interact(true);
 assert(s_run.horde_remaining==horde_duration&&!s_spawnCooldown&&!pad.mPressedButtonFlags);
 assert(s_hordePortals[0].respawnAt==100+3*60*ticks_per_second&&!horde_portal_visible(s_hordePortals[0]));
 assert(actors[id].disappearances==1);interact(true);assert(s_run.horde_remaining==horde_duration);
 cleanup_horde_portals();cleanup_horde_portals();assert(s_hordePortals[0].id==kNone);
 s_run.ticks=s_hordePortals[0].respawnAt-1;s_hordePortalDelay=0;update_horde_portals(&player);assert(creates==4);
 ++s_run.ticks;player.current.pos={};nextPosition=old;update_horde_portals(&player);assert(creates==4);
 nextPosition={12000,0,2000};s_hordePortalDelay=0;update_horde_portals(&player);assert(creates==5);ready(0);
 assert(s_hordePortals[0].position.absXZ(old)>=900);player.current.pos=s_hordePortals[0].position;
 interact(true);assert(s_run.horde_remaining==2*horde_duration); // Duration stacks, boost stays 3x.
 // Slopes in either direction and diagonally align the model and effects.
 for(const auto slope:std::array<std::array<float,2>,5>{{{0,0},{.35f,0},{-.35f,0},{0,.4f},{.3f,-.25f}}}){
  fresh();slopeX=slope[0];slopeZ=slope[1];update_horde_portals(&player);assert(creates==1);ready(0);
  const auto& p=s_hordePortals[0];auto& a=actors[p.id];
  for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col)assert(a.model.matrix[row][col]==a.effect.matrix[row][col]);
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
   float dot=0;for(unsigned r=0;r<3;++r)dot+=a.model.matrix[r][i]*a.model.matrix[r][j];
   assert(std::abs(dot-(i==j?1.f:0.f))<.0001f);
  }
  for(const auto corner:std::array<std::array<float,2>,4>{{{-250,-250},{250,-250},{250,250},{-250,250}}}){
   const auto& m=a.model.matrix;
   const float x=m[0][0]*corner[0]+m[0][2]*corner[1]+m[0][3];
   const float y=m[1][0]*corner[0]+m[1][2]*corner[1]+m[1][3];
   const float z=m[2][0]*corner[0]+m[2][2]*corner[1]+m[2][3];
   const float floor=p.position.y+slopeX*(x-p.position.x)+slopeZ*(z-p.position.z);
   assert(y>floor&&y-floor<5); // Every rim corner clears a planar hillside.
  }
  const auto queried=groundQueries;std::any args[]={&a};
  for(unsigned i=0;i<60;++i){int result=0;before_horde_portal_execute(nullptr,args,&result,nullptr);}
  assert(groundQueries==queried); // No recurring floor scan during Execute.
  player.current.pos={p.position.x+140,p.position.y+slopeX*140,p.position.z};
  interact(true);assert(s_run.horde_remaining==horde_duration);
 }
 // Invalid/steep floor planes and uneven footprints reroll, mild bumps lift.
 for(unsigned reason=0;reason<5;++reason){fresh();
  if(reason==0)missingNormal=true;if(reason==1)missingFloor=true;
  if(reason==2)slopeX=2;if(reason==3)slopeX=std::numeric_limits<float>::quiet_NaN();if(reason==4)roughness=30;
  update_horde_portals(&player);assert(!creates);
 }
 fresh();roughness=8;update_horde_portals(&player);assert(creates==1&&s_hordePortals[0].lift==11);
 // Foreign native portals keep normal execution, even at our old position.
 daObjBossWarp_c foreign;foreign.id=999;std::any foreignArgs[]={&foreign};int result=0;
 assert(before_horde_portal_execute(nullptr,foreignArgs,&result,nullptr)==HOOK_CONTINUE);
 // Cleanup tracks pending creation through deactivation, never duplicates IDs.
 fresh();update_horde_portals(&player);retire_horde_portals();active=false;cleanup_horde_portals();
 assert(s_hordePortals[0].id!=kNone&&!service.deletes);
 auto& p=s_hordePortals[0];pending.erase(p.id);auto& a=actors[p.id];a.id=p.id;
 std::any args[]={&a};assert(before_horde_portal_execute(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 cleanup_horde_portals();cleanup_horde_portals();assert(s_hordePortals[0].id==kNone&&service.deletes==1);
 // Placement failures retry without consuming slots or overlapping other objects.
 for(unsigned reason=0;reason<5;++reason){fresh();badFloor=reason==0;nearGuard=reason==1;failCreate=reason==2;
  if(reason==3)s_chests[0]={true,nextPosition};if(reason==4)s_pots[0]={10,nextPosition};
  update_horde_portals(&player);assert(s_hordePortals[0].id==kNone);
 }
 std::cout<<"Survivors Horde portals: maps, native ownership, A action, stacking, new-site 3-minute respawn and cleanup passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_horde_portals.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
