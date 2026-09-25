"""Exercise actual manual-jump code with native action/physics boundaries mocked."""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / "src/jump_hooks.cpp").read_text()

def function(signature):
    start = source.index(signature + "(")
    return source[start:source.index("\n}", start) + 2]

fixture = r'''
#include <cassert>
#include <cmath>
constexpr int PAD_1=0, FALSE=0, kSwordItem=0x103;
bool parent=true,gale=false,pressed=true,s_galeInputCancelled=false,blocked=false,event=false,b=false;
float height=4;
bool r_jump_enabled(){return parent;} bool revalis_gale_enabled(){return gale;}
bool jump_pressed(int){return pressed;} int active_jump_binding(){return 0;}
bool dComIfGp_event_runCheck(){return event;} float jump_height_multiplier(){return height;}
struct mDoCPd_c { static bool getHoldB(int){return b;} static bool getTrigB(int){return b;} };
struct daAlink_c {
 enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_ATN_ACTOR_WAIT,PROC_ATN_ACTOR_MOVE,
 PROC_WAIT_TURN,PROC_MOVE_TURN,PROC_WOLF_WAIT,PROC_WOLF_MOVE,PROC_WOLF_DASH,
 PROC_WOLF_WAIT_TURN,PROC_WOLF_ATN_AC_MOVE,PROC_DOOR_OPEN,PROC_WARP,PROC_WOLF_AUTO_JUMP,
 PROC_WOLF_HOWL,PROC_WOLF_DIG,PROC_WOLF_TAG_JUMP};
 int mProcID=PROC_WOLF_WAIT,mGndPolyAtt1=0,mEquipItem=kSwordItem,mMoveAngle=99;
 bool wolf=true,input=false,demo=false,grab=false,success=true,jumpMode=false;
 int wolfCalls=0,humanCalls=0,cutCalls=0;
 struct Ground {bool hit=true;bool ChkGroundHit(){return hit;}void ClrGroundHit(){hit=false;}}mLinkAcch;
 struct Grab {void* getActor(){return nullptr;}}mGrabItemAcKeep;
 struct Angle {int y=0;}shape_angle;
 struct {Angle angle;}current;
 struct {float y=0;}speed;
 float speedF=0,mNormalSpeed=0;
 bool checkWolf(){return wolf;} bool checkFlyAtnWait(){return false;}
 bool checkModeFlg(int){return false;}bool getSumouMode(){return false;}
 bool checkPlayerDemoMode(){return demo;}bool checkEventRun(){return false;}
 bool checkMagneBootsFly(){return false;}bool checkMagneBootsOn(){return false;}
 bool checkNotJumpSinkLimit(){return false;}bool checkGrabAnime(){return false;}
 bool checkWolfGrabAnime(){return grab;}bool checkInputOnR(){return input;}
 void setJumpMode(){jumpMode=true;}
 int procWolfAutoJumpInit(int force){assert(force==1);++wolfCalls;if(!success)return 0;
   mProcID=PROC_WOLF_AUTO_JUMP;speed.y=10;speedF=mNormalSpeed=20;return 1;}
 int procAutoJumpInit(int){++humanCalls;speed.y=8;return 1;}
 int procCutJumpInit(int){++cutCalls;return 1;}
};
const daAlink_c* s_manualJumpOwner=nullptr;int marks=0,clears=0,sprintCalls=0;
bool r_action_context_active(daAlink_c*){return blocked;}
float sprint_jump_speed_multiplier(daAlink_c*){++sprintCalls;return 1;}
void apply_sprint_jump_speed(daAlink_c*,float){}
void mark_manual_jump_started(daAlink_c*){++marks;}
void clear_manual_jump(daAlink_c*){++clears;}
// FUNCTIONS
int main(){
 for(int proc: {daAlink_c::PROC_WOLF_WAIT,daAlink_c::PROC_WOLF_MOVE,daAlink_c::PROC_WOLF_DASH,
               daAlink_c::PROC_WOLF_WAIT_TURN,daAlink_c::PROC_WOLF_ATN_AC_MOVE}) {
   daAlink_c link;link.mProcID=proc;b=true;
   assert(start_ground_jump(&link));assert(link.wolfCalls==1&&!link.humanCalls&&!link.cutCalls);
   assert(link.speed.y==20&&link.speedF==0&&link.mNormalSpeed==0);
   assert(!link.mLinkAcch.hit&&link.jumpMode&&!s_manualJumpOwner&&!marks&&!sprintCalls);
   assert(!start_ground_jump(&link)); // cannot relaunch while airborne
 }
 daAlink_c moving;moving.input=true;assert(start_ground_jump(&moving));
 assert(moving.mNormalSpeed==20&&moving.speedF==20&&moving.shape_angle.y==99&&moving.current.angle.y==99);
 for(int proc: {daAlink_c::PROC_WOLF_HOWL,daAlink_c::PROC_WOLF_DIG,daAlink_c::PROC_WOLF_TAG_JUMP}) {
   daAlink_c link;link.mProcID=proc;assert(!start_ground_jump(&link));
 }
 daAlink_c link;
 parent=false;gale=true;assert(!start_ground_jump(&link)); // Gale cannot bypass wolf parent toggle
 parent=true;pressed=false;assert(!start_ground_jump(&link));pressed=true;
 blocked=true;assert(!start_ground_jump(&link));blocked=false;
 link.grab=true;assert(!start_ground_jump(&link));link.grab=false;
 link.demo=true;assert(!start_ground_jump(&link));link.demo=false;
 event=true;assert(!start_ground_jump(&link));event=false;
 link.success=false;assert(!start_ground_jump(&link)&&link.mLinkAcch.hit&&!link.jumpMode);
 assert(!start_ground_jump(nullptr));
 daAlink_c human;human.wolf=false;human.mProcID=daAlink_c::PROC_WAIT;b=false;
 parent=false;assert(start_ground_jump(&human)); // existing human Gale fallback
 assert(human.humanCalls==1&&human.wolfCalls==0&&human.speed.y==16&&marks==1);
 parent=true;human.mLinkAcch.hit=true;b=true;assert(start_ground_jump(&human)&&human.cutCalls==1);
}
'''
fixture = fixture.replace('#include <cmath>', '#include <cmath>\n#include <initializer_list>')
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(s) for s in (
    'bool ground_jump_context_ready', 'bool jump_state_ready',
    'void apply_manual_jump_height', 'void set_manual_jump_direction',
    'void apply_manual_jump_movement', 'bool start_ground_jump')))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Wolf jump passed: native launch, stationary/moving/dash, height, parent toggle, action guards and human regressions')
