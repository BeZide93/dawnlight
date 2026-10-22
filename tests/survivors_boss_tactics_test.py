"""Execute the combat director against deterministic native/geometry fixtures."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
state=(root/'src/survivors_boss_tactics_state.inc').read_text()
source=(root/'src/survivors_boss_tactics.inc').read_text()
registration=(root/'src/survivors.cpp').read_text()
registration=registration[registration.index('ModResult initialize_survivors('):registration.index('void shutdown_survivors()')]
registration=registration.replace('#include "survivors_roster_hooks.inc"',(root/'src/survivors_roster_hooks.inc').read_text())
spawn_posts=re.findall(r'POST\(SurvivorsEnemyArrow,\s*(\w+)\);',registration)
motion_pres=re.findall(r'PRE\(SurvivorsEnemySpeed,\s*(\w+)\);',registration)
profiles=sorted(set(re.findall(r'fpcNm_\w+_e',source+state)))
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include "survivors_enemy_hits.hpp"
#include "survivors_boss_tactics.hpp"
using namespace dawnlight::survivors;
using ActorId=int;using ProfileName=int;using s16=short;using u32=unsigned;
constexpr int kNone=-1;constexpr unsigned timed_elite_count=4;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* p,unsigned i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}cXyz& operator+=(cXyz b){x+=b.x;y+=b.y;z+=b.z;return *this;}
 float absXZ()const{return std::hypot(x,z);}float absXZ(cXyz b)const{return (*this-b).absXZ();}
 float abs()const{return std::sqrt(x*x+y*y+z*z);}void zero(){x=y=z=0;}};
struct csXyz{s16 x=0,y=0,z=0;csXyz()=default;csXyz(s16 a,s16 b,s16 c):x(a),y(b),z(c){}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
float cM_ssin(s16 a){return std::sin(a*3.14159265/32768);}float cM_scos(s16 a){return std::cos(a*3.14159265/32768);}
struct fopAc_ac_c;
struct cCcD_Obj{fopAc_ac_c* actor=nullptr;bool at=false;cXyz center;float radius=0;
 auto* GetAc(){return actor;}void OffAtSetBit(){at=false;}void OnAtSetBit(){at=true;}void SetC(cXyz p){center=p;}void SetR(float r){radius=r;}};
struct Morph{float frame=0,rate=1;float getFrame(){return frame;}void setPlaySpeed(float r){rate=r;}};
bool floorValid=true,pathValid=true,blocked=false;unsigned probes=0;
struct BG{bool LineCross(void*){return blocked;}}bg;BG& dComIfG_Bgsp(){return bg;}
struct Acch{fopAc_ac_c* actor=nullptr;auto* getMyAc(){return actor;}void OnLineCheck(){}void CrrPos(BG&){}bool ChkGroundHit(){return true;}};
using dBgS_Acch=Acch;
struct dBgS_LinChk{void Set(const cXyz*,const cXyz*,fopAc_ac_c*){}};
struct CC{void Set(cCcD_Obj*){}}cc;CC* dComIfG_Ccsp(){return &cc;}
struct fopAc_ac_c{int id=1,profile=0,health=100;struct Pos{cXyz pos;csXyz angle;}current,old,home;
 csXyz shape_angle;cXyz speed,eyePos,scale{1,1,1};float speedF=0,gravity=0;struct{cXyz position;}attention_info;
};
struct Native:fopAc_ac_c{
 fopAc_ac_c& enemy=*this;
 ActorId mRiderID=kNone;int mIsSlowAttack=0;int action=0,mode=0,mAction=0,mMode=0,mActionMode=0,mActionMode1=0,mActionMode2=0,mMoveMode=0,mOcState=0;
 int field_0x5b4=0,field_0x676=0,field_0x678=0,mCurAction=0,field_0x672=0,mActionPhase=0;
 int field_0x1228[2]{},mStates[16]{},arg0=0,mAnm=0;
 int mTimer3=0; s16 timers[6]{},mTimers[6]{},mTimer[6]{},field_0x69c[6]{},field_0x6c0[6]{},field_0xc44[10]{};
 s16 target_ya=0,search_angle_y=0,mYAngleToPlayer=0,mPlayerAngleY=0,field_0x68c=0;
 s16 field_0x6fc=0;bool mCreateFireBreath=false,mDrawTriangleAt=false;
 cXyz mPositions[16],mPositionsCopy[16],mMagicPos[2],mMagicSpeed[2],field_0x8dc[16],mTrianglePos,mMouthPos;
 struct{cXyz m_pos[50],field_0x320[50];}field_0x65c;
 struct{cXyz m_pos[20],field_0x140[20],field_0x230[20];}field_0xdec,field_0x11d8;
 cCcD_Obj mMagicSphs[2],mAtSph;Morph morph;Morph* mpModelMorf1=&morph;Morph* mpModelMorf2=&morph;Morph* mpMorf=&morph;
 Acch mAcch;
 enum{ACTION_OPACI_CHASE_e=15,ACTION_WAIT_e=2,ACT_F_FLY=10,ACT_ATTACK=11,ACT_B2_WAIT=1,TYPE_BULLET_B=2,TYPE_BULLET_C=3,ANM_FLY_BRAKE=7,ANM_CATCH_START=6,ANM_FLY=9};
 void setActionMode(int a,int m){mAction=mActionMode=mActionMode1=a;mMode=mMoveMode=mOcState=mActionMode2=m;}
 void setBck(int,int,float,float){}
};
'''
fixture+='enum{'+','.join(profiles)+'};\n'
types=sorted(set(re.findall(r'(?:static_cast|reinterpret_cast)<(?:const )?(\w+)\*>',source)) - {'fopAc_ac_c','s16','ActorId','e_kr_class','dBgS_Acch'})
fixture+='\n'.join('using '+t+'=Native;' for t in types)+'\nusing daAlink_c=Native;\nstruct e_kr_class:Native{cXyz field_0x678;};\n'
fixture+=r'''
struct Enemy{ActorId id=kNone;int profile=0,eliteSlot=0;bool initialized=true,credited=false,retiring=false,elite=true,dormant=false;
 unsigned species=0,stagger=0,throwLife=0,specialCooldown=0,arrowVolley=0;bool extraAttackReady=false;cCcD_Obj extraAttack;
 cXyz spawnPosition,lootPosition,encounterTarget;};
std::array<Enemy,32>s_enemies;std::unordered_map<ActorId,fopAc_ac_c*>actors;
struct Child{ActorId id=kNone,parent=kNone;bool retiring=false,shell=false;};std::array<Child,64>s_enemyArrows;
struct HitSource{EnemyAttackCycle cycle;EnemyAttackUsePtr projectile;};std::unordered_map<ActorId,HitSource>s_enemyHitSources;
struct Roster{float height=0;};std::array<Roster,4>native_roster;
Native player;bool playing=true,arena=true;
enum class Phase{Running,Upgrade};struct Run{Phase phase=Phase::Running;unsigned ticks=0,roll=0;unsigned next_random(){return roll++;}}s_run;
bool playable(){return playing;}bool in_arena(){return arena;}
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id==id&&id!=kNone)return &e;return nullptr;}
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
Enemy* enemy_child_root(ActorId id){if(auto* e=tracked(id))return e;for(auto& c:s_enemyArrows)if(c.id==id&&!c.retiring)return tracked(c.parent);return nullptr;}
bool active_enemy_arrow(ActorId id){for(auto& c:s_enemyArrows)if(c.id==id&&!c.retiring)return true;return false;}
bool native_boss_profile(int p){return p==fpcNm_B_TN_e||p==fpcNm_E_RDB_e;}
ActorId fopAcM_GetID(const fopAc_ac_c* a){return a->id;}int fopAcM_GetName(const fopAc_ac_c* a){return a->profile;}
int fopAcM_GetRoomNo(fopAc_ac_c*){return 0;}auto* daAlink_getAlinkActorClass(){return &player;}
float fopAcM_searchPlayerDistance(fopAc_ac_c* a){return (a->current.pos-player.current.pos).abs();}
s16 fopAcM_searchPlayerAngleY(fopAc_ac_c* a){return cM_atan2s(player.current.pos.x-a->current.pos.x,player.current.pos.z-a->current.pos.z);}
bool walkable_spawn_path(cXyz,cXyz& p,float){++probes;p.y=0;return pathValid;}
bool elite_floor_position(cXyz& p){p.y=0;return floorValid;}
bool dry_arena_floor(cXyz& p,float,float){p.y=0;return floorValid;}
struct Body{Morph* morph;};Body extended_body(fopAc_ac_c* a,int){return {&static_cast<Native*>(a)->morph};}
Enemy* encounter_owner(fopAc_ac_c* a){return tracked(a->id);}
bool structural_enemy_child(s16 p){return p==fpcNm_E_TH_BALL_e;}
bool s_spawningEnemyArrows=false;
unsigned spawns=0;std::vector<unsigned> spawnTicks;ActorId nextSpawnId=kNone;
ActorId fopAcM_createChild(int,ActorId,u32,const cXyz*,int,const csXyz*,const cXyz*,int,void*){++spawns;spawnTicks.push_back(s_run.ticks);return nextSpawnId;}
void summon_ghost_riders(Native*,const Enemy&){}
std::string animationArchive;int animationResource=0;
void encounter_animation(Morph* m,const char* archive,int resource,int){m->frame=0;animationArchive=archive;animationResource=resource;}
// Boundary stubs for the tracking/budget stages; dispatch their real production
// registration order, then execute the actual boss callback and flight code.
void after_enemy_arrow(ModContext*,void* args,void* result,void*){
 const ActorId id=*static_cast<ActorId*>(result);
 if(id!=kNone)s_enemyArrows[0]={id,mods::arg<ActorId>(args,1),false,false};
}
void after_enemy_attack_spawn(ModContext*,void* args,void* result,void*){
 const ActorId id=*static_cast<ActorId*>(result);if(!active_enemy_arrow(id))return;
 auto& cycle=s_enemyHitSources[mods::arg<ActorId>(args,1)].cycle;cycle.observe(1,1,1);
 s_enemyHitSources[id].projectile=cycle.current();
}
'''
source='HookAction before_dynalfos_path(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}\nvoid clear_zelda_light_orbs(ActorId){}\n'+source
source+='\nHookAction before_elite_attack_motion(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}\n'
source+='HookAction before_enemy_speed(ModContext*,void* args,void*,void*){auto* actor=mods::arg<dBgS_Acch*>(args,0)->getMyAc();actor->current.pos+=actor->speed*99.f;return HOOK_CONTINUE;}\n'
source+='void dispatch_motion(void* args){\n'+''.join(f'{callback}(nullptr,args,nullptr,nullptr);\n' for callback in motion_pres)+'}\n'
source+='\nvoid dispatch_child_spawn(void* args,ActorId* id){\n'+''.join(
    f'{callback}(nullptr,args,id,nullptr);\n' for callback in spawn_posts)+'}\n'
main=r'''
void reset(){s_enemies={};s_enemyArrows={};s_bossTactics.clear();actors.clear();s_enemyHitSources.clear();s_run={};
 std::destroy_at(&player);std::construct_at(&player);player.id=100;floorValid=pathValid=playing=arena=true;blocked=false;probes=spawns=0;spawnTicks.clear();nextSpawnId=kNone;}
Enemy& own(Native& a,int profile,int slot=0){a.profile=profile;actors[a.id]=&a;auto& e=s_enemies[a.id-1];e.id=a.id;e.profile=profile;e.eliteSlot=slot;return e;}
int main(){
 reset();Native boss;auto& e=own(boss,fpcNm_B_TN_e);
 // Pending timed actors and awakened guards block waves; dormant/credited bosses don't.
 assert(active_boss_encounter());e.initialized=false;assert(active_boss_encounter());e.initialized=true;
 e.eliteSlot=4;e.dormant=true;assert(!active_boss_encounter());e.dormant=false;assert(active_boss_encounter());e.eliteSlot=0;e.credited=true;assert(!active_boss_encounter());e.credited=false;
 // Repeated heavy hits cannot refresh stagger, but no damage gate is added.
 assert(boss_accept_stagger(e));for(int i=0;i<44;++i){s_bossTactics[e.id].stagger.tick();assert(!boss_accept_stagger(e));}
 s_bossTactics[e.id].stagger.tick();assert(boss_accept_stagger(e));
 // Ten active seconds; menus don't advance; actual damage restarts the clock.
 boss.current.pos={5000,0,0};boss.old.pos=boss.current.pos;boss.mActionMode1=9;
 update_boss_watchdogs(&player);for(int i=0;i<299;++i)update_boss_watchdogs(&player);assert(probes==0);
 playing=false;for(int i=0;i<1000;++i)update_boss_watchdogs(&player);assert(probes==0);playing=true;
 boss.health=90;boss_received_damage(e,90);for(int i=0;i<299;++i)update_boss_watchdogs(&player);assert(probes==0);
 update_boss_watchdogs(&player);assert(probes>0&&boss.health==90&&boss.mActionMode1==9);
 assert(boss.current.pos.absXZ(player.current.pos)>=600&&boss.current.pos.absXZ(player.current.pos)<1050);
 assert((boss.current.pos-boss.old.pos).abs()==0&&s_bossTactics[e.id].grace==30);
 // Portal/ghost children use attached lifetimes but are NOT structural weapons.
 Native portal;portal.id=10;portal.profile=fpcNm_E_PZ_e;actors[10]=&portal;
 s_enemyArrows[0]={10,e.id,false,true};
 s_bossTactics[e.id].watchdog.quiet=300;update_boss_watchdogs(&player);assert(s_enemyArrows[0].retiring);
 // No unsafe teleport when all candidates fail; retries bounded to once/second.
 reset();Native stranded;auto& stuck=own(stranded,fpcNm_E_TH_e);stranded.current.pos={9000,0,0};pathValid=false;
 for(int i=0;i<301;++i)update_boss_watchdogs(&player);assert(probes==12&&stranded.current.pos.x==9000);
 for(int i=0;i<29;++i)update_boss_watchdogs(&player);assert(probes==12);
 pathValid=true;update_boss_watchdogs(&player);assert(probes==13&&stranded.current.pos.x!=9000);
 stuck.dormant=true;auto quiet=s_bossTactics[stuck.id].watchdog.quiet;update_boss_watchdogs(&player);assert(s_bossTactics[stuck.id].watchdog.quiet==quiet);
 // Sampled lead is horizontal and capped; teleports don't become huge prediction offsets.
 player.current.pos={30,200,0};player.old.pos={0,0,0};auto lead=boss_lead_target();assert(lead.x==270&&lead.y==200);
 player.current.pos.x=1000;assert(boss_lead_target().x==1000);
 // Two live bosses cannot reserve overlapping major attacks; dead owners release the lease.
 reset();Native one,two;two.id=2;auto& a=own(one,fpcNm_E_RDB_e);auto& b=own(two,fpcNm_B_GND_e);
 assert(boss_major_ready(a,90));assert(!boss_major_ready(b,90));s_run.ticks=90;assert(boss_major_ready(b,90));b.credited=true;assert(boss_major_ready(a,90));
 // Contact safety is shared by body/projectiles, independent per boss and doesn't
 // turn an unaccepted collision into a cooldown.
 cCcD_Obj attack;attack.actor=&one;cCcD_Obj target;target.actor=&player;std::any contact[]={nullptr,&attack,&target};
 assert(before_boss_attack_safety(nullptr,contact,nullptr,nullptr)==HOOK_CONTINUE);boss_record_contact(&attack);
 assert(before_boss_attack_safety(nullptr,contact,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);s_run.ticks+=15;
 assert(before_boss_attack_safety(nullptr,contact,nullptr,nullptr)==HOOK_CONTINUE);
 // Shadow Beast stops in world-space melee range, including boosted crossings.
 reset();Native beast;beast.scale={2,2,2};beast.mAcch.actor=&beast;auto& shadow=own(beast,fpcNm_E_S1_e);
 beast.mAction=2;beast.mMode=1;beast.current.pos={0,0,-550};beast.speedF=20;
 before_boss_tactics(shadow,&beast);assert(beast.mAction==3&&beast.mMode==0&&beast.speedF==0);
 beast.mAction=2;beast.current.pos={0,0,-900};before_boss_tactics(shadow,&beast);
 beast.speed={0,0,39};beast.current.pos.z+=beast.speed.z;std::any shadowPath[]={&beast.mAcch};
 dispatch_motion(shadowPath);assert(std::abs(beast.current.pos.z+500)<.01f);
 // Correct native animation archive and matching attack-window metadata.
 for(unsigned combo:{0u,1u}){
  auto& tactics=s_bossTactics[shadow.id];tactics.attacking=false;tactics.combo=combo;
  beast.mAction=3;beast.mMode=2;boss_after_patterns(shadow,&beast);
  assert(animationArchive=="E_S2"&&animationResource==int(combo?6:5));
  assert(beast.mAnm==animationResource&&beast.mIsSlowAttack==int(combo));
  const auto start=beast.current.pos;
  for(int tick=0;tick<12;++tick)before_boss_tactics(shadow,&beast);
  assert((beast.current.pos-start).absXZ()==0); // No director lunge into Link.
 }
 for(int gate=0;gate<7;++gate){
  reset();beast.mAction=2;beast.mMode=1;beast.current.pos={0,0,-550};beast.speedF=20;
  auto& excluded=own(beast,fpcNm_E_S1_e);
  if(gate==0)excluded.elite=false;if(gate==1)excluded.dormant=true;if(gate==2)excluded.credited=true;
  if(gate==3)excluded.retiring=true;if(gate==4)excluded.initialized=false;if(gate==5)playing=false;
  if(gate==6)excluded.stagger=1;
  before_boss_tactics(excluded,&beast);assert(beast.current.pos.z==-550);
  if(gate!=6)assert(beast.mAction==2&&beast.speedF==20);
 }
 // Ordinary two-hit combo boundaries and shorter single-action guard variant.
 reset();Native bok;auto& bo=own(bok,fpcNm_E_OC_e);bok.mActionMode=4;bok.mOcState=1;
 before_boss_tactics(bo,&bok);bok.mActionMode=3;after_boss_tactics(bo,&bok);assert(bok.mActionMode==4&&s_bossTactics[bo.id].combo==1);
 before_boss_tactics(bo,&bok);bok.mActionMode=3;after_boss_tactics(bo,&bok);assert(s_bossTactics[bo.id].recovery==24);
 s_bossTactics.clear();bo.eliteSlot=4;bok.mActionMode=4;before_boss_tactics(bo,&bok);bok.mActionMode=3;after_boss_tactics(bo,&bok);assert(s_bossTactics[bo.id].recovery==24&&bok.mActionMode==3);
 // Both Stalfos attacks retain alternation and halve the added recovery.
 {reset();Native sf;auto& sk=own(sf,fpcNm_E_SF_e);
 for(int sequence=0;sequence<2;++sequence){sf.mAction=4;sf.mActionPhase=0;before_boss_tactics(sk,&sf);
  assert(sf.mAction==(sequence==0?5:4));sf.mAction=3;after_boss_tactics(sk,&sf);
  assert(s_bossTactics[sk.id].recovery==12);s_bossTactics[sk.id].recovery=0;
 }}
 // Stalhound samples at the native alignment -> tell boundary, not entry.
 {reset();Native hound;auto& sh=own(hound,fpcNm_E_SH_e);hound.field_0x676=3;hound.field_0x678=1;
 player.current.pos=player.old.pos={0,0,500};before_boss_tactics(sh,&hound);
 assert(!s_bossTactics[sh.id].targetLocked);
 for(int i=0;i<25;++i){player.old.pos=player.current.pos;player.current.pos.x+=10;before_boss_tactics(sh,&hound);}
 hound.field_0x678=2;after_boss_tactics(sh,&hound);
 auto& pounce=s_bossTactics[sh.id];assert(pounce.targetLocked&&pounce.target.x==310&&pounce.target.z==500);
 const auto heading=boss_target_yaw(&hound,pounce.target);
 assert(pounce.yaw==heading&&hound.current.angle.y==heading&&hound.shape_angle.y==heading);
 player.current.pos={-500,0,0};before_boss_tactics(sh,&hound);hound.field_0x678=3;after_boss_tactics(sh,&hound);
 assert(pounce.yaw==heading&&hound.current.angle.y==heading&&hound.shape_angle.y==heading);
 // A fresh combo must sample anew, and pauses must not commit a new heading.
 hound.field_0x676=2;after_boss_tactics(sh,&hound);
 assert(hound.field_0x676==3);before_boss_tactics(sh,&hound);assert(!pounce.targetLocked);
 playing=false;hound.field_0x678=2;after_boss_tactics(sh,&hound);assert(!pounce.targetLocked);
 playing=true;after_boss_tactics(sh,&hound);assert(pounce.targetLocked&&pounce.yaw!=heading);}
 // Darkhammer's wait changes once, never compounds; Death Sword magic locks aim once.
 reset();Native hammer;auto& ha=own(hammer,fpcNm_E_TH_e);hammer.mAction=3;hammer.mMode=2;hammer.mTimers[0]=100;
 boss_mobile_patterns(ha,&hammer);assert(hammer.mTimers[0]==45);hammer.mTimers[0]=20;boss_mobile_patterns(ha,&hammer);assert(hammer.mTimers[0]==20);
 reset();Native death;auto& de=own(death,fpcNm_E_VT_e);player.current.pos={500,0,0};death.field_0x1228[0]=2;
 boss_after_patterns(de,&death);assert(std::abs(death.mMagicSpeed[0].abs()-65)<.01f);auto velocity=death.mMagicSpeed[0];player.current.pos={0,0,500};boss_after_patterns(de,&death);assert((death.mMagicSpeed[0]-velocity).abs()==0);
 // Carrier telegraph, straight committed pass, single hit budget and safe recovery.
 reset();Native bird;auto& bi=own(bird,fpcNm_E_YC_e);bird.current.pos={0,300,800};bird.mAction=Native::ACT_ATTACK;
 for(int i=0;i<17;++i){boss_carrier_pass(&bird,bi);assert(!bird.mAtSph.at);}
 auto aim=s_bossTactics[bi.id].target;player.current.pos.x=600;
 boss_carrier_pass(&bird,bi);assert(bird.mAtSph.at&&(s_bossTactics[bi.id].target-aim).abs()==0);
 for(int i=0;i<30;++i)boss_carrier_pass(&bird,bi);assert(!bird.mAtSph.at&&bird.mAction==Native::ACT_F_FLY);
 // The mounted pass waits for an actual successful native arrow before its tell.
 reset();Native mount,rider;auto& mounted=own(mount,fpcNm_E_YC_e);rider.id=3;actors[3]=&rider;mount.mRiderID=3;
 s_enemyArrows[0]={3,mount.id,false,true};boss_carrier_pass(&mount,mounted);
 assert(s_bossTactics[mount.id].waitingVolley&&!mount.mAtSph.at);
 s_enemyArrows[1]={10,mount.id,false,false};const csXyz shotAngle{};const cXyz shotPos{};
 std::any arrowArgs[]={s16(fpcNm_E_ARROW_e),ActorId(3),u32(0),&shotPos,0,&shotAngle};ActorId shotId=10;
 after_boss_child_spawn(nullptr,arrowArgs,&shotId,nullptr);
 assert(!s_bossTactics[mount.id].waitingVolley&&s_bossTactics[mount.id].motionTick==0);
 // Bloat repositions for exactly 0.7 s before the second wind-up, then rests 1.5 s.
 reset();Native bloat;auto& bl=own(bloat,fpcNm_E_YMB_e);bloat.mAction=2;bloat.mMode=5;
 s_bossTactics[bl.id].passes=1;bloat.current.pos={0,200,450};
 for(int i=0;i<20;++i){boss_mobile_patterns(bl,&bloat);assert(bloat.mMode==5);}
 boss_mobile_patterns(bl,&bloat);assert(bloat.mMode==0&&bloat.current.pos.x!=0);
 bloat.mMode=1;boss_after_patterns(bl,&bloat);assert(s_bossTactics[bl.id].passes==2);
 bloat.mMode=5;boss_mobile_patterns(bl,&bloat);assert(s_bossTactics[bl.id].recovery==45&&bloat.mAction==1);
 // Kargarok's actual movement correction follows the sampled line, even if native AI retargets.
 reset();e_kr_class dive;auto& kr=own(dive,fpcNm_E_KR_e);dive.mCurAction=3;dive.field_0x672=4;
 dive.current.pos={0,300,800};dive.mAcch.actor=&dive;boss_mobile_patterns(kr,&dive);
 assert(dive.field_0x672==7);auto committed=s_bossTactics[kr.id].target;player.current.pos.x=900;
 for(int i=1;i<15;++i)boss_mobile_patterns(kr,&dive);
 dive.current.pos={900,300,800};std::any pathArgs[]={static_cast<dBgS_Acch*>(&dive.mAcch)};
 before_boss_path(nullptr,pathArgs,nullptr,nullptr);assert(dive.current.pos.x==0&&dive.current.pos.z<800);
 assert((s_bossTactics[kr.id].target-committed).abs()==0);
 // Salves delay side shots 9 ticks and stop during retirement (caller contract).
 reset();Native head;auto& he=own(head,fpcNm_B_DS_e);auto& ht=s_bossTactics[he.id];ht.queuedShots=2;ht.shotDelay=9;
 for(unsigned i=0;i<=18;++i){s_run.ticks=i;boss_queue_shots(he,&head);}assert(spawns==2&&spawnTicks[0]==9&&spawnTicks[1]==18);
 // Stallord: stale head pose must not determine flight, including every third
 // fan. Native create() initializes mode 2 before the first process Execute.
 // Model executeBullet's Y*X rotation of (0,0,speedF), then native posMove.
 for(int type:{Native::TYPE_BULLET_B,Native::TYPE_BULLET_C})
 for(int volley=1;volley<=3;++volley)
 for(cXyz target:{cXyz(900,0,1200),cXyz(-900,700,1200),cXyz(900,-300,-1200),cXyz(-900,0,-1200)}) {
  reset();Native skull;auto& owner=own(skull,fpcNm_B_DS_e);
  skull.mMouthPos={100,400,50};player.current.pos=target;
  auto& volleyState=s_bossTactics[owner.id];volleyState.sequence=volley-1;
  const cXyz origin=skull.mMouthPos,aim=target+cXyz(0,60,0);
  const csXyz wrongPose(0x2000,0x1000,0);
  ActorId originalId=10;
  std::any fireArgs[]={s16(fpcNm_B_DS_e),owner.id,u32(type),&origin,0,&wrongPose};
  dispatch_child_spawn(fireArgs,&originalId);
  assert(volleyState.queuedShots==2 && volleyState.sequence==unsigned(volley));
  assert((s_bossTactics[10].target-aim).abs()==0 && volleyState.volley);
  player.current.pos={5000,2000,-5000}; // Committed projectiles do not home.
  volleyState.target=player.current.pos; // Other boss phases may update their own target.
  for(int lane=0;lane<3;++lane) {
   Native bullet;bullet.id=10+lane;bullet.profile=fpcNm_B_DS_e;bullet.arg0=type;
   bullet.mMode=2;bullet.speedF=100;bullet.current.angle=wrongPose;
   bullet.current.pos=skull.mMouthPos;
   if(lane) {
    skull.mMouthPos+=cXyz(70,20,-50);bullet.current.pos=skull.mMouthPos;
    s_enemyArrows[lane]={bullet.id,owner.id,false,false};
    nextSpawnId=bullet.id;volleyState.shotDelay=0;boss_queue_shots(owner,&skull);
   }
   auto& flight=s_bossTactics[bullet.id];
   update_boss_projectile(&bullet,false);
   assert(flight.projectileReady && bullet.speedF==125);
   const cXyz d=aim-bullet.current.pos;
   s16 yaw=cM_atan2s(d.x,d.z);
   cXyz expected=aim;
   if(volley==3) {
    assert(s16(bullet.current.angle.y-yaw)==s16((lane-1)*0x1000));
    assert(bullet.current.angle.x==-cM_atan2s(d.y,d.absXZ()));
   } else {
    if(lane)expected+=cXyz(cM_scos(yaw)*(lane==1?-180.f:180.f),0,-cM_ssin(yaw)*(lane==1?-180.f:180.f));
   }
   const csXyz fixed=bullet.current.angle;
   const cXyz before=bullet.current.pos;
   for(int tick=0;tick<4;++tick) {
    const float horizontal=cM_scos(bullet.current.angle.x)*bullet.speedF;
    bullet.speed={cM_ssin(bullet.current.angle.y)*horizontal,-cM_ssin(bullet.current.angle.x)*bullet.speedF,cM_scos(bullet.current.angle.y)*horizontal};
    bullet.current.pos+=bullet.speed;
    update_boss_projectile(&bullet,true);update_boss_projectile(&bullet,false);
    assert(bullet.current.angle.x==fixed.x && bullet.current.angle.y==fixed.y);
   }
   if(volley!=3) {
    const cXyz delta=expected-before;
    assert((bullet.speed-delta*(125.f/delta.abs())).abs()<.1f);
    assert((expected-bullet.current.pos).abs()<(expected-before).abs());
   }
  }
 }
 // Native/unowned bullets must retain their own movement.
 reset();Native unrelated;unrelated.profile=fpcNm_B_DS_e;unrelated.arg0=Native::TYPE_BULLET_C;
 unrelated.mMode=2;unrelated.speedF=100;unrelated.current.angle={123,456,789};
 update_boss_projectile(&unrelated,false);assert(unrelated.speedF==100&&unrelated.current.angle.y==456);
 // Deferred native initialization: don't disturb mode 1, commit on its exit.
 reset();Native waitingHead,waitingBullet;own(waitingHead,fpcNm_B_DS_e);
 waitingBullet.id=10;waitingBullet.profile=fpcNm_B_DS_e;waitingBullet.arg0=Native::TYPE_BULLET_C;
 s_enemyArrows[0]={10,waitingHead.id,false,false};
 auto& pending=s_bossTactics[10];pending.targetLocked=true;pending.target={1000,0,0};
 waitingBullet.mMode=1;update_boss_projectile(&waitingBullet,false);assert(!pending.projectileReady);
 waitingBullet.mMode=2;update_boss_projectile(&waitingBullet,true);
 assert(pending.projectileReady&&waitingBullet.current.angle.y==0x4000&&waitingBullet.speedF==125);
 // Zelda's fixed ten-way selector is 40/30/30; triangle placement does not follow Link.
 reset();Native zelda;auto& ze=own(zelda,fpcNm_E_HZELDA_e);int counts[4]{};
 for(unsigned i=0;i<10;++i){s_run.roll=i;zelda.mAction=1;zelda.mMode=0;s_bossTactics[ze.id].motionMode=0;boss_after_patterns(ze,&zelda);++counts[zelda.mAction];}
 assert(counts[3]==4&&counts[1]==3&&counts[2]==3);
 // Projectile speed applied once, never to native non-projectile actors.
 reset();Native shooter,arrow;arrow.id=10;arrow.profile=fpcNm_E_ARROW_e;auto& sh=own(shooter,fpcNm_E_RD_e);(void)sh;
 s_enemyArrows[0]={10,shooter.id,false,false};arrow.speedF=100;update_boss_projectile(&arrow,true);update_boss_projectile(&arrow,true);assert(std::abs(arrow.speedF-115)<.01f);
 // Darkhammer's registered attack sphere follows the spinning ball, including
 // native startup's hidden sphere. Other states/colliders and vanilla stay native.
 {reset();Native hammer,ball;auto& hammerOwner=own(hammer,fpcNm_E_TH_e);
 ball.id=10;ball.profile=fpcNm_E_TH_BALL_e;ball.current.pos={200,80,-300};
 ball.mAtSph.actor=&ball;ball.mAtSph.radius=55;s_enemyArrows[0]={10,hammer.id,false,false};
 std::any ballArgs[]={nullptr,static_cast<cCcD_Obj*>(&ball.mAtSph)};
 for(int mode=1;mode<=3;++mode){
  ball.mAction=1;ball.mMode=mode;ball.mAtSph.center={12200,80,-300};
  before_boss_ball_collision(nullptr,ballArgs,nullptr,nullptr);
  assert((ball.mAtSph.center-ball.current.pos).abs()==0&&ball.mAtSph.radius==55);
 }
 for(int action:{0,2,3,10}){
  ball.mAction=action;ball.mAtSph.center={12200,80,-300};
  before_boss_ball_collision(nullptr,ballArgs,nullptr,nullptr);
  assert(ball.mAtSph.center.x==12200);
 }
 ball.mAction=1;
 for(int guard=0;guard<7;++guard){
  arena=playing=true;s_run.phase=Phase::Running;hammerOwner.retiring=hammerOwner.credited=hammerOwner.dormant=false;
  s_enemyArrows[0].retiring=false;
  if(guard==0)arena=false;if(guard==1)playing=false;if(guard==2)s_run.phase=Phase::Upgrade;
  if(guard==3)hammerOwner.retiring=true;if(guard==4)hammerOwner.credited=true;if(guard==5)hammerOwner.dormant=true;
  if(guard==6)s_enemyArrows[0].retiring=true;
  ball.mAtSph.center={12200,80,-300};before_boss_ball_collision(nullptr,ballArgs,nullptr,nullptr);
  assert(ball.mAtSph.center.x==12200);
 }
 s_enemyArrows[0].retiring=false;cCcD_Obj defensive;defensive.actor=&ball;defensive.center={999,0,0};
 ballArgs[1]=&defensive;before_boss_ball_collision(nullptr,ballArgs,nullptr,nullptr);assert(defensive.center.x==999);
 s_enemyArrows={};ballArgs[1]=static_cast<cCcD_Obj*>(&ball.mAtSph);
 before_boss_ball_collision(nullptr,ballArgs,nullptr,nullptr);assert(ball.mAtSph.center.x==12200);
 }
 // Summon caps distinguish the minions and count existing owned summons.
 reset();Native phantom;auto& ph=own(phantom,fpcNm_E_PZ_e);for(int i=0;i<6;++i){assert(boss_pz_minion(ph,fpcNm_E_YK_e));++s_bossTactics[ph.id].summons;}assert(!boss_pz_minion(ph,fpcNm_E_YK_e));
 s_bossTactics[ph.id].summons=0;for(int i=0;i<3;++i){assert(boss_pz_minion(ph,fpcNm_E_S1_e));++s_bossTactics[ph.id].summons;}assert(!boss_pz_minion(ph,fpcNm_E_S1_e));
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
 cpp.write_text(fixture+state+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors boss tactics: watchdog, geometry retries, pause, spawn gate, stagger immunity, attack leases, combos, salves, fixed aim and cleanup boundaries passed')
