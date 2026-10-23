"""Bullbo pair creation, native rider state, shared damage and isolation."""
from pathlib import Path
import os, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <any>
#include <array>
#include <cassert>
#include <map>
#include <set>
#include <vector>
using ActorId=unsigned;using u32=unsigned;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_E_WB_e=1,fpcNm_E_RD_e=2,fpcNm_B_GG_e=3,ACT_F_WAIT=5;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* a,int i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}}
struct Vec{float x=0,y=0,z=0;};struct Angle{short x=0,y=0,z=0;};
struct fopAc_ac_c{ActorId id=kNone;int profile=0,health=100,field_0x560=100;u32 params=0;
 struct{Vec pos;}current;Angle shape_angle;float speedF=0;};
struct cCcD_Obj{fopAc_ac_c* actor=nullptr;cCcD_Obj* hit=nullptr;bool at=true,tg=true,shield=true;unsigned power=9;float radius=35;float GetR(){return radius;}void SetR(float r){radius=r;}
 fopAc_ac_c* GetAc(){return actor;}void SetAtAtp(unsigned p){power=p;}void OffAtSetBit(){at=false;}
 void OffTgSetBit(){tg=false;}bool ChkTgHit(){return hit;}cCcD_Obj* GetTgHitObj(){return hit;}void ResetTgHit(){hit=nullptr;}};
struct e_wb_class{fopAc_ac_c enemy;ActorId rd_id=kNone;unsigned status_flag=0;
 int demo_mode=2,action=0,ride_mode=0,arg0=0,leader=0,field_0x7a6=0,field_0x7a7=0;bool no_draw=true;cCcD_Obj cc_sph[7];};
struct e_rd_class{fopAc_ac_c enemy;ActorId boar_id=kNone;
 int weapon_type=1,timer[4]{};int boar_stand=0,ride_mode=0,actor_set=0,demo_mode=1,damage_timer=0,one_hit_kill=1,action=0,mode=0;
 cCcD_Obj cc_sph[3];};
ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
u32 fopAcM_GetParam(fopAc_ac_c* a){return a->params;}int fopAcM_GetRoomNo(fopAc_ac_c*){return 0;}
struct Enemy{ActorId id=1;int profile=fpcNm_E_WB_e;bool elite=true,retiring=false,credited=false,initialized=true,encounterReady=false;
 unsigned age=0,stagger=0;cCcD_Obj* childHit=nullptr;std::array<ActorId,2> bullboRiders{kNone,kNone};std::array<unsigned,3> bullboTargetsPending{};}owner;
struct Child{ActorId parent=1;bool retiring=false;};std::map<ActorId,Child> children;
std::map<ActorId,fopAc_ac_c*> actors;std::set<ActorId> pending;
Enemy* tracked(ActorId id){return id==owner.id?&owner:nullptr;}
Child* tracked_enemy_arrow(ActorId id){auto it=children.find(id);return it==children.end()?nullptr:&it->second;}
Enemy* enemy_child_root(ActorId id){if(auto* e=tracked(id))return e;auto* c=tracked_enemy_arrow(id);return c&&!c->retiring?tracked(c->parent):nullptr;}
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return pending.contains(id)||it==actors.end()?nullptr:it->second;}
bool fpcM_IsCreating(ActorId id){return pending.contains(id);}bool fpcM_IsExecuting(ActorId id){return actor_by_id(id)!=nullptr;}
bool arena=true,running=true,refuse=false;
bool in_arena(){return arena;}Enemy* encounter_owner(fopAc_ac_c* a){return a&&arena?tracked(a->id):nullptr;}
bool encounter_running(Enemy& e){return running&&e.initialized&&!e.retiring&&!e.credited;}
struct Spawn{int profile;ActorId parent,id;u32 params;};std::vector<Spawn> spawns;
ActorId fopAcM_createChild(int profile,ActorId parent,u32 params,const Vec*,int,const Angle*,void*,int,void*){
 ActorId id=refuse?kNone:ActorId(10+spawns.size());spawns.push_back({profile,parent,id,params});
 if(id!=kNone){pending.insert(id);children[id]={parent};}return id;}
