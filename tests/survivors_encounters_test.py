"""Execute the owned boss/flying AI and scoped native isolation hooks."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <vector>
using ActorId=unsigned;using s16=short;using s32=int;constexpr ActorId kNone=~0u;
enum {fpcNm_B_DS_e=1,fpcNm_E_YC_e,fpcNm_E_ZH_e,fpcNm_E_SH_e,fpcNm_E_DD_e,fpcNm_E_RDY_e,fpcNm_E_YK_e,fpcNm_E_S1_e};
struct ModContext{};enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* a,unsigned i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}}
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 void set(float a,float b,float c){x=a;y=b;z=c;}float absXZ()const{return std::sqrt(x*x+z*z);}float abs()const{return std::sqrt(x*x+y*y+z*z);}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz& operator+=(cXyz b){x+=b.x;y+=b.y;z+=b.z;return *this;}};
struct csXyz {s16 x=0,y=0,z=0;csXyz()=default;csXyz(s16 a,s16 b,s16 c):x(a),y(b),z(c){}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793);}
using process_method_func=int(*)(void*);int exec(void*){return 1;}int del(void*){return 1;}
struct process_method_class {process_method_func execute_method=exec;};process_method_class methods;
struct fopAc_ac_c {ActorId id=1;int profile=1;bool isActor=true;void* sub_method=&methods;
 struct {cXyz pos;csXyz angle;} current,old,home;csXyz shape_angle;cXyz speed,scale{1,1,1};float gravity=0,speedF=0;};
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}bool is_actor_process(void* a){return static_cast<fopAc_ac_c*>(a)->isActor;}
int fopAcM_GetRoomNo(fopAc_ac_c*){return 0;}int dComIfGp_getReverb(int){return 0;}
struct J3DAnmTransform{};J3DAnmTransform transform;
void* dComIfG_getObjectRes(const char*,int){return &transform;}
struct mDoExt_McaMorfSO {int plays=0,sets=0;float frame=0;bool stop=false;bool checkFrame(int f){return frame==f;}bool isStop(){return stop;}void setAnm(J3DAnmTransform*,int,float,float,float,float){++sets;}void play(int,int){++plays;}};
struct Texture {void play(){}};
struct Collision {bool at=false;float r=0,h=0;cXyz pos;
 void OffAtSetBit(){at=false;}void OnAtSetBit(){at=true;}void SetC(cXyz v){pos=v;}void SetR(float v){r=v;}void SetH(float v){h=v;}};
struct CC {unsigned sets=0;void Set(Collision* c){assert(c->at);++sets;}}cc;CC* dComIfG_Ccsp(){return &cc;}
struct Wall {void SetWall(float,float){}};struct Background {void CrrPos(int){}};int dComIfG_Bgsp(){return 0;}
struct e_yc_class:fopAc_ac_c {enum {ACT_F_FLY=10,ACT_ATTACK=11,ANM_FLY=9,ANM_CATCH_START=6,ANM_FLY_BRAKE=7};
 int mTimer[4]{};ActorId mRiderID=0;int mAction=0,mMode=0,mAnm=0;Collision mAtSph;Background mAcch;
 mDoExt_McaMorfSO morph;mDoExt_McaMorfSO* mpMorf=&morph;};
struct daB_DS_c:fopAc_ac_c {enum {ACT_B2_F_MOVE=2,ACT_B2_WAIT=1,ANM_HEAD_FWAIT=44,ANM_HEAD_FATTACK_A=41,ANM_HEAD_FATTACK_B=42,TYPE_BULLET_B=3};
 int mModeTimer=0,mBossPhase=0,mAction=0,mMode=0,animation=0;bool mIsOpeningDemo=true,mCreateFireBreath=false,mDrawZant=true,mNoDrawSword=true;
 float mEyeColorAlpha=0,mWallR=0,mGroundUpY=0;int mBh2AttackAngleF=0;cXyz mMouthPos,mSoundPos;Wall mAcchCir;Background mAcch;
 mDoExt_McaMorfSO morph,sword;mDoExt_McaMorfSO* mpMorf=&morph,*mpSwordMorf=&sword;Texture texture;Texture* mpSwordBrkAnm=&texture;
 void setBck(int a,int,float,float){animation=a;}};
struct daE_ZH_c:fopAc_ac_c {void* mS_Ball=nullptr;int mActionMode=0,mMoveMode=0,field_0x78c=0,animation=0;
 Background mBgc;Texture texture;Texture* mpBtkAnm=&texture;mDoExt_McaMorfSO morph;mDoExt_McaMorfSO* mpModelMorf=&morph;
 Collision field_0xc7c;void setBck(int a,int,float,float){animation=a;}};
struct e_rdy_class {fopAc_ac_c actor;float mPlayerCheckDist=1000,field_0xabc=0,mKnockback=0;int mAction=0,mMode=0,mAnm=0,field_0xa7b=0;mDoExt_McaMorfSO morph;mDoExt_McaMorfSO* mpMorf=&morph;};
struct e_yk_class:fopAc_ac_c {float mPlayerTrigger=1000;};using e_s1_class=fopAc_ac_c;
struct Enemy {ActorId id=kNone;int profile=0;bool elite=true,initialized=true,retiring=false,credited=false,dormant=false,idleAnimation=false,encounterReady=false;
 unsigned encounterTick=0,stagger=0;cXyz lootPosition,spawnPosition,encounterTarget;};
std::array<Enemy,4> s_enemies;Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id==id&&id!=kNone)return &e;return nullptr;}
bool arena=true;bool in_arena(){return arena;}bool roster_owned(fopAc_ac_c* a){return arena&&a&&tracked(a->id);}
enum class Phase {Running,Upgrade};struct Run {Phase phase=Phase::Running;}s_run;
bool transition=false;bool transitioning(){return transition;}
ActorId ownedBullet=kNone;bool active_enemy_arrow(ActorId id){return id!=kNone&&id==ownedBullet;}
bool standalone=true;bool enemy_spawner_process_active(){return standalone;}
bool arena_aggro(fopAc_ac_c* a,int profile){auto* e=a?tracked(a->id):nullptr;return arena&&e&&e->initialized&&!e->dormant&&!e->retiring&&!e->credited&&a->profile==profile&&s_run.phase==Phase::Running;}
fopAc_ac_c player;auto* daAlink_getAlinkActorClass(){return &player;}
float fopAcM_searchPlayerDistance(fopAc_ac_c* a){return (player.current.pos-a->current.pos).abs();}
void maintain_carrier_rider(e_yc_class*,const Enemy&){}
bool prepare_carrier_rider(e_rdy_class*){return false;}
struct Env {float daytime=180;}env;Env* dKy_getEnvlight(){return &env;}
struct Shot {ActorId parent;unsigned parameter;cXyz position;csXyz angle;};std::vector<Shot> shots;
ActorId fopAcM_createChild(int profile,ActorId parent,unsigned parameter,cXyz* pos,int,csXyz* angle,void*,int,void*){
 assert(profile==fpcNm_B_DS_e);shots.push_back({parent,parameter,*pos,*angle});return shots.size()+50;}
'''
fixture+=r'''
bool boss_attack_paused(const Enemy&){return false;}
bool boss_major_ready(Enemy&,unsigned){return true;}
HookAction boss_carrier_pass(e_yc_class*,Enemy&){return HOOK_SKIP_ORIGINAL;}
'''
main=r'''
int main(){
 player.id=100;player.current.pos={1000,0,1000};
 daB_DS_c head;head.id=1;head.profile=fpcNm_B_DS_e;head.current.pos={345,-1375,-1040};
 auto& e=s_enemies[0];e.id=1;e.profile=head.profile;e.initialized=false;e.spawnPosition={1000,300,1600};e.lootPosition={1000,0,1600};
 std::any headArgs[]={&head};
 assert(before_roster_head_action(nullptr,headArgs,nullptr,nullptr)==HOOK_CONTINUE);
 assert(head.current.pos.x==1000&&head.current.pos.y==300&&head.current.pos.z==1600);
 assert(!head.mIsOpeningDemo&&head.mBossPhase==1&&head.mEyeColorAlpha==255&&!head.mDrawZant);
 // Native action dispatch remains active. Only the tower path and hover height
 // are adapted; native animation clocks choose when fireballs are released.
 e.initialized=true;bool reached=false;before_roster_head_move(nullptr,headArgs,&reached,nullptr);assert(reached);
 head.mMode=100;for(int i=0;i<40;++i)before_roster_head_hover(nullptr,headArgs,nullptr,nullptr);
 assert(head.current.pos.y==80);head.mMode=3;
 for(int i=0;i<40;++i)before_roster_head_hover(nullptr,headArgs,nullptr,nullptr);
 assert(head.current.pos.y==320&&shots.empty());
 head.mAction=daB_DS_c::ACT_B2_F_MOVE;head.mMode=1;head.mModeTimer=1;
 player.current.pos.x=5000;before_roster_head_action(nullptr,headArgs,nullptr,nullptr);
 assert(head.mMode==3&&head.animation==daB_DS_c::ANM_HEAD_FATTACK_A);
 head.mMode=2;player.current.pos.x=10000;before_roster_head_action(nullptr,headArgs,nullptr,nullptr);assert(head.mMode==2);
 player.current.pos.x=1000;s_run.phase=Phase::Upgrade;before_roster_head_action(nullptr,headArgs,nullptr,nullptr);assert(head.mMode==2);s_run.phase=Phase::Running;
 arena=false;assert(before_roster_head_action(nullptr,headArgs,nullptr,nullptr)==HOOK_CONTINUE);arena=true;
 // Scoped creation flags: no boss-clear query or zone mutation leaks to story actors.
 before_roster_head_create(nullptr,headArgs,nullptr,nullptr);int cleared=1;
 assert(before_roster_head_cleared(nullptr,nullptr,&cleared,nullptr)==HOOK_SKIP_ORIGINAL&&cleared==0);
 assert(before_roster_head_zone(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 standalone=false;assert(before_roster_head_zone(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);standalone=true;
 daB_DS_c foreign;foreign.id=300;std::any foreignArgs[]={&foreign};
 before_roster_head_create(nullptr,foreignArgs,nullptr,nullptr);
 assert(before_roster_head_zone(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
 after_roster_head_create(nullptr,foreignArgs,nullptr,nullptr);
 assert(before_roster_head_zone(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 after_roster_head_create(nullptr,headArgs,nullptr,nullptr);
 assert(s_rosterHeadCreates.empty()&&before_roster_head_cleared(nullptr,nullptr,&cleared,nullptr)==HOOK_CONTINUE);
 ownedBullet=foreign.id;before_roster_head_create(nullptr,foreignArgs,nullptr,nullptr);
 assert(before_roster_head_cleared(nullptr,nullptr,&cleared,nullptr)==HOOK_SKIP_ORIGINAL);
 after_roster_head_create(nullptr,foreignArgs,nullptr,nullptr);ownedBullet=kNone;
 // A hand works with no orb, locks its aim before slamming and only attacks in the slam window.
 daE_ZH_c hand;hand.id=2;hand.profile=fpcNm_E_ZH_e;hand.current.pos=player.current.pos+cXyz(0,240,0);
 auto& h=s_enemies[1];h.id=2;h.profile=hand.profile;std::any handArgs[]={&hand};unsigned handHits=0;
 for(unsigned phase=0;phase<180;++phase){
  if(phase==70)player.current.pos.x+=500;
  before_roster_hand_action(nullptr,handArgs,nullptr,nullptr);after_roster_hand_collision(nullptr,handArgs,nullptr,nullptr);
  if(phase>=60)assert(h.encounterTarget.x==1000);
  assert(hand.field_0xc7c.at==(phase>=90&&phase<108));handHits+=hand.field_0xc7c.at;
 }
 assert(handHits==18&&hand.mS_Ball==nullptr&&hand.mActionMode==4);
 h.encounterTick=100;h.stagger=1;after_roster_hand_collision(nullptr,handArgs,nullptr,nullptr);assert(!hand.field_0xc7c.at);h.stagger=0;
 h.credited=true;after_roster_hand_collision(nullptr,handArgs,nullptr,nullptr);assert(!hand.field_0xc7c.at);h.credited=false;
 // Carrier follows native brake/telegraph/catch phases without a rider pointer.
 e_yc_class bird;bird.id=3;bird.profile=fpcNm_E_YC_e;bird.current.pos=player.current.pos+cXyz(0,260,0);
 auto& b=s_enemies[2];b.elite=false;b.id=3;b.profile=bird.profile;std::any birdArgs[]={&bird};
 before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);assert(bird.mRiderID==kNone);
 bird.mTimer[2]=0;before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);
 for(int i=0;i<45;++i)before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);
 assert(bird.mMode==3&&bird.mTimer[0]==15);bird.mTimer[0]=0;
 before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);assert(bird.mMode==4);
 auto locked=b.encounterTarget;player.current.pos.x+=500;bird.morph.frame=11;
 before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);assert(bird.mAtSph.at&&b.encounterTarget.x==locked.x);
 bird.morph.frame=12;before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);assert(!bird.mAtSph.at);
 bird.morph.stop=true;before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);assert(bird.mAction==e_yc_class::ACT_F_FLY);
 // Clock restoration includes nested non-owned actors and out-of-order post callbacks.
 fopAc_ac_c dog;dog.id=4;dog.profile=fpcNm_E_SH_e;s_enemies[3].id=4;s_enemies[3].profile=dog.profile;
 std::any dogArgs[]={process_method_func(exec),static_cast<void*>(&dog)};
 std::any otherArgs[]={process_method_func(exec),static_cast<void*>(&player)};
 before_roster_time(nullptr,dogArgs,nullptr,nullptr);assert(env.daytime==0);
 before_roster_time(nullptr,otherArgs,nullptr,nullptr);assert(env.daytime==180);
 after_roster_time(nullptr,dogArgs,nullptr,nullptr);assert(env.daytime==180);
 after_roster_time(nullptr,otherArgs,nullptr,nullptr);assert(env.daytime==0);
 after_roster_time(nullptr,dogArgs,nullptr,nullptr);assert(env.daytime==180&&s_rosterTimes.empty());
 dogArgs[0]=process_method_func(del);before_roster_time(nullptr,dogArgs,nullptr,nullptr);assert(env.daytime==180);
 // Dormant building guards ignore every native action (including sound-driven
 // EXCITE), stay anchored, and resume native dispatch immediately after a hit.
 e_rdy_class guard;guard.actor.id=4;guard.actor.profile=fpcNm_E_RDY_e;
 auto& g=s_enemies[3];g={};g.id=4;g.profile=fpcNm_E_RDY_e;g.dormant=true;g.lootPosition={20,40,60};
 std::any guardArgs[]={&guard};
 for(int action=0;action<32;++action){
  guard.mAction=action;guard.mMode=5;guard.actor.speed={3,-5,8};guard.actor.speedF=12;
  guard.actor.current.pos={900,-500,900};guard.field_0xabc=20;guard.mKnockback=10;guard.field_0xa7b=1;
  assert(before_roster_rdy_action(nullptr,guardArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
  assert(guard.mAction==0&&guard.mMode==0&&guard.mAnm==0x4c);
  assert((guard.actor.current.pos-g.lootPosition).abs()==0&&(guard.actor.old.pos-g.lootPosition).abs()==0);
  assert(guard.actor.speed.abs()==0&&guard.actor.speedF==0&&guard.field_0xabc==0&&guard.mKnockback==0&&guard.field_0xa7b==0);
 }
 assert(guard.morph.sets==1); // idle keeps playing rather than restarting
 g.dormant=false;guard.mAction=3;
 assert(before_roster_rdy_action(nullptr,guardArgs,nullptr,nullptr)==HOOK_CONTINUE&&guard.mAction==3);
 g.dormant=true;arena=false;
 assert(before_roster_rdy_action(nullptr,guardArgs,nullptr,nullptr)==HOOK_CONTINUE);arena=true;
 // Creation-ID reuse starts a fresh encounter because state lives in the Enemy slot.
 b={};b.id=3;b.profile=bird.profile;before_roster_yc_action(nullptr,birdArgs,nullptr,nullptr);assert(b.encounterReady&&bird.mAction==e_yc_class::ACT_F_FLY&&bird.mTimer[2]==150);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_encounters.inc').read_text().replace('#include "survivors_boss_specials.inc"','')+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors encounters: head placement/fire/recovery, riderless dives, orb-free slams, pause, ownership and scoped clock/flags passed')
