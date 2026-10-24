"""Title-only stage routing, pose/input/camera isolation and native logo flow."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
sdk=root/'dusklight'
fixture=r'''
#include "generated/roundtable_layout.hpp"
namespace roundtable_layout=dawnlight::roundtable_layout;
#include <any>
#include <algorithm>
#include <chrono>
#include "survivors_title_flow.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <initializer_list>
#include <vector>
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
constexpr int fpcNm_PLAY_SCENE_e=5;
bool s_hubArrivalPending=false,s_hub=true;float s_hubArrivalSeconds=0;unsigned s_map=0;
constexpr int fpcNm_OPENING_SCENE_e=1,fpcNm_ALINK_e=2,fpcNm_SCENE_EXIT_e=3,fpcNm_SCENE_EXIT2_e=4;
bool s_active=true,s_gameStarted=false,hasScene=true,roomReady=true,zeldaLive=true;
const char* stageName=arenas[0].stage;s8 roomNo=arenas[0].room;
const char* dComIfGp_getStartStageName(){return stageName;}
s8 dComIfGp_getStartStageRoomNo(){return roomNo;}
struct scene_class{int name=fpcNm_OPENING_SCENE_e;}scene;
using Scene=scene_class;
bool transition=false,resetting=false,returning=false,sceneCreating=false;
bool fpcM_IsCreating(int id){assert(id==17);return sceneCreating;}
bool transitioning(){return transition;}
bool paused=false;bool dComIfGp_isPauseFlag(){return paused;}
struct dScnPly_c{static bool isPause(){return false;}};
bool nextStage=false;bool dComIfGp_isEnableNextStage(){return nextStage;}
namespace mDoRst {bool isReset(){return resetting;}bool isReturnToMenu(){return returning;}}
unsigned reloads=0;bool reloadSelected=false;
bool overlapBusy=false,rejectScene=false;int queuedScene=0;
unsigned requestAttempts=0,stageWrites=0,runtimeResets=0,audioChanges=0;
bool fopOvlpM_IsDoingReq(){return overlapBusy;}
int fopScnM_ChangeReq(Scene*,int name,int,unsigned){
 ++requestAttempts;if(rejectScene||overlapBusy)return 0;
 queuedScene=name;
 if(name==fpcNm_OPENING_SCENE_e){++reloads;reloadSelected=s_active;}
 return 1;
}
const char* nextName="unchanged";s8 nextRoom=7,nextLayer=3;
const char* dComIfGp_getNextStageName(){return nextName;}
s8 dComIfGp_getNextStageRoomNo(){return nextRoom;}
s8 dComIfGp_getNextStageLayer(){return nextLayer;}
void dComIfGp_offEnableNextStage(){nextStage=false;}
void dComIfGp_setNextStage(const char*,s16,s8,s8);
void reset_runtime(){++runtimeResets;s_gameStarted=false;s_hubArrivalPending=false;s_hubArrivalSeconds=0;}
void mDoAud_setSceneName(const char* name,int room,int layer){assert(name==nextName&&room==nextRoom&&layer==nextLayer);++audioChanges;}
void dComIfGs_setRestartRoomParam(int n){assert(n==0);}
namespace JUtility{struct TColor{int r,g,b,a;TColor(int r,int g,int b,int a):r(r),g(g),b(b),a(a){}};}
struct Fader{int getStatus(){return 1;}}fader;
struct mDoGph_gInf_c{
 static Fader* getFader(){return &fader;}
 static void setFadeColor(JUtility::TColor& c){assert(c.r==0&&c.g==0&&c.b==0&&c.a==255);}
};
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
 ActorId id=7;struct{cXyz pos{0,-400,760};}current;
 enum{PROC_WAIT=1,ANM_WAIT=25};int mProcID=99,animation=0,sets=0;
 unsigned mItemButton=7,mItemTrigger=3;
 float mStickValue=1,mMoveValue=1,mNormalSpeed=4,speedF=4;
 cXyz speed{1,2,3},position;s16 yaw=0;
 Frame mUnderFrameCtrl[1],mUpperFrameCtrl[1];
 void procWaitInit(){mProcID=PROC_WAIT;}
 bool checkUnderMove0BckNoArc(int n){return animation==n;}
 void setSingleAnime(int n,float rate,float frame,s16 end,float morf){assert((rate==0||rate==1)&&frame==0&&end==-1&&morf==0);animation=n;++sets;for(auto* ctrl:{&mUnderFrameCtrl[0],&mUpperFrameCtrl[0]})ctrl->rate=rate;}
 void setPlayerPosAndAngle(const cXyz* p,s16 a,bool force){assert(force);position=*p;yaw=a;}
}link,foreign;
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
ActorId fopAcM_GetID(daAlink_c* a){return a->id;}
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
float floorY=-400;
struct Bg{float GroundCross(dBgS_ObjGndChk* p){assert(p->pos.x==0&&((p->pos.y==-300&&p->pos.z==760)||(p->pos.y==600&&p->pos.z==760)));return roomReady?floorY:std::nanf("");}}bg;
Bg& dComIfG_Bgsp(){return bg;}
bool hub_center_ground(cXyz& anchor){
 cXyz probe(0,link.current.pos.y+1000,760);dBgS_ObjGndChk ground;ground.SetPos(&probe);
 const float height=dComIfG_Bgsp().GroundCross(&ground);
 if(!std::isfinite(height)||height<=-1.e8f)return false;anchor.set(0,height,0);return true;
}
struct dCamera_c{cXyz mEye,mCenter,mUp;float fov=0;int sets=0;struct{void Val(cXyz){}}mDirection;
 void Set(cXyz c,cXyz e,float f,s16 bank){assert(bank==0);mCenter=c;mEye=e;fov=f;++sets;}}camera;
int s_hubZelda=12;
void* actor_by_id(int id){assert(id==12);return zeldaLive?&scene:nullptr;}
bool owned_hub_zelda(void* p){return p!=nullptr;}
bool modelReady=true;bool title_model_ready(daAlink_c*){return modelReady;}
constexpr unsigned PAD_1=0;
unsigned padTrigger=0;bool menuVisible=false;
struct mDoCPd_c{
 static unsigned getHold(unsigned){return 0;}static unsigned getTrig(unsigned){return padTrigger;}
 static bool getTrigA(unsigned){return padTrigger&1;}static bool getTrigStart(unsigned){return padTrigger&2;}
};
bool ui_visible(){return menuVisible;}
void finish_survivors_title_logo(void*);
struct LogoAnimation{float frame=3,end=60;void setFrame(float f){frame=f;}float getEndFrame(){return end;}};
struct LogoPane{unsigned starts=0;void alphaAnimeStart(int n){assert(n==0);++starts;}};
struct daTitle_c{
 LogoAnimation mBck,mBpk,mBrk,mBtk;LogoPane pane;LogoPane* field_0x600=&pane;
 int field_0x604=4,field_0x5f9=0,field_0x5fa=0,mIsDispLogo=1,mProcID=2,mWaitTimer=0;
 void fastLogoDispInit();void fastLogoDisp();void keyWaitInit();void keyWait();void nextScene_init();
};
unsigned presentationSyncs=0,enterSounds=0;
namespace dusk::interp{void request_presentation_sync(){++presentationSyncs;}}
#define IF_DUSK(...) __VA_ARGS__
constexpr int Z2SE_TITLE_ENTER=42;
void mDoAud_seStart(int sound,void*,int a,int b){assert(sound==42&&a==0&&b==0);++enterSounds;}
'''
native_fixture=r'''
// Model the SDK's next-stage enable latch and Draw's independent PLAY request,
// which the old always-successful changeOpeningScene stub did not exercise.
void dComIfGp_setNextStage(const char* name,s16 point,s8 room,s8 layer){
 std::any args[]{name,point,room,layer};
 if(before_title_stage(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL)return;
 ++stageWrites;
 if(!nextStage){nextStage=true;nextName=mods::arg<const char*>(args,0);
  nextRoom=mods::arg<s8>(args,2);nextLayer=mods::arg<s8>(args,3);}
}
void native_draw_transition(){
 if(!transition&&nextStage)fopScnM_ChangeReq(&scene,fpcNm_PLAY_SCENE_e,0,5);
}
void route_opening(void* args){
 before_title_opening(nullptr,nullptr,nullptr,nullptr);
 before_title_stage(nullptr,args,nullptr,nullptr);
 after_title_opening(nullptr,nullptr,nullptr,nullptr);
 assert(!s_titleOpeningDepth);
}
'''
main=r'''
int main(){
 int outerCall=0,innerCall=0;
 after_title_opening(nullptr,&innerCall,nullptr,nullptr);assert(!s_titleOpeningDepth);
 before_title_opening(nullptr,&outerCall,nullptr,nullptr);
 after_title_opening(nullptr,&innerCall,nullptr,nullptr);assert(s_titleOpeningDepth==1);
 before_title_opening(nullptr,&innerCall,nullptr,nullptr);assert(s_titleOpeningDepth==2);
 after_title_opening(nullptr,&innerCall,nullptr,nullptr);assert(s_titleOpeningDepth==1);
 after_title_opening(nullptr,&outerCall,nullptr,nullptr);assert(!s_titleOpeningDepth&&s_titleOpeningCalls.empty());
 for(int rate:{30,60,120}){
  TitleFlow flow;flow.play();
  for(int i=0;i<rate*5;++i)flow.tick(1.f/rate,false,false,false,false);
  assert(flow.camera()==0&&!flow.logo);
  for(int i=0;i<rate*6;++i)flow.tick(1.f/rate,true,false,false,false);
  assert(flow.camera()==1&&!flow.logo&&flow.armed);
  flow.tick(1.f/rate,true,false,true,true);assert(flow.logo&&!flow.show_logo());
  flow.tick(1.f/rate,true,false,false,false);assert(flow.show_logo());
  flow.play();assert(flow.camera()==0&&!flow.logo&&!flow.armed);
  // A held Play input cannot reveal the logo, including during the move.
  for(int i=0;i<rate*3/2;++i)flow.tick(1.f/rate,true,false,true,true);
  assert(flow.camera()>0&&flow.camera()<1&&!flow.logo&&!flow.armed);
  flow.tick(1.f/rate,true,false,false,false);assert(flow.armed);
  flow.tick(1.f/rate,true,false,true,true);
  assert(flow.logo&&!flow.show_logo()&&flow.camera()<1);
  const float revealedAt=flow.camera();
  flow.tick(1.f/rate,true,false,false,false);
  assert(flow.show_logo()&&flow.camera()>revealedAt&&flow.camera()<1);
  for(int i=0;i<rate*3;++i)flow.tick(1.f/rate,true,false,false,false);
  assert(flow.show_logo()&&flow.camera()==1);
  // New input during the menu fade still waits for the camera phase.
  flow.play();flow.tick(.1f,true,false,false,false);
  flow.tick(.1f,true,false,true,true);assert(!flow.logo&&flow.camera()==0);
 }
 assert(survivors_title_active());
 int startDemoResult=123;
 assert(before_title_start_demo(nullptr,nullptr,&startDemoResult,nullptr)==HOOK_SKIP_ORIGINAL&&startDemoResult==-2);
 // Deselecting the mode must not release the still-loading custom title.
 s_active=false;
 assert(survivors_title_active());
 assert(!survivors_title_preparing());
 assert(before_title_start_demo(nullptr,nullptr,&startDemoResult,nullptr)==HOOK_SKIP_ORIGINAL&&startDemoResult==-2);
 s_active=true;
 for(int which=1;which<5;++which){
  if(which==1)s_gameStarted=true;if(which==2)scene.name=999;
  if(which==3)hasScene=false;if(which==4)roomNo=0;
  assert(!survivors_title_active());
  startDemoResult=123;assert(before_title_start_demo(nullptr,nullptr,&startDemoResult,nullptr)==HOOK_CONTINUE&&startDemoResult==123);
  s_active=true;s_gameStarted=false;scene.name=1;hasScene=true;roomNo=51;
 }
 std::any args[]{static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
 route_opening(args);
 assert(std::strcmp(mods::arg<const char*>(args,0),arenas[0].stage)==0);
 assert(mods::arg<s16>(args,1)==0&&mods::arg<s8>(args,2)==51&&mods::arg<s8>(args,3)==0);
 for(int which=0;which<5;++which){
  std::any original[]{static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
  if(which==0)s_active=false;if(which==1)s_gameStarted=true;
  if(which==2)original[1]=s16(0);if(which==3)original[2]=s8(1);if(which==4)original[3]=s8(0);
  route_opening(original);
  assert(std::strcmp(mods::arg<const char*>(original,0),"F_SP102")==0);
  s_active=true;s_gameStarted=false;
 }
 stage_actor_data_class record{{'L','i','n','k','N','a','m','e'}};fopAcM_prm_class params;
 std::any actorArgs[]{&record,&params};
 assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_CONTINUE);
 assert(((params.base.parameters>>12)&0x1f)==16&&((params.base.parameters>>24)&0xff)==0xff);
 assert(params.base.position.x==0&&params.base.position.z==760&&params.base.position.y==-400);
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
 assert(link.sets==1&&link.position.z==760&&link.position.y==-400&&link.yaw==s16(0x8000));
 assert(link.mUnderFrameCtrl[0].frame==0&&link.mUpperFrameCtrl[0].frame==0);
 assert(link.mUnderFrameCtrl[0].rate==0&&link.mUpperFrameCtrl[0].rate==0);
 floorY=std::nanf("");before_title_wait(nullptr,linkArgs,&result,nullptr);assert(link.position.y==-400);
 after_title_camera(nullptr,cameraArgs,nullptr,nullptr);
 assert(camera.sets==0); // No valid floor: do not install a camera on a NaN anchor.
 floorY=-400;after_title_camera(nullptr,cameraArgs,nullptr,nullptr);
 assert(camera.sets==1&&std::abs(camera.mEye.x-700)<.001f&&std::abs(camera.mEye.z-1250)<.001f&&camera.mEye.y==0&&camera.mCenter.x==-80&&camera.fov==52);
 zeldaLive=false;assert(before_title_logo(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==-1);
 zeldaLive=true;roomReady=false;before_title_logo(nullptr,nullptr,&result,nullptr);assert(result==-1);
 roomReady=true;floorY=-400;
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
 assert(camera.sets==2&&std::abs(camera.mEye.x-640.f)<.01f&&
  camera.mEye.y==-150&&std::abs(camera.mEye.z-850)<.01f&&camera.fov==48);
 assert(survivors_title_preparing());
 transition=true;assert(survivors_title_preparing()); // Fade-in does not serialize resource loads.
 nextStage=true;assert(!survivors_title_preparing());nextStage=false;
 resetting=true;assert(!survivors_title_preparing());resetting=false;transition=false;
 modelReady=false;assert(!title_scene_ready());modelReady=true;
 // Fall detection runs after private input. It must not replace WAIT with FALL.
 result=123;assert(before_title_fall(nullptr,linkArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==0);
 assert(before_title_fall(nullptr,otherArgs,&result,nullptr)==HOOK_CONTINUE);
 assert(before_title_player_draw(nullptr,linkArgs,&result,nullptr)==HOOK_CONTINUE);
 roomReady=false;floorY=std::nanf("");
 before_title_wait(nullptr,linkArgs,&result,nullptr);assert(s_titlePoseOwner==kNone);
 assert(before_title_player_draw(nullptr,linkArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(before_title_ui(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 // Grounding and pose do not have to wait for Zelda to finish loading.
 roomReady=true;zeldaLive=false;floorY=-400;before_title_wait(nullptr,linkArgs,&result,nullptr);assert(s_titlePoseOwner==link.id&&!title_scene_ready());
 zeldaLive=true;assert(title_scene_ready());
 // Startup is already loading its first scene: do not request a duplicate.
 scene.name=999;request_title_preview();update_title_preview();assert(!reloads&&s_titleReloadPending&&!s_titleFlow.launched);
 std::any startup[]{static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
 route_opening(startup);assert(!s_titleReloadPending);
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
 // A loaded save replaces only its initial hub entrance, in PLAY_SCENE.
 s_gameStarted=true;s_hubArrivalPending=true;scene.name=fpcNm_PLAY_SCENE_e;
 assert(survivors_hub_arrival_active()&&!survivors_title_active());
 info.procname=fpcNm_ALINK_e;params.base.position.x=999;params.base.position.z=999;
 before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr);
 assert(params.base.position.x==0&&params.base.position.z==760&&params.base.angle.y==s16(0x8000));
 assert(((params.base.parameters>>12)&0x1f)==16&&((params.base.parameters>>24)&0xff)==0xff);
 assert(before_title_start_demo(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==-2);
 info.procname=99;assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_CONTINUE);
 assert(before_title_logo(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
 assert(before_title_ui(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
 for(int gate=0;gate<9;++gate){
  if(gate==0)s_active=false;if(gate==1)s_gameStarted=false;if(gate==2)s_hub=false;
  if(gate==3)s_map=1;if(gate==4)scene.name=1;if(gate==5)scene.name=999;
  if(gate==6)nextStage=true;if(gate==7)resetting=true;if(gate==8)returning=true;
  assert(!survivors_hub_arrival_active());
  s_active=true;s_gameStarted=true;s_hub=true;s_map=0;scene.name=fpcNm_PLAY_SCENE_e;
  nextStage=resetting=returning=false;
 }
 before_title_wait(nullptr,linkArgs,&result,nullptr);
 assert(link.position.z==760&&link.position.y==-400&&title_scene_ready());
 // Missing scenery, fade-in and pause must not consume the visible rise.
 for(int gate=0;gate<4;++gate){
  if(gate==0)roomReady=false;if(gate==1)zeldaLive=false;if(gate==2)transition=true;if(gate==3)paused=true;
  s_titleClock=std::chrono::steady_clock::now()-std::chrono::seconds(1);
  after_title_camera(nullptr,cameraArgs,nullptr,nullptr);assert(s_hubArrivalSeconds==0);
  roomReady=zeldaLive=true;transition=paused=false;
 }
 for(int rate:{30,60,120}){
  s_hubArrivalPending=true;s_hubArrivalSeconds=0;
  for(int i=0;i<rate*10;++i)advance_hub_arrival(1.f/rate,false);
  assert(hub_arrival_progress()==0&&s_hubArrivalPending);
  for(int i=0;i<rate;++i)advance_hub_arrival(1.f/rate,true);
  assert(s_hubArrivalPending&&hub_arrival_progress()>.9f&&hub_arrival_progress()<1);
  for(int i=0;i<rate;++i)advance_hub_arrival(1.f/rate,true);
  assert(!s_hubArrivalPending&&hub_arrival_progress()==1&&link.mUnderFrameCtrl[0].rate==1);
 }
 link.mItemButton=7;after_title_input(nullptr,linkArgs,nullptr,nullptr);assert(link.mItemButton==7);
 assert(before_title_wait(nullptr,linkArgs,&result,nullptr)==HOOK_CONTINUE);
 assert(before_title_start_demo(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
 // Repeated carousel changes at different asynchronous load phases. The hub
 // request has already been routed when selection changes to vanilla (Play).
 s_gameStarted=false;scene.name=fpcNm_OPENING_SCENE_e;roomNo=arenas[0].room;
 for(int cycle=0;cycle<60;++cycle){
  s_active=true;stageName=arenas[0].stage;roomNo=arenas[0].room;
  request_title_preview();
  std::any hub[]{static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
  route_opening(hub);assert(!s_titleReloadPending);
  s_active=false;request_title_preview();sceneCreating=true;
  const auto previousReloads=reloads;
  transition=(cycle%2)==0;update_title_preview();assert(reloads==previousReloads&&s_titleReloadPending);
  transition=false;update_title_preview();assert(reloads==previousReloads&&s_titleReloadPending);
  result=123;assert(before_title_start_demo(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==-2);
  info.procname=fpcNm_ALINK_e;params.base.parameters=0;
  before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr);
  assert(((params.base.parameters>>12)&0x1f)==16&&((params.base.parameters>>24)&0xff)==0xff);
  info.procname=99;assert(before_title_stage_actor(nullptr,actorArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
  assert(before_title_controls(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
  // A further choice while no scene can be looked up must survive the gap.
  hasScene=false;s_active=(cycle%3)!=0;request_title_preview();
  if(s_active)play_survivors(nullptr,nullptr);
  update_title_preview();assert(reloads==previousReloads&&s_titleReloadPending);
  hasScene=true;sceneCreating=false;update_title_preview();
  assert(reloads==previousReloads+1&&reloadSelected==s_active&&!s_titleReloadPending);
  std::any next[]{static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
  route_opening(next);
  stageName=mods::arg<const char*>(next,0);roomNo=mods::arg<s8>(next,2);
  assert((std::strcmp(stageName,arenas[0].stage)==0)==s_active);
  assert(survivors_title_active()==s_active);
  result=123;
  assert(before_title_start_demo(nullptr,nullptr,&result,nullptr)==(s_active?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE));
  assert(result==(s_active?-2:123));
  assert(!s_gameStarted&&!s_hubArrivalPending&&s_titleFlow.launched==s_active);
  update_title_preview();assert(reloads==previousReloads+1);
 }
 // Log regression: vanilla demo38_01 is still running after Survivors is
 // selected. Its setNextStage must not route the hub through native Draw's
 // PLAY_SCENE path (which starts BS_SHUTTER_START without a TITLE actor).
 s_active=true;s_gameStarted=false;stageName="F_SP102";roomNo=0;nextStage=false;
 request_title_preview();queuedScene=0;
 auto writes=stageWrites;
 dComIfGp_setNextStage("F_SP102",100,0,10);
 dComIfGp_setNextStage("F_SP102",101,0,10);
 native_draw_transition();
 assert(stageWrites==writes&&!nextStage&&!queuedScene&&s_titleReloadPending);
 // IsPeek may be false at either edge of an active overlap. No request or
 // level-data mutation is allowed then, nor after a rejected ChangeReq.
 const auto attempts=requestAttempts,resets=runtimeResets,audio=audioChanges;
 overlapBusy=true;transition=false;update_title_preview();
 assert(requestAttempts==attempts&&stageWrites==writes&&s_titleReloadPending);
 overlapBusy=false;rejectScene=true;update_title_preview();
 assert(requestAttempts==attempts+1&&stageWrites==writes&&s_titleReloadPending);
 assert(runtimeResets==resets&&audioChanges==audio&&!nextStage&&!queuedScene);
 rejectScene=false;play_survivors(nullptr,nullptr);update_title_preview();
 assert(queuedScene==fpcNm_OPENING_SCENE_e&&!nextStage&&!s_titleReloadPending&&s_titleFlow.launched);
 assert(!std::strcmp(nextName,arenas[0].stage)&&nextRoom==51&&nextLayer==0);
 assert(stageWrites==writes+1&&runtimeResets==resets+1&&audioChanges==audio+1);
 native_draw_transition();assert(queuedScene==fpcNm_OPENING_SCENE_e);
 // A late event cannot overwrite the accepted preview or arm another reload.
 dComIfGp_setNextStage("F_SP102",100,0,10);native_draw_transition();
 assert(stageWrites==writes+1&&!nextStage&&!s_titleReloadPending&&queuedScene==fpcNm_OPENING_SCENE_e);
 // Reverse switch: the outgoing hub cannot activate a map-tool exit either.
 stageName=arenas[0].stage;roomNo=51;s_active=false;request_title_preview();
 dComIfGp_setNextStage("D_MN08",0,0,0);assert(!nextStage);
 update_title_preview();assert(!std::strcmp(nextName,"F_SP102")&&nextRoom==0&&nextLayer==10);
 native_draw_transition();assert(queuedScene==fpcNm_OPENING_SCENE_e&&!nextStage);
 // Once vanilla is loaded, its own transitions work. NAME_SCENE / actual
 // gameplay requests also retain their parameters and native enable flag.
 stageName="F_SP102";roomNo=0;
 for(int gate=0;gate<3;++gate){
  s_active=gate!=0;scene.name=gate==1?999:(gate==2?fpcNm_PLAY_SCENE_e:fpcNm_OPENING_SCENE_e);
  s_gameStarted=gate==2;nextStage=false;queuedScene=0;
  dComIfGp_setNextStage("F_SP108",21,1,13);native_draw_transition();
  assert(nextStage&&!std::strcmp(nextName,"F_SP108")&&nextRoom==1&&nextLayer==13&&queuedScene==fpcNm_PLAY_SCENE_e);
 }
 // Sample the production camera through the whole path. Distance, side angle,
 // height and FOV change together; revealing the logo introduces no jump.
 s_active=true;s_gameStarted=false;scene.name=fpcNm_OPENING_SCENE_e;
 stageName=arenas[0].stage;roomNo=51;nextStage=false;transition=true;
 reset_title_presentation();s_titleFlow.play();
 float lastRadius=0,lastAngle=0,lastFov=0;cXyz lastEye;
 for(int i=0;i<=240;++i){
  s_titleFlow.elapsed=TitleFlow::menuFade+TitleFlow::cameraDuration*(i/240.f);
  if(i==60)s_titleFlow.logo=true;
  after_title_camera(nullptr,cameraArgs,nullptr,nullptr);
  const float radius=std::hypot(camera.mEye.x,camera.mEye.z-760);
  const float angle=std::atan2(camera.mEye.x,camera.mEye.z-760);
  if(i){
   assert(radius<lastRadius&&angle>lastAngle&&camera.fov<lastFov);
   assert(camera.mEye.y<lastEye.y);
   assert(std::hypot(camera.mEye.x-lastEye.x,camera.mEye.z-lastEye.z)<10);
  }
  if(i==120){assert(radius<800&&radius>500&&angle>1&&angle<1.5f&&camera.fov<52&&camera.fov>48);}
  lastRadius=radius;lastAngle=angle;lastFov=camera.fov;lastEye=camera.mEye;
 }
 assert(std::abs(camera.mEye.x-640.f)<.01f&&
  std::abs(camera.mEye.z-850)<.01f&&camera.mEye.y==-150&&camera.fov==48);
 // Skip the native logo animation with a fresh input, but never reuse the
 // reveal edge or advance to NAME_SCENE on the animation-completion input.
 transition=false;s_titleReloadPending=false;s_titlePoseOwner=link.id;
 s_titleFlow.logo=true;s_titleFlow.revealEdge=false;
 daTitle_c title;std::any titleArgs[]{static_cast<void*>(&title)};
 title.mBck.end=40;title.mBpk.end=50;title.mBrk.end=60;title.mBtk.end=70;
 for(int gate=0;gate<9;++gate){
  padTrigger=1;
  if(gate==0)s_active=false;if(gate==1)scene.name=fpcNm_PLAY_SCENE_e;
  if(gate==2)s_titleFlow.logo=false;if(gate==3)s_titleFlow.revealEdge=true;
  if(gate==4)s_titleReloadPending=true;if(gate==5)menuVisible=true;
  if(gate==6)transition=true;if(gate==7)roomReady=false;if(gate==8)padTrigger=0;
  assert(before_title_logo_animation(nullptr,titleArgs,nullptr,nullptr)==HOOK_CONTINUE);
  assert(title.mProcID==2&&title.mBck.frame==3&&!presentationSyncs);
  s_active=true;scene.name=fpcNm_OPENING_SCENE_e;s_titleFlow.logo=true;s_titleFlow.revealEdge=false;
  s_titleReloadPending=menuVisible=transition=false;roomReady=true;
 }
 const float cameraProgress=s_titleFlow.camera();padTrigger=16;
 assert(before_title_logo_animation(nullptr,titleArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(title.mBck.frame==39&&title.mBpk.frame==49&&title.mBrk.frame==59&&title.mBtk.frame==69);
 assert(title.mWaitTimer==0&&title.mProcID==3&&title.mIsDispLogo==1&&title.field_0x5f9==1&&title.field_0x5fa==1);
 assert(title.pane.starts==1&&presentationSyncs==1&&!enterSounds&&s_titleFlow.camera()==cameraProgress);
 padTrigger=0;title.keyWait();assert(title.mProcID==3&&!enterSounds);
 padTrigger=1;title.keyWait();assert(title.mProcID==4&&enterSounds==1);
 std::cout<<"Survivors title: scene routing, smooth camera, early logo reveal and native logo animation skip passed\n";
}
'''
native_event=(sdk/'src/d/d_event_manager.cpp').read_text()
# 0xff does not disable DEFAULT_START; -2 is the native no-demo sentinel.
assert 'if (mException.mEventInfoIdx == -2)' in native_event
assert 'evId = getEventIdx("DEFAULT_START", 0xFF, -1);' in native_event
native_title=(sdk/'src/d/actor/d_a_title.cpp').read_text()
assert 'else if (getDemoPrm() == 1)' in native_title
assert 'fopScnM_ChangeReq(playScene, fpcNm_NAME_SCENE_e, 0, 5);' in native_title
def extract_function(text,signature):
 start=text.index(signature);opening=text.index('{',start);depth=0
 for end in range(opening,len(text)):
  depth+=(text[end]=='{')-(text[end]=='}')
  if not depth:return text[start:end+1]+'\n'
 raise AssertionError(signature)
# Execute the real SDK final-frame/key-wait routines with lightweight animation
# objects, plus our production adapter, rather than mocking successful skipping.
native_logo=''.join(extract_function(native_title,'void daTitle_c::'+name+'()')
 for name in ['fastLogoDispInit','fastLogoDisp','keyWaitInit','keyWait','nextScene_init'])
native_logo+=extract_function((root/'src/survivors_title_logo.cpp').read_text(),
 'void finish_survivors_title_logo(void* actor)')
native_stage=(sdk/'src/d/d_stage.cpp').read_text()
assert 'if (!enabled) {\n        enabled = true;' in native_stage
native_scene=(sdk/'src/d/d_s_play.cpp').read_text()
assert 'fopScnM_ChangeReq(i_this, fpcNm_PLAY_SCENE_e, l_wipeType[wipe], 5)' in native_scene
source=(root/'src/survivors.cpp').read_text()
assert source.index('PRE(SurvivorsStage, before_title_stage)')<source.index('PRE(SurvivorsStage, before_stage)')
assert source.index('PRE(SurvivorsArenaActor, before_title_stage_actor)')<source.index('PRE(SurvivorsArenaActor, before_arena_stage_actor)')
assert 'PRE(SurvivorsOpening, before_title_opening)' in source
assert 'POST(SurvivorsOpening, after_title_opening)' in source
assert 'PRE(SurvivorsTitleLogoAnimation, before_title_logo_animation)' in source
assert 'src/survivors_title_logo.cpp' in (root/'CMakeLists.txt').read_text()
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'title.cpp';exe=Path(tmp)/'title'
 cpp.write_text(fixture+(root/'src/survivors_title.inc').read_text()+native_fixture+native_logo+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'include'),'-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
