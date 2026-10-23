"""Exercise combat steering against the SDK's speed and root-motion equations."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv)>1 else root/'dusklight'
native = (sdk/'src/d/actor/d_a_alink.cpp').read_text()
pos = native[native.index('void daAlink_c::posMove()'):]
speed = pos[pos.index('    speedF = mNormalSpeed'):pos.index('    if (getZoraSwim()')]
start = pos.index('    if (checkRootTransClearMode())')
root_motion = pos[start:pos.index('        if (checkMagneBootsOn())', start)] + '}\n'
fixture = r'''
#include <any>
#include <cassert>
#include <cmath>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using f32=float;
struct cXyz{float x=0,y=0,z=0;cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}};
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}}
struct daAlink_c{
 enum{PROC_WAIT,PROC_MOVE,PROC_CUT_TURN,PROC_CUT_NORMAL,PROC_CUT_FINISH,PROC_GUARD_ATTACK,PROC_GUARD_SLIP,PROC_GUARD_BREAK,PROC_DAMAGE,PROC_JUMP,PROC_DODGE,PROC_GET_ITEM,PROC_CUT_LARGE_JUMP,PROC_CUT_LARGE_JUMP_LAND,MODE_PLAYER_FLY};
 bool sprintHeld=false;bool doButton(){return sprintHeld;}bool doTrigger(){return false;}bool checkInputOnR(){return sprintHeld;}
 unsigned id=7;int mProcID=PROC_WAIT;bool matching=true;
 struct{cXyz pos;struct{short y=0;}angle;}current;
 struct{bool ground=true;bool ChkGroundHit(){return ground;}}mLinkAcch;
 struct{struct{struct{float mMaxSpeed=20;}m;}mMove;}hio;
 decltype(hio)* mpHIO=&hio;
 short mMoveAngle=0;float mStickValue=0,mMoveValue=0,mNormalSpeed=0,mMaxSpeed=0,mSpeedModifier=0,speedF=0,field_0x33a0=30;
 cXyz field_0x3798,field_0x34d4;float field_0x3464=0,field_0x3468=0;short field_0x2ff0=0;
 int mProcVar5_value=0;struct{int field_0x3012=0;}mProcVar5;
 struct Frame{float getOldFrameRate(){return .25f;}}frame;Frame* field_0x2060=&frame;
 static constexpr int PROC_CLIMB_UP_START=100;
 bool checkUpperAnime(int){return matching;}
 bool checkRootTransClearMode(){return true;}bool checkRootTransXClearMode(){return true;}bool checkRootTransZClearMode(){return true;}
 bool checkModeFlg(int){return false;}
 void native_motion(cXyz sp108);
};
// Native root block contains an unrelated climb-zero branch.
namespace cXyzConstants{const cXyz Zero{};}
float cM_scos(short a){return std::cos(a*3.141592653589793f/32768);}
float cM_ssin(short a){return std::sin(a*3.141592653589793f/32768);}
unsigned fopAcM_GetID(daAlink_c* p){return p->id;}
bool playing=true;bool playable(){return playing;}unsigned s_player=7;Run s_run;
enum class SwordAction{None,Normal,Spin,Jump};SwordAction s_swordAction=SwordAction::None;
struct{bool active=false;int animation=1;}s_ranged;
void track_melee_facing(daAlink_c*){}
bool abilityBusy=false;bool jump_abilities_busy_for(daAlink_c*){return abilityBusy;}
bool jump_strike_pose(daAlink_c* p){return p->mProcID==daAlink_c::PROC_CUT_LARGE_JUMP||p->mProcID==daAlink_c::PROC_CUT_LARGE_JUMP_LAND;}
bool normal_sword_proc(daAlink_c* p){return p->mProcID==daAlink_c::PROC_CUT_NORMAL||p->mProcID==daAlink_c::PROC_CUT_FINISH;}
bool sword_action_available(daAlink_c* p){return p->mProcID==daAlink_c::PROC_WAIT||p->mProcID==daAlink_c::PROC_MOVE;}
bool shield_pose(const daAlink_c* p){return p->mProcID==daAlink_c::PROC_GUARD_ATTACK||p->mProcID==daAlink_c::PROC_GUARD_SLIP||p->mProcID==daAlink_c::PROC_GUARD_BREAK;}
'''
main = r'''
int main(){
 s_run.start(1);s_run.shield=1;daAlink_c link;cXyz previous;
 std::any args[]={&link,&previous};
 for(int proc:{daAlink_c::PROC_CUT_NORMAL,daAlink_c::PROC_CUT_FINISH,daAlink_c::PROC_CUT_TURN,daAlink_c::PROC_MOVE,daAlink_c::PROC_GUARD_ATTACK,daAlink_c::PROC_GUARD_SLIP})
 for(float stick:{0.f,.25f,1.f})for(short direction:{short(0),short(0x4000),short(-0x4000),short(0x7000)}){
  link.mProcID=proc;link.mMoveValue=stick;link.mStickValue=stick*.2f;link.mMoveAngle=direction;
  s_swordAction=proc==daAlink_c::PROC_CUT_TURN?SwordAction::Spin:SwordAction::Normal;s_ranged.active=proc==daAlink_c::PROC_MOVE;
  link.current.pos={10,20,30};link.field_0x3798={10,20,30};link.mNormalSpeed=2;link.mMaxSpeed=4;link.mSpeedModifier=.9f;
  before_auto_movement(nullptr,args,nullptr,nullptr);
  // Native transAnimeProc can directly translate the actor/anchor and also
  // leave a root delta for posMove. Keep vertical motion, neutralize XZ only.
  previous={1,2,3};link.field_0x34d4={25,6,35};link.current.pos={-40,24,-20};link.field_0x3798={-40,24,-20};
  after_auto_movement(nullptr,args,nullptr,nullptr);
  assert(link.current.pos.x==10&&link.current.pos.z==30&&link.current.pos.y==24);
  assert(link.field_0x3798.x==10&&link.field_0x3798.z==30&&link.field_0x3798.y==24&&previous.y==2);
  link.native_motion(previous);
  assert(std::abs(link.current.pos.x-(10+20*stick*cM_ssin(direction)))<.0001f);
  assert(std::abs(link.current.pos.z-(30+20*stick*cM_scos(direction)))<.0001f);
  assert(link.speedF==20*stick&&link.field_0x3464==0&&link.field_0x3468==0);
  after_auto_position(nullptr,args,nullptr,nullptr);
  if(shield_pose(&link))assert(link.mNormalSpeed==2); // Native guard recoil can still reach zero and finish.
 }
 // Talent sprint survives automatic sword movement at each fractional speed.
 for(unsigned rank=0;rank<=2;++rank){
  s_run.talents.ranks[unsigned(Talent::Sprint)]=1;s_run.talents.ranks[unsigned(Talent::SprintSpeed)]=rank;
  link.sprintHeld=true;link.mProcID=daAlink_c::PROC_CUT_NORMAL;s_swordAction=SwordAction::Normal;link.mMoveValue=1;
  before_auto_movement(nullptr,args,nullptr,nullptr);assert(link.mMaxSpeed==25+2.5f*rank&&link.mNormalSpeed==link.mMaxSpeed);
 }
 link.sprintHeld=false;s_run.talents={};
 // Fate affects walking and automatic combat without sticky speed after expiry.
 link.mProcID=daAlink_c::PROC_MOVE;s_swordAction=SwordAction::None;
 for(auto effect:{FateEffect::Sprint,FateEffect::Slow}){
  s_run.apply_fate(effect);
  for(float stick:{0.f,.25f,1.f}){
   link.mMoveValue=stick;before_auto_movement(nullptr,args,nullptr,nullptr);
   assert(link.mMaxSpeed==(effect==FateEffect::Sprint?30.f:10.f));
   assert(link.mNormalSpeed==link.mMaxSpeed*stick);
  }
 }
 s_run.clear_fate();before_auto_movement(nullptr,args,nullptr,nullptr);assert(link.mMaxSpeed==20);
 // End-of-swing handoff: native checkCutCancelNextMode zeros speed, then
 // enters WAIT/MOVE before posMove; the scheduler clears sword ownership.
 // Recovery blend/foot speed must not slow any following locomotion frames.
 s_ranged.active=false;
 for(int proc:{daAlink_c::PROC_WAIT,daAlink_c::PROC_MOVE})for(float stick:{0.f,.25f,1.f}){
  s_swordAction=SwordAction::Normal;link.mProcID=proc;link.mMoveValue=stick;link.mMoveAngle=0;
  for(int frame=0;frame<12;++frame){
   if(frame)s_swordAction=SwordAction::None;
   link.mNormalSpeed=0;link.mSpeedModifier=.9f;link.current.pos={0,0,0};
   before_auto_movement(nullptr,args,nullptr,nullptr);
   previous={1,2,3};link.field_0x34d4={20,2,40};after_auto_movement(nullptr,args,nullptr,nullptr);
   link.native_motion(previous);after_auto_position(nullptr,args,nullptr,nullptr);
   assert(link.speedF==20*stick&&link.current.pos.z==20*stick);
  }
 }
 // Jump Strike stays in place without input and steers throughout launch,
 // flight and landing. Native horizontal root/lunge motion is removed.
 for(int proc:{daAlink_c::PROC_CUT_LARGE_JUMP,daAlink_c::PROC_CUT_LARGE_JUMP_LAND})
 for(bool ground:{false,true})for(float stick:{0.f,.25f,1.f})for(short direction:{short(0),short(0x4000),short(-0x4000)}){
  link.mProcID=proc;link.mLinkAcch.ground=ground;s_swordAction=SwordAction::Jump;
  link.mMoveValue=stick;link.mMoveAngle=direction;link.current.pos={10,30,20};link.field_0x3798={10,30,20};
  link.mNormalSpeed=90;before_auto_movement(nullptr,args,nullptr,nullptr);
  previous={0,4,0};link.field_0x34d4={80,6,100};link.current.pos={99,35,99};
  after_auto_movement(nullptr,args,nullptr,nullptr);link.native_motion(previous);
  assert(link.current.pos.y==35&&std::abs(link.current.pos.x-(10+20*stick*cM_ssin(direction)))<.001f&&std::abs(link.current.pos.z-(20+20*stick*cM_scos(direction)))<.001f);
 }
 abilityBusy=true;link.mNormalSpeed=77;before_auto_movement(nullptr,args,nullptr,nullptr);assert(link.mNormalSpeed==77);abilityBusy=false;link.mLinkAcch.ground=true;
 // No correction during damage, jumps, item-get events, pause, another actor,
 // an airborne attack, or a dodge.
 link.mProcID=daAlink_c::PROC_CUT_NORMAL;link.mMoveValue=.03f;link.mNormalSpeed=8;
 s_swordAction=SwordAction::Normal;before_auto_movement(nullptr,args,nullptr,nullptr);assert(link.mNormalSpeed==0);
 after_auto_movement(nullptr,args,nullptr,nullptr);after_auto_position(nullptr,args,nullptr,nullptr);
 s_ranged.active=true;s_swordAction=SwordAction::Normal;
 for(int reason=0;reason<8;++reason){
  link.mProcID=reason==0?daAlink_c::PROC_DAMAGE:reason==1?daAlink_c::PROC_JUMP:reason==2?daAlink_c::PROC_GET_ITEM:reason==6?daAlink_c::PROC_DODGE:daAlink_c::PROC_MOVE;
  playing=reason!=3;link.id=reason==4?9:7;link.mLinkAcch.ground=reason!=5;link.matching=reason!=6;s_run.phase=reason==7?Phase::Treasure:Phase::Running;
  link.mNormalSpeed=7;link.mSpeedModifier=.8f;link.current.pos={1,2,3};
  before_auto_movement(nullptr,args,nullptr,nullptr);
  link.current.pos.x=42;after_auto_movement(nullptr,args,nullptr,nullptr);
  assert(link.mNormalSpeed==7&&link.mSpeedModifier==.8f&&link.current.pos.x==42);
 }
}
'''
# The engine's Zero constant cannot be declared inline with the tiny Vec fixture.
root_motion = root_motion.replace('cXyz::Zero', 'cXyzConstants::Zero')
physics = 'void daAlink_c::native_motion(cXyz sp108){\n'+speed+'''
 current.pos.x+=speedF*cM_ssin(current.angle.y);current.pos.z+=speedF*cM_scos(current.angle.y);
 const float temp_f30=0,temp_f29=1;float sp0C=0;
'''+root_motion+'current.pos.x+=field_0x3464;current.pos.z+=field_0x3468;}\n'
source = (root/'src/survivors_movement.inc').read_text()
hooks = (root/'src/survivors.cpp').read_text()
assert 'DEFINE_HOOK(&daAlink_c::transAnimeProc, SurvivorsMovement)' in hooks
assert 'PRE(SurvivorsMovement, before_auto_movement); POST(SurvivorsMovement, after_auto_movement);' in hooks
assert 'POST(SurvivorsPosition, after_auto_position)' in hooks
with tempfile.TemporaryDirectory() as tmp:
 cpp,binary=Path(tmp)/'test.cpp',Path(tmp)/'test'
 cpp.write_text(fixture+physics+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Survivors movement: steering, no drift/root lunges, guard completion and native-action exclusions passed')
