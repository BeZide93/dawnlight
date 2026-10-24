"""Native door ordering, hub save/close and scene routing without starting combat early."""
from pathlib import Path
import re,subprocess,tempfile,os
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
hub=(root/'src/survivors_hub.inc').read_text()
def extract(source,name):
 m=re.search(r'^(?:void\*?|bool|HookAction) '+name+r'\([^;{}]*\)\s*\{',source,re.M);assert m,name
 start=source.index('{',m.start());depth=0
 for i in range(start,len(source)):
  depth+=(source[i]=='{')-(source[i]=='}')
  if not depth:return source[m.start():i+1]
fixture=r'''
bool s_victory=false,s_deathPending=false;

#include <any>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
#include "survivors_maps.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;using u16=unsigned short;using s16=short;using s8=signed char;using s32=int;
constexpr unsigned kNone=~0u;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* a,int i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}
 template<class T>T& arg_ref(void* a,int i){return *std::any_cast<T>(&static_cast<std::any*>(a)[i]);}}
constexpr unsigned dEvtCnd_CANDOOR_e=4;
struct EventInfo{bool eligible=false,doorCommand=false;
 bool chkCondition(unsigned)const{return eligible;}bool checkCommandDoor()const{return doorCommand;}};
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};
struct fopAc_ac_c{unsigned id;EventInfo eventInfo{};int room=51,frontRoom=51,backRoom=0,name=1;bool isActor=true;struct{cXyz pos{};}current{};};
constexpr int fpcNm_DOOR20_e=1,fpcNm_KNOB20_e=2,fpcNm_DBDOOR_e=3,fpcNm_BOSS_DOOR_e=4,
 fpcNm_L1BOSS_DOOR_e=5,fpcNm_L1MBOSS_DOOR_e=6,fpcNm_L5BOSS_DOOR_e=7,fpcNm_SPIRAL_DOOR_e=8;
bool fopAcM_IsActor(void* p){return p&&static_cast<fopAc_ac_c*>(p)->isActor;}
int fopAcM_GetRoomNo(fopAc_ac_c* a){return a->room;}int fopAcM_GetName(fopAc_ac_c* a){return a->name;}
struct door_param2_c{static int getFRoomNo(fopAc_ac_c* a){return a->frontRoom;}
 static int getBRoomNo(fopAc_ac_c* a){return a->backRoom;}};
std::vector<cXyz> labels;
void hub_label(const char* text,cXyz position){assert(std::string(text)=="Start Run");labels.push_back(position);}
struct daAlink_c:fopAc_ac_c{};
daAlink_c link{{1}};fopAc_ac_c door{2};
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
bool doorAlive=true;fopAc_ac_c* actor_by_id(unsigned id){return doorAlive&&id==2?&door:nullptr;}
Records s_records;Run s_run;RunOptions s_options;
bool s_active=true,s_gameStarted=true,s_saveReady=true,s_restart=false,s_progressSaving=false,s_arenaMeshReady=true;
bool s_hub=true,s_hubDeparture=false,s_hubDoorStarted=false,s_arrivingRun=false,s_allowHubDoor=false,s_hubSaveRequested=false;
enum class HubMenu{None,Talents,Run};HubMenu s_hubMenu=HubMenu::None;
unsigned s_hubClosingWindow=0,s_hubDoor=kNone,s_hubDoorTicks=0,s_player=1,s_map=0,s_requestedMap=0;
u16 s_hubDoorPriority=0,s_hubDoorFlags=0;
bool visible=false,event=false,arenaPresent=true,transition=false,resetting=false;
bool save_modal_active(){return false;}
bool in_arena(){return s_active&&s_gameStarted&&arenaPresent;}
bool ui_visible(){return visible;}bool transitioning(){return transition;}
bool dComIfGp_event_runCheck(){return event;}bool dComIfGp_isPauseFlag(){return false;}
bool midna_dialog_busy(){return false;}
bool playable(){return in_arena()&&!visible&&!event&&!transition;}
namespace mDoRst{bool isReset(){return resetting;}bool isReturnToMenu(){return false;}}
void dComIfGs_setRestartRoomParam(int){}
const Arena& arena(){return arenas[s_map];}
unsigned cleared=0,opened=0,orders=0;
void clear_arena_geometry(){++cleared;}
void open_window(){visible=true;++opened;}
void hub_menu_closed();void close_window(){visible=false;hub_menu_closed();}
struct Log{void warn(ModContext*,const char*){}void info(ModContext*,const char*){}}logger;auto* svc_log=&logger;ModContext* mod_ctx=nullptr;
HookAction before_hub_door(ModContext*,void*,void*,void*);
constexpr int dItemNo_SWORD_e=40;bool swordReceived=true;unsigned zeldaCalls=0;
bool dComIfGs_isItemFirstBit(int item){assert(item==dItemNo_SWORD_e);return swordReceived;}
void request_hub_zelda_call(){++zeldaCalls;}
bool acceptOrder=true;
int fopAcM_orderDoorEvent(fopAc_ac_c* a,fopAc_ac_c* b,u16 priority,u16 flags){
 std::any args[]={a,b,priority,flags};int result=0;
 if(before_hub_door(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL)return result;
 assert(s_allowHubDoor&&!visible&&!s_progressSaving&&!s_hubSaveRequested);
 ++orders;return acceptOrder;
}
'''
main=r'''
int main(){
 s_run.phase=Phase::Ready;
 // The wall-side hub chest waits for geometry and faces the room center.
 wallReady=false;update_hub(&link);assert(!chestActor.active);wallReady=true;
 floorReady=false;update_hub(&link);assert(!chestActor.active);
 floorReady=true;update_hub(&link);
 assert(chestActor.active&&chestActor.position.x==1650&&chestActor.position.z==0);
 assert(chestActor.position.y==40&&chestActor.yaw==-0x4000);
 s_hubDeparture=true;update_hub(&link);assert(!chestActor.active);s_hubDeparture=false;

 // Loading the save never opens a window and Ready leaves Link executable.
 assert(!visible&&s_hubMenu==HubMenu::None&&!pause_run_actor(false));
 assert(pause_run_actor(true));visible=true;assert(pause_run_actor(false));visible=false;
 std::any request[]={static_cast<fopAc_ac_c*>(&link),&door,u16(7),u16(9)};int result=4;
 swordReceived=false;
 assert(before_hub_door(nullptr,request,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(!visible&&zeldaCalls==1&&!orders&&s_hubMenu==HubMenu::None);
 start_run(nullptr,nullptr);assert(!s_hubDeparture&&!visible);
 swordReceived=true;
 assert(before_hub_door(nullptr,request,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(!result&&visible&&opened==1&&s_hubMenu==HubMenu::Run&&s_hubDoor==2&&orders==0);
 // A locked destination cannot bypass the disabled UI by calling Start directly.
 s_requestedMap=1;start_run(nullptr,nullptr);assert(!s_hubDeparture&&visible);
 s_options.debug.unlock_all_maps=true;assert(!map_available(1));
 debug_session_enabled=true;s_options.debug.unlock_all_maps=true;assert(map_available(1)==debug_enabled);
 debug_session_enabled=false;s_options.debug.unlock_all_maps=false;s_records.cleared_maps=1;assert(map_available(1));
 s_records.cleared_maps=0;s_requestedMap=0;
 // Back/cancel saves automatically but does not open the door or start a run.
 close_window();assert(s_hubSaveRequested&&s_hubMenu==HubMenu::None);
 update_hub(&link);assert(!orders&&!s_hubDeparture&&s_run.phase==Phase::Ready);
 s_hubSaveRequested=false;
 before_hub_door(nullptr,request,&result,nullptr);
 s_hubDoorTicks=599;
 start_run(nullptr,nullptr);assert(s_hubDoorTicks==0);assert(!visible&&s_hubDeparture&&s_hubSaveRequested&&s_run.phase==Phase::Ready);
 update_hub(&link);assert(!orders); // save queued
 s_hubSaveRequested=false;s_progressSaving=true;update_hub(&link);assert(!orders);
 s_progressSaving=false;s_hubClosingWindow=17;update_hub(&link);assert(!orders);s_hubClosingWindow=0;
 assert(!pause_run_actor(false)); // Link must rebuild CANDOOR after the menu/save.
 visible=true;update_hub(&link);assert(!orders);visible=false;
 event=true;update_hub(&link);assert(!orders);event=false;
 // Actor beforeProc cleared eligibility while the menu paused Link's Execute.
 link.eventInfo.eligible=false;door.eventInfo.eligible=true;
 update_hub(&link);assert(!orders&&!s_hubDoorStarted);
 assert(!pause_run_actor(false));link.eventInfo.eligible=true; // Native Link update.
 update_hub(&link);assert(orders==1&&!s_hubDoorStarted&&!s_allowHubDoor);
 // An enqueued request can be rejected at entry; retry instead of waiting forever.
 update_hub(&link);assert(orders==2&&!s_hubDoorStarted);
 link.eventInfo.doorCommand=door.eventInfo.doorCommand=true;event=true;
 update_hub(&link);assert(s_hubDoorStarted&&s_hubDoorTicks==0&&s_run.phase==Phase::Ready);
 assert(!pause_run_actor(false));
 update_hub(&link);assert(orders==2); // An accepted event is not replayed.
 link.eventInfo.doorCommand=door.eventInfo.doorCommand=false;event=false;
 for(unsigned map=0;map<arenas.size();++map){
  s_hub=s_hubDoorStarted=s_hubDeparture=true;s_map=0;s_requestedMap=map;
  std::any stage[]={static_cast<const char*>("D_MN06"),s16(8),s8(9),s8(2)};
  assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_CONTINUE);
  assert(!s_hub&&s_arrivingRun&&!s_hubDoorStarted&&!s_hubDeparture&&s_map==map);
  assert(std::string(std::any_cast<const char*>(stage[0]))==arenas[map].stage);
  assert(std::any_cast<s16>(stage[1])==arenas[map].point&&std::any_cast<s8>(stage[2])==arenas[map].room);
  // No run snapshot, timer, starting item or mobs until the destination is ready.
  assert(s_run.phase==Phase::Ready&&s_run.ticks==0&&s_run.ranks[0]==0);
  assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 }
 // Unconfirmed hub exits and all combat doors stay blocked.
 s_hub=true;s_hubDoorStarted=s_hubDeparture=false;s_hubMenu=HubMenu::None;visible=false;
 std::any stage[]={static_cast<const char*>("F_SP108"),s16(9),s8(9),s8(9)};
 assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 s_hub=false;s_run.phase=Phase::Running;
 assert(before_hub_door(nullptr,request,&result,nullptr)==HOOK_SKIP_ORIGINAL&&!visible);
 // A failed native order remains retryable without teleporting.
 s_hub=true;s_run.phase=Phase::Ready;s_hubDoor=2;s_hubDeparture=true;acceptOrder=false;s_hubDoorTicks=0;
 for(int i=0;i<91;++i)update_hub(&link);
 assert(!s_hubDeparture&&!s_hubDoorStarted);
 s_hubDeparture=s_hubDoorStarted=true;s_hubDoorTicks=600;event=false;update_hub(&link);
 assert(!s_hubDeparture&&!s_hubDoorStarted);
 // A queued-but-never-accepted order and missing eligibility must both recover.
 acceptOrder=true;s_hubDeparture=true;
 for(int i=0;i<91;++i)update_hub(&link);
 assert(!s_hubDeparture&&!s_hubDoorStarted);
 s_hubDeparture=true;link.eventInfo.eligible=false;
 for(int i=0;i<91;++i)update_hub(&link);
 assert(!s_hubDeparture&&!s_hubDoorStarted);
 const auto previousOpened=opened;
 before_hub_door(nullptr,request,&result,nullptr);assert(opened==previousOpened+1);
 close_window();s_hubSaveRequested=false;
 // Label recognition includes knob/midboss doors and the opposite room assignment.
 s_hubDoor=kNone;
 for(int profile=1;profile<=8;++profile){
  door.name=profile;door.room=50;door.frontRoom=0;door.backRoom=51;
  assert(hub_exit_actor(&door,nullptr)==&door);
  door.backRoom=0;assert(!hub_exit_actor(&door,nullptr));
  door.frontRoom=51;assert(hub_exit_actor(&door,nullptr)==&door);
  door.name=99;assert(!hub_exit_actor(&door,nullptr));
 }
 door.name=fpcNm_L1MBOSS_DOOR_e;door.current.pos={10,20,30};
 assert(!draw_hub_exit_label(&door,nullptr)&&labels.size()==1&&labels[0].y==420);
 door.current.pos={40,50,60};assert(!draw_hub_exit_label(&door,nullptr)&&labels.size()==2);
 door.name=99;door.room=0;door.frontRoom=0;s_hubDoor=door.id;
 assert(hub_exit_actor(&door,nullptr)==&door); // Captured custom door.
 door.isActor=false;assert(!hub_exit_actor(&door,nullptr));door.isActor=true;
 // Missing door and settings/talent close cannot accidentally authorize departure.
 doorAlive=false;s_hubDeparture=true;update_hub(&link);assert(!s_hubDeparture);
 s_hubMenu=HubMenu::Talents;hub_menu_closed();assert(s_hubSaveRequested&&s_hubMenu==HubMenu::None);
 // Other saves' stage routing is untouched.
 s_active=false;assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_CONTINUE);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+'''
