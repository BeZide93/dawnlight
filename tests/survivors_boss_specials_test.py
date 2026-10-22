"""Production repeat throws, mounted archery, summons and elite cooldown boundaries."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_enemy_balance.inc').read_text()+(root/'src/survivors_boss_specials.inc').read_text()
profiles=sorted(set(re.findall(r'fpcNm_\w+',source)))
fixture=r'''
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <vector>
struct { unsigned ticks=0; double endless_factor()const{return 1.0;} }s_run;
using ActorId=unsigned;using ProfileName=int;using s16=short;constexpr ActorId kNone=~0u;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* a,unsigned i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}}
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator*(float f)const{return{x*f,y*f,z*f};}
 cXyz& operator+=(cXyz b){x+=b.x;y+=b.y;z+=b.z;return *this;}};
struct csXyz{s16 x=0,y=0,z=0;csXyz()=default;csXyz(s16 a,s16 b,s16 c):x(a),y(b),z(c){}};
struct fopAc_ac_c{ActorId id=1;struct{cXyz pos;}current;csXyz shape_angle;cXyz speed;};
struct SourceHit{struct{int type,power,flags;}at;struct{int type,flags;}tg;int co;};
struct dCcD_SrcSph{struct{struct{int flags;SourceHit hit;}obj;int at[5],tg[5],co[1];}info;struct{struct{cXyz pos;float radius;}sph;}attr;};
constexpr int AT_TYPE_CSTATUE_SWING=1,dCcD_SE_SWORD=2,dCcD_SE_NONE=0;
struct dCcD_Stts{fopAc_ac_c* actor=nullptr;void Init(int,int,fopAc_ac_c* a){actor=a;}};
struct dCcD_Sph{bool active=false;int power=0;dCcD_Stts* status=nullptr;cXyz pos;
 void Set(const dCcD_SrcSph& s){power=s.info.obj.hit.at.power;}void SetStts(dCcD_Stts* s){status=s;}
 void SetC(cXyz p){pos=p;}void OnAtSetBit(){active=true;}void OffAtSetBit(){active=false;}};
struct CC{unsigned count=0;void Set(dCcD_Sph* s){assert(s->active&&s->status->actor&&s->power==4);++count;}}cc;
CC* dComIfG_Ccsp(){return &cc;}
struct Model{int writes=0;void setBaseTRMtx(int){++writes;}};
namespace mDoMtx_stack_c{void transS(cXyz){}void YrotM(s16){}int get(){return 0;}}
struct Native:fopAc_ac_c{
 fopAc_ac_c& actor=*this;ActorId mRiderID=kNone,mKargarokID=kNone;int mRideState=0,mDemoMode=3,mIFrameTimer=0,mKargarokDeleteTimer=0,mFadeTimer=0,field_0xa6e=0;
 s16 mPlayerAngleY=0;int mAction=0,mMode=0,mAnm=0,mMoveMode=0,mActionMode1=0,mActionMode=0,mStates[16]{};
 int action=0,mode=0,field_0x676=0,field_0x678=0,field_0x6c2=0,field_0x6c0=0;
 int timer[6]{},timers[6]{},mTimer[6]{},mTimers[6]{},field_0x69c[6]{},field_0x698[6]{};
 int* mpMorf=nullptr;const char* mpArcName="rider";Model model;Model* mpPartModels[16]{};cXyz mPositions[16];int swordBit=1;
 Native(){mpPartModels[13]=&model;}void setSwordAtBit(int n){swordBit=n;}
};
struct e_mf_class:Native{int field_0x6c0[6]{};};
using e_dn_class=Native;using e_sf_class=Native;using e_kr_class=Native;using e_mm_class=Native;
using daE_OC_c=Native;using e_rd_class=Native;using e_sh_class=Native;using e_s1_class=Native;
using e_rdy_class=Native;using e_yc_class=Native;using b_gnd_class=Native;using daE_YMB_c=Native;using daB_TN_c=Native;
struct Enemy{ActorId id=1;ProfileName profile=0;bool elite=true,dormant=false,initialized=true,retiring=false,credited=false;
 unsigned age=0,stagger=0,specialCooldown=0,summonCooldown=240,throwLife=0;bool extraAttackReady=false;dCcD_Stts extraStatus;dCcD_Sph extraAttack;}owner;
std::map<ActorId,fopAc_ac_c*>actors;std::set<ActorId>pending;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
bool fpcM_IsCreating(ActorId id){return pending.contains(id);}bool fopAcM_IsExecuting(ActorId id){return actors.contains(id)&&!pending.contains(id);}
ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetRoomNo(fopAc_ac_c*){return 0;}
Enemy* tracked(ActorId id){return id==owner.id?&owner:nullptr;}
struct Child{ActorId parent;bool retiring=false;};std::map<ActorId,Child>children;
Child* tracked_enemy_arrow(ActorId id){auto it=children.find(id);return it==children.end()?nullptr:&it->second;}
bool running=true;Enemy* encounter_owner(fopAc_ac_c* a){return tracked(a->id);}
bool encounter_running(Enemy& e){return running&&e.initialized&&!e.retiring&&!e.credited;}
unsigned animations=0;void encounter_animation(int*,const char*,int){++animations;}
fopAc_ac_c player;bool playerPresent=true;auto* daAlink_getAlinkActorClass(){return playerPresent?&player:nullptr;}
float distanceToPlayer=1000;float fopAcM_searchPlayerDistance(fopAc_ac_c*){return distanceToPlayer;}
s16 fopAcM_searchPlayerAngleY(fopAc_ac_c*){return 0;}
float cM_ssin(s16 a){return std::sin(a*3.14159265f/32768);}float cM_scos(s16 a){return std::cos(a*3.14159265f/32768);}
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265f);}
bool floorValid=true;bool dry_arena_floor(cXyz& p,float,float){p.y=25;return floorValid;}
struct Spawn{int profile;ActorId parent;unsigned params;cXyz pos;};std::vector<Spawn>spawns;
ActorId fopAcM_createChild(int profile,ActorId parent,unsigned params,const cXyz* pos,int,const csXyz*,const cXyz*,int,void*){
 spawns.push_back({profile,parent,params,*pos});pending.insert(100+spawns.size());return 100+spawns.size();}
'''
fixture+=r'''
struct Tactics{bool waitingVolley=false;unsigned pendingRiders=0,riderDelay=0,ridersUntil=0;int swordState=0,volley=0;s16 yaw=0;cXyz target{},throwTarget{};};
struct HitSource{int riders=0;};std::map<ActorId,HitSource>s_enemyHitSources;
std::map<ActorId,Tactics>s_bossTactics;
bool boss_attack_paused(const Enemy&){return false;}
bool boss_major_ready(Enemy&,unsigned){return true;}
cXyz boss_lead_target(float){return player.current.pos;}
s16 boss_target_yaw(fopAc_ac_c*,const cXyz&){return 0;}
'''
main=r'''
int main(){
 Native boss;actors[1]=&boss;std::any args[]={&boss};
 // Mounted child creation retries only after failure/deletion, never while pending.
 owner.profile=fpcNm_E_YC_e;maintain_carrier_rider(&boss,owner);assert(spawns.size()==1&&spawns[0].parent==1&&spawns[0].profile==fpcNm_E_RDY_e);
 maintain_carrier_rider(&boss,owner);assert(spawns.size()==1);
 Native rider;rider.id=boss.mRiderID;children[rider.id]={1};actors[rider.id]=&rider;pending.clear();
 maintain_carrier_rider(&boss,owner);assert(spawns.size()==1);
 std::any rideArgs[]={&rider};rider.mKargarokDeleteTimer=1;rider.mFadeTimer=1;rider.field_0xa6e=1;
 before_boss_carrier_ride(nullptr,rideArgs,nullptr,nullptr);
 assert(rider.mAction==0x20&&rider.mMode==1&&rider.mKargarokID==1&&rider.mRideState==2&&rider.mIFrameTimer==20);
 assert(!rider.mKargarokDeleteTimer&&!rider.mFadeTimer&&!rider.field_0xa6e&&rider.mTimer[0]==14);
 rider.mMode=2;before_boss_carrier_ride(nullptr,rideArgs,nullptr,nullptr);assert(rider.mMode==2&&animations==1);
 children[rider.id].retiring=true;assert(!prepare_carrier_rider(&rider));children[rider.id].retiring=false;
 pending.insert(1);assert(!prepare_carrier_rider(&rider));pending.clear();
 // Five native variants on safe ground; invalid sites and missing Link do not spawn.
 owner.profile=fpcNm_B_GND_e;spawns.clear();summon_ghost_riders(&boss,owner);assert(spawns.size()==3&&s_bossTactics[owner.id].pendingRiders==2);
 summon_ghost_riders(&boss,owner);assert(spawns.size()==5&&s_bossTactics[owner.id].pendingRiders==0);
 for(unsigned i=0;i<5;++i)assert(spawns[i].profile==fpcNm_E_FK_e&&spawns[i].parent==1&&(spawns[i].params&255)==i&&spawns[i].pos.y==25);
 floorValid=false;summon_ghost_riders(&boss,owner);assert(spawns.size()==5);floorValid=true;
 playerPresent=false;summon_ghost_riders(&boss,owner);assert(spawns.size()==5);playerPresent=true;s_bossTactics[owner.id].pendingRiders=0;s_bossTactics[owner.id].ridersUntil=0;
 // All native entries into charge share one cooldown; running animations survive.
 boss.mMoveMode=10;owner.specialCooldown=0;before_boss_gnd_charge(nullptr,args,nullptr,nullptr);assert(owner.specialCooldown==150);
 boss.mMoveMode=10;before_boss_gnd_charge(nullptr,args,nullptr,nullptr);assert(boss.mMoveMode==0&&boss.mActionMode==10);
 boss.mMoveMode=12;before_boss_gnd_charge(nullptr,args,nullptr,nullptr);assert(boss.mMoveMode==12);
 // Repeat throws run in both armor phases and do not reset the body action.
 owner.profile=fpcNm_B_TN_e;
 for(int phase:{3,9}){boss.mActionMode1=phase;owner.throwLife=0;owner.specialCooldown=0;boss.mStates[13]=phase==3?1:7;
  before_boss_tn_throw(nullptr,args,nullptr,nullptr);assert(boss.mStates[13]==2&&owner.specialCooldown==90&&owner.throwLife==44&&boss.mActionMode1==phase);
  boss.mStates[13]=3;boss.mPositions[13]={20,30,40};boss.mStates[15]=phase==9?1:0;boss.swordBit=1;
  after_boss_tn_throw_collision(nullptr,args,nullptr,nullptr);assert(owner.extraAttackReady&&owner.extraAttack.pos.x==20&&boss.swordBit==(phase==9?1:0));
  for(int i=0;i<44;++i)before_boss_tn_throw(nullptr,args,nullptr,nullptr);
  assert(owner.throwLife==0&&boss.mStates[13]==(phase==3?1:7));
  after_boss_tn_throw_collision(nullptr,args,nullptr,nullptr);assert(!owner.extraAttack.active);
 }
 assert(cc.count==2);owner.specialCooldown=0;distanceToPlayer=300;before_boss_tn_throw(nullptr,args,nullptr,nullptr);assert(owner.throwLife==0);
 distanceToPlayer=1000;running=false;before_boss_tn_throw(nullptr,args,nullptr,nullptr);assert(owner.throwLife==0);running=true;
 // Bloat has no Acch: its own post-move adapter multiplies flight only once.
 owner.stagger=0;boss.current.pos={};boss.speed={4,6,8};after_boss_ymb_move(nullptr,args,nullptr,nullptr);
 assert(boss.current.pos.x==2&&boss.current.pos.y==3&&boss.current.pos.z==4);
 owner.stagger=1;after_boss_ymb_move(nullptr,args,nullptr,nullptr);assert(boss.current.pos.x==2);owner.stagger=0;
 // Halve readiness/recovery timers; do not shorten native attack windows.
 owner.profile=fpcNm_E_S1_e;boss.mAction=3;boss.mMode=1;boss.mTimers[0]=30;
 for(int i=0;i<15;++i){advance_elite_cooldowns(&boss,owner);if(boss.mTimers[0]>0)--boss.mTimers[0];}assert(boss.mTimers[0]==0);
 boss.mMode=2;boss.mTimers[0]=30;advance_elite_cooldowns(&boss,owner);assert(boss.mTimers[0]==30);
 boss.mMode=1;owner.elite=false;advance_elite_cooldowns(&boss,owner);assert(boss.mTimers[0]==30);owner.elite=true;
 owner.dormant=true;advance_elite_cooldowns(&boss,owner);assert(boss.mTimers[0]==30);owner.dormant=false;
 owner.profile=fpcNm_E_RDB_e;advance_elite_cooldowns(&boss,owner);assert(boss.mTimers[0]==30);
 owner.profile=fpcNm_E_YC_e;boss.mTimer[2]=150;for(int i=0;i<75;++i){advance_elite_cooldowns(&boss,owner);--boss.mTimer[2];}assert(boss.mTimer[2]==0);
 Native foreign;foreign.id=9;std::any foreignArgs[]={&foreign};before_boss_tn_throw(nullptr,foreignArgs,nullptr,nullptr);assert(foreign.model.writes==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
 constants='constexpr int '+','.join(f'{p}={i+1}' for i,p in enumerate(profiles))+';\n'
 cpp.write_text(fixture+constants+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors boss specials: mounted archery, safe summons, shared charge cooldown, two-phase repeat throws/collision and elite clocks passed')
