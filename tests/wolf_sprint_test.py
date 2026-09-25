"""Exercise wolf sprint hooks with native movement/action boundaries mocked."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <algorithm>
#include <cassert>
#include <vector>
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}}
bool enabled=true,event=false;float multiplier=1;
bool wolf_sprint_enabled(){return enabled;}float wolf_speed_multiplier(){return multiplier;}
bool dComIfGp_event_runCheck(){return event;}
struct daAlink_c {
 enum {PROC_WOLF_WAIT,PROC_WOLF_MOVE,PROC_WOLF_DASH,PROC_WOLF_AUTO_JUMP,PROC_WOLF_DIG,PROC_MOVE};
 struct Move {float mADashMaxSpeed=30,mADashMaxSpeedSlow=15,mADashMaxSpeedSlow2=20;};
 struct Hio {struct {struct {Move m;}mWlMove;}mWolf;}hio;
 Hio* mpHIO=&hio;
 bool wolf=true,held=true,input=true,dash=true,demo=false,attention=false,slope=false,sink=false,grab=false;
 bool slow=false;
 struct Ground {bool hit=true;bool ChkGroundHit(){return hit;}}mLinkAcch;
 struct Grab {void* actor=nullptr;void* getActor(){return actor;}}mGrabItemAcKeep;
 struct {float y=12;}speed;
 int mProcID=PROC_WOLF_MOVE,field_0x2fc7=0,field_0x30d0=2;
 float mNormalSpeed=0,speedF=0,mMaxSpeed=30;
 bool checkWolf(){return wolf;}bool doButton(){return held;}bool checkInputOnR(){return input;}
 bool checkWolfDashMode(){return dash;}bool checkWolfSlowDash(){return slow;}
 bool checkPlayerDemoMode(){return demo;}bool checkEventRun(){return false;}
 bool checkAttentionState(){return attention;}bool checkSlope(){return slope;}
 bool checkNotJumpSinkLimit(){return sink;}bool checkWolfGrabAnime(){return grab;}
};
bool ground_movement_proc(daAlink_c* link){return link->mProcID==daAlink_c::PROC_WOLF_MOVE||link->mProcID==daAlink_c::PROC_WOLF_DASH;}
// HOOKS
int main(){
 daAlink_c link;auto* owner=&link;void* args[]={&owner};
 auto tick=[&](){return before_wolf_sprint_movement(nullptr,args,nullptr,nullptr);};
 // Every native tick reduces the dash timer, then accelerates toward the limit.
 // Repeated hooks must not compound the speed multiplier or replay dash init.
 for(float factor: {1.f,1.5f,3.f}) {
  multiplier=factor;link.mNormalSpeed=0;link.field_0x30d0=2;
  for(int frame=0;frame<100;++frame) {
   --link.field_0x30d0;assert(tick()==HOOK_CONTINUE);
   assert(link.field_0x30d0==2&&link.mMaxSpeed==30*factor);
   link.mNormalSpeed=std::min(link.mNormalSpeed+1,link.mMaxSpeed);
   link.speedF=link.mNormalSpeed;
   link.mMaxSpeed=30; // native checkNextActionWolf selects base limit again
  }
  assert(link.mNormalSpeed==30*factor);
 }
 link.slow=true;tick();assert(link.mMaxSpeed==45);link.slow=false;
 link.field_0x2fc7=2;tick();assert(link.mMaxSpeed==60);link.field_0x2fc7=0;
 // Dash's persistent limit is restored on release or when toggled off.
 link.mProcID=daAlink_c::PROC_WOLF_DASH;tick();assert(link.mMaxSpeed==90);
 link.held=false;tick();assert(link.mMaxSpeed==30&&!s_wolfSprintOwner);link.held=true;
 tick();enabled=false;tick();assert(link.mMaxSpeed==30&&!s_wolfSprintOwner);enabled=true;
 // Never activate sprint for targeting, scripted/special actions, carrying, or no input.
 for(int reason=0;reason<12;++reason) {
  daAlink_c guarded;owner=&guarded;guarded.field_0x30d0=7;guarded.mMaxSpeed=11;
  guarded.wolf=reason!=0;guarded.input=reason!=1;guarded.dash=reason!=2;
  guarded.mLinkAcch.hit=reason!=3;guarded.demo=reason==4;guarded.attention=reason==5;
  guarded.slope=reason==6;guarded.grab=reason==7;guarded.sink=reason==8;
  if(reason==9)guarded.mProcID=daAlink_c::PROC_WOLF_DIG;
  if(reason==10)guarded.mGrabItemAcKeep.actor=&link;
  event=reason==11;s_wolfSprintOwner=nullptr;
  tick();assert(guarded.mMaxSpeed==11&&guarded.field_0x30d0==7&&!s_wolfSprintOwner);
 }
 event=false;owner=&link;link.mProcID=daAlink_c::PROC_WOLF_MOVE;
 // Jump carries actual earned speed, not the configured cap, and never scales Y.
 for(float earned: {8.f,30.f,67.f}) {
  tick();link.speedF=earned;link.held=false; // release exactly at takeoff
  before_wolf_sprint_jump(nullptr,args,nullptr,nullptr);
  link.mProcID=daAlink_c::PROC_WOLF_AUTO_JUMP;s_wolfSprintOwner=nullptr;
  link.mNormalSpeed=link.speedF=20;link.mMaxSpeed=25;link.speed.y=12;
  int result=1;after_wolf_sprint_jump(nullptr,args,&result,nullptr);
  assert(link.mNormalSpeed==earned&&link.speedF==earned&&link.speed.y==12);
  assert(link.mMaxSpeed==std::max(25.f,earned));
  link.held=true;link.mProcID=daAlink_c::PROC_WOLF_MOVE;
 }
 // Failed jumps and jumps that were not started from our sprint remain native.
 for(int reason=0;reason<3;++reason) {
  tick();link.speedF=70;if(reason==0)enabled=false;if(reason==1)s_wolfSprintOwner=nullptr;
  before_wolf_sprint_jump(nullptr,args,nullptr,nullptr);
  link.mNormalSpeed=20;link.mProcID=daAlink_c::PROC_WOLF_AUTO_JUMP;int result=reason==2?0:1;
  after_wolf_sprint_jump(nullptr,args,&result,nullptr);assert(link.mNormalSpeed==20);
  enabled=true;link.mProcID=daAlink_c::PROC_WOLF_MOVE;
 }
 assert(s_wolfJumpMomentum.empty());
}
'''
fixture = fixture.replace('// HOOKS', (root / 'src/wolf_sprint.inc').read_text())
config = (root / 'src/config.cpp').read_text()
ui = (root / 'src/ui.cpp').read_text()
assert 'register_bool("wolf-sprint", false, s_wolfSprint)' in config
assert 'register_int("wolf-speed-percent", 100, s_wolfSpeedPercent)' in config
assert ui.index('"Sprint"') < ui.index('"Wolf Sprint"') < ui.index('"Wolf Speed"')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Wolf sprint passed: sustained dash, 100/150/300%, no compounding, slow areas, release/disable, guards and earned jump momentum')
