"""Production enemy variants, elite health/scale and native enemy-arrow ownership."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
def function(name):
 m=re.search(r'^bool '+name+r'\([^;{}]*\)\s*\{',core,re.M);start=core.index('{',m.start());depth=0
 for end in range(start,len(core)):
  depth+=(core[end]=='{')-(core[end]=='}')
  if not depth:return core[m.start():end+1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <vector>
#include "survivors_rules.hpp"
#include "actor_slot_index.hpp"
using dawnlight::ActorSlotIndex;
#include "survivors_roster.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;using ProfileName=int;using u32=unsigned;using s16=short;using s8=signed char;
unsigned s_map=0;constexpr ActorId kNone=~0u;constexpr int fpcNm_E_OC_e=1,fpcNm_E_BA_e=2,fpcNm_E_MS_e=3,fpcNm_E_RD_e=4,fpcNm_E_ARROW_e=5;
constexpr int fpcNm_E_TT_e=6,fpcNm_E_DN_e=7,fpcNm_E_MM_e=8,fpcNm_E_GE_e=9,fpcNm_E_KR_e=10,fpcNm_E_CR_e=11,fpcNm_E_BS_e=12,fpcNm_E_GI_e=13,fpcNm_E_SF_e=14,fpcNm_E_FS_e=15,fpcNm_E_BU_e=16,fpcNm_E_SM2_e=17,fpcNm_E_MF_e=18,fpcNm_E_CR_EGG_e=19,fpcNm_E_MM_MT_e=20,fpcNm_B_GG_e=21,fpcNm_B_TN_e=22;
constexpr int fpcNm_E_DD_e=23,fpcNm_E_SH_e=24,fpcNm_E_RDY_e=25,fpcNm_E_S1_e=26,fpcNm_E_YK_e=27,fpcNm_E_YC_e=28,fpcNm_B_DS_e=29,fpcNm_E_ZH_e=30;
constexpr int fpcNm_E_RDB_e=31,fpcNm_E_VT_e=32,fpcNm_E_YMB_e=33,fpcNm_E_PZ_e=34,fpcNm_E_TH_e=35,fpcNm_E_HZELDA_e=36,fpcNm_B_GND_e=37,fpcNm_E_TH_BALL_e=38,fpcNm_MANT_e=39,fpcNm_E_FK_e=40;
struct daB_DS_c{enum {TYPE_BULLET_B=3,TYPE_BULLET_C=4};};
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
ModContext* mod_ctx=nullptr;
namespace mods{template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}}
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};
struct csXyz{s16 x,y,z;};
using createFunc=int(*)(void*);
ActorId fopAcM_createChild(s16,ActorId,u32,const cXyz*,int,const csXyz*,const cXyz*,s8,createFunc);
struct fopAc_ac_c{int profile=0;int health=1,field_0x560=1;struct{cXyz pos;}current;cXyz scale{2,2,2};};
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
struct daAlink_c:fopAc_ac_c{};
struct Model{cXyz scale{1,1,1};void setBaseScale(cXyz s){scale=s;}};
struct Morf{Model model;Model* getModel(){return &model;}};
struct Sphere{float radius=0;void SetR(float f){radius=f;}};
struct daE_OC_c:fopAc_ac_c{Model model;Model* getOcModel(){return &model;}Sphere mSphs_cc[2],mSphs_at[2];};
struct e_ba_class{fopAc_ac_c enemy;Morf morph;Morf* mpMorf=&morph;Sphere mSph;};
struct e_ms_class:fopAc_ac_c{Morf morph;Morf* mpModelMorf=&morph;Sphere mSph1,mSph2;};
struct e_rd_class{fopAc_ac_c enemy;Sphere cc_sph[3];};
struct Enemy{ActorId id=kNone;ProfileName profile=0;bool initialized=false,credited=false,retiring=false,elite=false;
 unsigned age=0,endlessMinute=0;int eliteSlot=-1;bool dormant=false;cXyz lootPosition{},spawnPosition{};unsigned species=0,arrowVolley=0;};
std::array<Enemy,enemy_capacity>s_enemies;
ActorSlotIndex<ActorId> s_enemyIndex;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id!=kNone&&e.id==id)return &e;return nullptr;}
Run s_run;bool arenaPresent=true;bool in_arena(){return arenaPresent;}
struct ActorSpawnParams{u32 parameters;int argument,room_num;cXyz position;csXyz angle;cXyz scale;void* create_function;};
struct Arena{int room=0;}map;const Arena& arena(){return map;}
constexpr int MOD_OK=0;bool createFailed=false;ActorId nextId=1;
ProfileName lastProfile;ActorSpawnParams lastParams;
int create_standalone_actor(ProfileName p,const ActorSpawnParams& params,ActorId& id){
 lastProfile=p;lastParams=params;if(createFailed)return 1;id=nextId++;return MOD_OK;}
float cM_ssin(s16 a){return std::sin(a*3.14159265f/32768);}float cM_scos(s16 a){return std::cos(a*3.14159265f/32768);}
bool dryPath=true,lootFloor=true;bool loot_floor_position(cXyz& p){if(!lootFloor)return false;p.y=30;return true;}
bool walkable_spawn_path(cXyz,cXyz& p,float){if(dryPath)p.y=30;return dryPath;}
std::map<ActorId,fopAc_ac_c*>actors;std::set<ActorId>creating,deleted;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
bool fpcM_IsCreating(ActorId id){return creating.contains(id);}
struct Service{void delete_actor(ModContext*,ActorId id){deleted.insert(id);}}service;auto* svc_actor=&service;
'''
main=r'''
std::vector<s16> spreadAngles;
ActorId fopAcM_createChild(s16 profile,ActorId parent,u32 params,const cXyz* pos,int room,const csXyz* angle,const cXyz* scale,s8 arg,createFunc callback){
 if(s_spawningEnemyArrows)assert(profile==fpcNm_E_ARROW_e&&(parent==42||parent==43)&&params==7&&room==0&&
    angle->x==123&&angle->z==456&&scale->x==1&&pos->x==0&&arg==-1&&callback==nullptr);
 std::any args[]={profile,parent,params,pos,room,angle,scale,arg,callback};ActorId id=nextId++;
 if(before_enemy_arrow(nullptr,args,&id,nullptr)==HOOK_SKIP_ORIGINAL)return id;
 spreadAngles.push_back(angle->y);after_enemy_arrow(nullptr,args,&id,nullptr);return id;
}

int main(){
 s_run.start(42);s_run.ticks=18000;daAlink_c player;
 for(s_map=0;s_map<5;++s_map){std::set<unsigned>variants;
  for(unsigned i=0;i<1000;++i){s_enemies={};assert(spawn_enemy(&player,false));
   assert(s_enemyIndex.find(s_enemies,s_enemies[0].id)==&s_enemies[0]);
   variants.insert(s_enemies[0].species);
   assert(s_enemies[0].lootPosition.y==30&&s_enemies[0].spawnPosition.y==lastParams.position.y);
   assert(lastParams.scale.x==1&&lastParams.position.y==30+native_roster[s_enemies[0].species].height);
   if(lastProfile==fpcNm_E_KR_e)assert(lastParams.angle.z==0xff);
   if(lastProfile==fpcNm_E_RDY_e)assert(lastParams.angle.x==0xff&&(lastParams.parameters&0xff)==7);
  }
  assert(variants.size()==3+map_roster(s_map).specialist_count);
  for(unsigned i=0;i<4;++i){s_enemies={};assert(spawn_enemy(&player,true,i));
   const auto kind=map_roster(s_map).elites[i];
   assert(lastProfile==native_roster[unsigned(kind)].profile&&lastParams.scale.x==elite_scale(kind));
   assert(s_enemies[0].eliteSlot==int(i)&&s_enemies[0].species==unsigned(kind));
   if(kind==Species::Helmasaurus)assert(lastParams.argument==1);
  }
 }
 // If a large-chest footprint fails, retain the original validated spawn floor.
 lootFloor=false;
 for(unsigned kind=0;kind<species_count;++kind){s_enemies={};assert(create_enemy_at({100,30+native_roster[kind].height,200},0,kind,true));
  assert(s_enemies[0].lootPosition.y==30&&s_enemies[0].spawnPosition.y==30+native_roster[kind].height);}
 lootFloor=true;
 const int profiles[]={fpcNm_E_MS_e,fpcNm_E_BA_e,fpcNm_E_RD_e,fpcNm_E_OC_e};
 s_map=0;
 s_enemies={};dryPath=false;assert(!spawn_enemy(&player,true,0)&&s_enemies[0].id==kNone);dryPath=true;
 createFailed=true;assert(!spawn_enemy(&player,false)&&s_enemies[0].id==kNone);createFailed=false;
 assert(!create_enemy_at({},0,species_count,false));
 s_run.ticks=900;
 // Elite HP applies to every species, once, including difficulty; sizes do not accumulate.
 s_run.options.enemy_hp=10;
 daE_OC_c bok;e_ba_class bat;e_ms_class rat;e_rd_class bulblin;
 fopAc_ac_c* bases[]={&rat,&bat.enemy,&bulblin.enemy,&bok};
 for(unsigned i=0;i<4;++i){Enemy e;e.elite=true;e.profile=profiles[i];initialize_enemy(&e,bases[i]);
  assert(e.initialized&&bases[i]->health==1200&&bases[i]->field_0x560==1200);
  bases[i]->health=800;initialize_enemy(&e,bases[i]);assert(bases[i]->health==800);
  for(unsigned n=0;n<3;++n)scale_elite_colliders(&e,bases[i]);}
 assert(bok.model.scale.x==2&&rat.morph.model.scale.x==2&&bat.morph.model.scale.x==2);
 assert(rat.mSph1.radius==70&&rat.mSph2.radius==40&&bat.mSph.radius==60&&bulblin.cc_sph[2].radius==70);
 Enemy regular;regular.profile=fpcNm_E_RD_e;initialize_enemy(&regular,&bulblin.enemy);assert(bulblin.enemy.health==80);
 // Every original species scales, and previously spawned/injured actors keep
 // both their remaining HP and original max HP as the run advances.
 s_run.options.enemy_hp=1;
 fopAc_ac_c* speciesActors[]={&bok,&bat.enemy,&rat,&bulblin.enemy,&bulblin.enemy};
 for(unsigned kind=0;kind<5;++kind){
  s_run.ticks=duration;Enemy e;e.species=kind;e.profile=native_roster[kind].profile;
  auto* actor=speciesActors[kind];initialize_enemy(&e,actor);
  const int maximum=int(s_run.enemy_health(native_roster[kind].hp,false,s_map));assert(actor->health==maximum&&actor->field_0x560==maximum);
  s_run.ticks+=18000;initialize_enemy(&e,actor);assert(actor->health==maximum);
  actor->health=maximum-1;initialize_enemy(&e,actor);
  assert(actor->health==maximum-1&&actor->field_0x560==maximum);
 }
 // Untouched waiting guards remain relevant on discovery, then stop growing
 // on damage or activation. Repeated updates never compound HP or heal wounds.
 s_run.ticks=0;Enemy guard;guard.profile=fpcNm_E_OC_e;guard.elite=guard.dormant=true;
 initialize_enemy(&guard,&bok);assert(bok.health==120);
 s_run.ticks=10*60*ticks_per_second;
 for(unsigned i=0;i<3;++i)initialize_enemy(&guard,&bok);
 assert(bok.health==344&&bok.field_0x560==344);
 bok.health=300;s_run.ticks=duration;initialize_enemy(&guard,&bok);
 assert(bok.health==300&&bok.field_0x560==344);
 guard.dormant=false;bok.health=bok.field_0x560;initialize_enemy(&guard,&bok);
 assert(bok.health==344&&bok.field_0x560==344);
 guard={};guard.profile=fpcNm_E_OC_e;guard.elite=guard.dormant=true;
 initialize_enemy(&guard,&bok);assert(bok.health==905); // late respawn
 // Existing injured enemies scale once per Endless minute, preserving their HP fraction.
 s_run.phase=Phase::Victory;assert(s_run.continue_endless());
 Enemy endlessEnemy;endlessEnemy.profile=fpcNm_E_OC_e;
 initialize_enemy(&endlessEnemy,&bok);const int initialHp=bok.field_0x560;
 bok.health=initialHp/2;s_run.ticks+=1800;initialize_enemy(&endlessEnemy,&bok);
 assert(bok.field_0x560==int(std::ceil(initialHp*1.1)));
 const auto scaledHp=bok.health;initialize_enemy(&endlessEnemy,&bok);assert(bok.health==scaledHp);
 bok.health=0;s_run.ticks+=1800;initialize_enemy(&endlessEnemy,&bok);assert(bok.health==0);
 s_run.endless=false;
 // Native arrows are not enemies/loot sources. They remain owned after shooter deletion.
 s_enemies={};s_enemies[0].id=42;s_enemies[0].profile=fpcNm_E_RD_e;
 std::any args[]={s16(fpcNm_E_ARROW_e),ActorId(42),u32(0),static_cast<const cXyz*>(nullptr),0,static_cast<const csXyz*>(nullptr)};ActorId arrowId=100;
 assert(before_enemy_arrow(nullptr,args,&arrowId,nullptr)==HOOK_CONTINUE);
 after_enemy_arrow(nullptr,args,&arrowId,nullptr);assert(active_enemy_arrow(100));
 after_enemy_arrow(nullptr,args,&arrowId,nullptr); // no duplicate entries
 s_enemies[0]={};assert(active_enemy_arrow(100));creating.insert(100);retire_enemy_arrows();cleanup_enemy_arrows();
 assert(tracked_enemy_arrow(100)&&!active_enemy_arrow(100)&&deleted.empty());
 creating.clear();actors[100]=&player;cleanup_enemy_arrows();assert(deleted.contains(100));
 actors.clear();cleanup_enemy_arrows();assert(!tracked_enemy_arrow(100));
 // Bomskit eggs survive their owner; armor shells retire immediately with it.
 for(auto spec:{std::pair{fpcNm_E_CR_e,fpcNm_E_CR_EGG_e},std::pair{fpcNm_E_MM_e,fpcNm_E_MM_MT_e}}){
  s_enemyArrows={};s_enemies={};s_enemies[0].id=42;s_enemies[0].profile=spec.first;
  std::any childArgs[]={s16(spec.second),ActorId(42)};ActorId child=200;
  assert(before_enemy_arrow(nullptr,childArgs,&child,nullptr)==HOOK_CONTINUE);
  after_enemy_arrow(nullptr,childArgs,&child,nullptr);assert(active_enemy_arrow(200));
  s_enemies[0]={};assert(active_enemy_arrow(200)==(spec.first==fpcNm_E_CR_e));
  actors[200]=&player;retire_enemy_arrows();cleanup_enemy_arrows();assert(deleted.contains(200));
  actors.clear();cleanup_enemy_arrows();assert(!tracked_enemy_arrow(200));
 }
 // Pending armor creation may complete after a level-up pauses the run.
 s_enemyArrows={};s_enemies={};s_enemies[0].id=42;s_enemies[0].profile=fpcNm_E_MM_e;
 std::any armorArgs[]={s16(fpcNm_E_MM_MT_e),ActorId(42)};ActorId armorId=300;
 for(auto phase:{Phase::Upgrade,Phase::Treasure}){
  s_run.phase=phase;assert(before_enemy_arrow(nullptr,armorArgs,&armorId,nullptr)==HOOK_CONTINUE);
 }
 after_enemy_arrow(nullptr,armorArgs,&armorId,nullptr);assert(active_enemy_arrow(300));
 s_run.phase=Phase::Running;
 // Projectile saturation leaves room for structural shells.
 s_enemyArrows={};for(unsigned i=0;i<enemy_capacity*3;++i)s_enemyArrows[i].id=1000+i;
 s_enemies[1].id=43;s_enemies[1].profile=fpcNm_E_RD_e;
 std::any shotArgs[]={s16(fpcNm_E_ARROW_e),ActorId(43)};ActorId shot=301;
 assert(before_enemy_arrow(nullptr,shotArgs,&shot,nullptr)==HOOK_SKIP_ORIGINAL&&shot==kNone);
 assert(before_enemy_arrow(nullptr,armorArgs,&armorId,nullptr)==HOOK_CONTINUE);
 s_enemies[0].retiring=true;assert(before_enemy_arrow(nullptr,armorArgs,&armorId,nullptr)==HOOK_SKIP_ORIGINAL);
 s_enemies[0].retiring=false;s_enemies[0].credited=true;armorId=300;
 assert(before_enemy_arrow(nullptr,armorArgs,&armorId,nullptr)==HOOK_SKIP_ORIGINAL);
 // Shadow arrows and Stallord fireballs share pause/teardown ownership.
 for(auto spec:{std::pair{fpcNm_E_RDY_e,fpcNm_E_ARROW_e},std::pair{fpcNm_B_DS_e,fpcNm_B_DS_e}}){
  s_enemyArrows={};s_enemies={};s_enemies[0].id=42;s_enemies[0].profile=spec.first;
  std::any childArgs[]={s16(spec.second),ActorId(42),u32(daB_DS_c::TYPE_BULLET_B),static_cast<const cXyz*>(nullptr),0,static_cast<const csXyz*>(nullptr)};ActorId child=250;
  assert(before_enemy_arrow(nullptr,childArgs,&child,nullptr)==HOOK_CONTINUE);
  after_enemy_arrow(nullptr,childArgs,&child,nullptr);assert(active_enemy_arrow(child));
  s_enemies[0]={};assert(active_enemy_arrow(child));
  retire_enemy_arrows();creating.insert(child);cleanup_enemy_arrows();assert(tracked_enemy_arrow(child)&&!active_enemy_arrow(child));
  creating.clear();actors[child]=&player;cleanup_enemy_arrows();assert(deleted.contains(child));
  actors.clear();cleanup_enemy_arrows();assert(!tracked_enemy_arrow(child));
 }
 // Boss weapons/cloaks finish creating through pauses; portal attacks do not.
 for(auto spec:{std::pair{fpcNm_E_TH_e,fpcNm_E_TH_BALL_e},std::pair{fpcNm_B_GND_e,fpcNm_MANT_e},std::pair{fpcNm_E_PZ_e,fpcNm_E_PZ_e}}){
  s_enemyArrows={};s_enemies={};s_enemies[0].id=42;s_enemies[0].profile=spec.first;
  std::any childArgs[]={s16(spec.second),ActorId(42)};ActorId child=401;
  s_run.phase=Phase::Upgrade;
  assert(before_enemy_arrow(nullptr,childArgs,&child,nullptr)==(spec.first==fpcNm_E_PZ_e?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE));
  s_run.phase=Phase::Running;child=401;
  assert(before_enemy_arrow(nullptr,childArgs,&child,nullptr)==HOOK_CONTINUE);
  after_enemy_arrow(nullptr,childArgs,&child,nullptr);assert(active_enemy_arrow(child));
  s_enemies[0].credited=true;assert(!active_enemy_arrow(child));
  actors[child]=&player;cleanup_enemy_arrows();assert(deleted.contains(child));
  actors.clear();cleanup_enemy_arrows();assert(!tracked_enemy_arrow(child));
 }
 // Native phase-two breath survives its head, then retires on run cleanup.
 s_enemyArrows={};s_enemies={};s_enemies[0].id=42;s_enemies[0].profile=fpcNm_B_DS_e;
 std::any breathArgs[]={s16(fpcNm_B_DS_e),ActorId(42),u32(daB_DS_c::TYPE_BULLET_C)};ActorId breath=402;
 assert(before_enemy_arrow(nullptr,breathArgs,&breath,nullptr)==HOOK_CONTINUE);
 after_enemy_arrow(nullptr,breathArgs,&breath,nullptr);s_enemies={};assert(active_enemy_arrow(breath));
 retire_enemy_arrows();assert(!active_enemy_arrow(breath));s_enemyArrows={};
 // Explicit factors replace, rather than multiply, promoted-elite damage.
 for(auto spec:{std::pair{fpcNm_E_DN_e,2u},std::pair{fpcNm_E_RDB_e,1u},std::pair{fpcNm_B_TN_e,2u},
     std::pair{fpcNm_E_VT_e,4u},std::pair{fpcNm_B_DS_e,4u},std::pair{fpcNm_E_YC_e,4u},
     std::pair{fpcNm_E_YMB_e,3u},std::pair{fpcNm_E_TH_e,3u},std::pair{fpcNm_E_HZELDA_e,4u},
     std::pair{fpcNm_B_GND_e,5u},std::pair{fpcNm_E_PZ_e,5u}}){
  Enemy e;e.elite=true;e.profile=spec.first;assert(enemy_attack_factor(e)==spec.second);
  if(!native_boss_profile(e.profile)){e.elite=false;assert(enemy_attack_factor(e)==1);}
 }
 // Rider arrows belong to the carrier, keep its factor after death, and never
 // inherit a reused enemy slot. Attached riders/ghosts retire with their root.
 s_enemyArrows={};s_enemies={};auto& carrier=s_enemies[0];carrier.id=42;carrier.profile=fpcNm_E_YC_e;carrier.elite=true;
 std::any riderArgs[]={s16(fpcNm_E_RDY_e),ActorId(42)};ActorId rider=500;
 after_enemy_arrow(nullptr,riderArgs,&rider,nullptr);assert(enemy_damage_factor(rider)==4);
 fopAc_ac_c riderActor;riderActor.profile=fpcNm_E_RDY_e;actors[rider]=&riderActor;
 std::any mountedShot[]={s16(fpcNm_E_ARROW_e),ActorId(rider),u32(0),static_cast<const cXyz*>(nullptr),0,static_cast<const csXyz*>(nullptr)};ActorId flyingArrow=501;
 assert(before_enemy_arrow(nullptr,mountedShot,&flyingArrow,nullptr)==HOOK_CONTINUE);
 after_enemy_arrow(nullptr,mountedShot,&flyingArrow,nullptr);assert(enemy_damage_factor(flyingArrow)==4&&tracked_enemy_arrow(flyingArrow)->parent==42);
 carrier={};carrier.id=43;carrier.profile=fpcNm_E_OC_e;
 assert(!active_enemy_arrow(rider)&&active_enemy_arrow(flyingArrow)&&enemy_damage_factor(flyingArrow)==4);
 ActorId rejected=502;assert(before_enemy_arrow(nullptr,mountedShot,&rejected,nullptr)==HOOK_CONTINUE); // retired rider cannot execute/create
 retire_enemy_arrows();assert(!active_enemy_arrow(flyingArrow));s_enemyArrows={};actors.clear();s_enemies={};
 s_enemies[0].id=42;s_enemies[0].profile=fpcNm_B_GND_e;
 std::any ghostArgs[]={s16(fpcNm_E_FK_e),ActorId(42)};ActorId ghost=503;
 s_run.phase=Phase::Upgrade;assert(before_enemy_arrow(nullptr,ghostArgs,&ghost,nullptr)==HOOK_SKIP_ORIGINAL&&ghost==kNone);
 s_run.phase=Phase::Running;ghost=503;after_enemy_arrow(nullptr,ghostArgs,&ghost,nullptr);
 assert(active_enemy_arrow(ghost)&&enemy_damage_factor(ghost)==5);
 s_enemies[0].credited=true;assert(!active_enemy_arrow(ghost));creating.insert(ghost);cleanup_enemy_arrows();
 assert(tracked_enemy_arrow(ghost));creating.clear();actors[ghost]=&player;cleanup_enemy_arrows();assert(deleted.contains(ghost));
 actors.clear();cleanup_enemy_arrows();assert(!tracked_enemy_arrow(ghost));s_enemies={};
 // Normal archers fire single, single, triple; Endless keeps every shot triple.
 // Recursive side arrows do not advance the original-shot counter.
 for(int profile:{fpcNm_E_RD_e,fpcNm_E_RDY_e,fpcNm_E_OC_e})for(bool endless:{false,true}){
  s_enemyArrows={};s_enemies={};actors.clear();spreadAngles.clear();
  s_run.phase=Phase::Running;s_run.endless=endless;s_enemies[0].id=42;s_enemies[0].profile=profile;
  cXyz pos{},scale{1,1,1};csXyz angle{123,1000,456};
  const bool archer=profile!=fpcNm_E_OC_e;unsigned total=0;
  for(unsigned shot=1;shot<=6;++shot){
   const auto before=spreadAngles.size();
   const auto id=fopAcM_createChild(s16(fpcNm_E_ARROW_e),42,7,&pos,0,&angle,&scale,s8(-1),nullptr);
   const unsigned count=archer&&(endless||shot%3==0)?3:1;total+=count;
   assert(spreadAngles.size()==total&&angle.y==1000&&!s_spawningEnemyArrows);
   if(count==3){assert(spreadAngles[before+1]==1000-0x900&&spreadAngles[before+2]==1000+0x900);}
   if(archer){assert(active_enemy_arrow(id));assert(s_enemies[0].arrowVolley==shot%3);}
  }
  if(archer){unsigned tracked=0;for(const auto& a:s_enemyArrows)if(a.id!=kNone){++tracked;assert(a.parent==42);}assert(tracked==total);}
 }
 s_run.endless=false;s_enemyArrows={};s_enemies={};actors.clear();spreadAngles.clear();
 // Cadence belongs to each shooter; rejected/failed shots do not advance it.
 s_enemies[0].id=42;s_enemies[0].profile=fpcNm_E_RD_e;
 s_enemies[1].id=43;s_enemies[1].profile=fpcNm_E_RDY_e;
 cXyz pos{},scale{1,1,1};csXyz angle{123,1000,456};
 auto shoot=[&](ActorId parent){return fopAcM_createChild(s16(fpcNm_E_ARROW_e),parent,7,&pos,0,&angle,&scale,s8(-1),nullptr);};
 shoot(42);shoot(43);shoot(42);assert(spreadAngles.size()==3);
 s_run.phase=Phase::Upgrade;assert(shoot(42)==kNone&&s_enemies[0].arrowVolley==2);
 s_run.phase=Phase::Running;ActorId failed=kNone;
 after_enemy_arrow(nullptr,args,&failed,nullptr);assert(s_enemies[0].arrowVolley==2);
 shoot(42);assert(spreadAngles.size()==6&&s_enemies[0].arrowVolley==0);
 shoot(43);assert(spreadAngles.size()==7&&s_enemies[1].arrowVolley==2);
 // Actor-slot reuse starts at the first shot, not a previous archer's third.
 s_enemies[1]={};s_enemies[1].id=43;s_enemies[1].profile=fpcNm_E_RDY_e;
 shoot(43);assert(spreadAngles.size()==8&&s_enemies[1].arrowVolley==1);
 s_run.endless=false;s_enemyArrows={};s_enemies={};actors.clear();
 // Foreign children and failed creates are ignored; capacity/pause reject new owned arrows.
 arrowId=101;after_enemy_arrow(nullptr,args,&arrowId,nullptr);assert(!tracked_enemy_arrow(101));
 s_enemies[0].id=42;s_enemies[0].profile=fpcNm_E_RD_e;
 arrowId=kNone;after_enemy_arrow(nullptr,args,&arrowId,nullptr);assert(!tracked_enemy_arrow(kNone));
 s_run.phase=Phase::Upgrade;assert(before_enemy_arrow(nullptr,args,&arrowId,nullptr)==HOOK_SKIP_ORIGINAL&&arrowId==kNone);
 s_run.phase=Phase::Running;for(auto& a:s_enemyArrows)a.id=nextId++;
 assert(before_enemy_arrow(nullptr,args,&arrowId,nullptr)==HOOK_SKIP_ORIGINAL);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 native=(root/'src/survivors_roster_native.inc').read_text().split('// Suppress only')[0]
 cpp.write_text(fixture+native+'void initialize_extended_enemy(Enemy&,fopAc_ac_c*){assert(false);}' +(root/'src/survivors_enemy_types.inc').read_text()+(root/'src/survivors_enemy_balance.inc').read_text().split('// Add one decrement')[0]+(root/'src/survivors_enemy_arrows.inc').read_text()+
  function('create_enemy_at')+function('spawn_enemy')+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors enemies: map-specific variants, five elite sequences, dry placement, HP/scale, projectile ownership/caps and deferred cleanup passed')
