"""Title-only stage routing, pose/input/camera isolation and native logo flow."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
sdk=root/'dusklight'
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <initializer_list>
#include "survivors_maps.hpp"
using namespace dawnlight::survivors;
using s8=signed char;using s16=short;using u32=unsigned;
struct ModContext{};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{
 template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}
 template<class T>T& arg_ref(void* a,int n){return *std::any_cast<T>(&static_cast<std::any*>(a)[n]);}
}
constexpr int fpcNm_OPENING_SCENE_e=1,fpcNm_ALINK_e=2,fpcNm_SCENE_EXIT_e=3,fpcNm_SCENE_EXIT2_e=4;
bool s_active=true,s_gameStarted=false,hasScene=true,bowlLive=true,zeldaLive=true;
const char* stageName=arenas[0].stage;s8 roomNo=arenas[0].room;
const char* dComIfGp_getStartStageName(){return stageName;}
s8 dComIfGp_getStartStageRoomNo(){return roomNo;}
struct Scene{int name=fpcNm_OPENING_SCENE_e;}scene;
struct dStage_roomControl_c{static int getProcID(){return 17;}};
Scene* fopScnM_SearchByID(int id){assert(id==17);return hasScene?&scene:nullptr;}
int fpcM_GetName(Scene* s){return s->name;}
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 void set(float a,float b,float c){x=a;y=b;z=c;}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}
 cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}};
struct stage_actor_data_class{char name[8];};
struct fopAcM_prm_class{struct{u32 parameters=0xdeadbeef;cXyz position{999,-400,999};struct{s16 x=0,y=0,z=0;}angle;}base;};
struct Info{int procname;}info{fpcNm_ALINK_e};
Info* dStage_searchName(const char* n){assert(std::strlen(n)==8);return &info;}
bool arena_story_actor(int n){return n==99;}
unsigned freed=0;void JKRFree(void*){++freed;}
struct Frame{float frame=0,rate=1;void setFrame(float f){frame=f;}void setRate(float f){rate=f;}};
struct daAlink_c{
 enum{PROC_WAIT=1,ANM_TRES_OPEN_SMALL=207};int mProcID=99,animation=0,sets=0;
 unsigned mItemButton=7,mItemTrigger=3;
 float mStickValue=1,mMoveValue=1,mNormalSpeed=4,speedF=4;
 cXyz speed{1,2,3},position;s16 yaw=0;
 Frame mUnderFrameCtrl[1],mUpperFrameCtrl[1];
 void procWaitInit(){mProcID=PROC_WAIT;}
 bool checkUnderMove0BckNoArc(int n){return animation==n;}
 void setSingleAnime(int n,float rate,float frame,s16 end,float morf){assert(rate==0&&frame==36&&end==-1&&morf==0);animation=n;++sets;}
 void setPlayerPosAndAngle(const cXyz* p,s16 a,bool force){assert(force);position=*p;yaw=a;}
}link,foreign;
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
bool hub_brazier_anchor(cXyz& p){p={0,-400,0};return bowlLive;}
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
float floorY=-400;
struct Bg{float GroundCross(dBgS_ObjGndChk* p){assert(p->pos.x==0&&p->pos.y==-300&&p->pos.z==260);return floorY;}}bg;
Bg& dComIfG_Bgsp(){return bg;}
struct dCamera_c{cXyz mEye,mCenter,mUp;float fov=0;int sets=0;struct{void Val(cXyz){}}mDirection;
 void Set(cXyz c,cXyz e,float f,s16 bank){assert(bank==0);mCenter=c;mEye=e;fov=f;++sets;}}camera;
int s_hubZelda=12;
void* actor_by_id(int id){assert(id==12);return zeldaLive?&scene:nullptr;}
bool owned_hub_zelda(void* p){return p!=nullptr;}
'''
main=r'''
int main(){
 assert(survivors_title_active());
 for(int which=0;which<5;++which){
  if(which==0)s_active=false;if(which==1)s_gameStarted=true;if(which==2)scene.name=999;
  if(which==3)hasScene=false;if(which==4)roomNo=0;
  assert(!survivors_title_active());s_active=true;s_gameStarted=false;scene.name=1;hasScene=true;roomNo=51;
 }
 std::any args[]{static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
 before_title_stage(nullptr,args,nullptr,nullptr);
 assert(std::strcmp(mods::arg<const char*>(args,0),arenas[0].stage)==0);
 assert(mods::arg<s16>(args,1)==0&&mods::arg<s8>(args,2)==51&&mods::arg<s8>(args,3)==0);
 for(int which=0;which<5;++which){
  std::any original[]{static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
  if(which==0)s_active=false;if(which==1)s_gameStarted=true;
  if(which==2)original[1]=s16(0);if(which==3)original[2]=s8(1);if(which==4)original[3]=s8(0);
  before_title_stage(nullptr,original,nullptr,nullptr);
  assert(std::strcmp(mods::arg<const char*>(original,0),"F_SP102")==0);
  s_active=true;s_gameStarted=false;
 }
 stage_actor_data_class record{{'L','i','n','k','N','a','m','e'}};fopAcM_prm_class params;
 std::any actorArgs[]{&record,&params};
 assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_CONTINUE);
 assert(((params.base.parameters>>12)&0x1f)==16&&((params.base.parameters>>24)&0xff)==0xff);
 assert(params.base.position.x==0&&params.base.position.z==260&&params.base.position.y==-400);
 assert(params.base.angle.y==s16(0x8000));
 for(int n:{99,fpcNm_SCENE_EXIT_e,fpcNm_SCENE_EXIT2_e}){
  info.procname=n;assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 }assert(freed==3);
 info.procname=88;assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_CONTINUE&&freed==3);
 std::any linkArgs[]{&link},otherArgs[]{&foreign},cameraArgs[]{&camera};
 // Private Link input is consumed; no global pad API exists in this fixture.
 after_title_input(nullptr,otherArgs,nullptr,nullptr);assert(foreign.mItemButton==7);
 after_title_input(nullptr,linkArgs,nullptr,nullptr);
 assert(!link.mItemButton&&!link.mItemTrigger&&!link.mStickValue&&!link.mMoveValue);
 assert(!link.speedF&&!link.mNormalSpeed&&!link.speed.y&&link.mProcID==daAlink_c::PROC_WAIT);
 int result=0;assert(before_title_wait(nullptr,otherArgs,&result,nullptr)==HOOK_CONTINUE);
 for(int i=0;i<200;++i)assert(before_title_wait(nullptr,linkArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
 assert(link.sets==1&&link.position.z==260&&link.position.y==-400&&link.yaw==s16(0x8000));
 assert(link.mUnderFrameCtrl[0].frame==36&&link.mUpperFrameCtrl[0].frame==36);
 assert(link.mUnderFrameCtrl[0].rate==0&&link.mUpperFrameCtrl[0].rate==0);
 floorY=std::nanf("");before_title_wait(nullptr,linkArgs,&result,nullptr);assert(link.position.y==-400);
 after_title_camera(nullptr,cameraArgs,nullptr,nullptr);
 assert(camera.sets==1&&camera.mEye.x==650&&camera.mEye.y==-70&&camera.mCenter.x==-70&&camera.fov==45);
 zeldaLive=false;assert(before_title_logo(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==-1);
 zeldaLive=true;bowlLive=false;before_title_logo(nullptr,nullptr,&result,nullptr);assert(result==-1);
 bowlLive=true;before_title_logo(nullptr,nullptr,&result,nullptr);assert(result==1);
 // File selection and gameplay in the SAME hub stage are not a title scene.
 for(int which=0;which<2;++which){
  if(which==0)scene.name=999;else{scene.name=1;s_gameStarted=true;}
  link.mItemButton=7;after_title_input(nullptr,linkArgs,nullptr,nullptr);assert(link.mItemButton==7);
  assert(before_title_wait(nullptr,linkArgs,&result,nullptr)==HOOK_CONTINUE);
  assert(before_title_logo(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
  after_title_camera(nullptr,cameraArgs,nullptr,nullptr);assert(camera.sets==1);
  info.procname=99;assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_CONTINUE&&freed==3);
 }
 std::cout<<"Survivors title: opening routing, scene isolation, kneeling pose, private input, framing and logo cue passed\n";
}
'''
native=(sdk/'src/d/actor/d_a_alink_demo.inc').read_text()
assert 'setSingleAnime(ANM_TRES_OPEN_SMALL, 1.0f, 36.0f, -1, 3.0f);' in native
native_title=(sdk/'src/d/actor/d_a_title.cpp').read_text()
assert 'else if (getDemoPrm() == 1)' in native_title
assert 'fopScnM_ChangeReq(playScene, fpcNm_NAME_SCENE_e, 0, 5);' in native_title
source=(root/'src/survivors.cpp').read_text()
assert source.index('PRE(SurvivorsStage, before_title_stage)')<source.index('PRE(SurvivorsStage, before_stage)')
assert source.index('PRE(SurvivorsArenaActor, before_title_stage_actor)')<source.index('PRE(SurvivorsArenaActor, before_arena_stage_actor)')
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'title.cpp';exe=Path(tmp)/'title'
 cpp.write_text(fixture+(root/'src/survivors_title.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
