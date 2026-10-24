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
constexpr int fpcNm_NPC_ZELDA_e=100,fpcNm_Demo_Item_e=101,dItemNo_SWORD_e=0x28,dEvtCnd_CANGETITEM_e=1,dEvtCnd_CANTALK_e=2;
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
std::map<u16,FlowNodeData> nodes;std::map<u16,u16> edges;
std::map<MessageId,std::vector<std::string>> texts;
std::map<MessageId,unsigned> cameraAttrs;
u16 nextNode=0x8000,nextEdge=0x8000,nextMessage=0x8000;
FlowEventFn rewardCallback=nullptr,talentsCallback=nullptr;
ModResult beginGraph(ModContext*,u16 group,FlowGraphHandle* out){assert(group==0);*out=1;return MOD_OK;}
ModResult allocNode(ModContext*,FlowGraphHandle,u16* out){*out=nextNode++;return MOD_OK;}
ModResult addEdges(ModContext*,FlowGraphHandle,const u16* targets,u16 count,u16* out){*out=nextEdge;for(unsigned i=0;i<count;++i)edges[nextEdge++]=targets[i];return MOD_OK;}
ModResult fillNode(ModContext*,FlowGraphHandle,u16 id,const FlowNodeData* node){nodes[id]=*node;return MOD_OK;}
ModResult commitGraph(ModContext*,FlowGraphHandle){return MOD_OK;}
ModResult removeGraph(ModContext*,FlowGraphHandle){return MOD_OK;}
ModResult registerEvent(ModContext*,const char* name,FlowEventFn fn,void*,FlowEventId* out){
 if(std::string(name)=="Survivors Zelda Talents"){talentsCallback=fn;*out=201;}
 else {assert(std::string(name)=="Survivors Zelda Ordon Sword");rewardCallback=fn;*out=200;}
 return MOD_OK;}
ModResult registerMessage(ModContext*,u16 group,const MessageVariantData* variants,size_t count,MessageId* id,MessageHandle* handle){
 assert(group==0&&count==6);*id=nextMessage++;*handle=*id;
 cameraAttrs[*id]=variants[0].entry.bytes[15];
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
 bool checkCommandItem(){return item;}void onCondition(unsigned c){conditions|=c;}bool chkCondition(unsigned c){return conditions&c;}}eventInfo;
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
using leafdraw_class=fopAc_ac_c;
constexpr int J3DShpFlag_Visible=1;
struct Shape {bool hidden=true;bool checkFlag(int){return hidden;}void show(){hidden=false;}void hide(){hidden=true;}};
struct Material {Shape shape;Shape* getShape(){return &shape;}};
struct ModelData {Material material;Material* getMaterialNodePointer(int i){assert(i==0);return &material;}};
using MtxP=int;
struct J3DModel {MtxP getAnmMtx(int joint){return 100+joint;}ModelData data;int matrix=0;ModelData* getModelData(){return &data;}
 void setBaseTRMtx(int m){matrix=m;}};
