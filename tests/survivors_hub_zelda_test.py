"""Exercise the real hub Zelda graph, actor ownership, rewards and async lifecycle."""
from pathlib import Path
import os, re, subprocess, sys, tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'dusklight'
midna = (root / 'src/survivors_midna.inc').read_text()
register = midna[midna.index('mods::flow::RegisteredMessage register_survivors_dialog('):midna.index('ModResult initialize_midna_dialog()')]
fixture = r'''
#include <any>
#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include "mods/svc/flow.hpp"
#include "mods/svc/actor.h"
using u32=uint32_t;using u8=uint8_t;using u16=uint16_t;using s16=int16_t;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_NPC_ZELDA_e=100,fpcNm_Demo_Item_e=101,dItemNo_SWORD_e=0x28,dEvtCnd_CANGETITEM_e=1;
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
std::map<u16,FlowNodeData> nodes;std::map<u16,u16> edges;
std::map<MessageId,std::vector<std::string>> texts;
u16 nextNode=0x8000,nextEdge=0x8000,nextMessage=0x8000;
FlowEventFn rewardCallback=nullptr;
ModResult beginGraph(ModContext*,u16 group,FlowGraphHandle* out){assert(group==0);*out=1;return MOD_OK;}
ModResult allocNode(ModContext*,FlowGraphHandle,u16* out){*out=nextNode++;return MOD_OK;}
ModResult addEdges(ModContext*,FlowGraphHandle,const u16* targets,u16 count,u16* out){*out=nextEdge;for(unsigned i=0;i<count;++i)edges[nextEdge++]=targets[i];return MOD_OK;}
ModResult fillNode(ModContext*,FlowGraphHandle,u16 id,const FlowNodeData* node){nodes[id]=*node;return MOD_OK;}
ModResult commitGraph(ModContext*,FlowGraphHandle){return MOD_OK;}
ModResult removeGraph(ModContext*,FlowGraphHandle){return MOD_OK;}
ModResult registerEvent(ModContext*,const char* name,FlowEventFn fn,void*,FlowEventId* out){
 assert(std::string(name)=="Survivors Zelda Ordon Sword");rewardCallback=fn;*out=200;return MOD_OK;}
ModResult registerMessage(ModContext*,u16 group,const MessageVariantData* variants,size_t count,MessageId* id,MessageHandle* handle){
 assert(group==0&&count==6);*id=nextMessage++;*handle=*id;
 for(size_t i=0;i<count;++i)texts[*id].emplace_back(reinterpret_cast<const char*>(variants[i].text),variants[i].text_size);
 return MOD_OK;
}
ModResult removeMessage(ModContext*,MessageHandle){return MOD_OK;}
FlowService flowService{.begin_graph=beginGraph,.allocate_node=allocNode,.add_edges=addEdges,.fill_node=fillNode,
 .commit_graph=commitGraph,.remove_graph=removeGraph,.register_event=registerEvent};
MessageService messageService{.register_message=registerMessage,.remove_message=removeMessage};
extern "C" {ModContext* mod_ctx=nullptr;const FlowService* svc_flow=&flowService;const MessageService* svc_message=&messageService;}
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};

struct fopAc_ac_c {
 ActorId id=12;int name=fpcNm_NPC_ZELDA_e;
 struct{unsigned flags=3;}attention_info;
 struct{cXyz pos;}current;
 struct EventInfo {bool item=false;unsigned conditions=0;
 bool checkCommandItem(){return item;}void onCondition(unsigned c){conditions|=c;}}eventInfo;
};
struct dMsgFlow_c {bool finished=false;unsigned getEventId(int* n){*n=0;return 0;}bool checkEndFlow(){return finished;}};
struct daNpc_Zelda_c:fopAc_ac_c {
 dMsgFlow_c mFlow;u8 field_0xf80=0;int mMode=0,mFlowNodeNo=3001;
 bool mTwilight=false,field_0xe34=false,talkDone=false;int mAttnFovY=110;
 struct{bool groundNone=true,wallNone=true;void ClrGrndNone(){groundNone=false;}void ClrWallNone(){wallNone=false;}}mAcch;
 struct Motion {int no=-1,calls=0;void setNo(int n,float,int,int){no=n;++calls;}int getNo(){return no;}}mFaceMotionSeqMngr,mMotionSeqMngr;
 struct{bool checkStagger(){return false;}}mStagger;
 struct Manager {fopAc_ac_c* actor=nullptr;void remove(){actor=nullptr;}fopAc_ac_c* getActorP(){return actor;}void entry(fopAc_ac_c* a){actor=a;}}mPlayerActorMngr;
 struct{void lookPlayer(int){}void lookNone(int){}int getMode(){return 0;}}mJntAnm;
 struct Angle{s16 y=0;}mCurAngle;
 struct{Angle angle;}home;
 s16 mPlayerAngle=0,targetYaw=0;unsigned turns=0,talkInits=0;
 bool chkActorInSight(fopAc_ac_c*,int,s16){return true;}
 bool srchPlayerActor(){return false;}
 void setAngle(s16 yaw){mCurAngle.y=yaw;}
 bool step(s16 yaw,int,int,int,int){++turns;setAngle(yaw);return true;}
 void initTalk(int flow,void*){assert(flow==3001);++talkInits;mPlayerAngle=targetYaw;}
 bool talkProc(void*,int,void*,int){return talkDone;}
 int wait(void*);int talk(void*);
}zelda,other;
struct daAlink_c:fopAc_ac_c {
 enum{PROC_GET_ITEM=10,ANM_GET_A=20};int mProcID=0;unsigned animations=0;
 struct{int p0=777,p1=888;void setParam0(int n){p0=n;}void setParam1(int n){p1=n;}}mDemo;
 struct{int field_0x300a=-7;}mProcVar1;
 void setSingleAnimeBase(int n){assert(n==ANM_GET_A);++animations;}
}link;
fopAc_ac_c giftItem;
ActorId s_player=7;
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
fopAc_ac_c* daPy_getPlayerActorClass(){return &link;}
ActorId fopAcM_GetID(const fopAc_ac_c* actor){return actor->id;}
int fopAcM_GetName(const fopAc_ac_c* actor){return actor->name;}
int8_t fopAcM_GetRoomNo(const fopAc_ac_c*){return 51;}
bool active=true,transition=false,play=true,pedestal=true,creating=false,live=false,failSpawn=false;
bool giftCreating=false,giftLive=false,failGift=false,eventEnded=false,eventRunning=false;
bool s_hubDeparture=false,s_progressSaving=false,s_hubSaveRequested=false;
bool survivors_hub_active(){return active;}
bool in_arena(){return active;}
bool transitioning(){return transition;}
bool playable(){return active&&play&&!transition&&!eventRunning;}
unsigned spawnCalls=0,deleteCalls=0,groundCalls=0,rewardCalls=0,giftSpawns=0,giftDeletes=0,giftOrders=0,eventResets=0;
bool fpcM_IsCreating(ActorId id){return (id==zelda.id&&creating)||(id==55&&giftCreating);}
fopAc_ac_c* actor_by_id(ActorId id){
 if(id==zelda.id&&live&&!creating)return &zelda;
 if(id==55&&giftLive&&!giftCreating)return &giftItem;
 return nullptr;
}
ModResult deleteActor(ModContext*,ActorId id){if(id==55)++giftDeletes;else{assert(id==zelda.id);++deleteCalls;}return MOD_OK;}
ActorService actorService{.delete_actor=deleteActor};
extern "C" {const ActorService* svc_actor=&actorService;}
void execItemGet(int n){assert(n==dItemNo_SWORD_e);++rewardCalls;}
struct{ActorId mPt2=kNone;}nativeEvent;
auto* dComIfGp_getEvent(){return &nativeEvent;}
void dComIfGp_event_reset(){++eventResets;nativeEvent.mPt2=kNone;eventRunning=false;}
bool dComIfGp_evmng_endCheck(const char* n){assert(std::string(n)=="DEFAULT_GETITEM");return eventEnded;}
void dComIfGp_event_setGtItm(int n){assert(n==dItemNo_SWORD_e);}
void dComIfGp_event_setItemPartnerId(ActorId id){assert(id==55);}
void fopAcM_orderItemEvent(fopAc_ac_c* item,int,int){assert(item==&giftItem&&item->eventInfo.conditions==dEvtCnd_CANGETITEM_e);++giftOrders;}
bool survivors_pedestal_anchor(cXyz& anchor){anchor={0,-400,0};return pedestal;}
struct dBgS_ObjGndChk {cXyz pos;void SetPos(cXyz* p){pos=*p;}};
float floorHeight=-400;
struct Background {float GroundCross(dBgS_ObjGndChk* ground){++groundCalls;assert(ground->pos.x==-320&&ground->pos.z==0);return floorHeight;}}bg;
Background& dComIfG_Bgsp(){return bg;}
struct Arena {int8_t room=51;};Arena arenas[1];
ModResult create_standalone_actor(int profile,const ActorSpawnParams& p,ActorId& out){

 if(profile==fpcNm_Demo_Item_e){
  assert(p.parameters==(0x17f00u|dItemNo_SWORD_e)&&p.room_num==51&&p.create_function==nullptr);
  assert(p.position.x==link.current.pos.x&&p.position.y==link.current.pos.y&&p.position.z==link.current.pos.z);
  ++giftSpawns;if(failGift)return MOD_UNAVAILABLE;out=55;giftCreating=true;return MOD_OK;
 }
 assert(profile==fpcNm_NPC_ZELDA_e&&p.parameters==0xffffff01u&&p.room_num==51);
 assert(p.position.x==-320&&p.position.y==-400&&p.position.z==0&&p.angle.x==3001&&p.angle.y==0x4000);
 assert(p.scale.x==1&&p.scale.y==1&&p.scale.z==1&&p.create_function==nullptr);
 ++spawnCalls;if(failSpawn)return MOD_UNAVAILABLE;out=zelda.id;creating=true;return MOD_OK;
}
'''
main = r'''
u16 word(const FlowNodeData& n,unsigned offset){return u16(n.bytes[offset]<<8|n.bytes[offset+1]);}
int main(){
 link.id=s_player;giftItem.id=55;giftItem.name=fpcNm_Demo_Item_e;
 assert(initialize_hub_zelda()==MOD_OK&&nodes.size()==2);
 const auto text=nodes.at(s_hubZeldaStart);assert(text.bytes[0]==1);
 for(const auto& t:texts.at(word(text,2)))assert(t.find("It's dangerous to go alone! Take this.")!=t.npos);
 const auto gift=nodes.at(word(text,4));assert(gift.bytes[0]==3&&gift.bytes[1]==s_hubZeldaReward.id());
 assert(edges.at(word(gift,2))==mods::flow::kEnd); // Reward follows the text, never before it.
 active=false;update_hub_zelda();assert(!spawnCalls);
 active=true;play=false;update_hub_zelda();assert(!spawnCalls);play=true;
 s_progressSaving=true;update_hub_zelda();assert(!spawnCalls);s_progressSaving=false;
 s_hubSaveRequested=true;update_hub_zelda();assert(!spawnCalls);s_hubSaveRequested=false;
 pedestal=false;update_hub_zelda();assert(!spawnCalls&&!groundCalls);pedestal=true;
 for(float invalid:{-1.e9f,-100.f,std::nanf("")}){
  s_hubZeldaRetry=0;floorHeight=invalid;update_hub_zelda();assert(!spawnCalls);
 }
 floorHeight=-400;failSpawn=true;s_hubZeldaRetry=0;update_hub_zelda();assert(spawnCalls==1&&s_hubZelda==kNone);
 for(int i=0;i<30;++i){update_hub_zelda();}assert(spawnCalls==1);
 failSpawn=false;update_hub_zelda();assert(spawnCalls==2&&creating&&s_hubZelda==12);
 for(int i=0;i<60;++i){update_hub_zelda();}assert(spawnCalls==2); // Pending create never duplicates.

 std::any ownArgs[]={&zelda},otherArgs[]={&other};other.id=99;
 assert(before_hub_zelda_param(nullptr,ownArgs,nullptr,nullptr)==HOOK_CONTINUE);
 assert(zelda.field_0xf80==1&&!zelda.mAcch.groundNone&&!zelda.mAcch.wallNone);
 before_hub_zelda_param(nullptr,otherArgs,nullptr,nullptr);
 assert(other.field_0xf80==0&&other.mAcch.groundNone&&other.mAcch.wallNone);
 // Execute the SDK's actual wait/talk bodies, not a guessed animation mapping.
 zelda.wait(nullptr);assert(zelda.mMotionSeqMngr.no==0&&zelda.mMode==2&&zelda.turns==0);
 other.wait(nullptr);assert(other.mMotionSeqMngr.no==1); // Native throne-room idle stays unchanged.
 zelda.mMode=0;zelda.targetYaw=0x4000;
 zelda.talk(nullptr);assert(zelda.turns==1&&zelda.mCurAngle.y==0x4000&&zelda.talkInits==1);
 zelda.talkDone=true;zelda.mFlow.finished=true;zelda.talk(nullptr);assert(zelda.mMode==3);
 zelda.mMode=0;zelda.wait(nullptr);assert(zelda.mMotionSeqMngr.no==0);
 creating=false;live=true;update_hub_zelda();assert(spawnCalls==2);
 for(int i=0;i<60;++i){update_hub_zelda();}assert(spawnCalls==2&&groundCalls==5); // No per-frame placement rays.
 std::any flowArgs[]={&zelda.mFlow,u16(3001)},otherFlow[]={&other.mFlow,u16(3001)};
 u16 entry=0;
 FlowEventContext event{.speaker_actor=&zelda},foreign{.speaker_actor=&other};
 rewardCallback(nullptr,&event,nullptr);assert(!rewardCalls);
 assert(before_hub_zelda_flow(nullptr,otherFlow,&entry,nullptr)==HOOK_CONTINUE);
 flowArgs[1]=u16(42);assert(before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr)==HOOK_CONTINUE);flowArgs[1]=u16(3001);
 assert(before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr)==HOOK_SKIP_ORIGINAL&&entry==s_hubZeldaStart);
 assert(!rewardCalls);rewardCallback(nullptr,&foreign,nullptr);rewardCallback(nullptr,nullptr,nullptr);assert(!rewardCalls);

 // Queue only after text; wait for the native conversation/event to finish.
 eventRunning=true;rewardCallback(nullptr,&event,nullptr);rewardCallback(nullptr,&event,nullptr);
 update_hub_zelda_gift();assert(!giftSpawns&&!rewardCalls&&s_hubZeldaGift.queued);
 eventRunning=false;update_hub_zelda_gift();assert(giftSpawns==1&&giftCreating&&!rewardCalls);
 for(int i=0;i<60;++i){update_hub_zelda_gift();}assert(giftSpawns==1);
 giftCreating=false;giftLive=true;update_hub_zelda_gift();assert(giftOrders==1&&!rewardCalls);
 std::any linkArgs[]={&link};
 before_hub_zelda_get(nullptr,linkArgs,nullptr,nullptr);assert(link.mDemo.p0==777); // Unaccepted event.
 nativeEvent.mPt2=55;eventRunning=true;giftItem.eventInfo.item=true;
 before_hub_zelda_get(nullptr,linkArgs,nullptr,nullptr);
 assert(link.mDemo.p0==0&&link.mDemo.p1==0&&s_hubZeldaGift.started);
 after_hub_zelda_get(nullptr,linkArgs,nullptr,nullptr);assert(!link.animations); // Native sheathing first.
 link.mProcID=daAlink_c::PROC_GET_ITEM;
 after_hub_zelda_get(nullptr,linkArgs,nullptr,nullptr);after_hub_zelda_get(nullptr,linkArgs,nullptr,nullptr);
 assert(link.animations==1&&link.mProcVar1.field_0x300a==0); // Full raise-item pose, once.
 update_hub_zelda_gift();assert(!rewardCalls);eventEnded=true;
 update_hub_zelda_gift();assert(rewardCalls==1&&s_hubZeldaGift.retiring);
 update_hub_zelda_gift();assert(rewardCalls==1);giftLive=false;update_hub_zelda_gift();assert(!hub_zelda_gift_busy());
 // A repeated conversation may present the same ordinary sword again, never rupees or a run upgrade.
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);rewardCallback(nullptr,&event,nullptr);
 failGift=true;update_hub_zelda_gift();assert(!hub_zelda_gift_busy()&&rewardCalls==1);failGift=false;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);rewardCallback(nullptr,&event,nullptr);
 update_hub_zelda_gift();giftCreating=false;giftLive=true;
 nativeEvent.mPt2=55;eventEnded=false;eventRunning=true;update_hub_zelda_gift();assert(s_hubZeldaGift.started);
 // Losing ownership/cancellation must not award a sword or reset the replacement event.
 nativeEvent.mPt2=99;const auto resets=eventResets;
 update_hub_zelda_gift();assert(rewardCalls==1&&eventResets==resets);
 giftLive=false;cleanup_hub_zelda_gift();nativeEvent.mPt2=kNone;eventRunning=false;
 // Timeout an unaccepted request; menus do not spend the retry budget.
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);rewardCallback(nullptr,&event,nullptr);
 update_hub_zelda_gift();giftCreating=false;giftLive=true;giftItem.eventInfo.item=false;
 play=false;for(int i=0;i<400;++i){update_hub_zelda_gift();}assert(s_hubZeldaGift.waitFrames==0);play=true;
 for(int i=0;i<301;++i){update_hub_zelda_gift();}assert(s_hubZeldaGift.retiring&&rewardCalls==1);
 giftLive=false;cleanup_hub_zelda_gift();
 play=false;after_hub_zelda_param(nullptr,ownArgs,nullptr,nullptr);assert(!zelda.attention_info.flags);
 after_hub_zelda_param(nullptr,otherArgs,nullptr,nullptr);assert(other.attention_info.flags==3);play=true;
 zelda.attention_info.flags=3;after_hub_zelda_param(nullptr,ownArgs,nullptr,nullptr);assert(zelda.attention_info.flags==3);
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);s_hubDeparture=true;
 rewardCallback(nullptr,&event,nullptr);assert(rewardCalls==1);
 update_hub_zelda();assert(deleteCalls==1&&s_hubZelda==12&&!s_hubZeldaRewardPending);
 s_hubDeparture=false;update_hub_zelda();assert(deleteCalls==2&&spawnCalls==2); // Wait for actual delete.
 live=false;update_hub_zelda();assert(spawnCalls==3&&creating);
 // Depart during resource loading. Keep ownership through create, then delete.
 transition=true;update_hub_zelda();assert(s_hubZelda==12&&s_hubZeldaRetiring&&deleteCalls==2);
 transition=false;update_hub_zelda();assert(spawnCalls==3&&s_hubZeldaRetiring);
 creating=false;live=true;update_hub_zelda();assert(deleteCalls==3&&spawnCalls==3);
 live=false;update_hub_zelda();assert(spawnCalls==4&&creating);
 active=false;update_hub_zelda();creating=false;live=true;update_hub_zelda();assert(deleteCalls==4);
 live=false;update_hub_zelda();assert(s_hubZelda==kNone&&spawnCalls==4);

 assert(before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr)==HOOK_CONTINUE);
 // Exit/reset while the presentation actor is still being created keeps the ID for cleanup.
 s_hubZeldaGift={};s_hubZeldaGift.id=55;giftCreating=true;
 retire_hub_zelda_gift();assert(s_hubZeldaGift.id==55&&s_hubZeldaGift.retiring);
 giftCreating=false;giftLive=true;const auto deletes=giftDeletes;cleanup_hub_zelda_gift();assert(giftDeletes==deletes+1);
 giftLive=false;cleanup_hub_zelda_gift();assert(!hub_zelda_gift_busy()&&rewardCalls==1);
 std::cout<<"Survivors Zelda: standing/turning, sword presentation/pose, deferred grant, event isolation and cleanup passed\n";
}
'''
# Type 1 must remain a valid native resource/heap/standing-animation path.
native = (sdk/'src/d/actor/d_a_npc_zelda.cpp').read_text()
assert 'heapSize[2]' in native and 'l_loadResPtrnList[2]' in native
assert 'mMotionSeqMngr.setNo(0,' in native and 'initTalk(mFlowNodeNo, NULL)' in native
core = (root/'src/survivors.cpp').read_text()
assert 'PRE(SurvivorsMidnaFlow, before_hub_zelda_flow);' in core
assert 'PRE(SurvivorsHubZeldaParam, before_hub_zelda_param);' in core
assert 'POST(SurvivorsGetItem, after_hub_zelda_get);' in core
assert 'POST(SurvivorsHubZeldaParam, after_hub_zelda_param);' in core
assert 'void reset_runtime() {\n    retire_hub_zelda();' in core
assert 'void update_survivors() {\n    update_hub_zelda();' in core
def extract_native(name):
    start=native.index('int daNpc_Zelda_c::'+name+'(')
    brace=native.index('{',start);depth=0
    for end in range(brace,len(native)):
        depth+=(native[end]=='{')-(native[end]=='}')
        if not depth:return native[start:end+1]
    raise AssertionError(name)
native_actions=extract_native('wait')+extract_native('talk')
with tempfile.TemporaryDirectory() as temp:
    cpp,binary=Path(temp)/'test.cpp',Path(temp)/'test'
    cpp.write_text(fixture+native_actions+register+(root/'src/survivors_hub_zelda.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-unused-parameter','-Wno-unused-variable','-Wno-implicit-fallthrough','-Wno-missing-field-initializers',
        '-I',str(sdk/'sdk/include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
