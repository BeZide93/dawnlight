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
#include <algorithm>
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
    bool newMotion=false;
    void setNo(int n,int,int,int) { no=n; step=0; }
    int getNo() { return no; }
    int getStepNo() { return step; }
    bool checkEntryNewMotion() { return newMotion; }
};
struct Model {
    float frame=0,end=100,speed=1;
    void setPlaySpeed(float value) { speed=value; }
    float getFrame() { return frame; }
    float getEndFrame() { return end; }
} body;
struct Event {
    bool accepted=false; int condition=0;
    bool checkCommandDemoAccrpt() { return accepted; }
    void onCondition(int n) { condition|=n; }
};
struct daNpc_Kn_c {
    std::array<Model*,2> mpModelMorf{{&body,nullptr}};
    int id=42, field_0x15bc=0, field_0x170c=0, field_0x170d=0;

    bool mNoDraw=true;
    cXyz field_0x16f4{0,0,0};
    cXyz shape_angle;
    Event eventInfo;
    struct { bool grounded=true; bool ChkGroundHit() { return grounded; } } mAcch;
    Motion mMotionSeqMngr,mFaceMotionSeqMngr;
    struct { cXyz pos,angle; } current;
    void setAngle(int) {}
    void offDownFlg() {} void offHeadLockFlg() {}

} boss;
struct Fighter { int id=42,divide=0; bool deleting=false,reset=false; };
std::array<Fighter,3> sFighters;
shade::Cinema sCinema;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int){return *static_cast<T*>(args);}}
Fighter* fighter(daNpc_Kn_c* actor){return actor==&boss ? &sFighters[0] : nullptr;}
struct Player {
    int id=1,starts=0,cancels=0;
    bool dead=false; bool checkDeadHP() { return dead; }
    struct { void setMoveAngle(int) {} } mDemo;
    struct { cXyz pos,angle; } current;
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
    int msg_idx=0,status=1,kills=0,deleteRequests=0;
    int getStatusLocal() { return status; }
    void setStatusLocal(int value) { status=value; if (value==19) ++deleteRequests; }
    void onKillMessageFlagLocal() { ++kills; status=1; }
} message;
struct Line { int value; int id() { return value; } };
std::array<Line,4> sLines{{{101},{102},{103},{104}}};
Line sBossTitle{105};
bool paused=false,ui=false,hasCamera=true,blockMessages=false,delayMessageOpen=false;
int eventResets=0,orders=0,deletes=0,wolfDeletes=0,cameraTicks=0,titleStarts=0;
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
struct daNpc_GWolf_c : daNpc_Kn_c {
    Model animation;
    Model* mAnm_p=&animation;
    bool mHide=true,mTwilight=false;
    cXyz mCurAngle;
    int mOrderEvtNo=0,mAnm=2,howls=0;
    int mCcStts=0,mBtkAnm=0,mBrkAnm=0; // unused by the cinematic state machine
    struct {cXyz pos,angle;} old,home;
    void setMotion(int motion,int,int) {
        mAnm=motion==4 ? 7 : 2; animation.frame=0;animation.speed=1;
        if (motion==4) ++howls;
    }
} wolf;
bool wolfReady=true,wolfPending=true;
daNpc_Kn_c* actor_by_id(int id) { return id==42 ? &boss : id==43 && wolfReady ? &wolf : nullptr; }
bool pending_or_live(int id) { return id==43 && wolfPending; }
void remove_actor(int id) { if(id==42) ++deletes; else if(id==43) ++wolfDeletes; }
constexpr int fpcNm_NPC_GWOLF_e=99,MOD_OK=0;
int spawn(int,const cXyz&,s16,unsigned,int& id) { id=43;return 0; }
using u8=unsigned char;
struct GXColor {u8 r,g,b,a;};
struct mDoGph_gInf_c {
    static inline GXColor color{};
    static inline float rate=0,speed=0;
    static inline bool enabled=false;
    static GXColor& getFadeColor(){return color;}
    static bool isFade(){return enabled;}
    static float getFadeSpeed(){return speed;}
    static float getFadeRate(){return rate;}
    static void setFadeRate(float value){rate=value;}
    static void fadeOut(float value,GXColor& c){speed=value;color=c;rate=0;enabled=true;}
    static void offFade(){enabled=false;}
};
constexpr int fopAcStts_NOPAUSE_e=1;
void fopAcM_OnStatus(daNpc_GWolf_c*,int) {}
void fopAcM_OffStatus(daNpc_GWolf_c*,int) {}
// WOLF_RUNTIME
Message* dMsgObject_getMsgObjectClass() { return &message; }
int fopMsgM_messageSetDemo(int id) {
    if (blockMessages || message.status!=1) return 0;
    if (id==105) ++titleStarts;
    // setMessageIndexDemo(false) only queues the request in the real engine.
    message.msg_idx=id; message.status=delayMessageOpen ? 1 : 14; return 50;
}
void cinema_camera(daNpc_Kn_c*) { ++cameraTicks; }
void cinema_sword_shot(daNpc_Kn_c*) {}