struct daPy_frameCtrl_c {
 float frame=0,rate=1;int end=50;bool ended=false,now=false;
 float getFrame(){return frame;}int getEnd(){return end;}float getRate(){return rate;}
 bool getEndFlg(){return ended;}bool getNowSetFlg(){return now;}
 void onEndFlg(){ended=true;}void onNowSetFlg(){now=true;}
 void setFrame(float f){frame=f;}void setRate(float r){rate=r;}void offEndFlg(){ended=now=false;}
 bool checkAnmEnd();
};
struct daAlink_c:fopAc_ac_c {
 enum{PROC_GET_ITEM=10,ANM_GET_MASTER_SWORD=20};int mProcID=0,animation=-1;unsigned animations=0,calculations=0;
 daPy_frameCtrl_c mUnderFrameCtrl[1],mUpperFrameCtrl[1];
 J3DModel ordon,master;J3DModel* mpSwAModel=&ordon;J3DModel* mSwordModel=&ordon;
 bool swordDraw=true;
 bool checkUnderMove0BckNoArc(int n){return animation==n;}
 bool checkSwordDraw(){return swordDraw;}
 J3DModel body;J3DModel* mpLinkModel=&body;
 int mLeftHandJntNo=9,mLeftItemJntNo=10;
 MtxP getLeftHandMatrix();MtxP getLeftItemMatrix();
 void modelCalc(J3DModel* m){assert(m==mpSwAModel);++calculations;}
 struct{int p0=777,p1=888;void setParam0(int n){p0=n;}void setParam1(int n){p1=n;}}mDemo;
 struct{int field_0x300a=-7;}mProcVar1;
 void setSingleAnimeBase(int n){assert(n==ANM_GET_MASTER_SWORD);animation=n;++animations;}
}link;
fopAc_ac_c giftItem;
bool swordReceived=false,msgTalking=false;unsigned talentOpens=0;
bool dMsgObject_isTalkNowCheck(){return msgTalking;}
void survivors_open_talents(){++talentOpens;}
bool dComIfGs_isItemFirstBit(int item){assert(item==dItemNo_SWORD_e);return swordReceived;}
struct dKy_tevstr_c {};
struct Environment {void settingTevStruct(int,cXyz*,dKy_tevstr_c*){}
 void setLightTevColorType_MAJI(J3DModel*,dKy_tevstr_c*){}}g_env_light;
