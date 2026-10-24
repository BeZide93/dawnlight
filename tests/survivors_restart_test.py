"""Run restart and hub return must route differently through scene reload/arrival."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
def extract(name):
    m=re.search(r'^(?:void|HookAction) '+name+r'\([^;{}]*\)\s*\{',core,re.M)
    assert m,name
    brace=core.index('{',m.start());depth=0
    for end in range(brace,len(core)):
        depth+=(core[end]=='{')-(core[end]=='}')
        if not depth:return core[m.start():end+1]
    raise AssertionError(name)
update=extract('update_survivors')
assert 'if (s_restart) {\n        reload_run_scene();\n        return;' in update
assert 'if (restart) restart_run(nullptr,nullptr);' in (root/'src/survivors_midna.inc').read_text()
arrival=update[update.index('    if (s_run.phase == Phase::Loading) {'):update.index('    if(s_hub)update_hub(link);')]
fixture=r'''
#include <any>
#include <array>
#include <cassert>
#include <string>
#include "survivors_rules.hpp"
#include "survivors_maps.hpp"
using namespace dawnlight::survivors;
using s16=short;using s8=signed char;constexpr unsigned kNone=~0u;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T& arg_ref(void* p,unsigned i){return *std::any_cast<T>(&static_cast<std::any*>(p)[i]);}}
namespace mDoRst{bool isReset(){return false;}bool isReturnToMenu(){return false;}}
struct cXyz{float x=0,y=0,z=0;};
struct daAlink_c{bool demo=false;bool checkPlayerDemoMode()const{return demo;}struct{cXyz pos;}current;struct{s16 y=0;}shape_angle;}link;
cXyz s_arenaSpawnPosition;s16 s_arenaSpawnYaw=0;bool s_arenaSpawn=false;
void clear_reachable_floor(){}
Run s_run;RunOptions s_options;Records s_records;GearInventory s_gear;
bool s_active=true,s_gameStarted=true,s_restart=false,s_arrivingRun=false,s_hub=false;
bool s_hubDeparture=false,s_hubDoorStarted=false,s_hubSaveRequested=false,s_resultsStored=false;
bool s_arenaMeshReady=false,playing=true,grounded=true,stagePending=false;
enum class HubMenu{None,Run};HubMenu s_hubMenu=HubMenu::None;
unsigned s_map=0,s_requestedMap=0,s_hubDoor=kNone,s_player=7,s_spawnFailures=0,s_warmup=0;
std::array<int,10>s_orbs{};
unsigned starts=0,reloads=0,retirements=0,attackClears=0,lootClears=0,closes=0;
std::string destination;int destinationRoom=-1,destinationPoint=-1;
const Arena& arena(){return arenas[s_map];}
bool in_arena(){return true;}bool playable(){return playing&&!stagePending;}
bool arena_grounded(daAlink_c*){return grounded;}
void load_records(){}void end_survivors_integrity(){}void reset_ending(){}
void close_window(){++closes;}void retire_enemies(){++retirements;}
void clear_attacks(){++attackClears;}void clear_loot(){++lootClears;}
void clear_arena_geometry(){s_arenaMeshReady=false;s_warmup=0;}
void update_survivors_equipment_chest(bool,int,int){}void update_hub_brazier(bool){}
void prepare_save(){}void dComIfGs_setRestartRoomParam(int){}
HookAction before_stage(ModContext*,void*,void*,void*);
void dComIfGp_setNextStage(const char* stage,s16 point,s8 room,s8 layer){
 std::any args[]{stage,point,room,layer};
 assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 destination=std::any_cast<const char*>(args[0]);destinationPoint=std::any_cast<s16>(args[1]);destinationRoom=std::any_cast<s8>(args[2]);
 ++reloads;stagePending=true;
}
void begin_run(){
 assert(s_run.phase==Phase::Ready&&s_arenaMeshReady&&!s_restart&&!s_hub);
 ++starts;s_run.start(123,s_options,s_records.talents);s_run.equip_gear(s_gear);
 if(s_run.starting_item_pending)s_run.phase=Phase::TreasureChoice;
}
'''
main=r'''
int main(){
 for(unsigned map=0;map<arenas.size();++map)for(bool startingItem:{false,true}){
  s_map=map;s_requestedMap=99;s_hub=false;s_arrivingRun=false;s_restart=false;
  s_records={};s_records.total_xp=meta_threshold(10);s_records.talents.ranks[unsigned(Talent::StartingItem)]=startingItem;
  const auto records=s_records.encode();s_options.enemy_hp=4;
  s_run.start(42,s_options,s_records.talents);s_run.ticks=1234;s_run.kills=50;s_run.ranks[0]=10;s_run.end(false);
  s_resultsStored=true;s_orbs.fill(9);s_player=7;
  const auto priorStarts=starts,priorReloads=reloads,priorRetired=retirements;
  restart_run(nullptr,nullptr);assert(s_restart&&s_arrivingRun&&s_requestedMap==map);
  reload_run_scene();assert(reloads==priorReloads+1&&retirements==priorRetired+1);
  assert(s_map==map&&!s_hub&&s_arrivingRun&&!s_restart&&s_player==kNone&&!s_resultsStored);
  assert(destination==arenas[map].stage&&destinationRoom==arenas[map].room&&destinationPoint==arenas[map].point);
  assert(s_run.phase==Phase::Loading&&s_run.ticks==0&&s_run.kills==0&&s_run.ranks[0]==0);
  assert(s_records.encode()==records&&s_options.enemy_hp==4&&s_orbs[0]==0);
  // Scene transition, airborne arrival and missing geometry cannot start combat.
  for(int i=0;i<20;++i)arrival(&link);assert(starts==priorStarts);
  stagePending=false;grounded=false;for(int i=0;i<20;++i)arrival(&link);assert(starts==priorStarts);
  grounded=true;link.demo=true;for(int i=0;i<35;++i)arrival(&link);
  assert(s_run.phase==Phase::Loading&&s_warmup==0&&starts==priorStarts);
  link.demo=false;link.current.pos={1000+float(map),0,0};link.shape_angle.y=2345;
  for(int i=0;i<15;++i)arrival(&link);
  if(map==0)assert(s_arenaSpawn&&s_arenaSpawnPosition.x==1000&&s_arenaSpawnYaw==2345);
  assert(s_run.phase==Phase::Ready&&s_arrivingRun&&starts==priorStarts);
  playing=false;s_arenaMeshReady=true;arrival(&link);assert(starts==priorStarts&&s_arrivingRun);
  playing=true;arrival(&link);
  assert(starts==priorStarts+1&&!s_arrivingRun&&!s_hub&&s_run.ticks==0&&s_run.options.enemy_hp==4);
  assert(s_run.phase==(startingItem?Phase::TreasureChoice:Phase::Running));
  arrival(&link);assert(starts==priorStarts+1);
  // Results' Return to hub clears any stale direct-start intent.
  s_arrivingRun=true;s_run.end(false);replay(nullptr,nullptr);reload_run_scene();
  assert(s_map==0&&s_hub&&!s_arrivingRun&&s_requestedMap==map&&destination==arenas[0].stage);
  stagePending=false;s_arenaMeshReady=true;for(int i=0;i<20;++i)arrival(&link);
  assert(s_run.phase==Phase::Ready&&starts==priorStarts+1);
 }
 assert(retirements==reloads&&attackClears==reloads&&lootClears==reloads);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'restart.cpp';exe=Path(tmp)/'restart'
    cpp.write_text(fixture+'\n'.join(extract(n) for n in ['before_stage','replay','restart_run','reload_run_scene'])+'\nvoid arrival(daAlink_c* link){\n'+arrival+'}\n'+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),'-I',str(root/'include'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors restart: same-map reload, cleanup, readiness gates, automatic start, starting item and hub return passed')
