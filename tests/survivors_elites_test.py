"""Production elite schedule, authored placements, dormant AI and one-shot loot."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
runtime=(root/'src/survivors.cpp').read_text()
loot=(root/'src/survivors_loot.inc').read_text()
def extract(source,name):
    m=re.search(r'^(?:void|bool|unsigned|HookAction) '+name+r'\(',source,re.M)
    assert m,name
    start=source.index('{',m.start());depth=0
    for end in range(start,len(source)):
        depth+=(source[end]=='{')-(source[end]=='}')
        if not depth:return source[m.start():end+1]
fixture=r'''
namespace enemy_hud {void tick(){}}

bool s_victory=false;unsigned s_victoryTicks=0;void update_victory(){++s_victoryTicks;}

#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <iostream>
#include "survivors_rules.hpp"
#include "../include/survivors_maps.hpp"
#include "survivors_roster.hpp"
using namespace dawnlight::survivors;
using s16=short;using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float n)const{return{x*n,y*n,z*n};}
 float abs2(cXyz b)const{auto d=*this-b;return d.x*d.x+d.y*d.y+d.z*d.z;}
 float absXZ(cXyz b)const{auto d=*this-b;return std::sqrt(d.x*d.x+d.z*d.z);}};
struct fopAc_ac_c{struct{cXyz pos;}current;struct{s16 y=0;}shape_angle;int health=120;};
struct daAlink_c:fopAc_ac_c{ActorId id=1;}player;
ActorId fopAcM_GetID(daAlink_c* actor){return actor->id;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
struct daE_OC_c:fopAc_ac_c{ActorId id=0;float speedF=0;cXyz speed;unsigned animations=0;
 void setBck(int id,unsigned type,float,float){assert(id==0x18&&type==2);++animations;}
};
'''
fixture+=r'''
struct Enemy{ActorId id=kNone;int eliteSlot=-1;bool elite=false,dormant=false,retiring=false,credited=false,initialized=true,idleAnimation=false;unsigned age=0;cXyz lootPosition;};
std::array<Enemy,enemy_capacity>s_enemies;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id==id)return &e;return nullptr;}
ActorId fopAcM_GetID(daE_OC_c* a){return a->id;}
int dComIfGs_getMaxLife(){return 15;}
bool s_arenaSpawn=true; cXyz s_arenaSpawnPosition{};
Run s_run;unsigned s_map=0,s_spawnCooldown=0,s_spawnFailures=0;
const Arena& arena(){return arenas[s_map];}
bool arenaPresent=true;bool in_arena(){return arenaPresent;}
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int i){return std::any_cast<T>(static_cast<std::any*>(args)[i]);}}
struct fopAcM_prm_class{struct{unsigned parameters=0;cXyz position;struct{s16 y=0;}angle;}base;int room_no=51;};
constexpr int fpcNm_DOOR20_e=3,fpcNm_DBDOOR_e=4,fpcNm_BOSS_DOOR_e=5,fpcNm_L1BOSS_DOOR_e=6;
struct stage_tgsc_data_class{char name[8]{};decltype(fopAcM_prm_class::base) base;};
struct stage_tgsc_class{int num=0;stage_tgsc_data_class* m_entries=nullptr;};
struct RoomData{stage_tgsc_class* doors=nullptr;
 auto* getDrTg(){return doors;}auto* getDoor(){return doors;}}roomData,stageData;
RoomData* dComIfGp_roomControl_getStatusRoomDt(int){return &roomData;}
RoomData* dComIfGp_getStage(){return &stageData;}
struct dStage_KeepDoorInfo{int mNum=0;stage_tgsc_data_class mDrTgData[64];}keepDoors,keepRoomDoors;
auto* dStage_GetKeepDoorInfo(){return &keepDoors;}auto* dStage_GetRoomKeepDoorInfo(){return &keepRoomDoors;}
struct NameInfo{int procname;};
NameInfo* dStage_searchName(const char* name){static NameInfo doorInfo{fpcNm_DOOR20_e};
 return std::strcmp(name,"door")==0?&doorInfo:nullptr;}
float cM_ssin(s16 a){return std::sin(float(a)*3.14159265f/32768.f);}float cM_scos(s16 a){return std::cos(float(a)*3.14159265f/32768.f);}
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct dBgS_LinChk{void Set(const cXyz*,const cXyz*,void*){}};
bool groundMissing=false,wallBlocked=false,barrierBlocked=false;int groundRoom=-2,groundCode=0;
struct Bg{float GroundCross(dBgS_ObjGndChk*){return groundMissing ? -1.e9f : 0.f;}
 int GetRoomId(dBgS_ObjGndChk&){return groundRoom==-2?arena().room:groundRoom;}
 int GetGroundCode(dBgS_ObjGndChk&){return groundCode;}
 bool LineCross(dBgS_LinChk*){return wallBlocked;}}bg;
Bg& dComIfG_Bgsp(){return bg;}
bool arena_path_clear(cXyz,cXyz,float){return !barrierBlocked;}
bool dry_arena_floor(cXyz& p,float rise,float drop){if(p.y>drop||p.y < -rise||groundMissing||groundCode==4||groundCode==5||groundCode==10||
 (groundRoom!=-2&&groundRoom!=arena().room))return false;
 p.y=0;return true;}
bool needs_reachable_floor(){return false;}bool sample_reachable_arena_floor(cXyz&){return false;}
bool reachable=true;bool reachable_arena_floor(cXyz){return reachable;}void update_reachable_floor(){}
bool rejectLongPath=false,narrowLanding=false;
bool walkable_spawn_path(cXyz from,cXyz& to,float){return !groundMissing&&!wallBlocked&&!barrierBlocked &&
 (!rejectLongPath||from.absXZ(to)<1000) && (!narrowLanding||std::abs(to.x-from.x)<300); }
bool failCreate=false;unsigned nextId=1,ordinarySpawns=0;
ActorId distantId=kNone;fopAc_ac_c distantActor;
fopAc_ac_c* actor_by_id(ActorId id){return id==distantId?&distantActor:&player;}
bool create_enemy_at(cXyz pos,s16,unsigned kind,bool elite,int slot=-1,bool dormant=false){
 if(dormant)assert(kind==unsigned(guard_species(s_map)));
 if(failCreate)return false;
 for(auto& e:s_enemies)if(e.id==kNone){e={};e.id=nextId++;e.elite=elite;e.eliteSlot=slot;e.dormant=dormant;e.lootPosition=pos;return true;}
 return false;
}
bool spawn_enemy(daAlink_c*,bool elite,int slot=-1){if(!elite)++ordinarySpawns;return create_enemy_at({0,0,500},0,0,elite,slot);}
constexpr unsigned kPaidChestCount=2,kSmallChestCount=kPaidChestCount,kChestCount=2+elite_reward_capacity;
struct Chest{bool active=false,free=false,fixed=false,opening=false;s16 yaw=0;cXyz position;uint64_t spawnOrder=0;};
std::array<Chest,kChestCount>s_chests;
bool s_chestsExhausted=false;int potion_slot(){return -1;}
unsigned s_selectedChest=kChestCount;uint64_t s_chestSequence=0;
bool s_active=true,paused=false;ActorId s_player=1;int life=12;unsigned finishes=0,lifeWrites=0;
bool playable(){return arenaPresent&&!paused;}int dComIfGs_getLife(){return life;}
int dComIfGs_getMaxLifeGauge(){return 12;}
void dComIfGs_setLife(unsigned value){life=int(value);++lifeWrites;}
void finish(bool victory){++finishes;s_run.end(victory);s_victory=false;}void begin_defeat(){s_run.end(false);}
unsigned rupees=0,rupeeWrites=0;
unsigned dComIfGs_getRupee(){return rupees;}unsigned dComIfGs_getRupeeMax(){return 300;}
void dComIfGs_setRupee(unsigned value){rupees=value;++rupeeWrites;}
void update_combat_feedback(){}
void update_enemy_effects(){}void update_weapons(daAlink_c*){}void update_orbs(daAlink_c*){} void update_exp_magnets(daAlink_c*){}
void clear_attacks(){}void update_chests(daAlink_c*){}void update_pots(daAlink_c*){}
void update_palace_ferries(){}
void update_horde_portals(daAlink_c*){}

'''
fixture+='void update_boss_watchdogs(daAlink_c*){}\n'+extract((root/'src/survivors_boss_tactics_state.inc').read_text(),'timed_boss')+extract((root/'src/survivors_boss_tactics_state.inc').read_text(),'active_boss_encounter')
main=r'''
void fresh(unsigned map){distantId=kNone;s_victory=false;s_victoryTicks=0;s_arenaSpawnPosition={};s_arenaSpawn=true;rejectLongPath=narrowLanding=false;s_map=map;roomData={};stageData={};keepDoors={};keepRoomDoors={};s_run.start(42);s_enemies={};s_chests={};s_chestsExhausted=false;s_chestSequence=0;s_selectedChest=kChestCount;clear_elite_sites();player.current.pos={};reset_elite_encounters();
 groundMissing=wallBlocked=barrierBlocked=failCreate=false;groundRoom=-2;groundCode=0;s_spawnCooldown=s_spawnFailures=ordinarySpawns=0;}
unsigned guards(){unsigned n=0;for(auto& e:s_enemies)n+=e.id!=kNone&&e.dormant;return n;}
unsigned waves(){unsigned n=0;for(auto& e:s_enemies)n+=e.id!=kNone&&e.elite&&!e.dormant;return n;}
void door(float x,float z){fopAcM_prm_class p;p.room_no=arena().room;p.base.parameters=unsigned(arena().room)<<13;p.base.position={x,0,z};capture_elite_site(fpcNm_DOOR20_e,p);}
void add_floor(float size){capture_elite_floor({-size,0,-size},{size,0,-size},{-size,0,size},1,false);capture_elite_floor({size,0,-size},{size,0,size},{-size,0,size},1,false);}
int main(){
 fresh(0);s_run.banned_items.fill(true);s_chests[0].active=true;
 Enemy exhausted;exhausted.elite=true;exhausted.eliteSlot=0;
 drop_elite_chest(exhausted,&player);assert(s_chestsExhausted&&s_eliteEncounters[0].defeated);
 for(const auto& c:s_chests)assert(!c.active);
 s_run.banned_items.fill(false);exhausted.eliteSlot=1;drop_elite_chest(exhausted,&player);
 for(const auto& c:s_chests)assert(!c.active);

 // Actual encounter scheduler follows the selected timer, including the final boss.
 for(unsigned minutes:{5u,10u,20u}){
  fresh(0);s_run.options.debug.run_minutes=minutes;
  for(unsigned boss=1;boss<=4;++boss){
   s_run.ticks=boss*s_run.boss_interval()-1;update_elite_spawns(&player);assert(waves()==boss-1);
   s_run.ticks=boss*s_run.boss_interval();update_elite_spawns(&player);assert(waves()==boss);
  }
 }

 // Endless repeats the four map bosses every five minutes, including debug runs.
 for(unsigned minutes:{5u,20u}){
  fresh(0);s_run.options.debug.run_minutes=minutes;s_run.ticks=s_run.run_duration();
  s_run.phase=Phase::Victory;assert(s_run.continue_endless());
  for(unsigned i=0;i<4;++i)s_eliteEncounters[i].respawnAt=s_run.endless_start+(i+1)*elite_interval;
  for(unsigned n=0;n<12;++n){
   s_run.ticks=s_run.endless_start+(n+1)*elite_interval-1;update_elite_spawns(&player);assert(waves()==0);
   ++s_run.ticks;update_elite_spawns(&player);assert(waves()==1);
   for(auto& enemy:s_enemies)if(enemy.id!=kNone){
    assert(enemy.eliteSlot==int(n%4));drop_elite_chest(enemy,&player);enemy={};
   }
   const auto count=s_chestSequence;update_elite_spawns(&player);assert(waves()==0&&s_chestSequence==count);
  }
  s_spawnCooldown=0;update_spawns(&player);assert(ordinarySpawns>0);
 }
 // Compact arenas have no map guards, even with captured doors or large floor.
 for(unsigned map:{0u,4u}){fresh(map);add_floor(10000);door(2000,0);assert(s_eliteDoorSites.size()==1);
  for(unsigned minute=0;minute<20;++minute){s_run.ticks=minute*60*30;update_elite_spawns(&player);
   assert(guards()==0&&waves()==minute/5);}
 }
 // Unaccepted creation retries; deletion is not a defeat and grants no chest.
 fresh(0);s_run.ticks=elite_interval;failCreate=true;update_elite_spawns(&player);assert(waves()==0);
 failCreate=false;update_elite_spawns(&player);assert(waves()==1);s_enemies[0]={};update_elite_spawns(&player);assert(waves()==1);
 for(const auto& c:s_chests)assert(!c.active);
 auto killed=s_enemies[0];drop_elite_chest(killed,&player);assert(s_eliteEncounters[0].defeated&&s_chests[2].active&&s_chests[2].free);
 assert(!s_chests[0].active&&!s_chests[1].active);s_chests[2]={};drop_elite_chest(killed,&player);assert(!s_chests[2].active);
 s_enemies[0]={};update_elite_spawns(&player);assert(waves()==0);
 // Flying corpses project deeply to a validated floor, unlike spawn placement.
 for(float height:{300.f,1500.f,12000.f}) {
  fresh(0);Enemy flyer;flyer.elite=true;flyer.eliteSlot=0;flyer.lootPosition={100,0,200};
  player.current.pos={500,height,600};auto landing=player.current.pos;
  assert(!elite_floor_position(landing));assert(loot_floor_position(landing)&&landing.y==0);
  drop_elite_chest(flyer,&player);assert(s_chests[2].active&&s_chests[2].position.y==0);
  assert(s_chests[2].position.x==500&&s_chests[2].position.z==600);
 }
 s_chests[2]={};
 for(unsigned map:{1u,2u}) {
  fresh(map);add_floor(10000);reachable=false;update_elite_spawns(&player);assert(guards()==0);
  reachable=true;update_elite_spawns(&player);assert(guards()==4);
 }
 // Missing corpse ground falls back to its last validated floor.
 killed.eliteSlot=1;killed.lootPosition={100,0,200};groundMissing=true;drop_elite_chest(killed,&player);
 assert(s_chests[2].position.x==100&&s_chests[2].position.y==0&&s_chests[2].position.z==200);groundMissing=false;
 // A full large-chest pool evicts its oldest, never a small/selected/open chest.
 fresh(0);s_chests[0].active=true;s_chests[0].fixed=true;s_chests[0].position={99,0,99};
 for(unsigned i=kSmallChestCount;i<kChestCount;++i) {
  s_chests[i].active=true;s_chests[i].free=true;s_chests[i].spawnOrder=++s_chestSequence;
 }
 s_chests[kSmallChestCount].opening=true;s_selectedChest=kSmallChestCount+1;
 Enemy replacement;replacement.eliteSlot=0;player.current.pos={800,0,900};
 const auto oldSequence=s_chestSequence;drop_elite_chest(replacement,&player);
 assert(s_eliteEncounters[0].rewarded&&s_chests[kSmallChestCount+2].spawnOrder==oldSequence+1);
 assert(s_chests[kSmallChestCount].spawnOrder==1&&s_chests[kSmallChestCount+1].spawnOrder==2);
 assert(s_chests[0].fixed&&s_chests[0].position.x==99&&s_chests[0].spawnOrder==0);
 drop_elite_chest(replacement,&player);assert(s_chestSequence==oldSequence+1); // One reward per kill.
 // Exact milestone bypasses ordinary spawn cooldown, ordinary horde continues.
 fresh(0);s_run.ticks=elite_interval;s_spawnCooldown=30;update_spawns(&player);assert(waves()==1&&ordinarySpawns==0);
 for(int n=0;n<10;++n){s_spawnCooldown=0;update_spawns(&player);assert(ordinarySpawns==0);}
 // Killing the timed boss resumes waves; already-spawned enemies survive the gate.
 for(auto& e:s_enemies)if(timed_boss(e)){drop_elite_chest(e,&player);e.credited=true;}
 s_spawnCooldown=0;update_spawns(&player);assert(ordinarySpawns==1);
 // Spawn warning counts consecutive failures and clears after recovery/cap/end.
 fresh(0);failCreate=true;
 for(unsigned i=0;i<10;++i){s_spawnCooldown=0;update_spawns(&player);}assert(s_spawnFailures==10);
 failCreate=false;s_spawnCooldown=0;update_spawns(&player);assert(s_spawnFailures==0);
 s_spawnFailures=10;s_run.ticks=duration;s_spawnCooldown=0;update_spawns(&player);assert(s_spawnFailures==0);
 // Moving to the rear must release slots held by distant ordinary enemies.
 fresh(0);for(unsigned n=0;n<s_run.target_population();++n)assert(create_enemy_at({},0,0,false));
 distantId=s_enemies[0].id;distantActor.current.pos={7000,0,0};
 s_enemies[0].age=100;update_spawns(&player);assert(!s_enemies[0].retiring&&ordinarySpawns==0);
 s_enemies[0].age=10*ticks_per_second;s_spawnCooldown=0;update_spawns(&player);
 assert(s_enemies[0].retiring&&ordinarySpawns==1&&s_run.kills==0);
 for(const auto& chest:s_chests)assert(!chest.active); // Cleanup must not grant rewards.
 fresh(0);assert(create_enemy_at({},0,0,true,0,true));distantId=s_enemies[0].id;
 s_enemies[0].age=6000;update_spawns(&player);assert(!s_enemies[0].retiring); // Stationary bosses persist.
 // Burst spawns are tripled but respect the current normal-enemy cap.
 fresh(0);s_run.horde_remaining=horde_duration;s_spawnCooldown=0;update_spawns(&player);assert(ordinarySpawns==3);
 for(unsigned density=1;density<=5;++density){
  fresh(0);s_run.options.horde=density;s_run.horde_remaining=horde_duration;
  for(unsigned n=0;n<s_run.target_population()-1;++n)assert(create_enemy_at({},0,0,false));
  s_spawnCooldown=0;update_spawns(&player);assert(ordinarySpawns==1);
  unsigned normal=0;for(const auto& e:s_enemies)normal+=e.id!=kNone&&!e.elite;
  assert(normal==345);s_spawnFailures=10;s_spawnCooldown=0;update_spawns(&player);assert(ordinarySpawns==1&&s_spawnFailures==0);
 }
 // Debug population caps also bound Horde bursts; bosses and dormant guards
 // neither consume the normal cap nor disappear when it is zero.
 if(debug_enabled){
  debug_session_enabled=true;
  for(unsigned limit:{0u,1u,7u,42u,345u}){
   fresh(0);s_run.options.debug.max_normal_enemies=limit;
   s_run.options.horde=5;s_run.horde_remaining=horde_duration;
   assert(create_enemy_at({},0,0,true,4,true));
   if(limit)for(unsigned n=0;n<limit-1;++n)assert(create_enemy_at({},0,0,false));
   s_spawnCooldown=0;update_spawns(&player);
   assert(normal_enemy_population()==limit&&ordinarySpawns==unsigned(limit>0)&&guards()==1);
   s_spawnCooldown=0;update_spawns(&player);assert(normal_enemy_population()==limit);
   s_run.ticks=elite_interval;s_spawnCooldown=0;update_spawns(&player);
   assert(waves()==1&&normal_enemy_population()==limit);
  }
  debug_session_enabled=false;
 }
 // Four spread-out random guards, from the entire current room rather than Link's spawn ring.
 for(unsigned map:{1u,2u}){fresh(map);add_floor(10000);update_elite_spawns(&player);assert(guards()==4);
  for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)assert(s_enemies[i].lootPosition.absXZ(s_enemies[j].lootPosition)>=1800);
  for(auto& e:s_enemies)if(e.id!=kNone)e.age=6000;
  s_spawnCooldown=0;update_spawns(&player);for(auto& e:s_enemies)if(e.elite)assert(!e.retiring);
 }
 // An entrance corridor is not a valid guard landing; no fallback to Link.
 fresh(1);add_floor(1000);for(unsigned frame=0;frame<20;++frame)update_elite_spawns(&player);assert(guards()==0);
 // Distant open land is accepted even when a direct path from Link is blocked.
 fresh(1);add_floor(10000);rejectLongPath=true;update_elite_spawns(&player);assert(guards()==4);
 for(const auto&e:s_enemies)if(e.dormant)assert(e.lootPosition.absXZ(s_arenaSpawnPosition)>=3000);
 // Locally narrow or blocked land still fails, despite broad map sampling.
 fresh(1);add_floor(10000);narrowLanding=true;update_elite_spawns(&player);assert(guards()==0);
 fresh(1);add_floor(10000);player.current.pos={5000,0,0};update_elite_spawns(&player);assert(guards()==4);
 for(const auto&e:s_enemies)if(e.dormant)assert(e.lootPosition.absXZ(s_arenaSpawnPosition)>=3000 && e.lootPosition.absXZ(player.current.pos)>=1800);
 // One per authored building entrance; repeated layer records deduplicate.
 fresh(3);add_floor(10000);door(-4000,0);door(4000,0);door(0,6000);door(0,6000);update_elite_spawns(&player);assert(guards()==3);
 for(unsigned i=0;i<3;++i)assert(s_enemies[i].lootPosition.absXZ(s_eliteDoorSites[i].position)>=450);
 // Suspended/uncreated shared stage doors are recovered from native records.
 // Only doors adjoining room 0 count, regardless of the owning actor room.
 fresh(3);add_floor(10000);stage_tgsc_data_class rawDoors[4];
 for(unsigned i=0;i<4;++i){std::memcpy(rawDoors[i].name,"mapDoor",8);
  rawDoors[i].base.position={float(i)*2500-4000,600,0};
  rawDoors[i].base.parameters=((i+1)<<13);}
 rawDoors[3].base.parameters=(4u<<13)|(5u<<19);
 assert(!dStage_searchName(rawDoors[0].name));
 stage_tgsc_class doors{4,rawDoors};stageData.doors=&doors;
 update_elite_spawns(&player);assert(guards()==3&&s_eliteDoorSites.size()==3);
 update_elite_spawns(&player);assert(guards()==3); // cache refresh cannot append duplicates
 // Invalid room, holes, hazards, walls and exit curtains never accept placements.
 fresh(1);add_floor(10000);
 for(unsigned bad=0;bad<5;++bad){groundMissing=bad==0;groundRoom=bad==1?99:-2;groundCode=bad==2?4:0;wallBlocked=bad==3;barrierBlocked=bad==4;update_elite_spawns(&player);assert(guards()==0);}
 groundMissing=wallBlocked=barrierBlocked=false;groundRoom=-2;groundCode=0;update_elite_spawns(&player);assert(guards()==4);

 // Guards respawn three active minutes after the kill, not after spawning.
 fresh(3);add_floor(10000);door(2000,0);update_elite_spawns(&player);
 const auto guardSlot=s_enemies[0].eliteSlot;const auto home=s_enemies[0].lootPosition;
 s_run.ticks=713;drop_elite_chest(s_enemies[0],&player);s_enemies[0]={};
 const unsigned deadline=713+guard_respawn_interval;
 assert(s_eliteEncounters[guardSlot].respawnAt==deadline);
 s_run.ticks=deadline-1;update_elite_spawns(&player);assert(guards()==0);
 s_run.ticks=deadline;s_run.phase=Phase::Upgrade;update_elite_spawns(&player);assert(guards()==0);
 s_run.phase=Phase::Running;s_spawnCooldown=30;update_spawns(&player);assert(guards()==1);
 assert(s_enemies[0].lootPosition.absXZ(home)==0&&!s_eliteEncounters[guardSlot].defeated);
 // A second life respawns but never repeats the first free chest.
 const auto previousChest=s_chests[2].position;player.current.pos={900,0,100};
 drop_elite_chest(s_enemies[0],&player);assert(s_chests[2].active&&!s_chests[3].active);
 assert(s_chests[2].position.absXZ(previousChest)==0);
 drop_elite_chest(s_enemies[0],&player);assert(!s_chests[4].active);
 s_run.ticks+=guard_respawn_interval;update_elite_spawns(&player);
 assert(s_eliteEncounters[guardSlot].defeated); // Do not duplicate a still-tracked corpse.
 s_enemies[0]={};failCreate=true;update_elite_spawns(&player);assert(guards()==0);
 failCreate=false;update_elite_spawns(&player);assert(guards()==1&&s_enemies[0].dormant);
 // Outdoor respawns are separated from both the old position and other guards.
 for(unsigned map:{1u,2u}){fresh(map);add_floor(10000);update_elite_spawns(&player);
  const auto previous=s_enemies[0].lootPosition;drop_elite_chest(s_enemies[0],&player);s_enemies[0]={};
  s_run.ticks=guard_respawn_interval;groundMissing=true;update_elite_spawns(&player);assert(guards()==3);
  groundMissing=false;failCreate=true;update_elite_spawns(&player);assert(guards()==3);
  assert(s_eliteEncounters[timed_elite_count].position.absXZ(previous)==0);
  failCreate=false;update_elite_spawns(&player);assert(guards()==4&&s_enemies[0].dormant);
  assert(s_enemies[0].lootPosition.absXZ(previous)>=1800);
  for(unsigned j=1;j<4;++j)assert(s_enemies[0].lootPosition.absXZ(s_enemies[j].lootPosition)>=1800);
 }
 // All eight guard slots can leave every possible drop uncollected for 20 minutes.
 fresh(3);add_floor(30000);
 for(unsigned i=0;i<guard_capacity;++i)door(float(i)*3000-10000,0);
 unsigned drops=0;
 for(unsigned tick=0;tick<duration;++tick){
  s_run.ticks=tick;update_elite_spawns(&player);
  for(auto& e:s_enemies)if(e.id!=kNone){drop_elite_chest(e,&player);++drops;e={};}
 }
 assert(drops==elite_reward_capacity-1);
 for(unsigned i=kPaidChestCount;i<kPaidChestCount+guard_capacity+3;++i)assert(s_chests[i].active&&s_chests[i].free);
 assert(!s_chests[kPaidChestCount+guard_capacity+3].active);
 s_run.ticks=duration;update_elite_spawns(&player);assert(guards()==0&&waves()==1);
 for(auto& e:s_enemies)if(e.id!=kNone){drop_elite_chest(e,&player);e={};}
 assert(s_chests[kPaidChestCount+guard_capacity+3].active&&s_chests[kPaidChestCount+guard_capacity+3].free);
 // Dormant AI suppresses only idle action. Collision/damage Execute stays native.
 fresh(3);add_floor(10000);door(2000,0);update_elite_spawns(&player);
 daE_OC_c actor;actor.id=s_enemies[0].id;actor.speedF=10;actor.speed={2,3,4};std::any args[]={&actor};
 assert(before_bokoblin_wait(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(actor.speedF==0&&actor.speed.x==0&&actor.speed.y==3&&actor.animations==1);
 before_bokoblin_wait(nullptr,args,nullptr,nullptr);assert(actor.animations==1);
 s_enemies[0].dormant=false;assert(before_bokoblin_wait(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 // Every five active seconds: no payouts during menus, pause, death or results.
 for(unsigned rank=0;rank<=5;++rank) {
  fresh(0);life=12;rupees=rupeeWrites=0;s_run.talents.ranks[unsigned(Talent::AutoRupees)]=rank;
  std::any incomeArgs[]={&player};s_run.ticks=149;
  for(auto phase:{Phase::Ready,Phase::Upgrade,Phase::TreasureChoice,Phase::Treasure,Phase::Victory,Phase::Defeat}) {
   s_run.phase=phase;after_player(nullptr,incomeArgs,nullptr,nullptr);assert(s_run.ticks==149&&!rupeeWrites);
  }
  s_run.phase=Phase::Running;paused=true;after_player(nullptr,incomeArgs,nullptr,nullptr);assert(!rupeeWrites&&s_run.ticks==149);
  paused=false;after_player(nullptr,incomeArgs,nullptr,nullptr);assert(rupees==rank&&rupeeWrites==unsigned(rank!=0));
  after_player(nullptr,incomeArgs,nullptr,nullptr);assert(rupees==rank);
  s_run.ticks=299;after_player(nullptr,incomeArgs,nullptr,nullptr);assert(rupees==2*rank);
  rupees=299;s_run.ticks=449;after_player(nullptr,incomeArgs,nullptr,nullptr);assert(rupees==(rank?300:299));
 }
 // The actual level transition heals immediately, before choosing an upgrade.
 fresh(0);life=1;lifeWrites=0;s_run.xp=s_run.required_xp()-1;
 std::any levelArgs[]={&player};after_player(nullptr,levelArgs,nullptr,nullptr);
 assert(life==1&&!lifeWrites&&s_run.phase==Phase::Running);
 ++s_run.xp;after_player(nullptr,levelArgs,nullptr,nullptr);
 assert(life==12&&lifeWrites==1&&s_run.phase==Phase::Upgrade);
 after_player(nullptr,levelArgs,nullptr,nullptr);assert(lifeWrites==1);
 // Greed blocks level-up and periodic regen while boosting actual passive payouts.
 fresh(0);life=1;lifeWrites=0;s_run.apply_fate(FateEffect::Greed);s_run.xp=s_run.required_xp();
 after_player(nullptr,levelArgs,nullptr,nullptr);assert(life==1&&!lifeWrites&&s_run.phase==Phase::Upgrade);
 s_run.phase=Phase::Running;s_run.xp=0;s_run.ticks=s_run.life_regen_interval()-1;
 after_player(nullptr,levelArgs,nullptr,nullptr);assert(life==1&&!lifeWrites);
 rupees=0;s_run.talents.ranks[unsigned(Talent::AutoRupees)]=1;
 s_run.ticks=149;after_player(nullptr,levelArgs,nullptr,nullptr);assert(rupees==1);
 s_run.ticks=299;after_player(nullptr,levelArgs,nullptr,nullptr);assert(rupees==3);
 s_run.clear_fate();s_run.xp=s_run.required_xp();after_player(nullptr,levelArgs,nullptr,nullptr);
 assert(life==12&&lifeWrites==1);
 // Endless 40:00 preserves passive regen and full level-up healing.
 fresh(0);s_run.endless=true;s_run.ticks=40*60*ticks_per_second-1;life=1;lifeWrites=0;
 after_player(nullptr,levelArgs,nullptr,nullptr);assert(life==2&&lifeWrites==1);
 s_run.xp=s_run.required_xp();after_player(nullptr,levelArgs,nullptr,nullptr);
 assert(life==12&&s_run.phase==Phase::Upgrade&&lifeWrites==2);
 // Quarter-heart regeneration uses active time, every rank, and preserves native max life.
 for(unsigned rank=0;rank<=5;++rank) {
  fresh(0);life=4;lifeWrites=0;s_run.talents.ranks[unsigned(Talent::LifeRegen)]=rank;
  const unsigned interval=(20-rank)*ticks_per_second;assert(s_run.life_regen_interval()==interval);
  std::any regenArgs[]={&player};assert(!s_run.life_regen_due());
  s_run.ticks=interval-1;
  for(auto phase:{Phase::Ready,Phase::Upgrade,Phase::TreasureChoice,Phase::Treasure,Phase::Victory,Phase::Defeat}) {
   s_run.phase=phase;after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==4&&!lifeWrites&&s_run.ticks==interval-1);
  }
  s_run.phase=Phase::Running;paused=true;after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==4&&!lifeWrites);
  paused=false;player.id=2;after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==4&&!lifeWrites);player.id=1;
  after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==5&&lifeWrites==1);
  after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==5&&lifeWrites==1);
  s_run.ticks=2*interval-1;after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==6&&lifeWrites==2);
  life=11;s_run.ticks=3*interval-1;after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==12&&lifeWrites==3);
  s_run.ticks=4*interval-1;after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==12&&lifeWrites==3);
  life=5;s_run.ticks=(duration/interval+1)*interval-1;after_player(nullptr,regenArgs,nullptr,nullptr);
  assert(life==6); // Still regenerates during the last boss beyond 20 minutes.
  // Talent and upgrade regeneration retain their combined interval beyond 40:00.
  s_run.endless=true;s_run.ranks[unsigned(Upgrade::LifeRegen)]=rank;
  const auto endlessInterval=s_run.life_regen_interval();
  assert(endlessInterval==(20-2*rank)*ticks_per_second);
  life=4;lifeWrites=0;s_run.ticks=(40*60*ticks_per_second/endlessInterval+1)*endlessInterval-1;
  after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==5&&lifeWrites==1);
  life=0;s_run.ticks=interval-1;after_player(nullptr,regenArgs,nullptr,nullptr);assert(life==0&&s_run.phase==Phase::Defeat);
 }
 finishes=0;life=12;
 // Production timer completion: pause and UI phases never consume run time.
 fresh(0);s_run.ticks=duration-1;std::any playerArgs[]={&player};paused=true;
 after_player(nullptr,playerArgs,nullptr,nullptr);assert(s_run.ticks==duration-1&&finishes==0);
 paused=false;s_run.phase=Phase::Upgrade;after_player(nullptr,playerArgs,nullptr,nullptr);assert(finishes==0);
 s_run.phase=Phase::Running;after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_run.ticks==duration&&s_run.phase==Phase::Running&&finishes==0&&waves()==4);
 assert(ordinarySpawns==0); // No new horde after the final milestone.
 s_run.timed_elites=3;after_player(nullptr,playerArgs,nullptr,nullptr);assert(finishes==0);
 s_run.timed_elites=4;after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_run.phase==Phase::Running&&finishes==0); // Kill callback, not kill-count polling, starts victory.
 s_victory=true;s_victoryTicks=10*ticks_per_second-1;
 const auto elapsed=s_run.ticks;after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_run.phase==Phase::Victory&&finishes==1&&s_run.ticks==elapsed);
 after_player(nullptr,playerArgs,nullptr,nullptr);assert(finishes==1);
 fresh(0);life=0;after_player(nullptr,playerArgs,nullptr,nullptr);assert(s_run.phase==Phase::Defeat&&s_run.ticks==0);
 reset_elite_encounters();for(const auto& e:s_eliteEncounters)assert(!e.placed&&!e.defeated);
 clear_elite_sites();assert(s_eliteFloors.empty()&&s_eliteDoorSites.empty());
 std::cout<<"Survivors elites: schedule, all map placements, respawns, dormancy and oldest-chest replacement passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test'
    cpp.write_text(fixture+"\nunsigned s_gearNoticeTicks=0;void update_gear_notice(){if(s_gearNoticeTicks)--s_gearNoticeTicks;}void clear_gear_notices(){s_gearNoticeTicks=0;}\n"+(root/'src/survivors_elite_positions.inc').read_text()+
        (root/'src/survivors_elites.inc').read_text()+extract(loot,'chest_spawning_allowed')+extract(loot,'find_chest_spawn_slot')+extract(loot,'drop_elite_chest')+
        extract(runtime,'normal_enemy_population')+extract(runtime,'update_spawns')+extract(runtime,'after_player')+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