bool floorReady=true,wallReady=true;
struct dBgS_LinChk{void Set(cXyz* a,cXyz* b,void*){assert(a->x==0&&b->x==10000&&a->z==0&&b->z==0);}cXyz GetCross(){return {1800,100,0};}};
struct dBgS_ObjGndChk{void SetPos(cXyz*){}};
struct Ground{bool LineCross(dBgS_LinChk*){return wallReady;}float GroundCross(dBgS_ObjGndChk*){return floorReady?40:-1.e9f;}}ground;
Ground& dComIfG_Bgsp(){return ground;}
struct GearChest {bool active=false;cXyz position;s16 yaw=0;}chestActor;
void update_survivors_equipment_chest(bool enabled,const cXyz& p,s16 yaw){
 chestActor.active=enabled;if(enabled){chestActor.position=p;chestActor.yaw=yaw;}
}
'''+'\n'.join(extract(core,n) for n in ['map_available','hub_menu_closed','start_run','before_stage','pause_run_actor'])+'\n'+'\n'.join(extract(hub,n) for n in ['before_hub_door','hub_chest_position','update_hub','hub_exit_actor','draw_hub_exit_label'])+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),'-I',str(root/'include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors hub: free movement, native door gating, save/close, cancellation, routing and recovery passed')
