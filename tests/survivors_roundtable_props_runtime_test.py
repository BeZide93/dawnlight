"""Native Carry dispatch, spawn parameters and room-owned teardown regressions."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/survivors_roundtable_props.cpp').read_text();production=s[s.index('namespace dawnlight {'):]
fixture=r'''
#include "roundtable_prop_source.hpp"
#include "generated/roundtable_props.hpp"
#include <any>
#include <cassert>
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <iostream>
using u8=unsigned char;using s16=short;using ActorId=unsigned;
constexpr int MOD_OK=0,cPhs_ERROR_e=5,fpcNm_Obj_Carry_e=42;
constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
void* mod_ctx=nullptr;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
using process_method_func=int(*)(void*);
namespace mods{template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
struct process_method_class{process_method_func create_method,delete_method,execute_method,is_delete_method;};
struct leafdraw_method_class{process_method_class base;process_method_func draw_method;};
struct fopAc_ac_c{int name=fpcNm_Obj_Carry_e;unsigned id=0;const void* sub_method=nullptr;};
int preInits=0;struct daObjCarry_c:fopAc_ac_c{void preInit(){++preInits;}};
bool validActor=true;bool is_actor_process(void* p){return p&&validActor;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetName(fopAc_ac_c* a){return a->name;}
int cancels=0;void fopAcM_cancelCarryNow(fopAc_ac_c*){++cancels;}void fopAcM_cancelHookCarryNow(fopAc_ac_c*){++cancels;}
int sourceMode=0,archiveMounts=0;
void* mDoExt_getArchiveHeap(){return nullptr;}
struct JKRArchive{
 enum{MOUNT_DVD,MOUNT_DIRECTION_HEAD};std::vector<u8> bytes;
 static JKRArchive* mount(const char* path,int,void*,int){assert(std::string(path)=="/res/Stage/D_MN09/R08_00.arc");++archiveMounts;if(sourceMode==2)return nullptr;auto* a=new JKRArchive;a->bytes.resize(144);a->bytes[3]=1;std::memcpy(a->bytes.data()+4,"ACTR",4);a->bytes[11]=4;a->bytes[15]=16;unsigned types[]{7,13,2,4};for(int i=0;i<4;++i){std::memcpy(a->bytes.data()+16+i*32,"carry00",7);a->bytes[16+i*32+29]=types[i]*2;}if(sourceMode==1){a->bytes.resize(48);a->bytes[11]=1;}if(sourceMode==3)a->bytes[0]=255;return a;}
 void* getResource(unsigned,const char* path){assert(std::string(path)=="room.dzr");return sourceMode==4?nullptr:bytes.data();}unsigned getExpandedResSize(const void*){return bytes.size();}void unmount(){delete this;}
};
struct Log{void warn(void*,const char*){}}logService;auto* svc_log=&logService;

struct ActorSpawnParams{unsigned parameters;int argument,room_num;struct{float x,y,z;}position,scale;struct{s16 x,y,z;}angle;};
struct Spawn{unsigned id;ActorSpawnParams params;};std::vector<Spawn> spawned;std::vector<unsigned> deleted;
bool rejectSpawn=false;unsigned nextID=100;
struct Service{
 int create_child_actor(void*,int profile,unsigned owner,const ActorSpawnParams* p,unsigned* id){
  assert(profile==fpcNm_Obj_Carry_e&&owner==10);if(rejectSpawn)return 1;
  *id=nextID++;spawned.push_back({*id,*p});return 0;}
 int delete_actor(void*,unsigned id){deleted.push_back(id);return 0;} // Deferred, exercised below.
}service;auto* svc_actor=&service;
int create(void*){return 4;}int remove(void*){return 1;}int execute(void*){return 1;}int draw(void*){return 1;}
leafdraw_method_class methods{{create,remove,execute,nullptr},draw};
'''
main=r'''
int main(){
 using namespace dawnlight;
 for(sourceMode=0;sourceMode<=4;++sourceMode){
  const auto mounts=archiveMounts;
  rejectSpawn=true;update_roundtable_props(10);assert(rooms[10].next==0&&spawned.empty());rejectSpawn=false;
  for(int i=0;i<20;++i)update_roundtable_props(10);
  assert(spawned.size()==std::size(roundtable_layout::props)&&owners.size()==36);
  assert(archiveMounts==mounts+1);
  auto expected=roundtable_props::default_types;if(sourceMode<=1)expected[0]=7;if(!sourceMode)expected[1]=13;
  assert(types==expected);
  for(unsigned i=0;i<spawned.size();++i){
   const auto& p=spawned[i].params;const auto& layout=roundtable_layout::props[i];
   assert(p.room_num==51&&p.argument==-1&&p.parameters==0x003fffff);
   assert(((unsigned(p.angle.z)>>1)&31)==expected[layout.kind]);
   assert((p.angle.z&0x1041)==0x1040); // break on rolling, no MoveBG, direct item mode
   assert(p.angle.x==-1&&p.scale.x==1&&p.scale.y==1&&p.scale.z==1);
   assert(p.position.x==layout.x&&p.position.y==layout.y&&p.position.z==layout.z);
   daObjCarry_c actor;actor.id=spawned[i].id;actor.sub_method=&methods;
   for(auto method:{create,execute,draw}){
    std::any args[]{method,static_cast<void*>(&actor)};int result=0;
    assert(before_roundtable_prop_process(args,&result)==HOOK_CONTINUE); // Native physics/model/shadow remain in charge.
   }
  }
  daObjCarry_c actor;actor.id=spawned[0].id;actor.sub_method=&methods;
  std::any args[]{process_method_func(remove),static_cast<void*>(&actor)};int result=0;
  assert(before_roundtable_prop_process(args,&result)==HOOK_CONTINUE&&owners.size()==35);
  update_roundtable_props(10);assert(spawned.size()==36); // Broken/removed pieces don't respawn this visit.
  release_roundtable_props(10);assert(rooms.empty()&&deleted.size()==35);
  actor.id=spawned[1].id;
  args[0]=process_method_func(create);assert(before_roundtable_prop_process(args,&result)==HOOK_SKIP_ORIGINAL&&result==5);assert(preInits>0);
  args[0]=process_method_func(draw);assert(before_roundtable_prop_process(args,&result)==HOOK_SKIP_ORIGINAL&&result==1);
  args[0]=process_method_func(execute);auto count=deleted.size();
  assert(before_roundtable_prop_process(args,&result)==HOOK_SKIP_ORIGINAL&&deleted.size()==count+1);
  assert(cancels>0); // Carry/hook attachment released on retired Execute.
  args[0]=process_method_func(remove);assert(before_roundtable_prop_process(args,&result)==HOOK_CONTINUE);
  actor.id=9999;args[0]=process_method_func(execute);assert(before_roundtable_prop_process(args,&result)==HOOK_CONTINUE);
  validActor=false;assert(before_roundtable_prop_process(args,&result)==HOOK_CONTINUE);validActor=true;
  shutdown_roundtable_props();assert(rooms.empty()&&owners.empty()&&!sourceRead);
  spawned.clear();deleted.clear();
 }
 std::cout<<"Hub Carry: all 36 native spawns, source fallbacks, untouched physics/shadows, break removal and deferred teardown passed\n";
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.cpp').write_text(fixture+production+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(ROOT/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