unsigned presentationDraws=0;
void mDoExt_modelUpdateDL(J3DModel* model){assert(model==link.mpSwAModel);++presentationDraws;}
unsigned talkOrders=0;
void fopAcM_orderTalkEvent(fopAc_ac_c* a,fopAc_ac_c* b,int,int){assert(a==&link&&b==&zelda);++talkOrders;}
fopAc_ac_c* cameraSpeaker=nullptr;
void dComIfGp_setMesgCameraInfoActor(fopAc_ac_c* a,fopAc_ac_c*,fopAc_ac_c*,fopAc_ac_c*,fopAc_ac_c*,
 fopAc_ac_c*,fopAc_ac_c*,fopAc_ac_c*,fopAc_ac_c*,fopAc_ac_c*){cameraSpeaker=a;}
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
void execItemGet(int n){assert(n==dItemNo_SWORD_e);swordReceived=true;++rewardCalls;}
struct{ActorId mPt2=kNone;}nativeEvent;
auto* dComIfGp_getEvent(){return &nativeEvent;}
bool dComIfGp_event_runCheck(){return eventRunning;}
void dComIfGp_event_reset(){++eventResets;nativeEvent.mPt2=kNone;eventRunning=false;}
bool dComIfGp_evmng_endCheck(const char* n){assert(std::string(n)=="DEFAULT_GETITEM");return eventEnded;}
void dComIfGp_event_setGtItm(int n){assert(n==dItemNo_SWORD_e);}
void dComIfGp_event_setItemPartnerId(ActorId id){assert(id==55);}
void fopAcM_orderItemEvent(fopAc_ac_c* item,int,int){assert(item==&giftItem&&item->eventInfo.conditions==dEvtCnd_CANGETITEM_e);++giftOrders;}
bool hub_brazier_anchor(cXyz& anchor){anchor={0,-400,0};return pedestal;}
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
 assert(initialize_hub_zelda()==MOD_OK&&nodes.size()==6);
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
 // Exit reminder is its own native talk graph and camera target, with no gift.
 const auto call=nodes.at(s_hubZeldaCallStart);assert(call.bytes[0]==1&&word(call,4)==mods::flow::kEnd);
 assert(cameraAttrs.at(word(call,2))==1); // Native camera focuses actor slot 1 (Zelda).
 for(const auto& t:texts.at(word(call,2)))assert(t.find("Wait! Come here!")!=t.npos);
 request_hub_zelda_call();request_hub_zelda_call();assert((s_hubZeldaPendingTalk!=HubZeldaTalk::None));
 update_hub_zelda_call();assert(!talkOrders); // Wait for native eligibility.
 link.eventInfo.conditions=zelda.eventInfo.conditions=dEvtCnd_CANTALK_e;
 update_hub_zelda_call();assert(talkOrders==1);
 nativeEvent.mPt2=zelda.id;eventRunning=true;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);
 assert(entry==s_hubZeldaCallStart&&(s_hubZeldaActiveTalk!=HubZeldaTalk::None)&&(s_hubZeldaPendingTalk==HubZeldaTalk::None)&&cameraSpeaker==&zelda);
 rewardCallback(nullptr,&event,nullptr);assert(!hub_zelda_gift_busy()&&!rewardCalls);
 update_hub_zelda_call();assert((s_hubZeldaActiveTalk!=HubZeldaTalk::None));
 nativeEvent.mPt2=99;const auto callResets=eventResets;update_hub_zelda_call();
 assert((s_hubZeldaActiveTalk==HubZeldaTalk::None)&&eventResets==callResets); // Foreign events are untouched.
 eventRunning=false;nativeEvent.mPt2=kNone;
 request_hub_zelda_call();play=false;
 for(int i=0;i<120;++i){update_hub_zelda_call();}assert(s_hubZeldaCallFrames==0);
 play=true;for(int i=0;i<91;++i){update_hub_zelda_call();}assert((s_hubZeldaPendingTalk==HubZeldaTalk::None));
 swordReceived=true;request_hub_zelda_call();assert((s_hubZeldaPendingTalk==HubZeldaTalk::None));swordReceived=false;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);assert(entry==s_hubZeldaStart);
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
 // The equipped sword follows the weapon grip; only this gift loses its floating mesh.
 std::any itemDraw[]={static_cast<leafdraw_class*>(&giftItem)},foreignDraw[]={static_cast<leafdraw_class*>(&other)};
 int drawResult=0;
 assert(before_hub_zelda_gift_draw(nullptr,itemDraw,&drawResult,nullptr)==HOOK_CONTINUE);
 after_hub_zelda_item_matrix(nullptr,linkArgs,nullptr,nullptr);
 // Run the SDK's real hand/item accessors: the wrist and weapon grip are
 // different joints. Attaching to the wrist misaligns the sword in GETSWL.
 assert(link.getLeftHandMatrix()!=link.getLeftItemMatrix());
 assert(link.ordon.matrix==link.getLeftItemMatrix()&&!link.ordon.data.material.shape.hidden&&link.calculations==1);
 assert(before_hub_zelda_gift_draw(nullptr,itemDraw,&drawResult,nullptr)==HOOK_SKIP_ORIGINAL&&drawResult==1);
 assert(before_hub_zelda_gift_draw(nullptr,foreignDraw,&drawResult,nullptr)==HOOK_CONTINUE);
 auto& lower=link.mUnderFrameCtrl[0];auto& upper=link.mUpperFrameCtrl[0];
 lower.frame=20;upper.frame=20;
 before_hub_zelda_get_tick(nullptr,linkArgs,nullptr,nullptr);
 assert(lower.rate==1&&upper.rate==1&&link.mProcVar1.field_0x300a==0); // Raise plays first.
 lower.frame=49;upper.frame=49;
 for(int i=0;i<120;++i){
  before_hub_zelda_get_tick(nullptr,linkArgs,nullptr,nullptr);
  assert(lower.frame==49&&upper.frame==49&&lower.rate==0&&upper.rate==0);
  // Execute the SDK's real zero-rate/end-flag logic. The native item proc
  // must take its message branch, never replace GETSWL with GET_A_WAIT.
  assert(!lower.checkAnmEnd()&&link.mProcVar1.field_0x300a==1);
  lower.now=false; // Native updateFrame clears the just-set marker next tick.
 }
 assert(link.animations==1);
 // First-time gifts render the hand model without prematurely granting it.
 assert(!swordReceived&&presentationDraws==0);
 link.swordDraw=false;after_hub_zelda_item_matrix(nullptr,linkArgs,nullptr,nullptr);
 assert(!link.ordon.data.material.shape.hidden&&s_hubZeldaGift.handSword);
 assert(before_hub_zelda_gift_draw(nullptr,itemDraw,&drawResult,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(presentationDraws==1&&!swordReceived);
 link.swordDraw=true;after_hub_zelda_item_matrix(nullptr,linkArgs,nullptr,nullptr);
 before_hub_zelda_gift_draw(nullptr,itemDraw,&drawResult,nullptr);
 assert(presentationDraws==1); // Owned equipment uses its native draw exactly once.
 update_hub_zelda_gift();assert(!rewardCalls);eventEnded=true;
 update_hub_zelda_gift();assert(rewardCalls==1&&s_hubZeldaGift.retiring);
 assert(link.ordon.data.material.shape.hidden&&!s_hubZeldaGift.handSword);
 assert(swordReceived&&s_hubSaveRequested&&s_hubZeldaPendingTalk==HubZeldaTalk::AfterGift);
 const auto talksBefore= talkOrders;
 update_hub_zelda_call();assert(talkOrders==talksBefore); // Wait for item cleanup/save.
 update_hub_zelda_gift();assert(rewardCalls==1);giftLive=false;update_hub_zelda_gift();assert(!hub_zelda_gift_busy());
 // Continue automatically after the gift, only once the native save and item are done.
 update_hub_zelda_call();assert(talkOrders==talksBefore);
 s_hubSaveRequested=false;update_hub_zelda_call();assert(talkOrders==talksBefore+1);
 nativeEvent.mPt2=zelda.id;eventRunning=true;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);
 assert(entry==s_hubZeldaAfterGiftStart&&s_hubZeldaActiveTalk==HubZeldaTalk::AfterGift);
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);assert(entry==s_hubZeldaAfterGiftStart);
 const auto afterGift=nodes.at(entry);assert(afterGift.bytes[0]==1&&word(afterGift,4)==mods::flow::kEnd);
 const std::string prefix="Come to me after your journey to get some rest and strengthen your ";
 const auto red=mods::flow::MessageBuilder{}.text_color(MESSAGE_COLOR_RED).build(MESSAGE_LANGUAGE_ENGLISH);
 const auto normal=mods::flow::MessageBuilder{}.text_color(MESSAGE_COLOR_DEFAULT).build(MESSAGE_LANGUAGE_ENGLISH);
 const std::string redTag(reinterpret_cast<const char*>(red.text().data()),red.text().size()-1);
 const std::string normalTag(reinterpret_cast<const char*>(normal.text().data()),normal.text().size()-1);
 for(const auto& t:texts.at(word(afterGift,2)))assert(t==prefix+redTag+"Talents"+normalTag+"."+std::string(1,0));
 rewardCallback(nullptr,&event,nullptr);talentsCallback(nullptr,&event,nullptr);
 assert(!hub_zelda_gift_busy()&&!s_hubZeldaTalentsPending&&rewardCalls==1);
 eventRunning=false;nativeEvent.mPt2=kNone;update_hub_zelda_call();assert(!talentOpens);
 // Later conversations (including existing saves) open talents instead of another gift.
 nativeEvent.mPt2=zelda.id;eventRunning=true;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);assert(entry==s_hubZeldaTalentsStart);
 const auto help=nodes.at(entry);
 for(const auto& t:texts.at(word(help,2)))assert(t=="Let me help you on your journey."+std::string(1,0));
 const auto open=nodes.at(word(help,4));assert(open.bytes[0]==3&&open.bytes[1]==s_hubZeldaTalentsEvent.id());
 assert(edges.at(word(open,2))==mods::flow::kEnd);
 rewardCallback(nullptr,&event,nullptr);assert(!hub_zelda_gift_busy());
 talentsCallback(nullptr,&foreign,nullptr);assert(!s_hubZeldaTalentsPending);
 talentsCallback(nullptr,&event,nullptr);talentsCallback(nullptr,&event,nullptr);
 update_hub_zelda_call();assert(!talentOpens); // No UI over an active dialogue.
 eventRunning=false;nativeEvent.mPt2=kNone;msgTalking=true;
 update_hub_zelda_call();assert(!talentOpens);
 msgTalking=false;s_progressSaving=true;update_hub_zelda_call();assert(!talentOpens);
 s_progressSaving=false;update_hub_zelda_call();update_hub_zelda_call();assert(talentOpens==1);
 talentsCallback(nullptr,&event,nullptr);update_hub_zelda_call();assert(talentOpens==1);
 // A replaced/cancelled conversation cannot open a menu over another event.
 nativeEvent.mPt2=zelda.id;eventRunning=true;before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);
 talentsCallback(nullptr,&event,nullptr);nativeEvent.mPt2=99;update_hub_zelda_call();
 eventRunning=false;nativeEvent.mPt2=kNone;update_hub_zelda_call();assert(talentOpens==1&&!s_hubZeldaTalentsPending);
 // Still allow retrying failed first gifts on a swordless save.
 swordReceived=false;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);rewardCallback(nullptr,&event,nullptr);
 failGift=true;update_hub_zelda_gift();assert(!hub_zelda_gift_busy()&&rewardCalls==1);failGift=false;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);rewardCallback(nullptr,&event,nullptr);
 update_hub_zelda_gift();giftCreating=false;giftLive=true;
 nativeEvent.mPt2=55;eventEnded=false;eventRunning=true;update_hub_zelda_gift();assert(s_hubZeldaGift.started);
 after_hub_zelda_get(nullptr,linkArgs,nullptr,nullptr);
 link.ordon.data.material.shape.show(); // Preserve originally visible blades too.
 after_hub_zelda_item_matrix(nullptr,linkArgs,nullptr,nullptr);
 assert(s_hubZeldaGift.handSword);
 // Losing ownership/cancellation must not award a sword or reset the replacement event.
 swordReceived=false;s_hubSaveRequested=false; // Also model an interrupted first gift.
 nativeEvent.mPt2=99;const auto resets=eventResets;
 const auto calcs=link.calculations;lower.rate=1;lower.ended=false;
 before_hub_zelda_get_tick(nullptr,linkArgs,nullptr,nullptr);assert(lower.rate==1);
 after_hub_zelda_item_matrix(nullptr,linkArgs,nullptr,nullptr);
 assert(link.calculations==calcs&&!s_hubZeldaGift.handSword&&!link.ordon.data.material.shape.hidden);
 update_hub_zelda_gift();assert(rewardCalls==1&&eventResets==resets);
 assert(!swordReceived&&!s_hubSaveRequested&&s_hubZeldaPendingTalk==HubZeldaTalk::None);
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
assert '    update_hub_zelda();' in core
def extract_body(source, signature):
    start=source.index(signature)
    native=source
    brace=native.index('{',start);depth=0
    for end in range(brace,len(native)):
        depth+=(native[end]=='{')-(native[end]=='}')
        if not depth:return native[start:end+1]
    raise AssertionError(signature)
