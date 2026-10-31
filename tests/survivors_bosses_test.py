"""Native boss entry states, arena portability and child/story isolation."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_bosses.inc').read_text()
profiles=sorted(set(re.findall(r'fpcNm_\w+',source)))
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <vector>
using ActorId=unsigned;using ProfileName=int;using s16=short;using s8=signed char;using u32=unsigned;
constexpr ActorId kNone=~0u;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 float absXZ()const{return std::hypot(x,z);}void zero(){x=y=z=0;}};
struct csXyz{s16 x=0,y=0,z=0;};
struct fopAc_ac_c {ActorId id=1;int profile=0,health=200,field_0x560=200;
 struct{cXyz pos;}current,old;csXyz shape_angle;float speedF=0,gravity=0;cXyz speed;
 struct{unsigned flags=0;int distances[1]{};}attention_info;};
constexpr unsigned fopAc_AttnFlag_BATTLE_e=1,fopAc_attn_BATTLE_e=0;
struct Morf{float rate=1;void setPlaySpeed(float v){rate=v;}};
struct Native:fopAc_ac_c {
 Morf morf;Morf* mpModelMorf=&morf;
 fopAc_ac_c& enemy=*this;
 enum{ACTION_OPACI_CHASE_e=15,ACTION_OPACI_FLY_e=12,ACTION_WAIT_e=2,ACTION_ATTACK_e=3,ACT_CHANGEDEMO=8};
 int mAction=0,mMode=0,mDemoMode=0,mNoDraw=1,mDemoCamMode=0,mNoDrawTimer=0,mActionMode=0,mMoveMode=0,mGakeChkType=0;
 int field_0x1e08=0,field_0x1e0a=0,field_0xc44[10]{},field_0xc7d=0,field_0x2740=0,field_0x3969=0;bool mDrawHorse=true;
 cXyz field_0x2710,field_0x69c;int mTimers[4]{},field_0x5dc=0;float mPlayerDist=0; s16 mPlayerAngleY=0;
 ActorId mBallID=kNone,mMantChildID=kNone;
 int mAlphaType=0,mFadeAwayTimer=0,mGlowLightA=0,mEyeAlpha=0,mFootSmokeAlpha=0,mWeponEfMode=0,animation=0;
 bool mDrawRopes=true,mRopesEnabled=true;int field_0x70c=0,field_0x6d8=0,field_0x6dc=0,field_0x710=1,field_0x717=0,field_0x764=0;
 float field_0x6cc=0,field_0x6d0=0,field_0x6d4=0;
 int mMsgSetTimer=30,mMissedBallCount=1;bool mDrawTriangleAt=false;cXyz mTrianglePos;
 int field_0x7d3=3,field_0x844=0,mPointNum=0,mCurrentPoint=0,field_0x846=0,field_0x7d8=1;
 cXyz mPointPos[10];int field_0x780[10]{},field_0x6b8[4]{};
 struct{struct{s16 y=0;}angle;}dummy;
 int mActionMode1=0,mNextBreakPart=0,changes=0;
 void setActionMode(int a,int m){mAction=mActionMode=mActionMode1=a;mMode=mMoveMode=m;}
 void setBck(int a,int,float,float){animation=a;}void setBreakPart(int n){mNextBreakPart=n;}
 void executeZakoChangeDemo(){++changes;}
};
// Only Phantom uses current.angle in these adapters.
'''
fixture=fixture.replace('struct{cXyz pos;}current,old;', 'struct{cXyz pos;csXyz angle;}current,old;')
fixture+=r'''
using e_rdb_class=Native;using e_th_class=Native;using b_gnd_class=Native;using mant_class=Native;
using daE_VA_c=Native;using daE_YMB_c=Native;using daE_PZ_c=Native;using e_hzelda_class=Native;using daB_TN_c=Native;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* a,unsigned n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
struct Enemy{ActorId id=kNone;int profile=0;bool encounterReady=false,initialized=true,retiring=false,credited=false;unsigned stagger=0,age=0,specialCooldown=0,summonCooldown=240,encounterTick=0;
 cXyz lootPosition,spawnPosition,encounterTarget;};
std::array<Enemy,32>s_enemies;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id==id&&id!=kNone)return &e;return nullptr;}
struct Child{ActorId parent;bool retiring=false;};std::map<ActorId,Child>children;
Child* tracked_enemy_arrow(ActorId id){auto it=children.find(id);return it==children.end()?nullptr:&it->second;}
bool active_enemy_arrow(ActorId id){auto* c=tracked_enemy_arrow(id);return c&&!c->retiring;}
bool arena=true;bool in_arena(){return arena;}Enemy* encounter_owner(fopAc_ac_c* a){return arena?tracked(a->id):nullptr;}
bool is_actor_process(void*){return true;}ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}int fopAcM_GetRoomNo(fopAc_ac_c*){return 7;}
struct{int mColpatWeather=7,wether_pat0=8,wether_pat1=9;}g_env_light;int cDmr_SkipInfo=6;
std::map<ActorId,fopAc_ac_c*>actors;std::set<ActorId>pending;
fopAc_ac_c* actor_by_id(ActorId id){return actors.contains(id)?actors[id]:nullptr;}
bool fpcM_IsCreating(ActorId id){return pending.contains(id);}bool fopAcM_IsExecuting(ActorId id){return actors.contains(id)&&!pending.contains(id);}
fopAc_ac_c player;auto* daAlink_getAlinkActorClass(){return &player;}
s16 fopAcM_searchPlayerAngleY(fopAc_ac_c*){return 123;}
float distanceToPlayer=300;float fopAcM_searchPlayerDistance(fopAc_ac_c*){return distanceToPlayer;}
float cM_ssin(s16 a){return std::sin(a*3.14159265f/32768);}float cM_scos(s16 a){return std::cos(a*3.14159265f/32768);}
bool floorValid=true,pathValid=true;bool dry_arena_floor(cXyz& p,float,float){p.y=500;return floorValid;}
bool elite_floor_position(cXyz& p){p.y=500;return floorValid;}
bool walkable_spawn_path(cXyz,cXyz&,int){return pathValid;}
enum class Species{ShadowKeese,ShadowBeast};enum class Phase{Running,Upgrade};struct{Phase phase=Phase::Running;double endless_factor(){return 1.;}}s_run;
bool encounter_running(Enemy& e){return e.initialized&&!e.retiring&&!e.credited&&s_run.phase==Phase::Running;}
unsigned summons=0;void summon_ghost_riders(b_gnd_class*,const Enemy&){++summons;}
unsigned spawned=0;bool create_enemy_at(cXyz,s16,unsigned,bool){for(auto& e:s_enemies)if(e.id==kNone){e.id=50+spawned++;return true;}return false;}
ActorId childOwner=kNone;unsigned childParams=0,childCreates=0;
ActorId fopAcM_createChild(int,ActorId parent,u32 param,const cXyz*,int,const csXyz*,const cXyz*,s8,void*){
 childOwner=parent;childParams=param;++childCreates;return 88;}
'''
fixture+=r'''
struct Tactics{unsigned pattern=0,summons=0,recovery=0;int pzStage=0;ActorId summoner=kNone;};
std::map<ActorId,Tactics>s_bossTactics;
void update_king_waves(e_rdb_class*,Enemy&){}
void update_zelda_light_volley(e_hzelda_class*,Enemy&){}
bool boss_attack_paused(const Enemy&){return false;}
bool boss_major_ready(Enemy&,unsigned){return true;}
bool boss_pz_minion(Enemy&,int){return true;}
cXyz boss_lead_target(float){return player.current.pos;}
s16 boss_target_yaw(fopAc_ac_c*,const cXyz&){return 0;}
'''
main=r'''
int main(){
 player.current.pos={100,500,200};Native boss;boss.current.pos={1000,500,200};boss.profile=fpcNm_E_PZ_e;
 auto& owner=s_enemies[0];owner.id=boss.id;owner.profile=boss.profile;owner.spawnPosition={200,500,300};owner.lootPosition={200,0,300};
 std::any args[]={&boss};
 // Scoped flags during create/execute, including nested unrelated actors.
 std::any scope[]={0,static_cast<void*>(&boss)};before_boss_scope(nullptr,scope,nullptr,nullptr);
 assert(cDmr_SkipInfo==0&&before_boss_story_write(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 Native foreign;foreign.id=99;foreign.profile=boss.profile;std::any nested[]={0,static_cast<void*>(&foreign)};
 before_boss_scope(nullptr,nested,nullptr,nullptr);assert(before_boss_story_write(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
 after_boss_scope(nullptr,nested,nullptr,nullptr);assert(boss_script_scope());g_env_light.mColpatWeather=1;
 after_boss_scope(nullptr,scope,nullptr,nullptr);assert(cDmr_SkipInfo==6&&g_env_light.mColpatWeather==7&&!boss_script_scope());
 // Dead/missing structural children never reach unguarded native dereferences.
 boss.profile=owner.profile=fpcNm_B_GND_e;int result=0;
 assert(before_boss_gnd_execute(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1&&childCreates==1);
 pending.insert(88);before_boss_gnd_execute(nullptr,args,&result,nullptr);assert(childCreates==1);
 Native cloak;actors[88]=&cloak;pending.clear();before_boss_gnd_execute(nullptr,args,&result,nullptr);
 assert(!boss.mDrawHorse&&boss.mActionMode==10&&boss.mDemoCamMode==0&&cloak.field_0x3969==1);
 boss.mActionMode=19;before_boss_gnd_execute(nullptr,args,&result,nullptr);assert(boss.mActionMode==10);
 // Native melee and both magic projectiles remain in opaque Death Sword AI.
 owner.encounterReady=false;boss.profile=owner.profile=fpcNm_E_VT_e;
 assert(before_boss_va(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE&&boss.mAction==15&&boss.mMode==12);
 assert(boss.mAlphaType==2&&!boss.mDrawRopes&&!boss.mRopesEnabled);
 boss.mAction=12;boss.mMode=0;before_boss_va(nullptr,args,nullptr,nullptr);assert(boss.mMode==10);
 boss.mMode=11;before_boss_va(nullptr,args,nullptr,nullptr);assert(boss.mMode==11);
 boss.mMode=12;before_boss_va(nullptr,args,nullptr,nullptr);assert(boss.mMode==12);
 boss.mMode=2;before_boss_va(nullptr,args,nullptr,nullptr);assert(boss.mAction==15&&boss.mMode==12);
 // Native Bloat charge aims at Link, not Lake Hylia's elevation; visible as human.
 owner.encounterReady=false;before_boss_ymb(nullptr,args,nullptr,nullptr);
 assert(boss.mAction==2&&boss.field_0x6d8==255&&boss.field_0x764==1&&boss.field_0x6d0+550==600);
 boss.mAction=2;boss.mMode=3;before_boss_ymb(nullptr,args,nullptr,nullptr);assert(boss.mAction==2&&boss.mMode==3);
 // A distant chase reaches magic after 2.5 active seconds, without resetting attacks.
 boss.mAction=15;boss.mMode=12;owner.encounterTick=0;distanceToPlayer=1000;
 for(int i=0;i<74;++i)before_boss_va(nullptr,args,nullptr,nullptr);
 assert(boss.mAction==15);before_boss_va(nullptr,args,nullptr,nullptr);
 assert(boss.mAction==12&&boss.mMode==10&&owner.encounterTick==0);
 boss.mMode=12;before_boss_va(nullptr,args,nullptr,nullptr);assert(boss.mMode==12);
 boss.mAction=15;boss.mMode=13;owner.encounterTick=200;
 before_boss_va(nullptr,args,nullptr,nullptr);assert(boss.mAction==15&&boss.mMode==12);
 // King spin only starts at a safe chase boundary; native bosses do not inherit elite clocks.
 boss.mAction=1;owner.specialCooldown=0;distanceToPlayer=500;
 before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.mAction==4&&owner.specialCooldown==180);
 boss.mMode=2;before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.mMode==2);
 boss.mAction=1;before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.mAction==3);
 assert(boss.shape_angle.y==123&&boss.current.angle.y==123);
 // Far Link still receives normal attacks, and spins keep the existing cooldown.
 for(int action:{1,2}){
  boss.mAction=action;boss.mMode=1;boss.field_0x6b8[0]=0;owner.specialCooldown=180;distanceToPlayer=2800;
  before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.mAction==3&&boss.mMode==0&&boss.shape_angle.y==123);
  boss.mAction=action;owner.specialCooldown=0;
  before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.mAction==4&&owner.specialCooldown==180);
 }
 distanceToPlayer=300;
 // Halve only King Bulblin's idle timer: 20 native ticks become 10.
 boss.mAction=1;boss.mMode=1;boss.field_0x6b8[0]=20;owner.specialCooldown=180;
 for(int tick=0;tick<10;++tick){
  if(boss.field_0x6b8[0]>0)--boss.field_0x6b8[0]; // native Execute tick
  before_boss_rdb(nullptr,args,nullptr,nullptr);
  assert(boss.field_0x6b8[0]==18-tick*2);
 }
 before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.field_0x6b8[0]==0);
 for(int action:{1,2,3,4,5}) {
  boss.mAction=action;boss.mMode=0;boss.field_0x6b8[0]=102;
  before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.field_0x6b8[0]==102);
  boss.mMode=2;before_boss_rdb(nullptr,args,nullptr,nullptr);
  assert(boss.field_0x6b8[0]==(action==1?101:102));
 }
 boss.mAction=1;s_run.phase=Phase::Upgrade;boss.field_0x6b8[0]=20;
 before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.field_0x6b8[0]==20);
 s_run.phase=Phase::Running;arena=false;
 before_boss_rdb(nullptr,args,nullptr,nullptr);assert(boss.field_0x6b8[0]==20);arena=true;
 // Ganondorf's distant trigger and independent eight-second summons.
 boss.mActionMode=10;owner.specialCooldown=0;owner.summonCooldown=0;distanceToPlayer=1000;
 before_boss_gnd_execute(nullptr,args,&result,nullptr);assert(boss.mActionMode==11&&boss.mMoveMode==10&&summons==0&&owner.summonCooldown==0);
 boss.mActionMode=10;owner.specialCooldown=150;before_boss_gnd_execute(nullptr,args,&result,nullptr);assert(summons==1&&owner.summonCooldown==240);
 boss.mActionMode=11;boss.mMoveMode=12;before_boss_gnd_execute(nullptr,args,&result,nullptr);assert(boss.mMoveMode==12&&summons==1);
 s_run.phase=Phase::Upgrade;boss.mActionMode=10;owner.summonCooldown=0;
 before_boss_gnd_execute(nullptr,args,&result,nullptr);assert(boss.mActionMode==10&&summons==1);s_run.phase=Phase::Running;
 // Zelda attack clocks and animations run at an absolute 2x, without compounding.
 owner.encounterReady=true;boss.mAction=1;boss.mTimers[0]=10;
 before_boss_hzelda(nullptr,args,nullptr,nullptr);assert(boss.mTimers[0]==9);
 after_boss_hzelda(nullptr,args,nullptr,nullptr);after_boss_hzelda(nullptr,args,nullptr,nullptr);assert(boss.morf.rate==2);
 boss.mAction=0;after_boss_hzelda(nullptr,args,nullptr,nullptr);assert(boss.morf.rate==1);
 s_run.phase=Phase::Upgrade;before_boss_hzelda(nullptr,args,nullptr,nullptr);assert(boss.mTimers[0]==9);
 s_run.phase=Phase::Running;owner.stagger=1;before_boss_hzelda(nullptr,args,nullptr,nullptr);assert(boss.mTimers[0]==9);owner.stagger=0;
 boss.mTimers[0]=10;before_boss_hzelda(nullptr,args,nullptr,nullptr);assert(boss.mTimers[0]==7);
 boss.mTimers[0]=2;before_boss_hzelda(nullptr,args,nullptr,nullptr);assert(boss.mTimers[0]==0);
 // Include Execute's native decrement: a 200-tick idle budget now takes 50
 // active frames instead of 100; charge/recovery still take 100.
 for(int action:{0,1,2,3,4}) {
  boss.mAction=action;boss.mTimers[0]=200;unsigned frames=0;
  while(boss.mTimers[0]){--boss.mTimers[0];before_boss_hzelda(nullptr,args,nullptr,nullptr);++frames;}
  assert(frames==(action==0?50u:100u));
 }
 // Native teleport selection receives only valid points; no empty point table.
 owner.encounterReady=false;before_boss_pz(nullptr,args,nullptr,nullptr);assert(boss.mPointNum==9);
 floorValid=false;before_boss_pz(nullptr,args,nullptr,nullptr);assert(boss.mPointNum==1);floorValid=true;
 s_bossTactics[owner.id].pattern=1;boss.mMoveMode=2;boss.field_0x7d8=5;after_boss_pz_attack(nullptr,args,nullptr,nullptr);assert(boss.field_0x7d8==5);
 s_bossTactics[owner.id].pattern=6;s_bossTactics[owner.id].pzStage=1;boss.field_0x7d8=2;after_boss_pz_attack(nullptr,args,nullptr,nullptr);assert(boss.field_0x7d8==3);
 // Portal minions are tracked ordinary enemies and respect pause/full/floor gates.
 owner.profile=fpcNm_E_PZ_e;children[20]={owner.id,false};cXyz pos{1,1500,2};ActorId id=0;
 std::any summon[]={s16(fpcNm_E_YK_e),ActorId(20),u32(0),static_cast<const cXyz*>(&pos)};
 before_boss_summon(nullptr,summon,&id,nullptr);assert(id==50&&spawned==1);
 s_run.phase=Phase::Upgrade;before_boss_summon(nullptr,summon,&id,nullptr);assert(id==kNone&&spawned==1);
 s_run.phase=Phase::Running;floorValid=false;before_boss_summon(nullptr,summon,&id,nullptr);assert(id==kNone&&spawned==1);floorValid=true;
 // Armor phase follows Survivors HP, retains native throw/unarmored AI.
 boss.mActionMode1=3;boss.health=99;boss.field_0x560=200;boss.mNextBreakPart=0;
 before_boss_tn_damage(nullptr,args,nullptr,nullptr);assert(boss.mActionMode1==8&&boss.mNextBreakPart==12);
 before_boss_tn_change(nullptr,args,nullptr,nullptr);assert(boss.changes==1&&boss.health==99);
 std::any foreignArgs[]={&foreign};assert(before_boss_va(nullptr,foreignArgs,nullptr,nullptr)==HOOK_CONTINUE);
 std::any baseArgs[]={static_cast<fopAc_ac_c*>(&foreign)};assert(before_boss_camera(nullptr,baseArgs,nullptr,nullptr)==HOOK_CONTINUE);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    constants='constexpr int '+','.join(f'{p}={i+1}' for i,p in enumerate(profiles))+';\n'
    cpp.write_text(fixture+constants+source+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors bosses: native entry/attack states, arena anchors, structural children, summon ownership and scoped story flags passed')
