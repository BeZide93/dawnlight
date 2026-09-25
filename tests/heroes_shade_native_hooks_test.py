"""Run the actual accessory and sword hooks against a minimal native API fixture.

This reproduces the missing lesson-7 sheath and exercises collider registration,
not just a second copy of the hit-window formula. No game assets are required.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
encounter = (root / "src/heroes_shade_encounter.cpp").read_text()


def function(name, result="HookAction"):
    start = encounter.index(f"{result} {name}(")
    end = encounter.index("\n}\n", start) + 3
    return encounter[start:end]


fixture = r'''
#include "heroes_shade_battle.hpp"
#include "heroes_shade_cinema.hpp"
#include "heroes_shade_stride.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cmath>
using s16 = std::int16_t;
namespace shade = dawnlight::shade;
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods {
template<class T> T arg(void* args, int index) { assert(index==0); return static_cast<T>(args); }
}
struct cXyz {
    float x=0, y=0, z=0;
    cXyz() = default;
    cXyz(float a,float b,float c):x(a),y(b),z(c) {}
    cXyz& operator+=(const cXyz& b) { x+=b.x; y+=b.y; z+=b.z; return *this; }
    float absXZ() const { return std::sqrt(x*x+z*z); }
    void zero() { x=y=z=0; }
    void set(float a,float b,float c) { x=a;y=b;z=c; }
    cXyz operator-(const cXyz& b) const { return {x-b.x,y-b.y,z-b.z}; }
};
s16 cLib_targetAngleY(const cXyz* from,const cXyz* to) {
    return static_cast<s16>(static_cast<int>(std::atan2(to->x-from->x,to->z-from->z)*32768.0f/3.14159265f));
}
void cLib_chasePos(cXyz* pos,const cXyz& target,float step) {
    const auto delta=target-*pos;
    const float distance=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    if(distance<=step) {*pos=target;return;}
    pos->x+=delta.x*step/distance;pos->y+=delta.y*step/distance;pos->z+=delta.z*step/distance;
}
using Mtx = float[3][4];
void MTXCopy(const Mtx in,Mtx out) { std::memcpy(out,in,sizeof(Mtx)); }
void MTXMultVec(const Mtx m,const cXyz* in,cXyz* out) {
    *out={in->x+m[0][3],in->y+m[1][3],in->z+m[2][3]};
}
struct Model {
    Mtx blade{},body{},base{};
    cXyz authoredBody{20,200,594};
    auto getAnmMtx(int joint) -> float (*)[4] { assert(joint==1 || joint==13); return joint==1 ? body : blade; }
    auto getBaseTRMtx() -> float (*)[4] { return base; }
    void setBaseTRMtx(const Mtx m) { MTXCopy(m,base); }
};
struct Morf {
    Model model;
    float frame=0;
    int calculations=0;
    Model* getModel() { return &model; }
    float getFrame() { return frame; }
    void modelCalc() {
        ++calculations;
        MTXCopy(model.base,model.body);
        model.body[0][3]+=model.authoredBody.x;
        model.body[1][3]+=model.authoredBody.y;
        model.body[2][3]+=model.authoredBody.z;
    }
};
struct Motion {
    int no=shade::helm_splitter, step=0;
    int getNo() { return no; }
    int getStepNo() { return step; }
    float blend=-1;
    void setNo(int n,float b,int,int) { no=n; blend=b; step=0; }
};
constexpr int AT_TYPE_NORMAL_SWORD=1,AT_TYPE_MASTER_SWORD=2;
struct Hit {int type=AT_TYPE_MASTER_SWORD;bool ChkAtType(int mask){return (type&mask)!=0;}};
struct Cylinder {
    Hit object; void* attacker=nullptr;
    bool ChkTgHit(){return hit;}
    Hit* GetTgHitObj(){return &object;}
    void* GetTgHitAc(){return attacker;}
    bool target=true,hit=true,shield=true,co=true;
    void OffCoSetBit(){co=false;}
    void OffTgShield(){shield=false;}
    void OffTgSetBit(){target=false;}void ClrTgHit(){hit=false;}
    cXyz center;
    const cXyz& GetC() const { return center; }
    float GetR() const { return 30; }
    float GetH() const { return 150; }
};
struct daPy_py_c {
    enum {CUT_TYPE_LARGE_JUMP_INIT=18,CUT_TYPE_LARGE_JUMP=19,CUT_TYPE_LARGE_JUMP_FINISH=20,CUT_TYPE_TURN_RIGHT=8,CUT_TYPE_TURN_LEFT=22,CUT_TYPE_LARGE_TURN_LEFT=23,CUT_TYPE_LARGE_TURN_RIGHT=24};
};
struct Player { std::array<Cylinder,3> mTgCyls; int cut=0; int getCutType(){return cut;} bool checkDeadHP(){return false;} } player;
Player* daAlink_getAlinkActorClass() { return &player; }
Player* daPy_getPlayerActorClass() { return &player; }
struct Sphere {
    cXyz center;
    float radius=0;
    int damage=0;
    bool enabled=false, hit=false, shield=false;
    void* target=nullptr;
    bool ChkAtHit() { return hit; }
    bool ChkAtShieldHit() { return shield; }
    void* GetAtHitAc() { return target; }
    void SetC(const cXyz& c) { center=c; }
    void SetR(float r) { radius=r; }
    void SetAtAtp(int p) { damage=p; }
    void OnAtSetBit() { enabled=true; }
    void OffAtSetBit() { enabled=false; }
    void ClrAtHit() { hit=shield=false; target=nullptr; }
};
struct cM3dGCps {
    cXyz start,end;
    float radius=0;
    void Set(const cXyz& a,const cXyz& b,float r) { start=a; end=b; radius=r; }
};
struct dCcD_Cps : Sphere, cM3dGCps {
    dCcD_Cps() { damage=2; }
    void SetAtVec(const cXyz&) {}
};
struct Fighter {
    int id=42, divide=0;
    float walkSpeed=0;
    bool forming=false,returning=false;
    int defeatPhase=-1;
    int formationTicks=0;
    shade::AttackChain chain;
    bool reset=false,swordContact=false;
    int offense=shade::helm_splitter;
    bool animationStarted=true, deleting=false, helmTurnPending=false;
    shade::BladeMotion blade;
    std::array<dCcD_Cps,2> bladeSweeps;
    cXyz jumpBodyOffset{5,0,10};
};
struct daNpc_Kn_c {
    bool owned=true, mNoDraw=false;
    int field_0x15af=1;
    int mType=6,field_0x170c=0,field_0x170d=0;
    bool mCreating=false;
    cXyz field_0x16f4{1,1,1};
    struct {void ClrCcMove(){}} mCcStts;
    void offDownFlg(){down=false;}void offHeadLockFlg(){}
    int mEvtNo=0, health=10;
    bool mSpeakEvent=false;
    void* mpPodModel=nullptr;
    unsigned mPodAnmFlags=0x41;
    int field_0x15cd=1;
    Fighter entry;
    Motion mMotionSeqMngr, mFaceMotionSeqMngr;
    int mActionMode=19, field_0x15bc=0, field_0x15bd=0, landings=0;
    int falls=0,warps=0,jumpFalls=0,jumpWarps=0;
    void teach06_warpDelete(void*){++jumpWarps;}
    void teach06_superJumpedDivide(void*){++jumpFalls;}
    void teach07_warpDelete(void*){++warps;}
    void teach07_superTurnAttackedDivide(void*){++falls;}
    struct Hio {struct {float attack_disappear_speed_h=12,attack_disappear_speed_v=18;} m;} hio;
    Hio* mpHIO=&hio;
    struct {void lookNone(int){}} mJntAnm;
    struct {void startCreatureVoice(int,int){} void startCollisionSE(int,int){}} mSound;
    struct {s16 y=0;} shape_angle;
    int mMode=2, field_0xdec=0;
    bool down=false;
    bool checkDownFlg() const { return down; }
    struct { bool grounded=false; bool ChkGroundHit() const { return grounded; } } mAcch;
    cXyz mTargetPos;
    float gravity=-3;
    void setLandingPrtcl() { ++landings; }
    Morf morf, ghost;
    std::array<Morf*,2> mpModelMorf{&morf,&ghost};
    struct { cXyz pos{100,0,300}; struct { s16 y=1234; } angle; } current;
    struct { cXyz pos; struct { s16 y=0; } angle; } home;
    bool formationBlocked=false;
    void setPos(const cXyz& pos) { current.pos=pos; }
    // Native formation contract: initialize a mirrored target, take a 6-unit
    // step, then request wait immediately when no teaching event is running.
    void teach06_divideMove(void*) {
        if(mMode==1) {
            const float yaw=(home.angle.y+(entry.divide==1 ? -0x1555 : 0x1555))*3.14159265f/32768;
            mTargetPos={home.pos.x+180*std::sin(yaw),home.pos.y,home.pos.z+180*std::cos(yaw)};
            mMotionSeqMngr.no=9;mMode=2;
        }
        const auto delta=mTargetPos-current.pos;
        const float distance=delta.absXZ();
        if(!formationBlocked && distance>0) {
            const float step=std::min(6.0f,distance)/distance;
            current.pos.x+=delta.x*step;current.pos.z+=delta.z*step;
        }
        mActionMode=15;
    }
    void setAngle(s16 y) { current.angle.y=y; }
    struct { cXyz position; } attention_info;
    cXyz eyePos;
    cXyz speed;
    float speedF=0;
    int getBackboneJointNo() { return 1; }
    Cylinder mCylCc;
    std::array<Sphere,2> mSphCc;
};
Fighter* fighter(daNpc_Kn_c* a) { return a->owned ? &a->entry : nullptr; }
shade::Battle sBattle;
shade::Cinema sCinema;
constexpr int kNone=-1, cPhs_COMPLEATE_e=4;
constexpr int Z2SE_KN_V_DAMAGE_L=1,Z2SE_HIT_SWORD=2,kShadeParams=0,fpcNm_NPC_KN_e=1;
int spawns=0;
bool pending_or_live(int id){return id>=1000;}
std::array<cXyz,2> spawnPositions;
std::array<s16,2> spawnAngles;
void spawn(int,const cXyz& pos,s16 yaw,int,int& id){
    spawnPositions[spawns%2]=pos;spawnAngles[spawns%2]=yaw;id=1000+ ++spawns;
}
std::array<Fighter,3> sFighters;
daNpc_Kn_c* formationBoss=nullptr;
daNpc_Kn_c* actor_by_id(int id){return id==sFighters[0].id ? formationBoss : nullptr;}
struct daObjKnBullet_c { int parentActorID=42; Sphere mCcSph; };
constexpr float kApproachSpeed=shade::stride::speed;
float fopAcM_GetMaxFallSpeed(daNpc_Kn_c*) { return -40; }
s16 playerYaw=0;
s16 fopAcM_searchPlayerAngleY(daNpc_Kn_c*) { return playerYaw; }
struct Space {
    int registered=0, expectedDamage=8;
    void Set(Sphere* sphere) { assert(sphere->enabled && sphere->damage==expectedDamage); ++registered; }
} space;
Space* dComIfG_Ccsp() { return &space; }
bool sStopping=false,paused=false,accepted=false;
int companionRemovals=0,musicStops=0,cinemaTicks=0;
bool arena(){return true;}
bool dComIfGp_isEnableNextStage(){return false;}
bool dComIfGp_event_runCheck(){return accepted;}
bool dComIfGp_isPauseFlag(){return paused;}
bool ui_document_visible(){return false;}
float cM_rndF(float count){return count-1;}
void stop_shade_music(){++musicStops;}
void release_cinema(){sCinema={};}
void cancel_trial(){}void suspend_trial(){}void finish_trial(){}
int removals=0,lastRemoved=-1;
void remove_actor(int id){++removals;lastRemoved=id;}
void retire_double(Fighter&,bool);
void remove_companions(bool recall=false){++companionRemovals;for(unsigned i=1;i<3;++i)retire_double(sFighters[i],recall);}
void tick_arena_hazards(){}
void select_phase(daNpc_Kn_c*,Fighter& entry){entry.reset=false;}
void cinema_pose(daNpc_Kn_c*,int){}
void begin_trial(daNpc_Kn_c*){}bool tick_trial(daNpc_Kn_c*){return false;}
void tick_cinema(daNpc_Kn_c*,Fighter&){++cinemaTicks;sCinema.tick(accepted,false,false);}
struct Wolf {int id=1001;void prepare(const cXyz&,s16){id=1001;}} sShadeWolf;
'''

checks = r'''
int main() {
    // Spawn requests overlap the boss, including yaw; pending actors are not
    // requested again. Async creation then follows the boss's current pose.
    for(unsigned phase : {6u,7u}) {
        sBattle={};sBattle.phase=phase;sFighters={};spawns=0;
        daNpc_Kn_c boss;boss.current.pos={450,20,-270};boss.shape_angle.y=7000;
        formationBoss=&boss;
        add_doubles(&boss);add_doubles(&boss);
        assert(spawns==2);
        for(int i=0;i<2;++i) {
            assert((spawnPositions[i]-boss.current.pos).absXZ()==0);
            assert(spawnPositions[i].y==boss.current.pos.y && spawnAngles[i]==boss.shape_angle.y);
            assert(sFighters[i+1].forming);
        }
        boss.current.pos={470,20,-250};
        for(int divide : {1,2}) {
            daNpc_Kn_c clone;clone.entry=sFighters[divide];
            assert(form_double(&clone,clone.entry));
            assert((clone.home.pos-boss.current.pos).absXZ()==0);
            assert(clone.home.angle.y==boss.shape_angle.y);
            assert((clone.current.pos-boss.current.pos).absXZ()<=6.001f);
            assert(clone.entry.forming && clone.mMode==2 && clone.mMotionSeqMngr.no==9);
            assert(clone.mActionMode==(phase==6 ? 14 : 20));
            trial_body_collision(nullptr,&clone,nullptr,nullptr);
            assert(!clone.mCylCc.co && clone.mCylCc.target);
            for(auto& sphere:clone.mSphCc)sphere.enabled=true;
            assert(sword_collision(nullptr,&clone,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
            for(const auto& sphere:clone.mSphCc)assert(!sphere.enabled);
            int ticks=1;
            while(clone.entry.forming && ticks<120) {
                const auto previous=clone.current.pos;
                assert(form_double(&clone,clone.entry));
                assert((clone.current.pos-previous).absXZ()<=6.001f);
                ++ticks;
            }
            assert(ticks==30 && !clone.entry.forming);
            assert((clone.current.pos-clone.mTargetPos).absXZ()<=1);
            assert(clone.mActionMode==(phase==6 ? 15 : 21) && clone.mMode==1);
            assert(!form_double(&clone,clone.entry));
        }
        assert(boss.current.pos.x==470 && boss.current.pos.z==-250);
        assert(!form_double(&boss,boss.entry));
        daNpc_Kn_c blocked;blocked.entry=sFighters[1];blocked.formationBlocked=true;
        for(int tick=0;tick<120;++tick)assert(form_double(&blocked,blocked.entry));
        assert(!blocked.entry.forming && blocked.mActionMode==(phase==6 ? 15 : 21));
        assert((blocked.current.pos-boss.current.pos).absXZ()==0); // no timeout teleport
    }
    // Phase retirement follows a moving boss at four times formation speed,
    // and never hides/removes a live double before it overlaps him.
    for(unsigned phase : {6u,7u}) {
        sBattle={};sBattle.phase=phase;sFighters={};
        daNpc_Kn_c boss,clone;formationBoss=&boss;
        boss.current.pos={600,0,300};clone.current.pos={0,0,300};
        clone.entry.id=1001;clone.entry.divide=1;clone.entry.forming=true;
        removals=0;retire_double(clone.entry,true);
        assert(clone.entry.returning && !clone.entry.forming && !clone.entry.deleting && !removals);
        assert(!skill_counter_action(&clone,clone.entry));
        sBattle.phase=0;sBattle.trial=shade::Trial::Fire;sBattle.recovery=45;
        int result=0;paused=true;
        assert(before_execute(nullptr,&clone,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(clone.entry.formationTicks==0);
        paused=false;
        assert(before_execute(nullptr,&clone,&result,nullptr)==HOOK_CONTINUE);
        assert(return_double(&clone,clone.entry));
        assert(clone.current.pos.x==24 && clone.mMotionSeqMngr.no==9 && clone.entry.offense==-1);
        assert(!clone.mCylCc.target && !removals && !clone.mNoDraw);
        const auto ticks=clone.entry.formationTicks;
        retire_double(clone.entry,true);assert(clone.entry.formationTicks==ticks);
        trial_body_collision(nullptr,&clone,nullptr,nullptr);assert(!clone.mCylCc.co);
        for(auto& sphere:clone.mSphCc)sphere.enabled=true;
        assert(sword_collision(nullptr,&clone,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        for(const auto& sphere:clone.mSphCc)assert(!sphere.enabled);
        boss.current.pos={672,0,396}; // target must follow the boss, not his old position
        int count=0;
        while(!clone.entry.deleting && ++count<100) {
            const auto previous=clone.current.pos;
            assert(return_double(&clone,clone.entry));
            assert((clone.current.pos-previous).absXZ()<=24.001f);
            if(!clone.entry.deleting)assert(!removals && !clone.mNoDraw);
        }
        assert(count<100 && removals==1 && lastRemoved==clone.entry.id && clone.mNoDraw);
        assert((clone.current.pos-boss.current.pos).absXZ()<=1);
        assert(!return_double(&boss,boss.entry));

        // A defeated double carries its original counter across phase/trial
        // changes rather than walking back or switching to the new lesson.
        daNpc_Kn_c fallen;fallen.entry.id=1002;fallen.entry.divide=2;
        fallen.entry.defeatPhase=phase;fallen.mMotionSeqMngr.no=18;
        fallen.mActionMode=phase==6 ? 16 : 22;
        retire_double(fallen.entry,true);
        assert(!fallen.entry.returning && !fallen.entry.deleting);
        sBattle.defeated_doubles=0;
        before_execute(nullptr,&fallen,&result,nullptr);
        assert(fallen.mType==(phase==6 ? 5 : 6));
        assert(skill_counter_action(&fallen,fallen.entry));
        assert(fallen.jumpFalls==(phase==6) && fallen.falls==(phase==7));
        fallen.mAcch.grounded=true;fallen.mMotionSeqMngr.no=19;fallen.mMotionSeqMngr.step=1;
        assert(skill_counter_action(&fallen,fallen.entry));
        assert(fallen.jumpWarps==(phase==6) && fallen.warps==(phase==7));
        assert(!sBattle.defeated_doubles); // cannot poison the next phase's mask
        retire_double(fallen.entry,false);
        assert(fallen.entry.deleting && lastRemoved==fallen.entry.id); // hard cleanup still immediate
    }
    // The production timeout transition recalls the surviving clone while
    // keeping a defeated clone's fall. Pending retirement blocks replacement.
    sBattle={};sFighters={};spawns=0;
    sBattle.phase=sBattle.phase_cursor=6;sBattle.remaining=1;sBattle.trials_started=4;
    daNpc_Kn_c transitionBoss;formationBoss=&transitionBoss;
    sFighters[1].id=1001;sFighters[1].divide=1;
    sFighters[2].id=1002;sFighters[2].divide=2;sFighters[2].defeatPhase=6;
    int transitionResult=0;
    before_execute(nullptr,&transitionBoss,&transitionResult,nullptr);
    assert(sBattle.phase==7 && sFighters[1].returning && !sFighters[1].deleting);
    assert(!sFighters[2].returning && !sFighters[2].deleting && sFighters[2].defeatPhase==6);
    assert(!spawns);
    formationBoss=nullptr;sFighters={};sBattle={};
    daNpc_Kn_c a;
    for(unsigned phase : {6u,7u}) {
        sBattle.phase=phase;
        for(int divide : {1,2}) {
            a.entry.divide=divide;
            assert(waiting_action(a.entry)==(phase==6 ? 15 : 21));
            a.entry.chain.begin(phase);
            assert(a.entry.chain.next()==shade::sword);
            assert(a.entry.chain.next()==shade::sword);
        }
    }
    a.entry={};sBattle={};
    // Cutscenes cannot register a sword hit or consume another lesson counter.
    sCinema.begin(false);
    a.mEvtNo=shade::phases[0].success;
    no_order(nullptr,&a,nullptr,nullptr);
    assert(sBattle.health==10 && a.mEvtNo==0);
    for (auto& sphere:a.mSphCc) { sphere.enabled=true; sphere.hit=true; }
    for (auto& sweep:a.entry.bladeSweeps) { sweep.enabled=true; sweep.hit=true; }
    assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    for (const auto& sphere:a.mSphCc) assert(!sphere.enabled && !sphere.hit);
    for (const auto& sweep:a.entry.bladeSweeps) assert(!sweep.enabled && !sweep.hit);
    sCinema={};
    // Every intermission excludes native success events; only fire pauses offensive blades.
    a.entry.offense=-1;
    for (auto trial:{shade::Trial::Shield,shade::Trial::Fire,shade::Trial::Eyes,shade::Trial::Wind}) {
        sBattle.trial=trial; a.mEvtNo=shade::phases[0].success;
        no_order(nullptr,&a,nullptr,nullptr);
        assert(sBattle.health==10 && a.mEvtNo==0);
        for (auto& sphere:a.mSphCc) { sphere.enabled=true; sphere.hit=true; }
        a.mCylCc.target=true;a.mCylCc.hit=true;
        trial_body_collision(nullptr,&a,nullptr,nullptr);
        assert(!a.mCylCc.target);
        assert(!a.mCylCc.hit);
        a.field_0x15af=1;
        const auto action=sword_collision(nullptr,&a,nullptr,nullptr);
        assert(action==(trial!=shade::Trial::Fire ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL));
        for (const auto& sphere:a.mSphCc) assert(sphere.enabled==(trial!=shade::Trial::Fire));
    }
    sBattle={};a.entry.offense=shade::helm_splitter;
    // The real crash path: lesson-7 heap has no sheath calculator. The hook
    // must bypass native init(modify=true), with a successful body result.
    bool result=false;
    assert(accessory_motion(nullptr,&a,&result,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(result && a.mPodAnmFlags==0 && a.field_0x15cd==0);
    // Ordinary story actors and properly allocated accessories stay native.
    a.owned=false; a.mPodAnmFlags=0x41; a.field_0x15cd=1; result=false;
    assert(accessory_motion(nullptr,&a,&result,nullptr)==HOOK_CONTINUE);
    assert(!result && a.mPodAnmFlags==0x41 && a.field_0x15cd==1);
    a.owned=true; a.mpPodModel=&player;
    assert(accessory_motion(nullptr,&a,&result,nullptr)==HOOK_CONTINUE);
    assert(a.mPodAnmFlags==0x41);

    auto sample=[&](float frame,float z) {
        a.morf.frame=frame;
        a.morf.model.blade[1][3]=100;
        a.morf.model.blade[2][3]=z;
        space.registered=0;
        assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        return space.registered;
    };
    assert(sample(0,0)==0);
    assert(sample(5,15)==4); // formerly outside the 40%-75% gate
    assert(a.mSphCc[0].center.x==60 && a.mSphCc[1].center.x==120);
    assert(a.mSphCc[0].center.y==100 && a.mSphCc[0].center.z==15);
    assert(a.mSphCc[0].radius==30);
    assert(sample(6,15)==0); // stationary blade does not cause late damage
    assert(!a.mSphCc[0].enabled && !a.mSphCc[1].enabled);
    assert(sample(55,30)==4); // actual late cut must not be discarded either
    a.mSphCc[0].shield=true; a.mSphCc[0].target=&player;
    assert(sample(56,45)==0);
    assert(sample(57,60)==0); // lowering the shield cannot revive this strike
    a.entry.blade={};
    assert(sample(0,0)==0);
    assert(sample(5,15)==4);
    a.mSphCc[1].hit=true; a.mSphCc[1].target=&player;
    assert(sample(6,30)==0); // one damaging contact per attack

    // Actual hook registration traces both blade points, not a distant
    // sphere frozen at the previous position. A shield hit on either swept
    // volume consumes the shared strike just like a native sphere hit.
    a.entry.blade={};
    assert(sample(0,-100)==0);
    assert(sample(1,100)==4);
    assert(a.entry.bladeSweeps[0].start.z==-100);
    assert(a.entry.bladeSweeps[0].end.z==100);
    a.entry.bladeSweeps[1].shield=true;
    a.entry.bladeSweeps[1].target=&player;
    assert(sample(2,110)==0);
    assert(!a.entry.bladeSweeps[0].enabled);

    a.entry.blade={}; a.entry.offense=shade::jump_strike;
    a.mMotionSeqMngr.no=shade::jump_strike;
    assert(sample(0,0)==0);
    assert(sample(1,10)==4);
    a.mSphCc[0].hit=true; a.mSphCc[0].target=&player;
    assert(sample(2,20)==0);
    assert(sample(3,30)==0); // still on the body, never rearms
    assert(sample(4,150)==0); // withdrawal sample 1
    assert(sample(5,180)==4); // withdrawal sample 2, second cut allowed
    assert(sample(6,0)==4); // return toward Link
    a.entry.bladeSweeps[0].hit=true;
    a.entry.bladeSweeps[0].target=&player;
    assert(sample(7,10)==0);
    assert(sample(8,150)==0);
    assert(sample(9,180)==0); // no third strike in the same animation

    a.entry.blade={}; a.entry.offense=shade::back_slice;
    a.mMotionSeqMngr.no=shade::back_slice;
    for (int step=0;step<2;++step) {
        a.mMotionSeqMngr.step=step;
        assert(sample(0,0)==0);
        assert(sample(5,15)==0); // sidestep and roll never hurt
    }
    a.mMotionSeqMngr.step=2;
    assert(sample(0,0)==2); // cut begins now, without tracing the roll
    assert(sample(5,15)==4);
    a.mMotionSeqMngr.no=27; // interrupted by a real block, stale offense ignored
    assert(sample(6,30)==0);
    a.mMotionSeqMngr.no=shade::back_slice;
    assert(sample(7,45)==0); // no sweep spanning the interruption
    sBattle.recovery=45;
    assert(sample(8,60)==0);
    sBattle.recovery=0; sBattle.dying=true;
    assert(sample(9,75)==0);
    sBattle.dying=false;
    a.entry.deleting=true;
    assert(sample(10,90)==0);

    // Switch from a special to an ordinary combo swing. Damage must change
    // on both current blade volumes, including the native fallback afterward.
    a.entry.deleting=false; a.entry.blade={}; a.entry.offense=shade::sword;
    a.mMotionSeqMngr.no=shade::sword; a.mMotionSeqMngr.step=0;
    space.expectedDamage=4;
    assert(sample(0,0)==0);
    assert(sample(30,10)==2);
    for (const auto& sphere:a.mSphCc) assert(sphere.damage==4);
    a.entry.offense=-1;
    for (auto& sphere:a.mSphCc) sphere.damage=8;
    assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_CONTINUE);
    for (const auto& sphere:a.mSphCc) assert(sphere.damage==4);
    a.entry.offense=shade::helm_splitter;
    space.expectedDamage=8;

    daObjKnBullet_c bullet;
    int createResult=0;
    after_bullet(nullptr,&bullet,&createResult,nullptr);
    assert(bullet.mCcSph.damage==0); // still creating
    createResult=cPhs_COMPLEATE_e;
    after_bullet(nullptr,&bullet,&createResult,nullptr);
    assert(bullet.mCcSph.damage==8); // light ball is a special attack
    bullet.parentActorID=99; bullet.mCcSph.damage=0;
    after_bullet(nullptr,&bullet,&createResult,nullptr);
    assert(bullet.mCcSph.damage==0); // story/unrelated projectile unchanged

    // Execute the real post-pose correction on both visible models. Authored
    // vertical travel and orientation stay intact; only X/Z root travel moves.
    a.entry.deleting=false;
    a.entry.offense=shade::helm_splitter;
    a.mMotionSeqMngr.no=shade::helm_splitter;
    for (auto* m:a.mpModelMorf) {
        m->model.base[0][0]=0.5f; m->model.base[0][2]=-0.8f;
        m->model.base[0][3]=100; m->model.base[1][3]=25; m->model.base[2][3]=300;
        m->modelCalc();
    }
    a.eyePos={120,250,894}; a.attention_info.position={120,280,894};
    const auto frame=a.morf.frame;
    after_jump_pose(nullptr,&a,nullptr,nullptr);
    for (auto* m:a.mpModelMorf) {
        assert(m->model.body[0][3]==105 && m->model.body[2][3]==310);
        assert(m->model.body[1][3]==225);
        assert(m->model.base[0][0]==0.5f && m->model.base[0][2]==-0.8f);
        assert(m->calculations==2);
    }
    assert(a.morf.frame==frame);
    assert(a.eyePos.x==105 && a.eyePos.y==250 && a.eyePos.z==310);
    assert(a.attention_info.position.y==280);
    a.entry.offense=shade::sword;
    after_jump_pose(nullptr,&a,nullptr,nullptr);
    assert(a.morf.calculations==2);
    a.entry.offense=shade::helm_splitter; a.owned=false;
    after_jump_pose(nullptr,&a,nullptr,nullptr);
    assert(a.morf.calculations==2);

    // Helm recovery normalizes the root BEFORE disabling the pose correction.
    a.owned=true; a.entry.offense=shade::helm_splitter;
    a.mMotionSeqMngr.no=shade::helm_splitter;
    const auto pos=a.current.pos;
    const auto yaw=a.current.angle.y;
    finish_helm_splitter(&a,a.entry);
    assert(a.mMotionSeqMngr.no==6 && a.mMotionSeqMngr.blend==0);
    assert(a.entry.helmTurnPending);
    assert(a.current.angle.y==static_cast<s16>(yaw+0x8000));
    assert(a.current.pos.x==pos.x && a.current.pos.z==pos.z);
    finish_helm_splitter(&a,a.entry); // cannot flip twice after switching stance
    assert(a.current.angle.y==static_cast<s16>(yaw+0x8000));
    a.mMotionSeqMngr.no=shade::helm_splitter; a.entry.animationStarted=false;
    finish_helm_splitter(&a,a.entry);
    assert(a.mMotionSeqMngr.no==shade::helm_splitter);

    // Reproduce native turn-plus-walk after the stance with Link behind.
    // The actual post-hook must retain yaw and vertical motion while blocking
    // forward/XZ translation until the remaining turn is small enough.
    a.entry.animationStarted=true; a.entry.helmTurnPending=true;
    a.current.angle.y=0; playerYaw=static_cast<s16>(0x8000);
    for (int turn=0;turn<16;++turn) {
        a.current.angle.y=static_cast<s16>(turn*0x800);
        a.speedF=3; a.speed={2,-4,3};
        after_approach(nullptr,&a,nullptr,nullptr);
        assert(a.entry.helmTurnPending && a.speedF==0);
        assert(a.speed.x==0 && a.speed.z==0 && a.speed.y==-4);
        assert(a.current.angle.y==static_cast<s16>(turn*0x800));
    }
    a.current.angle.y=playerYaw; a.speedF=3;
    after_approach(nullptr,&a,nullptr,nullptr);
    assert(!a.entry.helmTurnPending && a.speedF>0 && a.speedF<kApproachSpeed);
    // Link already in front: movement resumes immediately, including yaw wrap.
    a.entry.helmTurnPending=true;
    a.current.angle.y=32760; playerYaw=-32760; a.speedF=3;
    after_approach(nullptr,&a,nullptr,nullptr);
    assert(!a.entry.helmTurnPending && a.speedF>0 && a.speedF<kApproachSpeed);
    // Only the post-Helm turn is constrained, never an ordinary native approach.
    a.current.angle.y=0; playerYaw=static_cast<s16>(0x8000); a.speedF=3;
    after_approach(nullptr,&a,nullptr,nullptr);
    assert(a.speedF>0 && a.speedF<kApproachSpeed);
    float previous=a.speedF;
    for (int tick=0;tick<20;++tick) {
        a.speedF=3;after_approach(nullptr,&a,nullptr,nullptr);
        assert(a.speedF>=previous && a.speedF<=kApproachSpeed);
        assert(a.speedF-previous<=0.801f);previous=a.speedF;
    }
    assert(a.speedF==kApproachSpeed);
    a.speedF=0;after_approach(nullptr,&a,nullptr,nullptr);
    assert(a.entry.walkSpeed==0 && a.speedF==0);
    a.entry.helmTurnPending=true; a.owned=false; a.speedF=3;
    after_approach(nullptr,&a,nullptr,nullptr);
    assert(a.speedF==3);
    a.owned=true; sBattle.recovery=1;
    after_approach(nullptr,&a,nullptr,nullptr);
    assert(a.speedF==3);
    sBattle.recovery=0;

    // Recovery must let physical knockback finish, then select the native
    // landing/lying sequence at real floor contact. Exercise main and doubles,
    // both directions, and the spin double both with and without global recovery.
    for (int divide : {0,1,2}) for (int motion : {14,18}) for (int recovery : {0,45}) {
        daNpc_Kn_c fallen;
        fallen.entry.divide=divide;
        fallen.mActionMode=divide ? 22 : 19;
        fallen.mMotionSeqMngr.no=motion;
        fallen.speedF=-6; fallen.speed={0,12,0};
        sBattle.recovery=recovery;
        hold_recovery(&fallen);
        assert(fallen.speedF==-6 && fallen.speed.y==12);
        before_movement(nullptr,&fallen,nullptr,nullptr);
        assert(fallen.speed.y==12); // posMoveF owns gravity on this path
        fallen.speedF=0;
        before_movement(nullptr,&fallen,nullptr,nullptr);
        assert(fallen.speed.y==9); // posMove still needs gravity during recovery
        fallen.speed.y=-4;
        after_knockdown_movement(nullptr,&fallen,nullptr,nullptr);
        assert(fallen.mMotionSeqMngr.no==motion && fallen.landings==0); // airborne
        fallen.mAcch.grounded=true; fallen.speed.y=3;
        after_knockdown_movement(nullptr,&fallen,nullptr,nullptr);
        assert(fallen.mMotionSeqMngr.no==motion); // still ascending
        fallen.speed.y=-4; fallen.speedF=-6; fallen.field_0x15bc=1;
        after_knockdown_movement(nullptr,&fallen,nullptr,nullptr);
        assert(fallen.mMotionSeqMngr.no==(motion==18 ? 19 : 15));
        assert(fallen.speedF==0 && fallen.speed.y==0 && fallen.field_0x15bc==0);
        assert(fallen.mTargetPos.x==fallen.current.pos.x);
        assert(fallen.landings==1);
        after_knockdown_movement(nullptr,&fallen,nullptr,nullptr);
        assert(fallen.landings==1); // do not restart the landing/lying animation
        fallen.speedF=6; fallen.speed={1,2,3};
        hold_recovery(&fallen);
        assert(fallen.speedF==0 && fallen.speed.x==0 && fallen.speed.y==0 && fallen.speed.z==0);
    }
    daNpc_Kn_c finishing;
    finishing.mActionMode=3; finishing.mMotionSeqMngr.no=18;
    finishing.mAcch.grounded=true; finishing.speed.y=-1;
    after_knockdown_movement(nullptr,&finishing,nullptr,nullptr);
    assert(finishing.mMotionSeqMngr.no==18 && finishing.landings==0); // native Ending Blow
    finishing.mActionMode=22; finishing.entry.divide=1; finishing.owned=false;
    after_knockdown_movement(nullptr,&finishing,nullptr,nullptr);
    assert(finishing.landings==0); // story teacher unchanged
    finishing.owned=true; finishing.entry.deleting=true;
    after_knockdown_movement(nullptr,&finishing,nullptr,nullptr);
    assert(finishing.landings==0);
    finishing.entry.deleting=false; sBattle.dying=true;
    after_knockdown_movement(nullptr,&finishing,nullptr,nullptr);
    assert(finishing.landings==0);
    sBattle.dying=false; sBattle.recovery=45;
    finishing.mMotionSeqMngr.no=9; finishing.speed.y=0;
    before_movement(nullptr,&finishing,nullptr,nullptr);
    assert(finishing.speed.y==0); // recovery still holds non-falling fighters
    sBattle.recovery=0;

    // The actual hook advances eight ticks only during the vulnerable
    // ground window; ordinary and odd timer lengths expire in ceil(N/8) ticks.
    daNpc_Kn_c ending;
    ending.mActionMode=3; ending.down=true;
    ending.mMotionSeqMngr.no=19; ending.mMotionSeqMngr.step=1;
    for (int duration : {1,2,3,7,8,9,30,59,120}) {
        ending.field_0xdec=duration;
        int ticks=0;
        while (ending.field_0xdec>0) {
            assert(before_ending_blow_wait(nullptr,&ending,nullptr,nullptr)==HOOK_CONTINUE);
            --ending.field_0xdec; // native countdown once per waiting tick
            ++ticks;
        }
        assert(ticks==(duration+7)/8);
    }
    before_ending_blow_wait(nullptr,&ending,nullptr,nullptr);
    assert(ending.field_0xdec==0); // no unsigned/signed underflow at expiry
    for (int mode : {0,1,3}) {
        ending.mMode=mode; ending.field_0xdec=30;
        before_ending_blow_wait(nullptr,&ending,nullptr,nullptr);
        assert(ending.field_0xdec==30);
    }
    ending.mMode=2; ending.down=false;
    before_ending_blow_wait(nullptr,&ending,nullptr,nullptr);
    assert(ending.field_0xdec==30);
    ending.down=true; ending.mMotionSeqMngr.step=0;
    before_ending_blow_wait(nullptr,&ending,nullptr,nullptr);
    assert(ending.field_0xdec==30); // landing has not finished
    ending.mMotionSeqMngr.step=1; ending.mMotionSeqMngr.no=20;
    before_ending_blow_wait(nullptr,&ending,nullptr,nullptr);
    assert(ending.field_0xdec==30); // successful finishing-hit animation
    ending.mMotionSeqMngr.no=19; ending.entry.divide=1;
    before_ending_blow_wait(nullptr,&ending,nullptr,nullptr);
    assert(ending.field_0xdec==30);
    ending.entry.divide=0; ending.owned=false;
    before_ending_blow_wait(nullptr,&ending,nullptr,nullptr);
    assert(ending.field_0xdec==30); // native story lesson unaffected

    // The existing native success event must charge damage once and play a
    // short flinch once; repeated events during recovery cannot restart it.
    daNpc_Kn_c reflected;
    sBattle={}; sBattle.phase=2;
    reflected.mEvtNo=11; reflected.field_0x15bc=1;
    assert(no_order(nullptr,&reflected,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(sBattle.health==9 && reflected.health==9 && sBattle.recovery==45);
    assert(reflected.mMotionSeqMngr.no==29 && reflected.field_0x15bc==0);
    assert(reflected.mEvtNo==0);
    reflected.mMotionSeqMngr.no=0; reflected.mEvtNo=11;
    no_order(nullptr,&reflected,nullptr,nullptr);
    assert(sBattle.health==9 && reflected.mMotionSeqMngr.no==0);
    sBattle={}; reflected.mEvtNo=7;
    no_order(nullptr,&reflected,nullptr,nullptr);
    assert(sBattle.health==9 && reflected.mMotionSeqMngr.no==0); // other counters unchanged
    sBattle={}; sBattle.phase=2;
    reflected.entry.divide=1; reflected.mEvtNo=11;
    no_order(nullptr,&reflected,nullptr,nullptr);
    assert(sBattle.health==10 && reflected.mMotionSeqMngr.no==0);
    reflected.entry.divide=0; reflected.owned=false; reflected.mEvtNo=11;
    assert(no_order(nullptr,&reflected,nullptr,nullptr)==HOOK_CONTINUE);
    assert(sBattle.health==10 && reflected.mEvtNo==11);

    // Both ordinary spin directions and both Great Spins defeat each clone
    // independently, without requiring a hit on the boss or draining its HP.
    for(int cut : {8,22,23,24}) {
        sBattle={};sBattle.phase=7;player.cut=cut;
        daNpc_Kn_c boss,one,two;
        one.entry.divide=1;two.entry.divide=2;
        one.entry.forming=true;
        one.mCylCc.attacker=two.mCylCc.attacker=&player;
        one.current.angle.y=0;two.current.angle.y=static_cast<s16>(0x8000);
        playerYaw=0;
        assert(skill_counter_action(&one,one.entry));
        assert(sBattle.health==10 && sBattle.recovery==0 && one.mEvtNo==0);
        assert(one.mMotionSeqMngr.no==18 && one.speedF<0 && one.speed.y>0);
        assert(!one.mCylCc.target && one.entry.offense==-1 && !one.entry.forming);
        assert(defeated_double(one.entry) && !defeated_double(two.entry));
        // First clone disappears only after landing, while the second remains.
        assert(skill_counter_action(&one,one.entry) && one.falls==1 && one.warps==0);
        one.mAcch.grounded=true;one.speed.y=-2;
        after_knockdown_movement(nullptr,&one,nullptr,nullptr);
        assert(one.mMotionSeqMngr.no==19);
        skill_counter_action(&one,one.entry);assert(one.warps==0);
        one.mMotionSeqMngr.step=1;skill_counter_action(&one,one.entry);
        assert(one.warps==1 && one.mActionMode==23);
        // Run the production spawn routine after native deletion clears slots.
        sFighters={};spawns=0;add_doubles(&boss);
        assert(spawns==1 && sFighters[1].id==42 && sFighters[2].id>=1000);
        for(int i=0;i<120;++i)add_doubles(&boss);
        assert(spawns==1);
        assert(skill_counter_action(&two,two.entry));
        assert(two.mMotionSeqMngr.no==14 && two.speedF>0);
        assert(sBattle.health==10 && sBattle.defeated_doubles==6);
        sFighters={};for(int i=0;i<120;++i)add_doubles(&boss);
        assert(spawns==1); // neither defeated slot is replenished
        // Normal spin also produces the boss's usual success event, once.
        boss.mCylCc.attacker=&player;
        assert(skill_counter_action(&boss,boss.entry) && boss.mEvtNo==24);
        no_order(nullptr,&boss,nullptr,nullptr);assert(sBattle.health==9);
        assert(!skill_counter_action(&boss,boss.entry));
        // Recovery from a simultaneous boss hit must not freeze clone landing.
        int falls=two.falls;assert(skill_counter_action(&two,two.entry));assert(two.falls==falls+1);
        for(int i=0;i<45;++i)sBattle.tick([](float count){return count-1;});
        assert(sBattle.phase!=7 && sBattle.defeated_doubles==0);
    }
    // Jump Strike doubles must fall/land before departure, even if Link ends
    // the attack or the boss enters recovery. Neither may notify the teacher
    // to immediately warp out the group; defeated slots must remain empty.
    for(int cut : {19,20}) for(int side : {0,0x8000}) {
        sBattle={};sBattle.phase=6;player.cut=cut;
        sBattle.recovery=side ? 45 : 0; // boss may have executed first this frame
        daNpc_Kn_c boss,clone;
        clone.entry.divide=1;clone.current.angle.y=static_cast<s16>(side);playerYaw=0;
        clone.mCylCc.attacker=&player;clone.entry.forming=true;
        assert(skill_counter_action(&clone,clone.entry));
        assert(clone.mActionMode==16 && clone.speed.y>0 && !clone.entry.forming);
        assert(clone.mMotionSeqMngr.no==(side ? 14 : 18));
        assert(clone.field_0x15bd==0 && clone.mEvtNo==0 && sBattle.health==10);
        assert(sBattle.defeated_doubles==2 && !clone.mCylCc.target);
        player.cut=0; // The native tutorial used this change to remove the group.
        for(int i=0;i<4;++i)assert(skill_counter_action(&clone,clone.entry));
        assert(clone.jumpFalls==4 && clone.jumpWarps==0 && clone.warps==0);
        sBattle.recovery=45;
        assert(skill_counter_action(&clone,clone.entry) && clone.jumpFalls==5);
        clone.speed.y=-2;clone.mAcch.grounded=true;
        after_knockdown_movement(nullptr,&clone,nullptr,nullptr);
        assert(clone.mMotionSeqMngr.no==(side ? 15 : 19));
        skill_counter_action(&clone,clone.entry);assert(clone.jumpWarps==0);
        clone.mMotionSeqMngr.step=1;
        assert(skill_counter_action(&clone,clone.entry));
        assert(clone.mActionMode==17 && clone.jumpWarps==1 && clone.warps==0);
        sFighters={};spawns=0;
        for(int i=0;i<120;++i)add_doubles(&boss);
        assert(spawns==1 && sFighters[1].id==42); // only the surviving clone
        // Matching boss hit keeps the original Jump Strike success event.
        sBattle.recovery=0;player.cut=cut;boss.mCylCc.attacker=&player;
        assert(skill_counter_action(&boss,boss.entry) && boss.mEvtNo==21);
        no_order(nullptr,&boss,nullptr,nullptr);assert(sBattle.health==9);
    }
    // Keep native opening-swipe behavior; don't accept a normal jumping slash.
    sBattle={};sBattle.phase=6;
    daNpc_Kn_c opening;opening.entry.divide=1;opening.mCylCc.attacker=&player;
    for(int cut : {18,10,8,22,23,24}) {
        player.cut=cut;
        assert(!skill_counter_action(&opening,opening.entry));
        assert(!sBattle.defeated_doubles && opening.mActionMode==19);
    }

    // Normal spins open the owned phase's body collider; other attacks and
    // unowned story actors do not acquire a new vulnerability.
    sBattle={};sBattle.phase=7;player.cut=8;
    daNpc_Kn_c spinTarget;
    trial_body_collision(nullptr,&spinTarget,nullptr,nullptr);assert(!spinTarget.mCylCc.shield);
    spinTarget.mCylCc.shield=true;player.cut=1;
    trial_body_collision(nullptr,&spinTarget,nullptr,nullptr);assert(spinTarget.mCylCc.shield);
    assert(!skill_counter_action(&spinTarget,spinTarget.entry));
    player.cut=8;spinTarget.mCylCc.attacker=nullptr;
    assert(!skill_counter_action(&spinTarget,spinTarget.entry));
    spinTarget.mCylCc.attacker=&player;spinTarget.mCylCc.object.type=4;
    assert(!skill_counter_action(&spinTarget,spinTarget.entry));
    spinTarget.mCylCc.object.type=AT_TYPE_NORMAL_SWORD;sBattle.trial=shade::Trial::Wind;
    assert(!skill_counter_action(&spinTarget,spinTarget.entry));
    sBattle.trial=shade::Trial::None;sBattle.phase=6;
    assert(!skill_counter_action(&spinTarget,spinTarget.entry));
    sBattle.phase=7;spinTarget.owned=false;
    trial_body_collision(nullptr,&spinTarget,nullptr,nullptr);assert(spinTarget.mCylCc.shield);
    // Defeated clones retain neither body targets nor offensive sword sweeps.
    spinTarget.owned=true;spinTarget.entry.divide=1;sBattle.defeated_doubles=2;
    trial_body_collision(nullptr,&spinTarget,nullptr,nullptr);assert(!spinTarget.mCylCc.target);
    assert(sword_collision(nullptr,&spinTarget,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);

    // Final skill counters go straight to the cinematic, even while the old
    // lesson group warp is hidden/shrunk and the 45-tick recovery is pending.
    for(unsigned phase : {6u,7u}) for(int cut : {8,22,23,24,19,20}) for(int side : {0,0x8000}) {
        if((phase==6)!=(cut==19 || cut==20)) continue;
        sBattle={};sCinema={};sFighters={};accepted=paused=false;
        sBattle.phase=phase;sBattle.health=1;sBattle.trials_started=4;
        daNpc_Kn_c boss;playerYaw=side;player.cut=cut;
        boss.current.angle.y=0;boss.mCylCc.attacker=&player;
        assert(skill_counter_action(&boss,boss.entry));
        no_order(nullptr,&boss,nullptr,nullptr);
        assert(sBattle.health==0 && sBattle.recovery==45);
        const auto motion=boss.mMotionSeqMngr.no;
        const auto verticalSpeed=boss.speed.y;
        boss.mNoDraw=true;boss.field_0x16f4.set(0,0,0);
        boss.field_0x170c=1;boss.field_0x170d=14;boss.field_0x15bd=2;
        boss.mActionMode=phase==6 ? 17 : 23;
        companionRemovals=musicStops=cinemaTicks=0;
        int result=0;
        paused=true;before_execute(nullptr,&boss,&result,nullptr);
        assert(!sCinema.active() && sBattle.recovery==45);
        paused=false;before_execute(nullptr,&boss,&result,nullptr);
        assert(sBattle.dying && !sBattle.recovery && !sBattle.advance);
        assert(sCinema.victory && sCinema.shot==shade::Shot::Request && cinemaTicks==1);
        assert(companionRemovals==1 && musicStops==1);
        assert(sFighters[1].deleting && sFighters[2].deleting && !boss.entry.deleting);
        assert(!boss.mNoDraw && boss.field_0x16f4.x==1 && boss.field_0x16f4.y==1 && boss.field_0x16f4.z==1);
        assert(!boss.field_0x170c && !boss.field_0x170d && !boss.field_0x15bd);
        assert(boss.mActionMode==-1 && !boss.field_0x15af);
        assert(boss.mMotionSeqMngr.no==motion && boss.speed.y==verticalSpeed);
        accepted=true;before_execute(nullptr,&boss,&result,nullptr);
        assert(sCinema.shot==shade::Shot::Recover);
        for(int i=0;i<10;++i) before_execute(nullptr,&boss,&result,nullptr);
        assert(companionRemovals==1 && musicStops==1 && cinemaTicks==12);
        assert(!boss.entry.deleting && !boss.warps && !boss.jumpWarps);
    }
    sBattle={};sCinema={};accepted=paused=false;
    // Nonfatal hits retain normal recovery; story actors remain untouched.
    daNpc_Kn_c survivor;sBattle.health=1;sBattle.recovery=45;
    int executeResult=0;before_execute(nullptr,&survivor,&executeResult,nullptr);
    assert(!sCinema.active() && !sBattle.dying && sBattle.recovery==44);
    a.owned=false;
    assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_CONTINUE);
}
'''

start = encounter.index("void stop_blade_sweeps(")
end = encounter.index("\n}\n",start)+3
source = fixture + function("retire_double", "void") + function("waiting_action", "int") + function("cinema_landing", "bool") + encounter[start:end] + (root / "src/heroes_shade_spin.inc").read_text() + function("add_doubles", "void") + function("accessory_motion") + function("sword_collision") + function("trial_body_collision", "void") + function("after_jump_pose", "void") + function("finish_helm_splitter", "void") + function("after_approach", "void") + function("hold_recovery", "void") + function("before_movement", "void") + function("after_knockdown_movement", "void") + function("before_ending_blow_wait") + function("no_order") + function("after_bullet", "void") + function("begin_victory", "void") + function("before_execute") + checks
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "native_hooks.cpp"
    exe = Path(tmp) / "native_hooks"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
action = function("combat_action")
assert action.index("if (!entry)") < action.index("skill_counter_action(actor,*entry)")
assert action.index("skill_counter_action(actor,*entry)") < action.index("if (sBattle.recovery)")
assert action.index("return_double(actor,*entry)") < action.index("if (sCinema.active()")
print("Hero's Shade native hooks, Jump Strike/Spin landing and walking formation and persistent double defeat: passed")