native_actions=''.join(extract_body(native,'int daNpc_Zelda_c::'+name+'(') for name in ('wait','talk'))
native_actions+=extract_body((sdk/'src/d/actor/d_a_player.cpp').read_text(),'bool daPy_frameCtrl_c::checkAnmEnd()')
link_native=(sdk/'src/d/actor/d_a_alink_link.inc').read_text()
native_actions+=''.join(extract_body(link_native,'MtxP daAlink_c::'+name+'()') for name in ('getLeftHandMatrix','getLeftItemMatrix'))
sword_native=extract_body((sdk/'src/d/actor/d_a_alink.cpp').read_text(),'void daAlink_c::setItemMatrix(')
assert 'mSwordModel->setBaseTRMtx(mpLinkModel->getAnmMtx(mLeftItemJntNo));' in sword_native
assert 'PRE(SurvivorsGetItemTick, before_hub_zelda_get_tick);' in core
assert 'POST(SurvivorsItemMatrix, after_hub_zelda_item_matrix);' in core
assert 'PRE(SurvivorsTreasureDraw, before_hub_zelda_gift_draw);' in core
with tempfile.TemporaryDirectory() as temp:
    cpp,binary=Path(temp)/'test.cpp',Path(temp)/'test'
    cpp.write_text(fixture+native_actions+register+(root/'src/survivors_hub_zelda.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-unused-parameter','-Wno-unused-variable','-Wno-implicit-fallthrough','-Wno-missing-field-initializers',
        '-I',str(sdk/'sdk/include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
