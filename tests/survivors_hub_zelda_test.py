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
using u8=uint8_t;using u16=uint16_t;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_NPC_ZELDA_e=100;
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
 assert(std::string(name)=="Survivors Zelda rupees");rewardCallback=fn;*out=200;return MOD_OK;}
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
struct fopAc_ac_c {ActorId id=12;int name=fpcNm_NPC_ZELDA_e;struct{unsigned flags=3;}attention_info;};
struct dMsgFlow_c{};
struct daNpc_Zelda_c:fopAc_ac_c {dMsgFlow_c mFlow;}zelda,other;
ActorId fopAcM_GetID(const fopAc_ac_c* actor){return actor->id;}
int fopAcM_GetName(const fopAc_ac_c* actor){return actor->name;}
bool active=true,transition=false,play=true,pedestal=true,creating=false,live=false,failSpawn=false;
bool s_hubDeparture=false,s_progressSaving=false,s_hubSaveRequested=false;
bool survivors_hub_active(){return active;}
bool transitioning(){return transition;}
bool playable(){return active&&play&&!transition;}
unsigned spawnCalls=0,deleteCalls=0,groundCalls=0,rewardCalls=0;
int queuedRupees=0;
bool fpcM_IsCreating(ActorId id){return id==zelda.id&&creating;}
fopAc_ac_c* actor_by_id(ActorId id){return id==zelda.id&&live&&!creating?&zelda:nullptr;}
ModResult deleteActor(ModContext*,ActorId id){assert(id==zelda.id);++deleteCalls;return MOD_OK;}
ActorService actorService{.delete_actor=deleteActor};
extern "C" {const ActorService* svc_actor=&actorService;}
void dComIfGp_setItemRupeeCount(int n){assert(n==10);queuedRupees+=n;++rewardCalls;}
bool survivors_pedestal_anchor(cXyz& anchor){anchor={0,-400,0};return pedestal;}
struct dBgS_ObjGndChk {cXyz pos;void SetPos(cXyz* p){pos=*p;}};
float floorHeight=-400;
struct Background {float GroundCross(dBgS_ObjGndChk* ground){++groundCalls;assert(ground->pos.x==-320&&ground->pos.z==0);return floorHeight;}}bg;
Background& dComIfG_Bgsp(){return bg;}
struct Arena {int8_t room=51;};Arena arenas[1];
ModResult create_standalone_actor(int profile,const ActorSpawnParams& p,ActorId& out){
 assert(profile==fpcNm_NPC_ZELDA_e&&p.parameters==0xffffff01u&&p.room_num==51);
 assert(p.position.x==-320&&p.position.y==-400&&p.position.z==0&&p.angle.x==3001&&p.angle.y==0x4000);
 assert(p.scale.x==1&&p.scale.y==1&&p.scale.z==1&&p.create_function==nullptr);
 ++spawnCalls;if(failSpawn)return MOD_UNAVAILABLE;out=zelda.id;creating=true;return MOD_OK;
}
'''
main = r'''
u16 word(const FlowNodeData& n,unsigned offset){return u16(n.bytes[offset]<<8|n.bytes[offset+1]);}
int main(){
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
 std::any ownArgs[]={&zelda},otherArgs[]={&other};other.id=99;u8 type=0;
 assert(before_hub_zelda_type(nullptr,ownArgs,&type,nullptr)==HOOK_SKIP_ORIGINAL&&type==1);
 type=0;assert(before_hub_zelda_type(nullptr,otherArgs,&type,nullptr)==HOOK_CONTINUE&&type==0);
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
 // Native events make playable() false while the dialogue is running. Reward must still execute.
 play=false;rewardCallback(nullptr,&event,nullptr);rewardCallback(nullptr,&event,nullptr);
 assert(rewardCalls==1&&queuedRupees==10);play=true;
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);rewardCallback(nullptr,&event,nullptr);
 assert(rewardCalls==2&&queuedRupees==20); // One gift per new conversation.
 play=false;after_hub_zelda_param(nullptr,ownArgs,nullptr,nullptr);assert(!zelda.attention_info.flags);
 after_hub_zelda_param(nullptr,otherArgs,nullptr,nullptr);assert(other.attention_info.flags==3);play=true;
 zelda.attention_info.flags=3;after_hub_zelda_param(nullptr,ownArgs,nullptr,nullptr);assert(zelda.attention_info.flags==3);
 before_hub_zelda_flow(nullptr,flowArgs,&entry,nullptr);s_hubDeparture=true;
 rewardCallback(nullptr,&event,nullptr);assert(rewardCalls==2);
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
 std::cout<<"Survivors Zelda: graph/text, repeat rewards, identity, placement, pause and async cleanup passed\n";
}
'''
# Type 1 must remain a valid native resource/heap/standing-animation path.
native = (sdk/'src/d/actor/d_a_npc_zelda.cpp').read_text()
assert 'heapSize[2]' in native and 'l_loadResPtrnList[2]' in native
assert 'mMotionSeqMngr.setNo(0,' in native and 'initTalk(mFlowNodeNo, NULL)' in native
core = (root/'src/survivors.cpp').read_text()
assert 'PRE(SurvivorsMidnaFlow, before_hub_zelda_flow);' in core
assert 'PRE(SurvivorsHubZeldaType, before_hub_zelda_type);' in core
assert 'POST(SurvivorsHubZeldaParam, after_hub_zelda_param);' in core
assert 'void reset_runtime() {\n    retire_hub_zelda();' in core
assert 'void update_survivors() {\n    update_hub_zelda();' in core
with tempfile.TemporaryDirectory() as temp:
    cpp,binary=Path(temp)/'test.cpp',Path(temp)/'test'
    cpp.write_text(fixture+register+(root/'src/survivors_hub_zelda.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-missing-field-initializers',
        '-I',str(sdk/'sdk/include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
