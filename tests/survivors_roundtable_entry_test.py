"""Exercise production stage-creation relocation; live door transforms stay native."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/survivors_roundtable.inc').read_text()
start=source.index('HookAction before_roundtable_stage_actor(')
end=source.index('\nvoid after_roundtable',start)
production=source[start:end]
names=sorted(set(re.findall(r'fpcNm_\w+',production)))
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <cstring>
#include "generated/roundtable_layout.hpp"
using namespace dawnlight;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct Vec{float x=0,y=0,z=0;Vec operator-(Vec v)const{return{x-v.x,y-v.y,z-v.z};}};
using s16=short;using cXyz=Vec;
struct fopAc_ac_c{int name;struct{Vec pos;}home;};
int fopAcM_GetProfName(fopAc_ac_c* a){return a->name;}
struct fopAcM_prm_class{struct{unsigned parameters=0x12345678;Vec position;struct{short x=0,y=123,z=0;}angle;}base;int room_no=51;};
struct stage_actor_data_class{char name[8]={'S','h','u','t','t','e','r',0};};
namespace mods{template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
bool preparing=false,s_hub=true,arena=true;
bool survivors_title_preparing(){return preparing;}bool in_arena(){return arena;}
struct Info{int procname=0;}info;bool found=true;
Info* dStage_searchName(const char*){return found?&info:nullptr;}
int freed=0;void JKRFree(void*){++freed;}
'''
main=r'''
int main(){
 stage_actor_data_class record;
 auto invoke=[&](fopAcM_prm_class& p){std::any args[]={&record,&p};return before_roundtable_stage_actor(nullptr,args,nullptr,nullptr);};
 auto original=[](){fopAcM_prm_class p;p.base.position={0,-250,4000};return p;};
 for(int profile:{fpcNm_DOOR20_e,fpcNm_KNOB20_e,fpcNm_DBDOOR_e,fpcNm_BOSS_DOOR_e,fpcNm_L1BOSS_DOOR_e,fpcNm_L1MBOSS_DOOR_e,fpcNm_L5BOSS_DOOR_e,fpcNm_SPIRAL_DOOR_e}){
  info.procname=profile;
  // The old fixture copied Link's event position and hid the wrong match.
  // Test distinct door records throughout the former southern corridor.
  for(int room:{51,-1})for(float z:{3600.f,3900.f,4000.f,4200.f})for(float y:{-400.f,-250.f,0.f}){
   auto p=original();p.room_no=room;p.base.position={12,y,z};
   assert(invoke(p)==HOOK_CONTINUE);
   assert(p.base.position.x==0 && p.base.position.y==roundtable_layout::floorY && p.base.position.z==roundtable_layout::exitZ);
   assert(p.base.parameters==0x12345678 && p.base.angle.y==123 && p.room_no==room && freed==0);
   assert(invoke(p)==HOOK_CONTINUE && p.base.position.z==roundtable_layout::exitZ); // No accumulated translation.
  }
  for(int mode=0;mode<8;++mode){
   auto p=original();s_hub=mode!=0;arena=mode!=1;
   if(mode==2)p.room_no=50;
   if(mode==3)p.base.position.x=500;
   if(mode==4)p.base.position.z=-4000;
   if(mode==5)p.base.position.z=0;
   if(mode==6)p.base.position.z=roundtable_layout::exitZ;
   if(mode==7)p.base.position.z=INFINITY;
   const auto before=p.base.position;
   assert(invoke(p)==HOOK_CONTINUE && p.base.position.z==before.z && p.base.position.y==before.y);
  }
  s_hub=false;arena=false;preparing=true;auto title=original();invoke(title);
  assert(title.base.position.z==roundtable_layout::exitZ); // Hub title uses the same doorway.
  preparing=false;s_hub=true;arena=true;
 }
 // Native event casting requires equal home anchors, not merely nearby actors.
 info.procname=fpcNm_L1MBOSS_DOOR_e;auto shutter=original();invoke(shutter);
 info.procname=fpcNm_ALINK_e;auto player=original();player.base.parameters=0xff006000;invoke(player);
 assert(player.base.parameters==0xff006000 && player.base.angle.y==123);
 assert(player.base.position.x==shutter.base.position.x && player.base.position.y==shutter.base.position.y && player.base.position.z==shutter.base.position.z);
 fopAc_ac_c door{fpcNm_L1MBOSS_DOOR_e,{shutter.base.position}},link{fpcNm_ALINK_e,{player.base.position}};
 FindShtrCbPrms cast{fpcNm_L1MBOSS_DOOR_e,&link};
 assert(findShutterCallBack(&door,&cast)==&door);
 link.home.pos.z=3900;assert(findShutterCallBack(&door,&cast)==nullptr);
 // Save/title has already moved Link to restZ; a direct run restart is untouched.
 player.base.position.z=roundtable_layout::restZ;invoke(player);assert(player.base.position.z==roundtable_layout::restZ);
 s_hub=false;player=original();invoke(player);assert(player.base.position.z==4000);s_hub=true;
 for(int profile:{fpcNm_BG_e,fpcNm_KANKYO_e,fpcNm_ENVSE_e}){
  info.procname=profile;auto p=original();assert(invoke(p)==HOOK_CONTINUE);
  assert(p.base.position.z==4000);
 }
 info.procname=999;auto decoration=original();assert(invoke(decoration)==HOOK_SKIP_ORIGINAL && freed==1);
 found=false;auto unknown=original();assert(invoke(unknown)==HOOK_CONTINUE && freed==1);
 assert(std::strcmp(record.name,"Shutter")==0); // Shared stage record is never edited.
}
'''
native=(ROOT/'dusklight/src/d/d_event_manager.cpp').read_text()
cast=native[native.index('struct FindShtrCbPrms {'):native.index('fopAc_ac_c* dEvent_manager_c::specialCast_Shutter(')]
with tempfile.TemporaryDirectory() as temp:
 cpp=Path(temp)/'entry.cpp';exe=Path(temp)/'entry'
 cpp.write_text(fixture+'\nenum {'+','.join(names)+'};\n'+production+cast+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Roundtable entry: southern corridor records, pre-construction anchor, hub/title scoping, unrelated doors and replay safety passed')
