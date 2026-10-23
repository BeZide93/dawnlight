"""Authored light-lift discovery, parameter conversion, ownership and cleanup."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <vector>
using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz{float x=0,y=0,z=0;};struct csXyz{short x=0,y=0,z=0;};
using Vec=cXyz;
struct Base{unsigned parameters=0;cXyz position;csXyz angle;};
struct Scale{unsigned char x=10,y=10,z=10;};
struct fopAcM_prm_class{Base base;int room_no=0;Scale scale;};
struct stage_actor_data_class{char name[8]{};Base base;};
struct stage_tgsc_data_class{char name[8]{};Base base;Scale scale;};
struct dStage_nodeHeader{unsigned m_tag;int m_entryNum;const void* m_offset;};
struct dStage_fileHeader{int m_chunkCount=0;dStage_nodeHeader m_nodes[5]{};}file;
constexpr int fpcNm_Obj_Lv8Lift_e=10,fpcNm_Obj_Lv8OptiLift_e=11;
unsigned s_map=3;bool active=true,playing=true,resource=true;
struct Arena{int room=0;}map;
const Arena& arena(){return map;}bool in_arena(){return active;}bool playable(){return playing;}
const char* dComIfG_getRoomArcName(int room){assert(room==0);return "R00_00";}
void* dComIfG_getStageRes(const char* arc,const char* name){assert(!std::strcmp(arc,"R00_00")&&!std::strcmp(name,"room.dzr"));return resource?&file:nullptr;}
struct Point{cXyz m_position;};struct dPath{unsigned short m_num=2;Point* m_points=nullptr;};
Point points[3]{{{1,2,3}},{{100,2,3}},{{200,2,3}}};
std::map<unsigned,dPath> paths;
dPath* dPath_GetRoomPath(unsigned p,int){auto i=paths.find(p);return i==paths.end()?nullptr:&i->second;}
struct fopAc_ac_c{ActorId id;unsigned status=0;};
constexpr unsigned fopAcStts_UNK_0x20000000_e=0x20000000;
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
void fopAcM_OffStatus(fopAc_ac_c* a,unsigned bits){a->status&=~bits;}
std::map<ActorId,fopAc_ac_c> actors;std::set<ActorId> pending;
bool fpcM_IsCreating(ActorId id){return pending.contains(id);}
fopAc_ac_c* actor_by_id(ActorId id){auto i=actors.find(id);return i==actors.end()?nullptr:&i->second;}
struct ActorSpawnParams{unsigned parameters;int argument,room_num;cXyz position;csXyz angle;cXyz scale;void* create_function;};
constexpr int MOD_OK=0;unsigned requests=0,deletes=0;bool refuse=false;
ActorSpawnParams last{};
int create_standalone_actor(int profile,const ActorSpawnParams& p,ActorId& id){assert(profile==fpcNm_Obj_Lv8Lift_e);++requests;last=p;if(refuse)return 1;id=100+requests;pending.insert(id);return MOD_OK;}
void* mod_ctx=nullptr;
struct ActorService{void delete_actor(void*,ActorId id){assert(!pending.contains(id));++deletes;}}actorService;
auto* svc_actor=&actorService;
struct Log{unsigned infos=0,warnings=0;void info(void*,const char*){++infos;}void warn(void*,const char*){++warnings;}}logService;
auto* svc_log=&logService;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}}
'''
main=r'''
dStage_nodeHeader node(const char* tag,int n,const void* p){dStage_nodeHeader out{};std::memcpy(&out.m_tag,tag,4);out.m_entryNum=n;out.m_offset=p;return out;}
void frame(){s_palaceFerryRetry=0;update_palace_ferries();}
int main(){
 fopAcM_prm_class p;p.base.parameters=0xff074208;p.base.angle.y=123;
 for(bool enabled:{false,true})for(unsigned mapNo:{0u,1u,2u,3u,4u})for(int room:{-1,0,1}) {
  active=enabled;s_map=mapNo;p.room_no=room;
  assert(capture_palace_ferry(fpcNm_Obj_Lv8OptiLift_e,p)==(enabled&&mapNo==3&&room<=0));
  retire_palace_ferries();assert(s_palaceFerries.empty());
 }
 active=true;s_map=3;p.room_no=0;
 assert(!capture_palace_ferry(999,p));
 // An optLift in an inactive layer must be discovered without a stage-create
 // hook firing. Preserve its actual path/speed and scaled-node dimensions.
 stage_tgsc_data_class opt{};std::memcpy(opt.name,"optLift",8);opt.base=p.base;opt.scale={15,20,25};
 stage_actor_data_class standard{};std::memcpy(standard.name,"L8LiftX",8);standard.base.parameters=0xff064742;
 stage_actor_data_class unrelated{};std::memcpy(unrelated.name,"TBOX",5);
 file.m_chunkCount=4;
 file.m_nodes[0]=node("SCO2",1,&opt);
 file.m_nodes[1]=node("ACT0",1,&standard); // Same path 0x42: deduplicate across variants/layers.
 file.m_nodes[2]=node("ACTR",1,&unrelated);
 file.m_nodes[3]=node("XXXX",1,&standard);
 paths[0x42]={3,points};
 playing=false;frame();assert(!s_palaceFerryScanned&&!requests);
 playing=true;resource=false;frame();assert(!s_palaceFerryScanned&&!requests);
 resource=true;frame();assert(s_palaceFerryScanned&&s_palaceFerries.size()==1&&requests==1);
 assert((last.parameters&255)==0x42&&((last.parameters>>8)&15)==7&&((last.parameters>>12)&255)==255);
 assert(last.scale.x==1.5f&&last.scale.y==2.f&&last.scale.z==2.5f);
 assert(last.position.x==1&&last.position.y==2&&last.angle.y==123&&last.room_num==0);
 const auto id=s_palaceFerries[0].id;
 for(int i=0;i<100;++i){frame();cleanup_palace_ferries();}
 assert(requests==1&&s_palaceFerries[0].id==id&&!deletes); // Pending creation stays owned.
 pending.erase(id);actors[id]={id,fopAcStts_UNK_0x20000000_e|4};
 cleanup_palace_ferries();assert(s_palaceFerries[0].ready);
 const auto reports=logService.infos;cleanup_palace_ferries();assert(logService.infos==reports);
 std::any args[]={&actors[id]};
 assert(before_palace_ferry_suspend(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(actors[id].status==4); // No suspension flag, unrelated status retained.
 fopAc_ac_c foreign{999,fopAcStts_UNK_0x20000000_e};args[0]=&foreign;
 assert(before_palace_ferry_suspend(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 assert(foreign.status==fopAcStts_UNK_0x20000000_e);
 retire_palace_ferries();cleanup_palace_ferries();assert(deletes==1&&s_palaceFerries.size()==1);
 cleanup_palace_ferries();assert(deletes==2); // Asynchronous/refused deletion retries.
 actors.clear();cleanup_palace_ferries();assert(s_palaceFerries.empty());
 // No path => no actor stuck forever in native Create; a later valid path retries.
 paths.clear();frame();assert(requests==1&&s_palaceFerries.size()==1);
 paths[0x42]={1,points};frame();assert(requests==1);
 paths[0x42]={3,points};points[1].m_position.x=NAN;frame();assert(requests==1);
 points[1].m_position.x=100;refuse=true;frame();assert(requests==2&&s_palaceFerries[0].id==kNone);
 refuse=false;frame();assert(requests==3&&s_palaceFerries[0].id!=kNone);
 const auto pendingID=s_palaceFerries[0].id;retire_palace_ferries();cleanup_palace_ferries();
 assert(s_palaceFerries.size()==1&&pending.contains(pendingID));
 pending.erase(pendingID);actors[pendingID]={pendingID};cleanup_palace_ferries();assert(deletes==3);
 actors.clear();cleanup_palace_ferries();assert(s_palaceFerries.empty());
 // Room records are read-only, including the original Sol switch.
 assert(opt.base.parameters==0xff074208&&standard.base.parameters==0xff064742);
 file.m_chunkCount=0;frame();assert(s_palaceFerryScanned&&s_palaceFerries.empty());
 assert(logService.warnings>=2);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_palace_ferry.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Palace ferry: inactive-layer discovery, native parameter conversion, pending ownership, suspension isolation and cleanup passed')