cCcD_Obj sword,foreignHit;
bool roster_attack(cCcD_Obj* hit){return hit==&sword;}
void allow_roster_target(cCcD_Obj* t){t->tg=true;t->shield=false;}
'''
main = r'''
int main(){
 e_wb_class boar;boar.enemy.id=owner.id;boar.enemy.profile=fpcNm_E_WB_e;actors[owner.id]=&boar.enemy;
 std::any args[]={&boar};
 owner.initialized=false;assert(before_bullbo_action(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&spawns.empty());
 owner.initialized=true;refuse=true;
 assert(before_bullbo_action(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&spawns.size()==2);
 refuse=false;owner.age=1;before_bullbo_action(nullptr,args,nullptr,nullptr);assert(spawns.size()==2);
 owner.age=30;before_bullbo_action(nullptr,args,nullptr,nullptr);assert(spawns.size()==4);
 assert(spawns[2].params==0xff000101u&&spawns[3].params==0xff000202u);
 for(const auto& s:spawns)assert(s.profile==fpcNm_E_RD_e&&s.parent==owner.id);
 for(int i=0;i<60;++i)before_bullbo_action(nullptr,args,nullptr,nullptr);
 assert(spawns.size()==4&&!owner.encounterReady); // Pending IDs never duplicate.
 std::array<e_rd_class,2> riders;
 for(unsigned i=0;i<2;++i){auto& r=riders[i];r.enemy.id=owner.bullboRiders[i];r.enemy.profile=fpcNm_E_RD_e;r.enemy.params=spawns[2+i].params;
  actors[r.enemy.id]=&r.enemy;pending.erase(r.enemy.id);for(auto& t:r.cc_sph)t.actor=&r.enemy;}
 assert(before_bullbo_action(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE&&owner.encounterReady);
 assert(boar.action==ACT_F_WAIT&&boar.status_flag==3&&!boar.demo_mode&&!boar.no_draw);
 boar.action=6;boar.ride_mode=3;before_bullbo_action(nullptr,args,nullptr,nullptr);
 assert(boar.action==6&&boar.ride_mode==3); // Never reset an ongoing native charge.
 for(unsigned i=0;i<2;++i){auto& r=riders[i];std::any ra[]={&r};int result=0;
  assert(before_bullbo_rider(nullptr,ra,&result,nullptr)==HOOK_CONTINUE);
  assert(r.boar_id==owner.id&&r.boar_stand==int(i+1)&&r.ride_mode==2&&r.action==13&&r.damage_timer==0);
  assert(r.enemy.health==boar.enemy.health&&!r.actor_set&&!r.demo_mode&&!r.one_hit_kill);
  r.mode=30;before_bullbo_rider(nullptr,ra,&result,nullptr);assert(r.mode==30);
  r.cc_sph[0].hit=&foreignHit;r.cc_sph[1].hit=&sword;r.cc_sph[2].hit=&sword;
  assert(before_bullbo_rider_damage(nullptr,ra,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&owner.childHit==&sword);
  for(auto& t:r.cc_sph)assert(!t.hit);
  owner.childHit=nullptr;owner.retiring=true;r.cc_sph[0].hit=&sword;
  before_bullbo_rider_damage(nullptr,ra,nullptr,nullptr);assert(!owner.childHit&&!r.cc_sph[0].hit);
  assert(before_bullbo_rider(nullptr,ra,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);owner.retiring=false;
  owner.stagger=3;assert(before_bullbo_rider(nullptr,ra,&result,nullptr)==HOOK_SKIP_ORIGINAL);owner.stagger=0;
 }
 assert(boar.rd_id==riders[0].enemy.id);
 riders[1].weapon_type=2;riders[1].timer[1]=50;
 {std::any a[]={&riders[1]};int result=0;before_bullbo_rider(nullptr,a,&result,nullptr);assert(riders[1].timer[1]==49);}
 riders[0].timer[1]=50;
 {std::any a[]={&riders[0]};int result=0;before_bullbo_rider(nullptr,a,&result,nullptr);assert(riders[0].timer[1]==50);}

 // All new attacks retain native base power 1; rider targets remain hittable.
 auto collision=[&](cCcD_Obj& target){std::any a[]={0,&target};return before_field_boss_targets(nullptr,a,nullptr,nullptr);};
 for(auto& target:boar.cc_sph){target.actor=&boar.enemy;target.radius=40;collision(target);assert(target.radius==60);for(int i=0;i<8;++i)collision(target);assert(target.radius==60);}
 cCcD_Obj charge;charge.actor=&boar.enemy;collision(charge);assert(charge.power==1);
 for(auto& r:riders)for(auto& t:r.cc_sph){t.radius=35;collision(t);assert(t.radius==52.5f);collision(t);assert(t.radius==52.5f&&t.power==1&&!t.shield&&t.tg);}
 fopAc_ac_c arrow;arrow.id=500;children[500]={owner.id};cCcD_Obj shot;shot.actor=&arrow;collision(shot);assert(shot.power==1);
 owner.stagger=2;collision(charge);assert(!charge.at);owner.stagger=0;
 owner.profile=fpcNm_B_GG_e;charge.power=4;collision(charge);assert(charge.power==1);
 owner.elite=false;charge.power=4;collision(charge);assert(charge.power==4);owner.elite=true;
 owner.profile=fpcNm_E_WB_e;arena=false;charge.power=4;collision(charge);assert(charge.power==4);arena=true;
 // Retiring/pending children remain stopped even if the parent is gone.
 std::any ra[]={&riders[0]};int result=0;
 children[riders[0].enemy.id].retiring=true;owner.id=99;
 assert(before_bullbo_rider(nullptr,ra,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);owner.id=1;
 children[riders[0].enemy.id].retiring=false;
 // Foreign actors are untouched, including ordinary Bulblins.
 e_rd_class foreign;foreign.enemy.id=999;std::any fa[]={&foreign};
 assert(before_bullbo_rider(nullptr,fa,&result,nullptr)==HOOK_CONTINUE);
 assert(before_bullbo_rider_damage(nullptr,fa,nullptr,nullptr)==HOOK_CONTINUE);
 // Missing seat retries once per second; the surviving seat is never replaced.
 const auto survivor=owner.bullboRiders[1];actors.erase(owner.bullboRiders[0]);
 owner.age=31;assert(!maintain_bullbo_riders(&boar,owner)&&spawns.size()==4);
 owner.age=60;assert(!maintain_bullbo_riders(&boar,owner)&&spawns.size()==5);
 assert(owner.bullboRiders[1]==survivor);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_bullbo.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Bullbo: two native seats, pending retries, shared hits, attack power, pause/deletion and foreign isolation passed')
