"""Native pot parameters, map budgets, confirmed breaks and lifecycle isolation."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
core = (root / 'src/survivors.cpp').read_text()
start = core.index('bool pause_run_actor(')
pause = core[start:core.index('\nHookAction before_process', start)]
fixture = r'''
bool s_victory=false,s_deathPending=false;

#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include "survivors_rules.hpp"
#include "actor_slot_index.hpp"
using dawnlight::ActorSlotIndex;
using namespace dawnlight::survivors;
using ActorId=unsigned;using s16=short;constexpr ActorId kNone=~0u;
struct ModContext{};ModContext* mod_ctx=nullptr;enum HookAction{HOOK_CONTINUE};
namespace mods{
template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}
template<class T>T& arg_ref(void* a,int n){return std::any_cast<T&>(static_cast<std::any*>(a)[n]);}}
struct cXyz{float x=0,y=0,z=0;float absXZ(cXyz p)const{return std::hypot(x-p.x,z-p.z);}};
struct fopAc_ac_c{ActorId id=kNone;struct{cXyz pos;}current;};
struct Collider{void* tg=nullptr;void* at=nullptr;
 void* GetTgHitObj(){return tg;}void* GetAtHitObj(){return at;}
 void ResetTgHit(){tg=nullptr;}void ResetAtHit(){at=nullptr;}};
struct daObjCarry_c:fopAc_ac_c{Collider mCyl;};
struct daAlink_c:fopAc_ac_c{}player;
ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}
void* ownAttack=reinterpret_cast<void*>(1);bool own_collider(void* p){return p==ownAttack;}
std::map<ActorId,daObjCarry_c> actors;std::set<ActorId> pending;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:&it->second;}
bool fpcM_IsCreating(ActorId id){return pending.count(id);}
struct Service{unsigned deletes=0;void delete_actor(ModContext*,ActorId id){++deletes;actors.erase(id);}}service;
auto* svc_actor=&service;
bool s_hub=false,s_hubDeparture=false,s_hubDoorStarted=false;Run s_run;unsigned s_map=0;bool s_arenaMeshReady=true,s_allowDrop=false,active=true,paused=false,transition=false;
bool eventRunning=false,windowOpen=false;
bool ui_visible(){return windowOpen;}bool midna_dialog_busy(){return false;}
bool dComIfGp_event_runCheck(){return eventRunning;}bool dComIfGp_isPauseFlag(){return paused;}
float s_eliteFloorArea=12000000;bool badFloor=false,nearGuard=false,failCreate=false;unsigned sequence=0,creates=0;
bool save_modal_active(){return false;}
bool in_arena(){return active;}bool transitioning(){return transition;}bool playable(){return active&&!paused&&!transition;}
struct Arena{int room=51;}arenaData;const Arena& arena(){return arenaData;}
bool distributionReady=true;bool arena_distribution_ready(){return distributionReady;}
float arena_distribution_area(){return s_eliteFloorArea;}
bool random_arena_floor(cXyz& p){p={1000.f+500.f*(sequence%12),0,1000.f+500.f*(sequence/12)};++sequence;return !badFloor;}
bool separated_guard(cXyz,float){return !nearGuard;}
struct ActorSpawnParams{unsigned parameters;int argument,room_num;cXyz position;struct{s16 x,y,z;}angle;cXyz scale;void* create_function;};
constexpr int MOD_OK=0,fpcNm_Obj_Carry_e=7;
int create_standalone_actor(int profile,const ActorSpawnParams& p,ActorId& id){
 assert(profile==fpcNm_Obj_Carry_e&&p.parameters==0x3fff&&p.argument==-1&&p.room_num==arena().room);
 assert(p.angle.x==s16(0xffff)&&p.angle.z==0x1040&&p.scale.x==1&&p.scale.y==1&&p.scale.z==1);
 assert(p.position.absXZ(player.current.pos)>=300);
 if(failCreate)return 1;
 id=++creates;pending.insert(id);return MOD_OK;
}
enum{dItemNo_KAKERA_HEART_e,dItemNo_GREEN_RUPEE_e,dItemNo_BLUE_RUPEE_e,dItemNo_YELLOW_RUPEE_e,
 dItemNo_RED_RUPEE_e,dItemNo_PURPLE_RUPEE_e,dItemNo_ORANGE_RUPEE_e,dItemNo_SILVER_RUPEE_e,dItemNo_KEY_OF_FILONE_e};
std::vector<int> drops;
ActorId fopAcM_createItem(const cXyz*,int item,int bit,int room,void*,void*,int){
 assert(s_allowDrop&&bit==-1&&room==arena().room);drops.push_back(item);return 900;
}
ActorId create_run_pickup(cXyz pos,int item){
 const bool old=s_allowDrop;s_allowDrop=true;const auto id=fopAcM_createItem(&pos,item,-1,arena().room,nullptr,nullptr,0);s_allowDrop=old;return id;}

'''
main = r'''
void fresh(unsigned map=0){s_pots={};s_potTarget=s_potDelay=0;actors.clear();pending.clear();drops.clear();
 s_run.start(42);s_map=map;active=s_arenaMeshReady=true;paused=transition=badFloor=nearGuard=failCreate=s_allowDrop=false;
 sequence=creates=service.deletes=0;player={};}
void ready(unsigned slot){auto& p=s_pots[slot];pending.erase(p.id);actors[p.id].id=p.id;actors[p.id].current.pos=p.position;p.initialized=true;}
void break_pot(unsigned slot,bool item=true){auto* actor=static_cast<daObjCarry_c*>(actor_by_id(s_pots[slot].id));assert(actor);
 std::any args[]={actor,item,true,true};assert(before_pot_break(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 assert(!std::any_cast<bool>(args[1])); // Original still shatters, but cannot duplicate the drop.
}
int main(){
 fresh();eventRunning=true;
 assert(pause_run_actor(true)&&!pause_run_actor(false)); // Native heart-piece event can execute Link.
 eventRunning=false;windowOpen=true;assert(pause_run_actor(true)&&pause_run_actor(false));windowOpen=false;
 assert(pot_budget(0,0)==6&&pot_budget(0,1.e12f)==10);
 assert(pot_budget(1,0)==40&&pot_budget(2,0)==40&&pot_budget(1,1.e12f)==72);
 assert(pot_budget(3,0)==12&&pot_budget(4,0)==8&&pot_budget(99,1.e12f)==0);
 assert(pot_budget(1,100000000)<pot_budget(1,140000000));
 // Native creations are staggered, all requested slots fill, pending IDs are retained.
 for(unsigned map=0;map<5;++map){fresh(map);for(unsigned frame=0;frame<1000;++frame)update_pots(&player);
  assert(creates==pot_budget(map,s_eliteFloorArea));cleanup_pots();assert(s_pots[0].id!=kNone);
  for(unsigned i=0;i<creates;++i)for(unsigned j=i+1;j<creates;++j)assert(s_pots[i].position.absXZ(s_pots[j].position)>=250);
 }
 fresh();distributionReady=false;update_pots(&player);assert(creates==0&&s_potTarget==0);
 distributionReady=true;
 fresh();badFloor=true;update_pots(&player);assert(creates==0);badFloor=false;
 s_potDelay=0;nearGuard=true;update_pots(&player);assert(creates==0);nearGuard=false;
 s_potDelay=0;failCreate=true;update_pots(&player);assert(creates==0);failCreate=false;
 s_potDelay=0;update_pots(&player);assert(creates==1);update_pots(&player);assert(creates==1);
 // Only initialized, owned, actually broken pots grant a single native pickup.
 ready(0);break_pot(0);assert(drops.size()==1&&!s_allowDrop&&s_run.kills==0&&s_run.xp==0);
 break_pot(0);assert(drops.size()==1);actors.erase(s_pots[0].id);cleanup_pots();
 const auto oldPosition=s_pots[0].position;
 for(unsigned i=0;i<pot_respawn_interval-1;++i){++s_run.ticks;update_pots(&player);}
 assert(s_pots[0].id==kNone&&creates==s_potTarget);
 ++s_run.ticks;s_potDelay=0;update_pots(&player);
 assert(s_pots[0].id!=kNone&&!s_pots[0].broken&&creates==s_potTarget+1);
 assert(s_pots[0].position.absXZ(oldPosition)>=300);
 // Repeated attempts at the old location cannot bypass relocation.
 ready(0);break_pot(0);actors.erase(s_pots[0].id);cleanup_pots();
 s_run.ticks+=pot_respawn_interval;s_potDelay=0;badFloor=true;update_pots(&player);
 assert(s_pots[0].id==kNone&&s_pots[0].broken);badFloor=false;

 // Every currency tier and rare heart pieces occur, and Luck improves both.
 std::array<unsigned,9> plain{},lucky{};
 for(unsigned trial=0;trial<2;++trial){fresh();if(trial)s_run.ranks[unsigned(Upgrade::Luck)]=5;
  update_pots(&player);ready(0);
  for(unsigned i=0;i<100000;++i){s_pots[0].broken=false;break_pot(0);++(trial?lucky:plain)[drops.back()];}
 }
 for(unsigned count:plain)assert(count>0);
 assert(lucky[0]>plain[0]&&lucky[7]>plain[7]&&lucky[1]<plain[1]);
 // Keys use the exact same 2% base / 3% capped Luck threshold as heart pieces.
 assert(plain[8]>1800&&plain[8]<2200&&plain[0]>1800&&plain[0]<2200);
 assert(lucky[8]>2800&&lucky[8]<3200&&lucky[0]>2800&&lucky[0]<3200);
 s_run.endless=true;s_run.ticks=40*60*ticks_per_second;
 unsigned lateKeys=0;
 for(unsigned i=0;i<10000;++i){auto drop=s_run.roll_pot_drop();assert(drop!=Run::PotDrop::HeartPiece);lateKeys+=drop==Run::PotDrop::SmallKey;}
 assert(lateKeys>0);
 // Unrelated pots retain native behavior; destruction without loot remains empty.
 fresh();daObjCarry_c foreign;foreign.id=888;std::any foreignArgs[]={&foreign,true,true,true};
 before_pot_break(nullptr,foreignArgs,nullptr,nullptr);assert(std::any_cast<bool>(foreignArgs[1])&&drops.empty());
 update_pots(&player);ready(0);break_pot(0,false);assert(drops.empty()&&s_pots[0].broken);
 // Pause, another mode, transitions and teardown cannot create or reward pots.
 for(unsigned reason=0;reason<6;++reason){fresh();update_pots(&player);ready(0);
  if(reason==0)active=false;if(reason==1)transition=true;if(reason==2)s_run.phase=Phase::Upgrade;
  if(reason==3)s_pots[0].retiring=true;if(reason==4)s_pots[0].initialized=false;if(reason==5)s_run.phase=Phase::Victory;
  break_pot(0);assert(drops.empty());
 }
 fresh();paused=true;update_pots(&player);assert(creates==0);paused=false;
 update_pots(&player);ready(0);auto* actor=static_cast<daObjCarry_c*>(actor_by_id(s_pots[0].id));
 actor->mCyl.tg=ownAttack;actor->mCyl.at=reinterpret_cast<void*>(2);detach_pot_attacks();
 assert(!actor->mCyl.tg&&actor->mCyl.at==reinterpret_cast<void*>(2));
 // Deferred deletion retries; pending creation survives cleanup until it can be deleted.
 s_potDelay=0;update_pots(&player);const auto pendingId=s_pots[1].id;assert(pending.count(pendingId));
 retire_pots();cleanup_pots();assert(service.deletes==1&&s_pots[1].id==pendingId);
 ready(1);cleanup_pots();cleanup_pots();assert(service.deletes==2&&s_pots[0].id==kNone&&s_pots[1].id==kNone);
 assert(drops.empty());
 std::cout<<"Survivors pots: map budgets, native parameters, break rewards, Luck and lifecycle passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp = Path(temp) / 'test.cpp'
    exe = Path(temp) / 'test'
    cpp.write_text(fixture + (root / 'src/survivors_pots.inc').read_text() + pause + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
