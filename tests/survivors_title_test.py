"""Title-only stage routing, pose/input/camera isolation and native logo flow."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
sdk=root/'dusklight'
fixture=r'''
#include <any>
#include <chrono>
#include "survivors_title_flow.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <initializer_list>
#include "survivors_maps.hpp"
using namespace dawnlight::survivors;
using s8=signed char;using s16=short;using u32=unsigned;
using ActorId=unsigned;constexpr ActorId kNone=~0u;
using BOOL=int;constexpr BOOL FALSE=0;
using ModResult=int;constexpr ModResult MOD_OK=0;
struct ModError{};
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
struct scene_class{int name=fpcNm_OPENING_SCENE_e;}scene;
using Scene=scene_class;
bool transition=false,resetting=false,returning=false;
bool transitioning(){return transition;}
namespace mDoRst {bool isReset(){return resetting;}bool isReturnToMenu(){return returning;}}
unsigned reloads=0;bool reloadSelected=false;
int dComIfG_changeOpeningScene(Scene*,int name){assert(name==fpcNm_OPENING_SCENE_e);++reloads;reloadSelected=s_active;return 1;}
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
 ActorId id=7;struct{cXyz pos{0,-400,260};}current;
 enum{PROC_WAIT=1,ANM_WAIT=25};int mProcID=99,animation=0,sets=0;
 unsigned mItemButton=7,mItemTrigger=3;
 float mStickValue=1,mMoveValue=1,mNormalSpeed=4,speedF=4;
 cXyz speed{1,2,3},position;s16 yaw=0;
 Frame mUnderFrameCtrl[1],mUpperFrameCtrl[1];
 void procWaitInit(){mProcID=PROC_WAIT;}
 bool checkUnderMove0BckNoArc(int n){return animation==n;}
 void setSingleAnime(int n,float rate,float frame,s16 end,float morf){assert(rate==0&&frame==0&&end==-1&&morf==0);animation=n;++sets;}
 void setPlayerPosAndAngle(const cXyz* p,s16 a,bool force){assert(force);position=*p;yaw=a;}
}link,foreign;
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
ActorId fopAcM_GetID(daAlink_c* a){return a->id;}
bool hub_brazier_anchor(cXyz& p){p={0,-400,0};return bowlLive;}
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
float floorY=-400;
struct Bg{float GroundCross(dBgS_ObjGndChk* p){assert(p->pos.x==0&&((p->pos.y==-300&&p->pos.z==260)||(p->pos.y==600&&p->pos.z==0)));return floorY;}}bg;
Bg& dComIfG_Bgsp(){return bg;}
struct dCamera_c{cXyz mEye,mCenter,mUp;float fov=0;int sets=0;struct{void Val(cXyz){}}mDirection;
 void Set(cXyz c,cXyz e,float f,s16 bank){assert(bank==0);mCenter=c;mEye=e;fov=f;++sets;}}camera;
int s_hubZelda=12;
void* actor_by_id(int id){assert(id==12);return zeldaLive?&scene:nullptr;}
bool owned_hub_zelda(void* p){return p!=nullptr;}
bool modelReady=true;bool title_model_ready(daAlink_c*){return modelReady;}
constexpr unsigned PAD_1=0;
struct mDoCPd_c{static unsigned getHold(unsigned){return 0;}static unsigned getTrig(unsigned){return 0;}};
bool ui_visible(){return false;}
'''
main=r'''
int main(){
 for(int rate:{30,60,120}){
  TitleFlow flow;flow.play();
  for(int i=0;i<rate*5;++i)flow.tick(1.f/rate,false,false,false,false);
  assert(flow.camera()==0&&!flow.logo);
  for(int i=0;i<rate*4;++i)flow.tick(1.f/rate,true,false,false,false);
  assert(flow.camera()==1&&!flow.logo&&flow.armed);
  flow.tick(1.f/rate,true,false,true,true);assert(flow.logo&&!flow.show_logo());
  flow.tick(1.f/rate,true,false,false,false);assert(flow.show_logo());
  flow.play();assert(flow.camera()==0&&!flow.logo&&!flow.armed);
 }
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
 assert(link.mUnderFrameCtrl[0].frame==0&&link.mUpperFrameCtrl[0].frame==0);
 assert(link.mUnderFrameCtrl[0].rate==0&&link.mUpperFrameCtrl[0].rate==0);
 floorY=std::nanf("");before_title_wait(nullptr,linkArgs,&result,nullptr);assert(link.position.y==-400);
 after_title_camera(nullptr,cameraArgs,nullptr,nullptr);
 assert(camera.sets==1&&camera.mEye.x==650&&camera.mEye.y==-70&&camera.mCenter.x==-70&&camera.fov==45);
 zeldaLive=false;assert(before_title_logo(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==-1);
 zeldaLive=true;bowlLive=false;before_title_logo(nullptr,nullptr,&result,nullptr);assert(result==-1);
 bowlLive=true;floorY=-400;
 // Selection alone never starts logo/input, even if native A/Start is pressed.
 before_title_wait(nullptr,linkArgs,&result,nullptr);
 assert(title_scene_ready()&&!s_titleFlow.launched);
 before_title_logo(nullptr,nullptr,&result,nullptr);assert(result==-1);
 assert(before_title_ui(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(before_title_controls(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 const auto poseOwner=s_titlePoseOwner;
 assert(play_survivors(nullptr,nullptr)==MOD_OK&&s_titleFlow.launched&&s_titlePoseOwner==poseOwner);
 // The launch click only closes the menu. No timer may reveal the logo.
 before_title_logo(nullptr,nullptr,&result,nullptr);assert(result==-1);
 assert(before_title_controls(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 for(int i=0;i<100;++i)s_titleFlow.tick(.1f,true,true,false,true);
 assert(s_titleFlow.camera()==0&&!s_titleFlow.show_logo());
 for(int i=0;i<12;++i)s_titleFlow.tick(.1f,true,false,true,true);
 assert(s_titleFlow.camera()<.001f&&!s_titleFlow.show_logo());
 s_titleFlow.tick(.1f,true,false,true,true);assert(s_titleFlow.camera()>0&&s_titleFlow.camera()<1);
 for(int i=0;i<100;++i)s_titleFlow.tick(.1f,true,false,true,true);
 assert(s_titleFlow.camera()==1&&!s_titleFlow.show_logo()); // Holding the selection cannot reveal.
 s_titleFlow.tick(.1f,true,false,false,false);assert(s_titleFlow.armed&&!s_titleFlow.logo);
 s_titleFlow.tick(.1f,true,false,true,true);
 assert(s_titleFlow.logo&&!s_titleFlow.show_logo()); // Consume the reveal edge for one update.
 s_titleFlow.tick(.1f,true,false,true,false);
 before_title_logo(nullptr,nullptr,&result,nullptr);assert(result==1);
 assert(before_title_ui(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
 assert(before_title_controls(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
 after_title_camera(nullptr,cameraArgs,nullptr,nullptr);
 assert(camera.sets==2&&camera.mEye.x==360&&camera.mEye.y==-210&&camera.mEye.z==570&&camera.fov==40);
 modelReady=false;assert(!title_scene_ready());modelReady=true;
 // Fall detection runs after private input. It must not replace WAIT with FALL.
 result=123;assert(before_title_fall(nullptr,linkArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==0);
 assert(before_title_fall(nullptr,otherArgs,&result,nullptr)==HOOK_CONTINUE);
 assert(before_title_player_draw(nullptr,linkArgs,&result,nullptr)==HOOK_CONTINUE);
 bowlLive=false;floorY=std::nanf("");
 before_title_wait(nullptr,linkArgs,&result,nullptr);assert(s_titlePoseOwner==kNone);
 assert(before_title_player_draw(nullptr,linkArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(before_title_ui(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 // Grounding and pose do not have to wait for the asynchronous bowl model.
 floorY=-400;before_title_wait(nullptr,linkArgs,&result,nullptr);assert(s_titlePoseOwner==link.id&&!title_scene_ready());
 bowlLive=true;assert(title_scene_ready());
 // Startup is already loading its first scene: do not request a duplicate.
 scene.name=999;request_title_preview();update_title_preview();assert(!reloads&&!s_titleReloadPending&&!s_titleFlow.launched);
 scene.name=1;
 // Deselect -> vanilla; select again -> hub, including rapid carousel changes.
 s_active=false;request_title_preview();assert(s_titleReloadPending);
 transition=true;update_title_preview();assert(!reloads&&s_titleReloadPending);
 transition=false;update_title_preview();assert(reloads==1&&!reloadSelected&&!s_titleReloadPending);
 s_active=true;request_title_preview();resetting=true;update_title_preview();assert(reloads==1);
 resetting=false;returning=true;update_title_preview();assert(reloads==1);
 returning=false;update_title_preview();assert(reloads==2&&reloadSelected&&!s_titleReloadPending&&!s_titleFlow.launched);
 request_title_preview();transition=true;s_active=false;request_title_preview();s_active=true;request_title_preview();
 play_survivors(nullptr,nullptr);update_title_preview();assert(reloads==2);
 transition=false;update_title_preview();assert(reloads==3&&reloadSelected&&s_titleFlow.launched);
 // A click during a pending reload is retained, but the old actor stays hidden.
 assert(before_title_player_draw(nullptr,linkArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 before_title_wait(nullptr,linkArgs,&result,nullptr);assert(title_scene_ready());
 reset_title_presentation();assert(!s_titleFlow.launched&&s_titlePoseOwner==kNone&&!s_titleFlow.logo&&s_titleFlow.camera()==0);
 // File selection and gameplay in the SAME hub stage are not a title scene.
 for(int which=0;which<2;++which){
  if(which==0)scene.name=999;else{scene.name=1;s_gameStarted=true;}
  link.mItemButton=7;after_title_input(nullptr,linkArgs,nullptr,nullptr);assert(link.mItemButton==7);
  assert(before_title_wait(nullptr,linkArgs,&result,nullptr)==HOOK_CONTINUE);
  assert(before_title_logo(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
  assert(before_title_ui(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
  assert(before_title_controls(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
  assert(before_title_fall(nullptr,linkArgs,&result,nullptr)==HOOK_CONTINUE);
  after_title_camera(nullptr,cameraArgs,nullptr,nullptr);assert(camera.sets==2);
  info.procname=99;assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_CONTINUE&&freed==3);
 }
 std::cout<<"Survivors title: opening routing, scene isolation, kneeling pose, private input, framing and logo cue passed\n";
}
'''
native_title=(sdk/'src/d/actor/d_a_title.cpp').read_text()
assert 'else if (getDemoPrm() == 1)' in native_title
assert 'fopScnM_ChangeReq(playScene, fpcNm_NAME_SCENE_e, 0, 5);' in native_title
source=(root/'src/survivors.cpp').read_text()
assert source.index('PRE(SurvivorsStage, before_title_stage)')<source.index('PRE(SurvivorsStage, before_stage)')
assert source.index('PRE(SurvivorsArenaActor, before_title_stage_actor)')<source.index('PRE(SurvivorsArenaActor, before_arena_stage_actor)')
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'title.cpp';exe=Path(tmp)/'title'
 cpp.write_text(fixture+(root/'src/survivors_title.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'include'),'-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
