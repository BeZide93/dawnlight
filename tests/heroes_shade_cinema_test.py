"""Exercise the real cutscene/event/message lifecycle with small native stubs.

No game assets are required. The fixture extracts the production functions;
it checks who owns control, not a duplicate of their implementation.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/heroes_shade_encounter.cpp").read_text()


def function(name):
    start = source.index(f"void {name}(")
    return source[start:source.index("\n}\n", start) + 3]


fixture = r'''
#include "heroes_shade_cinema.hpp"
#include "heroes_shade_battle.hpp"
#include <array>
#include <cassert>
namespace shade = dawnlight::shade;
using ActorId = int; using fpc_ProcID = int; using MessageId = int; using s16 = short;
constexpr int kNone=-1;
struct cXyz {
    float x=0,y=0,z=0;
    cXyz()=default;
    cXyz(float a,float b,float c):x(a),y(b),z(c) {}
    void set(float a,float b,float c) { x=a; y=b; z=c; }
};
struct Motion {
    int no=0,step=0;
    void setNo(int n,int,int,int) { no=n; step=0; }
    int getNo() { return no; }
    int getStepNo() { return step; }
};
struct Event {
    bool accepted=false; int condition=0;
    bool checkCommandDemoAccrpt() { return accepted; }
    void onCondition(int n) { condition|=n; }
};
struct daNpc_Kn_c {
    int id=42, field_0x15bc=0, field_0x170c=0, field_0x170d=0;
    int arrivals=0,departures=0,illegalWarps=0;
    bool mNoDraw=true;
    cXyz field_0x16f4{0,0,0};
    Event eventInfo;
    struct { bool grounded=true; bool ChkGroundHit() { return grounded; } } mAcch;
    Motion mMotionSeqMngr,mFaceMotionSeqMngr;
    struct { cXyz pos; } current;
    void setAngle(int) {}
    void offDownFlg() {} void offHeadLockFlg() {}
    void ctrlWarp() {
        if (field_0x170c==3) {
            ++arrivals;
            if (++field_0x170d==6) { field_0x170c=0; field_0x170d=0; }
        } else if (field_0x170c==1) {
            ++departures;
            if (++field_0x170d==15) { field_0x170c=2; field_0x170d=0; }
        } else ++illegalWarps;
    }
} boss;
struct Fighter { int id=42; bool deleting=false,reset=false; };
std::array<Fighter,3> sFighters;
shade::Cinema sCinema;
struct Player {
    int id=1,starts=0,cancels=0;
    struct { void setMoveAngle(int) {} } mDemo;
    struct { cXyz pos; } current;
    void cancelOriginalDemo() { ++cancels; }
    void changeOriginalDemo() { ++starts; }
    void changeDemoMode(int,int,int,int) {}
} player;
namespace daPy_demo_c { constexpr int DEMO_WAIT_TURN_e=5; }
struct Camera {
    int id=2;
    struct {
        int mTrimSize=1,stops=0,starts=0,resets=0;
        cXyz Eye() { return {1,2,3}; } cXyz Center() { return {4,5,6}; }
        float Fovy() { return 55; }
        void Stop() { ++stops; } void Start() { ++starts; }
        void SetTrimSize(int n) { mTrimSize=n; }
        void Reset(cXyz,cXyz,float,s16) { ++resets; }
    } mCamera;
} camera;
struct Message {
    int msg_idx=0,status=1,kills=0;
    int getStatusLocal() { return status; }
    void onKillMessageFlagLocal() { ++kills; status=1; }
} message;
struct Line { int value; int id() { return value; } };
std::array<Line,4> sLines{{{101},{102},{103},{104}}};
Line sBossTitle{105};
bool paused=false,ui=false,hasCamera=true,blockMessages=false;
int eventResets=0,orders=0,deletes=0,cameraTicks=0,titleStarts=0;
bool dComIfGp_isPauseFlag() { return paused; }
bool ui_document_visible() { return ui; }
Player* daAlink_getAlinkActorClass() { return &player; }
Camera* dComIfGp_getCamera(int) { return hasCamera ? &camera : nullptr; }
int dComIfGp_getPlayerCameraID(int) { return 0; }
template<class T> int fopAcM_GetID(T* p) { return p->id; }
template<class T> int fpcM_GetID(T* p) { return p->id; }
int fopAcM_searchPlayerAngleY(daNpc_Kn_c*) { return 0; }
int cLib_targetAngleY(cXyz*,cXyz*) { return 0; }
void fopAcM_orderPotentialEvent(daNpc_Kn_c*,int,int,int) { ++orders; }
void dComIfGp_event_reset() { ++eventResets; boss.eventInfo.accepted=false; }
daNpc_Kn_c* actor_by_id(int id) { return id==42 ? &boss : nullptr; }
void remove_actor(int id) { assert(id==42); ++deletes; }
Message* dMsgObject_getMsgObjectClass() { return &message; }
int fopMsgM_messageSetDemo(int id) {
    if (blockMessages || message.status!=1) return 0;
    if (id==105) ++titleStarts;
    message.msg_idx=id; message.status=14; return 50;
}
void cinema_camera(daNpc_Kn_c*) { ++cameraTicks; }
'''

runtime_start = source.index("struct CinemaRuntime {")
runtime = source[runtime_start:source.index("} sCinemaRuntime;", runtime_start) + len("} sCinemaRuntime;")]

checks = r'''
void reset() {
    boss={}; player={}; camera={}; message={}; sFighters={};
    sCinema={}; sCinemaRuntime={};
    paused=ui=blockMessages=false; hasCamera=true;
    eventResets=orders=deletes=cameraTicks=titleStarts=0;
}
void tick() { tick_cinema(&boss,sFighters[0]); }
void accept(bool victory) {
    sCinema.begin(victory);
    tick(); assert(orders==1 && player.starts==0 && camera.mCamera.stops==0);
    boss.eventInfo.accepted=true; tick();
    assert(player.starts==1 && camera.mCamera.stops==1);
    assert(sCinemaRuntime.ownsEvent && camera.mCamera.mTrimSize==3);
}
int main() {
    reset(); accept(false);
    for (int i=0;i<17;++i) { tick(); assert(boss.mNoDraw); }
    for (int i=17;i<54;++i) tick();
    assert(boss.arrivals==6 && boss.illegalWarps==0 && !boss.mNoDraw);
    assert(sCinema.shot==shade::Shot::Words1);
    tick(); assert(message.msg_idx==101 && sCinemaRuntime.messageStarted);
    for (int i=0;i<30;++i) tick();
    assert(sCinema.shot==shade::Shot::Words1); // wait for the actual caption
    message.status=1; tick(); tick(); assert(message.msg_idx==102);
    message.status=1; tick(); assert(sCinema.shot==shade::Shot::Ready);
    for (int i=0;i<18;++i) tick();
    assert(sCinema.shot==shade::Shot::Ready && titleStarts==0);
    boss.mMotionSeqMngr.step=1; tick(); // native return to the combat stance
    assert(sCinema.shot==shade::Shot::BossName && titleStarts==0);
    tick(); assert(message.msg_idx==105 && titleStarts==1);
    // The banner owns the final shot until native fade-out actually finishes.
    for (int i=0;i<120;++i) tick();
    assert(sCinema.active() && titleStarts==1 && player.cancels==0 && eventResets==0);
    assert(!sFighters[0].reset && !sFighters[0].deleting);
    message.status=1; tick();
    assert(!sCinema.active() && sFighters[0].reset && !sFighters[0].deleting);
    assert(player.cancels==1 && eventResets==1 && camera.mCamera.starts==1);
    assert(camera.mCamera.mTrimSize==1 && deletes==0);
    assert(boss.field_0x16f4.x==1 && !boss.mNoDraw);
    // Replay can display a fresh title; cancellation releases only this banner.
    sCinema.begin(false); tick(); boss.eventInfo.accepted=true; tick();
    assert(player.starts==2 && sCinemaRuntime.ownsEvent);
    sCinema.enter(shade::Shot::BossName); tick();
    assert(titleStarts==2 && message.msg_idx==105);
    release_cinema(); assert(message.kills==1 && player.cancels==2);

    // Slower ready animations must not be cut off by the old 45-tick timer.
    reset(); accept(false); sCinema.enter(shade::Shot::Ready); cinema_pose(&boss,24);
    for (int i=0;i<60;++i) tick();
    assert(sCinema.shot==shade::Shot::Ready && titleStarts==0);
    boss.mMotionSeqMngr.step=1; tick(); tick();
    assert(sCinema.shot==shade::Shot::BossName && titleStarts==1);
    assert(camera.mCamera.mTrimSize==3 && player.cancels==0);

    reset(); boss.mMotionSeqMngr.no=19; accept(true);
    assert(boss.mMotionSeqMngr.no==22); // native rise, not a snap to standing
    for (int i=0;i<50;++i) tick();
    assert(sCinema.shot==shade::Shot::Recover);
    boss.mMotionSeqMngr.step=1; tick(); tick(); assert(message.msg_idx==103);
    message.status=1; tick(); tick(); assert(message.msg_idx==104);
    message.status=1; tick();
    assert(sCinema.shot==shade::Shot::Depart && deletes==0);
    for (int i=0;i<45;++i) tick();
    assert(sCinema.shot==shade::Shot::Afterglow && deletes==0 && boss.mNoDraw);
    assert(boss.departures==15 && boss.illegalWarps==0);
    for (int i=0;i<30;++i) tick();
    assert(deletes==1 && eventResets==1 && player.cancels==1 && !sCinema.active());
    assert(titleStarts==0); // no title during the victory scene

    reset(); boss.mMotionSeqMngr.no=18; boss.mAcch.grounded=false; accept(true);
    assert(boss.mMotionSeqMngr.no==18); // final knockback stays airborne
    for (int i=0;i<45;++i) tick();
    assert(sCinema.shot==shade::Shot::Recover);
    boss.mAcch.grounded=true; boss.mMotionSeqMngr.no=19; boss.mMotionSeqMngr.step=1;
    tick(); assert(boss.mMotionSeqMngr.no==22 && sCinema.shot==shade::Shot::Recover);
    boss.mMotionSeqMngr.step=1; tick(); assert(sCinema.shot==shade::Shot::Words1);

    // A busy/missing message, non-finishing pose or denied event cannot trap
    // the camera or permanently prevent replay.
    for (bool victory : {false,true}) {
        reset(); accept(victory); blockMessages=true;
        for (int i=0;i<1100 && sCinema.active();++i) tick();
        assert(!sCinema.active() && eventResets==1 && player.cancels==1);
        assert(deletes==(victory ? 1 : 0));
        reset(); sCinema.begin(victory);
        for (int i=0;i<180;++i) tick();
        assert(!sCinema.active() && eventResets==0 && camera.mCamera.starts==0);
    }
    reset(); sCinema.begin(false); boss.eventInfo.accepted=true; hasCamera=false;
    tick(); assert(!sCinema.active() && eventResets==1 && !boss.mNoDraw);

    reset(); accept(false); const int ticks=sCinema.ticks;
    paused=true; tick(); paused=false; ui=true; tick(); ui=false;
    assert(sCinema.ticks==ticks);
    // Explicit cancellation at every shot returns only the acquired controls.
    for (auto shot : {shade::Shot::Arrival,shade::Shot::Recover,shade::Shot::Words1,
                      shade::Shot::Words2,shade::Shot::Ready,shade::Shot::BossName,
                      shade::Shot::Depart,shade::Shot::Afterglow}) {
        reset(); accept(false); sCinema.enter(shot); release_cinema(); release_cinema();
        assert(eventResets==1 && player.cancels==1 && camera.mCamera.starts==1);
    }
    // Room teardown must not modify a replacement player, camera or message.
    reset(); accept(false);
    sCinemaRuntime.messageStarted=true; sCinemaRuntime.message=101;
    message.msg_idx=500; message.status=14; player.id=10; camera.id=20;
    release_cinema();
    assert(message.kills==0 && player.cancels==0 && camera.mCamera.starts==0);
    reset(); accept(false);
    sCinemaRuntime.messageStarted=true; sCinemaRuntime.message=101;
    message.msg_idx=101; message.status=14; release_cinema();
    assert(message.kills==1 && eventResets==1);
}
'''

with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "cinema.cpp"
    exe = Path(tmp) / "cinema"
    cpp.write_text(fixture + runtime + "\n" + "\n".join(function(name) for name in (
        "close_cinema_line", "release_cinema", "finish_cinema", "cinema_pose", "tick_cinema")) + checks)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(root / "src"),
                    str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade cutscene lifecycle, native warp pacing and control ownership: passed")