using u32=unsigned;
constexpr u32 Z2BGM_TN_MBOSS=0x100006C;
struct Audio {
    u32 main=100,stream=0xffffffff,sub=0xffffffff;
    int prepares=0,plays=0,stops=0,battles=0,subStops=0,handoffs=0;
    bool roomAudible=true;
    u32 getMainBgmID(){return main;}u32 getStreamBgmID(){return stream;}u32 getSubBgmID(){return sub;}
    void bgmStreamPrepare(u32 id){stream=id;++prepares;roomAudible=false;}
    void bgmStreamPlay(){++plays;}
    void bgmStreamStop(int){stream=0xffffffff;++stops;roomAudible=true;}
    void subBgmStart(u32 id){sub=id;stream=0xffffffff;++battles;roomAudible=false;}
    void subBgmStop(){++subStops;} // native Darknut stop alone delays room audio
    void subBgmStopInner(){sub=0xffffffff;++handoffs;roomAudible=true;}
} audio;
bool hasAudio=true,inArena=true,nextStage=false;
auto Z2GetAudioMgr(){return hasAudio ? &audio : nullptr;}
bool arena(){return inArena;}bool dComIfGp_isEnableNextStage(){return nextStage;}
#include "heroes_shade_music.inc"

'''

runtime_start = source.index("struct CinemaRuntime {")
runtime = source[runtime_start:source.index("} sCinemaRuntime;", runtime_start) + len("} sCinemaRuntime;")]

checks = r'''
void reset() {
    boss={}; body={}; player={}; camera={}; message={}; sFighters={};
    sCinema={}; sCinemaRuntime={};
    wolf={};wolf.mAnm_p=&wolf.animation;wolfReady=wolfPending=true;
    sShadeWolf={};sShadeWolf.id=43;wolfDeletes=0;
    mDoGph_gInf_c::enabled=false;mDoGph_gInf_c::rate=0;mDoGph_gInf_c::speed=0;
    audio={};sShadeMusic={};hasAudio=inArena=true;nextStage=false;
    paused=ui=blockMessages=delayMessageOpen=false; hasCamera=true;
    eventResets=orders=deletes=cameraTicks=titleStarts=0;
}
void tick() {
    tick_cinema(&boss,sFighters[0]);
    // Native twilight() may undo mNoDraw; the production Draw hook still wins.
    const bool noDraw=boss.mNoDraw;boss.mNoDraw=false;
    auto* actor=&boss;int result=0;
    const bool hidden=sFighters[0].deleting || sShadeWolf.hides_shade() ||
        (sCinema.shot==shade::Shot::Request && !sCinema.victory) || sCinema.shot==shade::Shot::Afterglow;
    assert((draw_cinema(nullptr,&actor,&result,nullptr)==HOOK_SKIP_ORIGINAL)==hidden);
    boss.mNoDraw=noDraw;
    if (!paused && !ui && wolf.mAnm_p) wolf.animation.frame+=wolf.animation.speed;
}
void accept(bool victory) {
    sCinema.begin(victory);
    tick(); assert(orders==1 && player.starts==0 && camera.mCamera.stops==0);
    boss.eventInfo.accepted=true; tick();
    assert(player.starts==1 && camera.mCamera.stops==1);
    assert(sCinemaRuntime.ownsEvent && camera.mCamera.mTrimSize==3);
    assert(audio.prepares==(victory ? 0 : 1) && audio.battles==0);
}
int main() {
    // Stop uses the immediate native handoff, restoring the underlying room
    // sequence without restarting it or waiting for Darknut's 510-tick delay.
    reset();start_shade_music(true);start_shade_music(false);stop_shade_music();
    assert(audio.main==100 && audio.roomAudible && audio.sub==kNoMusic && audio.handoffs==1);
    stop_shade_music();assert(audio.handoffs==1 && !sShadeMusic.active);
    // Stream-based room music is saved before the intro and restored on victory.
    reset();audio.stream=0x2000070;start_shade_music(true);start_shade_music(false);
    stop_shade_music();assert(audio.stream==0x2000070 && audio.plays==2);
    // Cancelling the intro also restores its replaced stream exactly once.
    reset();audio.stream=0x2000070;start_shade_music(true);stop_shade_music();
    assert(audio.stream==0x2000070 && audio.plays==2);
    // Without an accepted intro, the event timeout can start combat directly.
    reset();audio.stream=0x2000070;start_shade_music(false);stop_shade_music();
    assert(audio.stream==0x2000070 && audio.battles==1 && audio.plays==1);
    for(int exit=0;exit<5;++exit) {
        reset();audio.stream=0x2000070;start_shade_music(true);start_shade_music(false);
        if(exit==0) nextStage=true;
        if(exit==1) player.dead=true;
        if(exit==2) inArena=false;
        if(exit==3) audio.main=999; // game-over or another scene owns main
        if(exit==4) {audio.stream=777;audio.sub=888;} // replacement music is untouched
        stop_shade_music();
        assert(audio.plays==1 && !sShadeMusic.active);
        if(exit==4) assert(audio.stream==777 && audio.sub==888 && audio.handoffs==0);
        else assert(audio.sub==kNoMusic && audio.stream==kNoMusic && audio.handoffs==1);
    }
    reset();audio.sub=Z2BGM_TN_MBOSS;stop_shade_music();assert(audio.handoffs==0); // not owned
    reset();hasAudio=false;start_shade_music(true);assert(!sShadeMusic.active);
    hasAudio=true;start_shade_music(true);hasAudio=false;stop_shade_music();assert(!sShadeMusic.active);
    // A short intro stream may end during captions. The audio pre-hook must
    // install battle BGM before native room-unmute, not wait for caption/title.
    reset();accept(false);sCinema.enter(shade::Shot::Words1);
    for(int i=0;i<20;++i) advance_shade_music();
    assert(audio.battles==0 && audio.stream==kShadeIntroMusic);
    audio.stream=kNoMusic; // native stream has naturally detached
    advance_shade_music();
    assert(audio.battles==1 && !audio.roomAudible && sCinema.shot==shade::Shot::Words1);
    for(int i=0;i<20;++i) advance_shade_music();
    start_shade_music(false);assert(audio.battles==1); // no restart at combat entry
    stop_shade_music();assert(audio.roomAudible && !sShadeMusic.active);
    // Finishing the intro early uses the single native TN_MBOSS handoff.
    reset();start_shade_music(true);start_shade_music(false);
    assert(audio.stops==0 && audio.battles==1);
    // Cleanup and unrelated replacement cues disable automatic takeover.
    for(int scenario=0;scenario<6;++scenario) {
        reset();start_shade_music(true);audio.stream=kNoMusic;
        if(scenario==0) stop_shade_music();
        if(scenario==1) nextStage=true;
        if(scenario==2) player.dead=true;
        if(scenario==3) audio.main=999;
        if(scenario==4) audio.stream=777;
        if(scenario==5) audio.sub=888;
        advance_shade_music();assert(audio.battles==0);
    }
    // Accepted != opened: status 1 can persist for several native updates.
    // This reproduced the old bug: the camera was released on the next tick,
    // leaving a queued title to open only after the cinematic had ended.
    for (auto shot : {shade::Shot::Words1,shade::Shot::Words2,shade::Shot::BossName}) {
        reset(); accept(false); sCinema.enter(shot); delayMessageOpen=true;
        tick(); assert(sCinemaRuntime.messageStarted && message.status==1);
        for (int i=0;i<12;++i) tick();
        assert(sCinema.shot==shot && player.cancels==0 && eventResets==0);
        message.status=2; tick(); // native fade-in finally starts
        message.status=16; tick(); // visible hold
        assert(sCinema.shot==shot);
        message.status=17; tick(); // native fade-out still owns the camera
        assert(sCinema.shot==shot);
        message.status=1; tick(); // now idle really means completion
        assert(sCinema.shot!=shot);
    }
    // Pending titles must also be removed on abort/timeout, not left to open
    // after control is restored. Never cancel a replacement message.
    for (int scenario=0;scenario<3;++scenario) {
        reset(); accept(false); sCinema.enter(shade::Shot::BossName); delayMessageOpen=true;
        tick(); assert(message.status==1 && !sCinemaRuntime.messageOpened);
        if (scenario==0) { release_cinema(); release_cinema(); }
        if (scenario==1) for (int i=0;i<300 && sCinema.active();++i) tick();
        if (scenario==2) { message.msg_idx=500; release_cinema(); }
        assert(!sCinema.active() && player.cancels==1);
        assert(message.deleteRequests==(scenario==2 ? 0 : 1));
        assert(message.kills==(scenario==2 ? 0 : 1));
    }
    reset(); accept(false);
    for (int i=0;i<17;++i) { tick(); assert(boss.mNoDraw); }
    for (int i=0;i<400 && sCinema.shot==shade::Shot::Arrival;++i) {
        const bool before=sShadeWolf.swapped;
        tick();
        if (!before && sShadeWolf.swapped) assert(mDoGph_gInf_c::rate==1);
    }
    assert(wolf.howls==1 && !sShadeWolf.active && !boss.mNoDraw && wolfDeletes==0);
    assert(sCinema.shot==shade::Shot::Words1);
    tick(); assert(message.msg_idx==101 && sCinemaRuntime.messageStarted);
    for (int i=0;i<30;++i) tick();
    assert(sCinema.shot==shade::Shot::Words1); // wait for the actual caption
    message.status=1; tick(); tick(); assert(message.msg_idx==102);
    message.status=1; tick(); assert(sCinema.shot==shade::Shot::Ready);
    for (int i=0;i<18;++i) tick();
    assert(sCinema.shot==shade::Shot::Ready && titleStarts==0);
    body.frame=69; tick(); assert(titleStarts==0);
    body.frame=70; tick(); // cue while the pointing gesture is still running
    assert(sCinema.shot==shade::Shot::BossName && titleStarts==1);
    assert(message.msg_idx==105 && boss.mMotionSeqMngr.no==24 && boss.mMotionSeqMngr.step==0);
    // The banner owns the final shot until native fade-out actually finishes.
    for (int i=0;i<120;++i) tick();
    assert(sCinema.active() && titleStarts==1 && player.cancels==0 && eventResets==0);
    assert(!sFighters[0].reset && !sFighters[0].deleting);
    assert(audio.stream==kShadeIntroMusic && audio.prepares==1 && audio.plays==1 && audio.battles==0);
    message.status=1; tick();
    assert(!sCinema.active() && sFighters[0].reset && !sFighters[0].deleting);
    assert(player.cancels==1 && eventResets==1 && camera.mCamera.starts==1);
    assert(camera.mCamera.mTrimSize==1 && deletes==0);
    assert(audio.stream==kNoMusic && audio.sub==Z2BGM_TN_MBOSS && audio.battles==1 && audio.main==100);
    assert(boss.field_0x16f4.x==1 && !boss.mNoDraw);
    // Replay can display a fresh title; cancellation releases only this banner.
    sCinema.begin(false); tick(); boss.eventInfo.accepted=true; tick();
    assert(player.starts==2 && sCinemaRuntime.ownsEvent);
    sCinema.enter(shade::Shot::BossName); tick();
    assert(titleStarts==2 && message.msg_idx==105);
    release_cinema(); assert(message.kills==1 && player.cancels==2);

    // The cue scales with clip length, without waiting for the idle step.
    reset(); accept(false); sCinema.enter(shade::Shot::Ready); cinema_pose(&boss,24);
    body.end=160; body.frame=111;
    for (int i=0;i<60;++i) tick();
    assert(sCinema.shot==shade::Shot::Ready && titleStarts==0);
    body.frame=112; tick();
    assert(sCinema.shot==shade::Shot::BossName && titleStarts==1);
    assert(camera.mCamera.mTrimSize==3 && player.cancels==0);

    // Stale frames from TALK_A must not trigger the title before the ready
    // clip is installed; absent/invalid model data is handled by the timeout.
    reset(); accept(false); sCinema.enter(shade::Shot::Ready); cinema_pose(&boss,24);
    boss.mMotionSeqMngr.newMotion=true; body.frame=90; tick();
    assert(titleStarts==0);
    boss.mMotionSeqMngr.newMotion=false; body.end=0; tick(); assert(titleStarts==0);
    boss.mpModelMorf[0]=nullptr; tick(); assert(titleStarts==0);
    boss.mpModelMorf[0]=&body; body.end=100; body.frame=70; tick();
    assert(titleStarts==1 && boss.mMotionSeqMngr.step==0);

    reset(); boss.mMotionSeqMngr.no=19; accept(true);
    assert(boss.mMotionSeqMngr.no==22); // native rise, not a snap to standing
    for (int i=0;i<50;++i) tick();
    assert(sCinema.shot==shade::Shot::Recover);
    boss.mMotionSeqMngr.step=1; tick(); tick(); assert(message.msg_idx==103);
    message.status=1; tick(); tick(); assert(message.msg_idx==104);
    message.status=1; tick();
    assert(sCinema.shot==shade::Shot::Depart && deletes==0);
    float alpha=1;
    for (int i=0;i<400 && sCinema.shot==shade::Shot::Depart;++i) {
        const bool before=sShadeWolf.swapped;
        tick();
        if (!before && sShadeWolf.swapped) assert(mDoGph_gInf_c::rate==1 && wolf.mAnm==2);
        if (sShadeWolf.swapped) { assert(sShadeWolf.opacity<=alpha);alpha=sShadeWolf.opacity; }
    }
    assert(sCinema.shot==shade::Shot::Afterglow && deletes==0 && boss.mNoDraw);
    assert(!sShadeWolf.active && sShadeWolf.opacity==0 && !mDoGph_gInf_c::enabled);
    for (int i=0;i<30;++i) tick();
    assert(deletes==1 && eventResets==1 && player.cancels==1 && !sCinema.active());
    assert(titleStarts==0); // no title during the victory scene
    assert(audio.prepares==0 && audio.battles==0); // no intro/battle restart in outro

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
        for (int i=0;i<1700 && sCinema.active();++i) tick();
        assert(!sCinema.active() && eventResets==1 && player.cancels==1);
        assert(deletes==(victory ? 1 : 0));
        reset(); sCinema.begin(victory);
        for (int i=0;i<180;++i) tick();
        assert(!sCinema.active() && eventResets==0 && camera.mCamera.starts==0);
    }
    reset(); sCinema.begin(false); boss.eventInfo.accepted=true; hasCamera=false;
    tick(); assert(!sCinema.active() && eventResets==1 && !boss.mNoDraw);

    reset(); accept(false); const int ticks=sCinema.ticks; const float frame=wolf.animation.frame;
    paused=true; tick(); paused=false; ui=true; tick(); ui=false;
    assert(sCinema.ticks==ticks && wolf.animation.frame==frame);
    // Explicit cancellation at every shot returns only the acquired controls.
    for (auto shot : {shade::Shot::Arrival,shade::Shot::Recover,shade::Shot::Words1,
                      shade::Shot::Words2,shade::Shot::Ready,shade::Shot::BossName,
                      shade::Shot::Depart,shade::Shot::Afterglow}) {
        reset(); accept(false); sCinema.enter(shot); release_cinema(); release_cinema();
        assert(eventResets==1 && player.cancels==1 && camera.mCamera.starts==1 && !sShadeWolf.active);
    }
    // Async wolf loading must finish before camera ownership or the pan starts.
    reset();wolfReady=false;sCinema.begin(false);boss.eventInfo.accepted=true;
    for(int i=0;i<20;++i) tick();
    assert(player.starts==0 && sCinema.shot==shade::Shot::Request);
    wolfReady=true;tick();assert(player.starts==1 && sShadeWolf.visible());
    reset();wolfReady=false;sCinema.begin(false);
    for(int i=0;i<180;++i) tick();
    assert(deletes==1 && wolfDeletes==1 && !sCinema.active() && player.starts==0);
    // A replacement stage fade must survive cleanup; our white overlay must not.
    reset();sShadeWolf.flash(1);release_cinema();assert(!mDoGph_gInf_c::enabled);
    reset();sShadeWolf.flash(1);mDoGph_gInf_c::color={0,0,0,255};
    release_cinema();assert(mDoGph_gInf_c::enabled);
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
    wolf_source=(root / 'src/heroes_shade_wolf.inc').read_text()
    wolf_runtime=wolf_source[:wolf_source.index('DEFINE_HOOK(')]
    cpp.write_text(fixture.replace('// WOLF_RUNTIME', wolf_runtime) + runtime + "\n" + "\n".join(function(name) for name in (
        "close_cinema_line", "start_cinema_line", "release_cinema", "finish_cinema", "cinema_pose", "tick_cinema")) +
        source[source.index('HookAction draw_cinema('):source.index('void after_execute(')] + checks)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(root / "src"),
                    str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade cutscene lifecycle, music handoffs, wolf transformation pacing and control ownership: passed")
