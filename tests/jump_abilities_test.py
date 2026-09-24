"""Compile production ZR ability logic against a small native-engine fixture.

Run: python3 tests/jump_abilities_test.py. No game assets required.
The fixture validates transitions and ownership; animation/rendering need in-game QA.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
hooks = (root / 'src/jump_hooks.cpp').read_text()
config = (root / 'src/config.cpp').read_text()
abilities = (root / 'src/jump_abilities.inc').read_text()

def function(signature, source):
    start = source.index(signature + '(')
    return source[start:source.index('\n}', start) + 2]

fixture = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
using ActorId=int;using s16=short;using u16=unsigned short;
constexpr int kSwordItem=0x103;
constexpr int fpcM_ERROR_PROCESS_ID_e=-1, fpcNm_NI_e=7, PAD_1=0, MOD_OK=0;
constexpr bool TRUE=true,FALSE=false;
constexpr int dRes_ID_ALANM_BCK_WALKHBS_e=1;
struct cXyz {float x=0,y=0,z=0; cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 void set(float a,float b,float c){x=a;y=b;z=c;}
 cXyz operator+(const cXyz& b)const{return {x+b.x,y+b.y,z+b.z};}};
struct Angles {int x=0,y=0,z=0;};
struct fopAc_ac_c {int id=42,profile=fpcNm_NI_e;struct Pos {cXyz pos;Angles angle;} current,old;};
struct Hio {struct {struct {float mGravity=-3.4f,mMaxFallSpeed=-100,mCuccoJumpMaxSpeed=20,mCuccoFallMaxSpeed=-7;} m;} mAutoJump;} hio;
bool glide=false,gale=false,event=false,stage=false,pressed=false,held=false,bPressed=false,rjump=true;
int heightPercent=100,s_jumpHeight=1,galePercent=500,s_galeHeight=2,marks=0;
bool hasConfig=true;
int get_int(int handle,int fallback,int low,int high){return hasConfig?std::clamp(handle==s_galeHeight?galePercent:heightPercent,low,high):fallback;}
bool glide_enabled(){return glide;} bool revalis_gale_enabled(){return gale;}
bool dComIfGp_event_runCheck(){return event;} bool dComIfGp_isEnableNextStage(){return stage;}
struct daAlink_c: fopAc_ac_c {
 enum daAlink_PROC {PROC_WAIT,PROC_MOVE,PROC_CROUCH,PROC_AUTO_JUMP,PROC_FALL,PROC_LAND,PROC_CUT_JUMP,PROC_DAMAGE};
 enum {MODE_SWIMMING=1,ANM_JUMP_LAND=1,UPPER_1=1};
 u16 mProcID=PROC_WAIT;
 struct {bool ground=true;bool ChkGroundHit(){return ground;}} mLinkAcch;
 struct {fopAc_ac_c* actor=nullptr;auto* getActor(){return actor;}} mGrabItemAcKeep;
 struct {int field_0x3008=0;} mProcVar0;
 struct {int field_0x300c=0;} mProcVar2;
 cXyz speed{0,25,0};Angles shape_angle;Hio* mpHIO=&hio;
 float mNormalSpeed=0,mFallHeight=100,mMaxSpeed=26,field_0x3478=0,field_0x33e4=0,gravity=-3.4f;
 int mMoveAngle=123,mEquipItem=0;float speedF=0;
 int field_0x3198=0,field_0x2f99=0,launches=0,frees=0;
 bool input=false,wolf=false,cutscene=false,boots=false,heavy=false,swim=false,demo=false,sumo=false,action=false,valid=true,initSucceeds=true;
 bool checkInputOnR(){return input;} bool checkWolf(){return wolf;} bool checkEventRun(){return cutscene;}
 bool checkMagneBootsOn(){return boots;} bool checkBootsOrArmorHeavy(){return heavy;}
 bool checkModeFlg(int){return swim;} bool checkPlayerDemoMode(){return demo;} bool getSumouMode(){return sumo;}
 void freeGrabItem(){++frees;mGrabItemAcKeep.actor=nullptr;}
 void resetUpperAnime(int,float){} void setSpecialGravity(float g,float,bool){gravity=g;}
 void deleteEquipItem(bool,bool){} void setGrabItemActor(fopAc_ac_c* a){mGrabItemAcKeep.actor=a;}
 void setGrabUpperAnime(float){} void commonProcInit(daAlink_PROC p){mProcID=p;}
 void offModeFlg(int){} void setSingleAnimeBaseSpeed(int,int,float){} void setUpperAnime(int,int,int,int,int,int){}
 void setGrabItemPos(){} void procWaitInit();
 bool procCrouchInit();
 bool procAutoJumpInit(int);
 bool procCutJumpInit(bool){mProcID=PROC_CUT_JUMP;return true;}
};
int fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
int fopAcM_GetRoomNo(fopAc_ac_c*){return 3;}void fopAcM_offDraw(fopAc_ac_c*){}
fopAc_ac_c cucco;bool creating=false,live=false,busy=false,cancelOk=true,deleteOk=true,spawnOk=true;
int deletes=0,cancels=0,spawns=0;
bool fpcM_IsCreating(int id){return id==cucco.id&&creating;}
void fopAcM_SearchByID(int id,fopAc_ac_c** a){*a=(live&&id==cucco.id)?&cucco:nullptr;}
struct create_request {int id=42;} request;
struct create_tag {struct {void* mpTagData=&request;} base;create_tag* next=nullptr;} tag;
struct {create_tag* mpHead=&tag;} g_fpcCtTg_Queue;
#define NODE_GET_NEXT(n) ((n)->next)
bool fpcCtRq_IsDoing(create_request*){return busy;}
bool fpcCtRq_Cancel(create_request*){++cancels;if(cancelOk)creating=false;return cancelOk;}
struct ActorSpawnParams {int parameters,argument,room_num;cXyz position;Angles angle;cXyz scale;void* create_function;};
int create_standalone_actor(int profile,const ActorSpawnParams& p,int& id){
 assert(profile==fpcNm_NI_e&&p.parameters==0&&p.angle.x==-1&&p.room_num==3);
 ++spawns;if(!spawnOk)return 1;id=cucco.id;creating=true;return MOD_OK;
}
struct Service {int delete_actor(void*,int id){assert(id==cucco.id);++deletes;if(!deleteOk)return 1;live=false;return MOD_OK;}} service;
auto* svc_actor=&service;void* mod_ctx=nullptr;
struct Visual {int launches=0;void release(){}void prepare(){}void launch(const cXyz&){++launches;}} s_galeVisual;
daAlink_c* s_manualJumpOwner=nullptr;
int active_jump_binding(){return 0;}bool jump_pressed(int){return pressed;}bool jump_held(int){return held;}
namespace mDoCPd_c {bool getHoldB(int){return bPressed;}bool getTrigB(int){return bPressed;}}
bool r_action_context_active(daAlink_c* l){return l->action;}
bool ground_jump_context_ready(daAlink_c* l){return l->valid&&l->mLinkAcch.ground;}
void apply_manual_jump_movement(daAlink_c* l){l->mLinkAcch.ground=false;if(!l->input)l->mNormalSpeed=0;}
void mark_manual_jump_started(daAlink_c*){++marks;}
// HEIGHT
bool r_jump_enabled(){return rjump;}
void set_manual_jump_direction(daAlink_c* l){if(l->input)l->shape_angle.y=l->mMoveAngle;l->current.angle.y=l->shape_angle.y;}
float sprint_jump_speed_multiplier(daAlink_c*){return 1;}
void apply_sprint_jump_speed(daAlink_c*,float){}
void clear_manual_jump(daAlink_c*){}
// START_JUMP
// ABILITIES
void daAlink_c::procWaitInit(){jump_abilities_proc_change(this,PROC_WAIT);mProcID=PROC_WAIT;}
bool daAlink_c::procCrouchInit(){if(!initSucceeds)return false;jump_abilities_proc_change(this,PROC_CROUCH);mProcID=PROC_CROUCH;return true;}
bool daAlink_c::procAutoJumpInit(int){if(!initSucceeds)return false;++launches;jump_abilities_proc_change(this,PROC_AUTO_JUMP);mProcID=PROC_AUTO_JUMP;speed.y=25;mNormalSpeed=20;speedF=26;mMaxSpeed=26;return true;}
void close(float a,float b){assert(std::fabs(a-b)<0.001f);}
void setup(daAlink_c& l){
 l=daAlink_c{};l.id=1;s_jumpAbilities=JumpAbilities{};s_jumpAbilities.owner=&l;s_jumpAbilities.ownerId=l.id;
 glide=gale=event=stage=pressed=held=bPressed=creating=live=busy=false;
 cancelOk=deleteOk=spawnOk=rjump=true;g_fpcCtTg_Queue.mpHead=&tag;heightPercent=100;galePercent=500;
}
bool tick(daAlink_c& l,bool trigger,bool hold){s_jumpAbilities.handled=false;pressed=trigger;held=hold;return handle_jump_abilities(&l);}
void land(daAlink_c& l){l.mLinkAcch.ground=true;assert(tick(l,false,true));assert(s_jumpAbilities.charge==GaleCharge::Ready);assert(l.mProcID==daAlink_c::PROC_CROUCH);close(l.mNormalSpeed,0);close(l.speedF,0);}
void charge(daAlink_c& l){gale=true;assert(tick(l,true,true));assert(l.launches==1);assert(s_jumpAbilities.charge==GaleCharge::Airborne);assert(!tick(l,false,true));land(l);}
int main(){
 daAlink_c l;
 hasConfig=false;heightPercent=500;close(jump_height_multiplier(),1);hasConfig=true;
 for(int percent:{0,100,200,500,900}){
  setup(l);heightPercent=percent;float h=std::clamp(percent,100,500)/100.0f;
  close(jump_height_multiplier(),h);l.mNormalSpeed=12;
  apply_manual_jump_height(&l,jump_height_multiplier());close(l.speed.y*l.speed.y,625*h);close(l.mNormalSpeed,12);close(l.gravity,-3.4f);
 }
 // Default, file-value clamping, and addition in actual Gale launches.
 hasConfig=false;galePercent=1000;close(gale_height_bonus(),5);hasConfig=true;
 for(int jump:{100,200,500})for(int bonus:{-20,100,500,1000,2000}){
  setup(l);heightPercent=jump;galePercent=bonus;
  const float additional=std::clamp(bonus,100,1000)/100.0f;
  close(gale_height_bonus(),additional);
  gale=true;assert(tick(l,true,true));
  close(l.speed.y,25*std::sqrt(jump/100.0f)); // First jump gets no Gale bonus.
  land(l);assert(tick(l,false,false));close(l.speed.y,25*std::sqrt(jump/100.0f+additional));
 }
 setup(l);heightPercent=200;charge(l);assert(tick(l,false,false));close(l.speed.y,25*std::sqrt(7.0f));
 setup(l);assert(!tick(l,true,true));assert(l.launches==0); // Disabled leaves native/manual jump path alone.
 setup(l);gale=true;assert(tick(l,true,true));assert(l.launches==1);close(l.speed.y,25);
 assert(!tick(l,false,false));assert(s_jumpAbilities.charge==GaleCharge::Idle&&l.launches==1);
 l.mLinkAcch.ground=true;assert(!tick(l,false,true)); // Re-press/hold after cancelling must not reuse the old jump.
 setup(l);charge(l);assert(!handle_jump_abilities(&l)); // No double processing in one execute.
 assert(tick(l,false,false));assert(l.launches==2);close(l.speed.y,25*std::sqrt(6.0f));assert(!l.mLinkAcch.ground);assert(s_manualJumpOwner==&l);
 setup(l);heightPercent=500;charge(l);assert(tick(l,false,false));close(l.speed.y,25*std::sqrt(10.0f));
 setup(l);rjump=false;charge(l);assert(tick(l,false,false));assert(l.launches==2); // Gale includes the first jump.
 // Landing drives readiness, regardless of flight length; no elapsed-time charge.
 for(int airtime:{1,10,60,180}){
  setup(l);gale=true;assert(tick(l,true,true));
  for(int i=0;i<airtime;++i){assert(!tick(l,false,true));assert(s_jumpAbilities.charge==GaleCharge::Airborne);}
  land(l);assert(tick(l,false,false));assert(l.launches==2);
 }
 // Save exact pre-jump ground speed, not the native initializer's reset or the later stick direction.
 for(float speed:{0.0f,11.5f,23.0f,34.5f,69.0f}){
  setup(l);l.input=true;l.mProcID=daAlink_c::PROC_MOVE;l.mNormalSpeed=speed;l.mMoveAngle=-8192;
  charge(l);close(s_jumpAbilities.launchSpeed,speed);assert(s_jumpAbilities.launchAngle==-8192);
  l.mMoveAngle=16384;
  for(int i=0;i<90;++i){assert(tick(l,false,true));close(l.mNormalSpeed,0);close(l.speedF,0);}
  l.input=false;assert(tick(l,false,false));close(l.mNormalSpeed,speed);close(l.speedF,speed);
  assert(l.mMaxSpeed>=speed&&l.current.angle.y==-8192&&l.shape_angle.y==-8192);close(l.speed.y,25*std::sqrt(6.0f));
  assert(s_jumpAbilities.charge==GaleCharge::Idle); // The Gale launch must not arm itself again.
 }
 for(int scenario=0;scenario<7;++scenario){
  setup(l);charge(l);
  switch(scenario){case 0:gale=false;break;case 1:l.swim=true;break;case 2:event=true;break;case 3:stage=true;break;case 4:l.wolf=true;break;case 5:l.action=true;break;case 6:l.mLinkAcch.ground=false;break;}
  assert(!tick(l,false,false));assert(s_jumpAbilities.charge==GaleCharge::Idle);assert(l.launches==1);assert(l.mProcID==daAlink_c::PROC_WAIT);
 }
 setup(l);charge(l);jump_abilities_proc_change(&l,daAlink_c::PROC_DAMAGE);assert(s_jumpAbilities.charge==GaleCharge::Idle);
 setup(l);gale=true;assert(tick(l,true,true));jump_abilities_proc_change(&l,daAlink_c::PROC_FALL);assert(s_jumpAbilities.charge==GaleCharge::Airborne);
 jump_abilities_proc_change(&l,daAlink_c::PROC_LAND);l.mProcID=daAlink_c::PROC_LAND;land(l);
 setup(l);gale=true;assert(tick(l,true,true));jump_abilities_proc_change(&l,daAlink_c::PROC_DAMAGE);assert(s_jumpAbilities.charge==GaleCharge::Idle);
 setup(l);charge(l);l.initSucceeds=false;assert(!tick(l,false,false));assert(l.mProcID==daAlink_c::PROC_WAIT);
 setup(l);gale=true;l.initSucceeds=false;assert(!tick(l,true,true));assert(s_jumpAbilities.charge==GaleCharge::Idle);
 // Releasing to deploy Glide cancels pending Gale before the next airborne press.
 setup(l);gale=glide=true;assert(tick(l,true,true));assert(!tick(l,false,false));assert(tick(l,true,true));assert(creating&&s_jumpAbilities.charge==GaleCharge::Idle);
 for(auto proc:{daAlink_c::PROC_AUTO_JUMP,daAlink_c::PROC_FALL})for(float velocity:{25.0f,-60.0f}){
  setup(l);glide=true;l.mProcID=proc;l.mLinkAcch.ground=false;l.speed.y=velocity;l.mNormalSpeed=12;
  assert(tick(l,true,true));assert(creating);update_glide(&l);assert(!s_jumpAbilities.attached);
  creating=false;live=true;update_glide(&l);assert(s_jumpAbilities.attached);assert(l.mGrabItemAcKeep.actor==&cucco);
  assert(l.mProcID==daAlink_c::PROC_AUTO_JUMP&&l.launches==0);close(l.speed.y,std::max(velocity,-7.0f));close(l.mNormalSpeed,12);close(l.gravity,-1);
  l.mLinkAcch.ground=true;update_glide(&l);assert(!live&&!l.mGrabItemAcKeep.actor&&l.frees==1);assert(s_jumpAbilities.cucco==kNoGlideActor);close(l.gravity,-3.4f);
 }
 // A pending load is cancelled on landing; a busy request retains ownership for a later retry.
 setup(l);glide=true;l.mProcID=daAlink_c::PROC_FALL;l.mLinkAcch.ground=false;request_glide(&l);
 l.mLinkAcch.ground=true;busy=true;update_glide(&l);assert(s_jumpAbilities.retiring&&creating);
 busy=false;g_fpcCtTg_Queue.mpHead=nullptr;update_glide(&l);assert(s_jumpAbilities.cucco==cucco.id);
 g_fpcCtTg_Queue.mpHead=&tag;update_glide(&l);assert(!creating&&s_jumpAbilities.cucco==kNoGlideActor);
 // Time out an archive that never finishes loading.
 setup(l);glide=true;l.mProcID=daAlink_c::PROC_FALL;l.mLinkAcch.ground=false;request_glide(&l);
 for(int i=0;i<91;++i){update_glide(&l);}
 assert(!creating&&s_jumpAbilities.cucco==kNoGlideActor);
 // Never steal an existing carried actor or clear a replacement item.
 setup(l);fopAc_ac_c other;other.id=99;l.mGrabItemAcKeep.actor=&other;int oldSpawns=spawns;request_glide(&l);assert(spawns==oldSpawns);
 s_jumpAbilities.cucco=cucco.id;live=true;s_jumpAbilities.attached=true;retire_glide_actor(&l);assert(l.mGrabItemAcKeep.actor==&other&&l.frees==0);
 setup(l);glide=true;live=true;s_jumpAbilities.cucco=cucco.id;l.mGrabItemAcKeep.actor=&cucco;s_jumpAbilities.attached=true;
 deleteOk=false;retire_glide_actor(&l);assert(s_jumpAbilities.retiring&&l.frees==1);deleteOk=true;retire_glide_actor(&l);assert(s_jumpAbilities.cucco==kNoGlideActor&&l.frees==1);
 setup(l);charge(l);reset_jump_abilities(&l);assert(!s_jumpAbilities.owner&&l.mProcID==daAlink_c::PROC_WAIT);
}
'''
height = function('float gale_height_bonus', config) + '\n' + function('float jump_height_multiplier', config) + '\n' + function('void apply_manual_jump_height', hooks)
production = abilities[:abilities.index('HookAction before_jump_abilities_execute')].replace('#include "jump_gale_visual.inc"', '')
fixture = fixture.replace('// HEIGHT', height).replace('// ABILITIES', production)
fixture = fixture.replace('// START_JUMP', function('bool jump_state_ready', hooks) + '\n' + function('bool start_ground_jump', hooks))
assert 'register_int("jump-height-percent", 100, s_jumpHeight)' in config
assert 'register_int("gale-height-percent", 500, s_galeHeight)' in config
assert '"Gale Height", gale_height_config_var(), 100, 1000, 10, "%"' in (root/'src/ui.cpp').read_text()
assert 'register_bool("glide", false, s_glide)' in config
assert 'register_bool("revalis-gale", false, s_revalisGale)' in config
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Jump ability regression passed: height, charging, cancellation, gliding, actor ownership and cleanup')
