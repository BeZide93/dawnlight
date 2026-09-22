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
#include <array>
#include <cassert>
#include <cstring>
#include <cstdint>
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
    void zero() { x=y=z=0; }
    cXyz operator-(const cXyz& b) const { return {x-b.x,y-b.y,z-b.z}; }
};
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
struct Cylinder {
    bool target=true,hit=true;
    void OffTgSetBit(){target=false;}void ClrTgHit(){hit=false;}
    cXyz center;
    const cXyz& GetC() const { return center; }
    float GetR() const { return 30; }
    float GetH() const { return 150; }
};
struct Player { std::array<Cylinder,3> mTgCyls; } player;
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
    bool reset=false;
    int offense=shade::helm_splitter;
    bool animationStarted=true, deleting=false, helmTurnPending=false;
    shade::BladeMotion blade;
    std::array<dCcD_Cps,2> bladeSweeps;
    cXyz jumpBodyOffset{5,0,10};
};
struct daNpc_Kn_c {
    bool owned=true, mNoDraw=false;
    int field_0x15af=1;
    int mEvtNo=0, health=10;
    bool mSpeakEvent=false;
    void* mpPodModel=nullptr;
    unsigned mPodAnmFlags=0x41;
    int field_0x15cd=1;
    Fighter entry;
    Motion mMotionSeqMngr, mFaceMotionSeqMngr;
    int mActionMode=19, field_0x15bc=0, landings=0;
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
std::array<Fighter,3> sFighters;
struct daObjKnBullet_c { int parentActorID=42; Sphere mCcSph; };
constexpr float kApproachSpeed=6;
float fopAcM_GetMaxFallSpeed(daNpc_Kn_c*) { return -40; }
s16 playerYaw=0;
s16 fopAcM_searchPlayerAngleY(daNpc_Kn_c*) { return playerYaw; }
struct Space {
    int registered=0, expectedDamage=8;
    void Set(Sphere* sphere) { assert(sphere->enabled && sphere->damage==expectedDamage); ++registered; }
} space;
Space* dComIfG_Ccsp() { return &space; }
'''

checks = r'''
int main() {
    daNpc_Kn_c a;
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
    // Every intermission excludes native success events; the ward keeps offensive blades.
    a.entry.offense=-1;
    for (auto trial:{shade::Trial::Shield,shade::Trial::Fire,shade::Trial::Eyes,shade::Trial::Wind}) {
        sBattle.trial=trial; a.mEvtNo=shade::phases[0].success;
        no_order(nullptr,&a,nullptr,nullptr);
        assert(sBattle.health==10 && a.mEvtNo==0);
        for (auto& sphere:a.mSphCc) { sphere.enabled=true; sphere.hit=true; }
        a.mCylCc.target=true;a.mCylCc.hit=true;
        shield_body_collision(nullptr,&a,nullptr,nullptr);
        assert(a.mCylCc.target==(trial!=shade::Trial::Shield));
        assert(a.mCylCc.hit==(trial!=shade::Trial::Shield));
        a.field_0x15af=1;
        const auto action=sword_collision(nullptr,&a,nullptr,nullptr);
        assert(action==(trial==shade::Trial::Shield ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL));
        for (const auto& sphere:a.mSphCc) assert(sphere.enabled==(trial==shade::Trial::Shield));
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
    assert(!a.entry.helmTurnPending && a.speedF==kApproachSpeed);
    // Link already in front: movement resumes immediately, including yaw wrap.
    a.entry.helmTurnPending=true;
    a.current.angle.y=32760; playerYaw=-32760; a.speedF=3;
    after_approach(nullptr,&a,nullptr,nullptr);
    assert(!a.entry.helmTurnPending && a.speedF==kApproachSpeed);
    // Only the post-Helm turn is constrained, never an ordinary native approach.
    a.current.angle.y=0; playerYaw=static_cast<s16>(0x8000); a.speedF=3;
    after_approach(nullptr,&a,nullptr,nullptr);
    assert(a.speedF==kApproachSpeed);
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

    a.owned=false;
    assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_CONTINUE);
}
'''

start = encounter.index("void stop_blade_sweeps(")
end = encounter.index("\n}\n",start)+3
source = fixture + function("cinema_landing", "bool") + encounter[start:end] + function("accessory_motion") + function("sword_collision") + function("shield_body_collision", "void") + function("after_jump_pose", "void") + function("finish_helm_splitter", "void") + function("after_approach", "void") + function("hold_recovery", "void") + function("before_movement", "void") + function("after_knockdown_movement", "void") + function("before_ending_blow_wait") + function("no_order") + function("after_bullet", "void") + checks
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "native_hooks.cpp"
    exe = Path(tmp) / "native_hooks"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade missing-sheath guard and native blade-collider hooks: passed")
