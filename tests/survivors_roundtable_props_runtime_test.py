"""Exercise production dressing actors with a bounded fake native host."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/survivors_roundtable_props.cpp').read_text();production=s[s.index('namespace dawnlight {'):]
fixture=r'''
#include "roundtable_prop_source.hpp"
#include "generated/roundtable_props.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <iostream>
using u8=unsigned char;using u32=unsigned;using s16=short;using ActorId=unsigned;using ActorHandle=unsigned;using ProfileName=int;using ModResult=int;
struct ModError{};
constexpr int MOD_OK=0,cPhs_COMPLEATE_e=4,cPhs_ERROR_e=5,fpcDwPi_Obj_Carry_e=1,fopAcStts_UNK_0x40000_e=1,fopAc_ACTOR_e=1,fopAc_CULLBOX_CUSTOM_e=1;
constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
void* mod_ctx=nullptr;
namespace mods{int set_error(ModError*,int code,const char*){return code;}}
struct cXyz{float x,y,z;cXyz(float a=0,float b=0,float c=0):x(a),y(b),z(c){}};
struct fopAc_ac_c{unsigned id=1,param=0,parentActorID=10;struct{cXyz pos;}current;int tevStr=0;struct{unsigned flags=0;}attention_info;};
#define fopAcM_ct(a,T) ((void)0)
unsigned fopAcM_GetParam(fopAc_ac_c* a){return a->param;}unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
void fopAcM_SetMtx(fopAc_ac_c*,void*){}
struct request_of_phase_process_class{bool held=false;};
int sourceMode=0;
int archiveRefs=0,archiveLoads=0,archiveDeletes=0,archiveMounts=0,draws=0;bool delayed=false,failLoad=false,failHeap=false,failModel=false,ownerDeleted=false;
int dComIfG_resLoad(request_of_phase_process_class* p,const char*){++archiveLoads;if(!p->held){p->held=true;++archiveRefs;}return failLoad?5:delayed?1:4;}
void dComIfG_resDelete(request_of_phase_process_class* p,const char*){if(p->held){--archiveRefs;++archiveDeletes;p->held=false;}}
struct Shape{cXyz low{-40,-10,-30},high{40,90,30};const cXyz* getMin(){return &low;}const cXyz* getMax(){return &high;}} shape;
struct J3DModelData{unsigned getJointNum(){return 1;}unsigned getShapeNum(){return 1;}Shape* getShapeNodePointer(unsigned){return &shape;}} data;
struct J3DModel{J3DModelData* getModelData(){return &data;}void setBaseTRMtx(void*){}void setBaseScale(cXyz){}void calc(){}void* getBaseTRMtx(){return nullptr;}} model;
void* dComIfG_getObjectRes(const char*,const char*){return &data;}
J3DModel* mDoExt_J3DModel__create(J3DModelData*,int,int){return failModel?nullptr:&model;}
int fopAcM_entrySolidHeap(fopAc_ac_c* a,int(*f)(fopAc_ac_c*),unsigned){return failHeap?0:f(a);}
struct mDoMtx_stack_c{static void transS(float,float,float){}static void transM(float,float,float){}static void YrotM(short){}static void scaleM(float,float,float){}static void* get(){return nullptr;}};
struct Env{void settingTevStruct(int,cXyz*,int*){}void setLightTevColorType_MAJI(J3DModel*,int*){}}g_env_light;
void mDoExt_modelUpdateDL(J3DModel*){++draws;}
void* mDoExt_getArchiveHeap(){return nullptr;}
struct JKRArchive{
 enum{MOUNT_DVD,MOUNT_DIRECTION_HEAD};std::vector<u8> bytes;
 static JKRArchive* mount(const char* path,int,void*,int){assert(std::string(path)=="/res/Stage/D_MN09/R08_00.arc");++archiveMounts;if(sourceMode==2)return nullptr;auto* a=new JKRArchive;a->bytes.resize(144);a->bytes[3]=1;std::memcpy(a->bytes.data()+4,"ACTR",4);a->bytes[11]=4;a->bytes[15]=16;unsigned types[]{7,13,2,4};for(int i=0;i<4;++i){std::memcpy(a->bytes.data()+16+i*32,"carry00",7);a->bytes[16+i*32+29]=types[i]*2;}if(sourceMode==1){a->bytes.resize(48);a->bytes[11]=1;}if(sourceMode==3)a->bytes[0]=255;return a;}
 void* getResource(unsigned,const char* path){assert(std::string(path)=="room.dzr");return sourceMode==4?nullptr:bytes.data();}unsigned getExpandedResSize(const void*){return bytes.size();}void unmount(){delete this;}
};
struct Log{void warn(void*,const char*){}}logService;auto* svc_log=&logService;
struct ActorSpawnParams{unsigned parameters;int argument,room_num;struct{float x,y,z;}position,scale;};
struct ActorProfileDesc{const char* name;int priority_group;size_t process_size;int draw_priority,status,group,cull_type;int(*create_function)(void*);int(*delete_function)(void*);int(*execute_function)(void*);int(*is_delete_function)(void*);int(*draw_function)(void*);};
std::vector<unsigned> spawned,deleted;
struct Service{
 int register_actor(void*,const ActorProfileDesc*,ProfileName* p,ActorHandle* h){*p=90;*h=1;return 0;}
 int unregister_actor(void*,unsigned){return 0;}
 int create_child_actor(void*,int,unsigned owner,const ActorSpawnParams* p,unsigned* id){assert(owner==10);assert(p->room_num==51);spawned.push_back(p->parameters);*id=100+p->parameters;return 0;}
 int delete_actor(void*,unsigned id){deleted.push_back(id);return 1;} // Deferred deletion must be safe.
}service;auto* svc_actor=&service;
'''
main=r'''
int main(){
 using namespace dawnlight;
 // Optional source discovery must never suppress all models. Exercise partial,
 // missing, malformed and missing-resource archives through the production path.
 for(sourceMode=1;sourceMode<=4;++sourceMode){
  const auto mounts=archiveMounts;assert(initialize_roundtable_props(nullptr)==0);
  for(int i=0;i<20;++i)update_roundtable_props(10);
  assert(spawned.size()==std::size(roundtable_layout::props));assert(archiveMounts==mounts+1);
  auto expected=roundtable_props::default_types;if(sourceMode==1)expected[0]=7;
  assert(types==expected);
  const unsigned representatives[]{4,6,0,1};
  for(auto index:representatives){PropActor a;a.param=index;assert(create_prop(&a)==4);draw_prop(&a);}
  assert(archiveRefs==0);
  shutdown_roundtable_props();spawned.clear();deleted.clear();
 }
 sourceMode=0;archiveMounts=draws=0;
 assert(initialize_roundtable_props(nullptr)==0);
 for(int i=0;i<20;++i)update_roundtable_props(10);
 assert(spawned.size()==std::size(roundtable_layout::props));assert(archiveMounts==1);
 for(unsigned i=0;i<spawned.size();++i)assert(spawned[i]==i);
 {PropActor a;a.param=0;delayed=true;assert(create_prop(&a)==1);assert(archiveRefs==1);draw_prop(&a);assert(draws==0);delayed=false;assert(create_prop(&a)==4);assert(a.ready);draw_prop(&a);assert(draws==1);}
 assert(archiveRefs==0);
 for(int fail=0;fail<3;++fail){PropActor a;failLoad=fail==0;failHeap=fail==1;failModel=fail==2;assert(create_prop(&a)==5);assert(!a.ready);}
 failLoad=failHeap=failModel=false;assert(archiveRefs==0);
 {PropActor a;delayed=true;assert(create_prop(&a)==1);release_roundtable_props(10);assert(rooms.empty());assert(deleted.size()==spawned.size());delayed=false;assert(create_prop(&a)==5);draw_prop(&a);assert(draws==1);execute_prop(&a);assert(deleted.back()==a.id);}
 assert(archiveRefs==0);
 shutdown_roundtable_props();assert(!registration&&!sourceRead);
 std::cout<<"Native dressing: source cache, delayed/failed creation, owner retirement and archive cleanup passed\n";
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.cpp').write_text(fixture+production+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(ROOT/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
