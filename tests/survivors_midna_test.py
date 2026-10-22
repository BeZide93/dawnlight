"""Native SDK flow graph, Midna input ownership and deferred run termination."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1]) if len(sys.argv)>1 else root/'dusklight'
core=(root/'src/survivors.cpp').read_text()
def extract(name):
    match=re.search(r'^(?:bool|void) '+name+r'\([^;{}]*\)\s*\{',core,re.M)
    assert match,name
    start=core.index('{',match.start()); depth=0
    for end in range(start,len(core)):
        depth+=(core[end]=='{')-(core[end]=='}')
        if not depth:return core[match.start():end+1]
    raise AssertionError(name)
fixture=r'''
bool s_victory=false,s_deathPending=false;

#include <any>
#include <cassert>
#include <map>
#include <string>
#include <iostream>
#include "survivors_rules.hpp"
#include "../include/survivors_maps.hpp"
#include "mods/svc/flow.hpp"
using namespace dawnlight::survivors;
using u16=uint16_t;
std::map<u16,FlowNodeData> nodes;std::map<u16,u16> edges;
std::map<MessageId,std::vector<std::string>> texts;
u16 nextNode=0x8000,nextEdge=0x8000,nextMessage=0x8000;
FlowEventFn endEvent=nullptr,restartEvent=nullptr;
ModResult begin_graph(ModContext*,u16 group,FlowGraphHandle* out){assert(group==0);*out=1;return MOD_OK;}
ModResult allocate_node(ModContext*,FlowGraphHandle,u16* out){*out=nextNode++;return MOD_OK;}
ModResult add_edges(ModContext*,FlowGraphHandle,const u16* targets,u16 count,u16* out){*out=nextEdge;for(unsigned i=0;i<count;++i)edges[nextEdge++]=targets[i];return MOD_OK;}
ModResult fill_node(ModContext*,FlowGraphHandle,u16 id,const FlowNodeData* data){nodes[id]=*data;return MOD_OK;}
ModResult commit_graph(ModContext*,FlowGraphHandle){return MOD_OK;}
ModResult remove_graph(ModContext*,FlowGraphHandle){return MOD_OK;}
ModResult register_event(ModContext*,const char* name,FlowEventFn fn,void*,FlowEventId* out){
 if(std::string(name)=="Survivors End Run"){endEvent=fn;*out=200;}
 else {assert(std::string(name)=="Survivors Restart Run");restartEvent=fn;*out=201;}return MOD_OK;}
ModResult register_message(ModContext*,u16 group,const MessageVariantData* v,size_t count,MessageId* id,MessageHandle* handle){
 assert(group==0&&count==6);*id=nextMessage++;*handle=*id;
 for(size_t i=0;i<count;++i)texts[*id].emplace_back(reinterpret_cast<const char*>(v[i].text),v[i].text_size);
 return MOD_OK;
}
ModResult remove_message(ModContext*,MessageHandle){return MOD_OK;}
FlowService flowService{.begin_graph=begin_graph,.allocate_node=allocate_node,.add_edges=add_edges,.fill_node=fill_node,
 .commit_graph=commit_graph,.remove_graph=remove_graph,.register_event=register_event};
MessageService messageService{.register_message=register_message,.remove_message=remove_message};
extern "C"{ModContext* mod_ctx=nullptr;const FlowService* svc_flow=&flowService;const MessageService* svc_message=&messageService;}
struct dMsgFlow_c{};
struct Collider{bool active=true,hit=true;void OffAtSetBit(){active=false;}void ResetAtHit(){hit=false;}};
struct daAlink_c{
 enum{UPPER_2};unsigned id=7;bool dead=false,ride=true,trigger=false;unsigned calls=0,waits=0,upper=0;
 struct{bool hit=true;bool ChkGroundHit(){return hit;}}mLinkAcch;
 std::array<Collider,3>mAtCps;Collider mAtSph;int field_0x35a0=0,field_0x3594=123;
 bool checkDeadHP(){return dead;}bool checkMidnaRide(){return ride;}bool midnaTalkTrigger(){++calls;return trigger;}
 void resetUpperAnime(int,int){++upper;}void procWaitInit(){++waits;}
}link;
struct daMidna_c{dMsgFlow_c mMsgFlow;struct{bool talk=false;bool checkCommandTalk(){return talk;}}eventInfo;}midna;
struct daPy_py_c{static daMidna_c* getMidnaActor(){return &midna;}};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}}
bool gearOffer=false;unsigned gearEquips=0;
bool gear_notice_equip_available(){return gearOffer;}
void equip_gear_notice(){++gearEquips;gearOffer=false;}
unsigned fopAcM_GetID(daAlink_c* a){return a->id;}
unsigned s_player=7,s_midnaRequestFrames=0;bool s_midnaTalking=false,s_midnaEndPending=false,s_midnaRestartPending=false;
bool s_hub=false,s_hubDeparture=false,s_hubDoorStarted=false;Run s_run;bool arena=true,transition=false,pause=false,event=false,message=false,ui=false;
unsigned s_map=0,s_requestedMap=0,closed=0;bool s_restart=false,s_arrivingRun=false;
void close_window(){++closed;}
unsigned map=0;std::string stage=arenas[0].stage;int room=51;
bool save_modal_active(){return false;}
bool in_arena(){return arena&&arena_matches(map,stage,room);}
bool transitioning(){return transition;}
bool dComIfGp_isPauseFlag(){return pause;}struct dScnPly_c{static bool isPause(){return false;}};
bool dComIfGp_event_runCheck(){return event;}bool dMsgObject_isTalkNowCheck(){return message;}
bool ui_visible(){return ui;}int dMeter2Info_getGameOverType(){return 0;}
enum class SwordAction{None,Normal};SwordAction s_swordAction=SwordAction::Normal;
bool s_wavePending=true;unsigned s_comboStep=2,s_comboTimeout=5,cleared=0,ordered=0,ended=0;
void clear_attacks(){++cleared;}void finish(bool won){assert(!won);++ended;s_run.end(won);}
constexpr int METER2_USEBUTTON_Z=1;void dMeter2Info_onUseButton(int){}
template<class... T>void dComIfGp_setMesgCameraInfoActor(T...){}
void fopAcM_orderTalkEvent(daAlink_c* a,daMidna_c* b,int,int){assert(a==&link&&b==&midna);++ordered;}
'''
main=r'''
u16 word(const FlowNodeData& n,unsigned offset){return u16(n.bytes[offset]<<8|n.bytes[offset+1]);}
int main(){
 assert(initialize_midna_dialog()==MOD_OK&&nodes.size()==6&&s_midnaStart>=0x8000);
 auto setup=nodes.at(s_midnaStart);assert(setup.bytes[0]==3&&setup.bytes[1]==FLOW_EVENT_SELECT_VERTICAL&&setup.bytes[7]==4);
 auto prompt=nodes.at(edges.at(word(setup,2)));assert(prompt.bytes[0]==1);
 auto options=nodes.at(word(prompt,4));assert(options.bytes[0]==1);
 for(auto& text:texts.at(word(options,2))){assert(text.find("Cancel")!=text.npos&&text.find("End Run")!=text.npos&&text.find("Restart Run")!=text.npos);}
 auto choice=nodes.at(word(options,4));assert(choice.bytes[0]==2&&choice.bytes[1]==4&&word(choice,2)==FLOW_QUERY_SELECT_3_CANCEL);
 auto first=word(choice,6);assert(edges.at(first)==mods::flow::kEnd&&edges.at(first+3)==mods::flow::kEnd);
 auto end=nodes.at(edges.at(first+1));assert(end.bytes[0]==3&&end.bytes[1]==s_midnaEndEvent.id());
 assert(edges.at(word(end,2))==mods::flow::kEnd);
 auto restart=nodes.at(edges.at(first+2));assert(restart.bytes[0]==3&&restart.bytes[1]==s_midnaRestartEvent.id());
 assert(edges.at(word(restart,2))==mods::flow::kEnd);
 std::any input[]={&link};std::any flowArgs[]={&midna.mMsgFlow,u16(0xbb9)};
 // All maps including the native throne-room prohibition; current automatic
 // combat is interrupted and the same native trigger supports touch/custom binds.
 for(map=0;map<arenas.size();++map){
  stage=arenas[map].stage;room=arenas[map].room;s_run.start(1);reset_midna_dialog();link.trigger=true;
  unsigned orders=ordered;before_midna_call(nullptr,input,nullptr,nullptr);
  assert(ordered==orders+1&&midna_dialog_busy()&&!playable());
  assert(s_swordAction==SwordAction::None&&!s_wavePending&&s_comboStep==0&&s_comboTimeout==0);
  assert(!link.mAtSph.active&&!link.mAtCps[0].active&&link.field_0x35a0==123);
  assert(pause_run_actor(true)&&!pause_run_actor(false));
  before_midna_call(nullptr,input,nullptr,nullptr);assert(ordered==orders+1);
  u16 node=0;assert(before_midna_flow(nullptr,flowArgs,&node,nullptr)==HOOK_SKIP_ORIGINAL&&node==s_midnaStart);
  assert(s_midnaTalking&&!s_midnaRequestFrames);
  // Cancel: keep the run and stats. Native talk/message must finish first.
  event=message=true;midna.eventInfo.talk=true;update_midna_dialog();assert(midna_dialog_busy());
  event=message=false;update_midna_dialog();assert(midna_dialog_busy());
  midna.eventInfo.talk=false;update_midna_dialog();assert(!midna_dialog_busy()&&s_run.phase==Phase::Running&&ended==0);
 }
 map=0;stage=arenas[0].stage;room=51;s_run.start(1);
 // Midna equips the advertised drop without opening a dialogue, including
 // final-boss loot time and airborne players. One trigger handles one banner.
 reset_midna_dialog();s_victory=true;link.mLinkAcch.hit=false;gearOffer=true;
 const auto ordersBeforeGear=ordered;before_midna_call(nullptr,input,nullptr,nullptr);
 assert(gearEquips==1&&!gearOffer&&ordered==ordersBeforeGear&&!midna_dialog_busy());
 before_midna_call(nullptr,input,nullptr,nullptr);assert(gearEquips==1);
 gearOffer=true;ui=true;before_midna_call(nullptr,input,nullptr,nullptr);assert(gearEquips==1&&gearOffer);
 ui=false;gearOffer=false;s_victory=false;link.mLinkAcch.hit=true;
 // End Run commits only once, after native closure; no re-entry during messages.
 u16 node=0;before_midna_flow(nullptr,flowArgs,&node,nullptr);event=message=true;
 endEvent(nullptr,nullptr,nullptr);update_midna_dialog();assert(ended==0&&s_midnaEndPending);
 event=false;update_midna_dialog();assert(ended==0);
 message=false;update_midna_dialog();assert(ended==1&&s_run.phase==Phase::Defeat);
 update_midna_dialog();assert(ended==1);
 // Stage/actor/phase isolation and failed request recovery.
 for(int reason=0;reason<8;++reason){
  reset_midna_dialog();s_run.start(1);arena=reason!=0;transition=reason==1;pause=reason==2;event=reason==3;
  ui=reason==4;link.dead=reason==5;link.mLinkAcch.hit=reason!=6;
  if(reason==7)s_run.phase=Phase::TreasureChoice;
  unsigned orders=ordered;before_midna_call(nullptr,input,nullptr,nullptr);assert(ordered==orders&&!midna_dialog_busy());
 }
 arena=true;transition=pause=event=ui=link.dead=false;link.mLinkAcch.hit=true;s_run.start(1);
 before_midna_call(nullptr,input,nullptr,nullptr);for(int i=0;i<30;++i)update_midna_dialog();assert(!midna_dialog_busy());
 dMsgFlow_c other;std::any otherFlow[]={&other,u16(0xbb9)};
 assert(before_midna_flow(nullptr,otherFlow,&node,nullptr)==HOOK_CONTINUE);
 flowArgs[1]=u16(100);assert(before_midna_flow(nullptr,flowArgs,&node,nullptr)==HOOK_CONTINUE);flowArgs[1]=u16(0xbb9);
 arena=false;int orderResult=99;assert(before_midna_order(nullptr,nullptr,&orderResult,nullptr)==HOOK_CONTINUE&&orderResult==99);
 assert(before_midna_flow(nullptr,flowArgs,&node,nullptr)==HOOK_CONTINUE);
 arena=true;assert(before_midna_order(nullptr,nullptr,&orderResult,nullptr)==HOOK_SKIP_ORIGINAL&&orderResult==0);
 before_midna_flow(nullptr,flowArgs,&node,nullptr);endEvent(nullptr,nullptr,nullptr);transition=true;update_midna_dialog();
 assert(!midna_dialog_busy()&&ended==1); // No pending End Run leaks into a new scene.
 // Restart waits for native closure, credits once and requests direct same-map startup.
 for(map=0;map<arenas.size();++map) {
  stage=arenas[map].stage;room=arenas[map].room;s_map=map;s_requestedMap=99;s_restart=false;
  transition=false;s_run.start(7);reset_midna_dialog();const auto before=ended,closures=closed;
  restartEvent(nullptr,nullptr,nullptr);assert(!s_midnaRestartPending); // No active conversation.
  before_midna_flow(nullptr,flowArgs,&node,nullptr);event=message=true;midna.eventInfo.talk=true;
  restartEvent(nullptr,nullptr,nullptr);restartEvent(nullptr,nullptr,nullptr);
  endEvent(nullptr,nullptr,nullptr);assert(s_midnaRestartPending&&!s_midnaEndPending);
  update_midna_dialog();assert(!s_restart&&ended==before);
  event=message=false;update_midna_dialog();assert(!s_restart&&ended==before);
  midna.eventInfo.talk=false;update_midna_dialog();
  assert(s_restart&&s_arrivingRun&&s_requestedMap==map&&ended==before+1&&closed==closures+1&&s_run.phase==Phase::Defeat);
  assert(!midna_dialog_busy()&&!s_midnaRestartPending);
  update_midna_dialog();assert(ended==before+1&&closed==closures+1);
 }
 s_restart=false;s_run.start(7);before_midna_flow(nullptr,flowArgs,&node,nullptr);
 restartEvent(nullptr,nullptr,nullptr);transition=true;const auto before=ended;update_midna_dialog();
 assert(!s_restart&&!midna_dialog_busy()&&!s_midnaRestartPending&&ended==before);
 std::cout<<"Survivors Midna: native graph, Cancel/End/Restart/Back, all maps, combat pause, input and lifecycle passed\n";
}
'''
# Ensure native initialization still loads group 0 before the entry-node hook.
native=(sdk/'src/d/actor/d_a_midna.cpp').read_text()
assert 'mMsgFlow.init(this, 0xbb9, 0, NULL)' in native
assert 'dComIfGs_onEventBit(dSv_event_flag_c::M_067)' in core
assert core.index('PRE(SurvivorsPlayer, before_chest_action)')<core.index('PRE(SurvivorsPlayer, before_midna_call)')
functions='\n'.join(extract(n) for n in ['midna_dialog_busy','reset_midna_dialog','playable','pause_run_actor','replay','restart_run'])
with tempfile.TemporaryDirectory() as tmp:
    cpp,binary=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture+functions+(root/'src/survivors_midna.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-missing-field-initializers',
        '-I',str(root/'src'),'-I',str(sdk/'sdk/include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
