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
    cXyz center;
    const cXyz& GetC() const { return center; }
    float GetR() const { return 30; }
    float GetH() const { return 150; }
};
struct daPy_py_c {
    enum { CUT_TYPE_LARGE_JUMP_INIT=11, CUT_TYPE_LARGE_JUMP=12, CUT_TYPE_LARGE_JUMP_FINISH=13 };
};
struct Player : daPy_py_c {
    std::array<Cylinder,3> mTgCyls;
    int cut=0;
    int getCutType() const { return cut; }
} player;
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
constexpr unsigned AT_TYPE_NORMAL_SWORD=1, AT_TYPE_MASTER_SWORD=2;
struct AttackObject {
    unsigned type=AT_TYPE_MASTER_SWORD;
    bool ChkAtType(unsigned mask) const { return (type&mask)!=0; }
};
struct HurtCylinder {
    bool hit=false, targetEnabled=true, coEnabled=true;
    void* source=&player;
    AttackObject attack;
    AttackObject* object=&attack;
    bool ChkTgHit() const { return hit; }
    void* GetTgHitAc() const { return source; }
    const AttackObject* GetTgHitObj() const { return object; }
    void OffTgSetBit() { targetEnabled=false; }
    void OffCoSetBit() { coEnabled=false; }
};
struct Fighter {
    int id=42, divide=0;
    int offense=shade::helm_splitter;
    bool animationStarted=true, deleting=false, helmTurnPending=false;
    shade::BladeMotion blade;
    std::array<dCcD_Cps,2> bladeSweeps;
    cXyz jumpBodyOffset{5,0,10};
};
struct daNpc_Kn_c {
    bool owned=true, mNoDraw=false;
    int field_0x15af=1;
    HurtCylinder mCylCc;
    void* mpPodModel=nullptr;
    unsigned mPodAnmFlags=0x41;
    int field_0x15cd=1;
    Fighter entry;
    Motion mMotionSeqMngr;
    Morf morf, ghost;
    std::array<Morf*,2> mpModelMorf{&morf,&ghost};
    struct { cXyz pos{100,0,300}; struct { s16 y=1234; } angle; } current;
    void setAngle(s16 y) { current.angle.y=y; }
    struct { cXyz position; } attention_info;
    cXyz eyePos;
    cXyz speed;
    float speedF=0;
    int getBackboneJointNo() { return 1; }
    std::array<Sphere,2> mSphCc;
};
Fighter* fighter(daNpc_Kn_c* a) { return a->owned ? &a->entry : nullptr; }
shade::Battle sBattle;
constexpr float kApproachSpeed=6;
s16 playerYaw=0;
s16 fopAcM_searchPlayerAngleY(daNpc_Kn_c*) { return playerYaw; }
int removed=0;
void remove_actor(int id) { assert(id==42); ++removed; }
struct Space {
    int registered=0;
    void Set(Sphere* sphere) { assert(sphere->enabled && sphere->damage==2); ++registered; }
} space;
Space* dComIfG_Ccsp() { return &space; }
'''

checks = r'''
int main() {
    daNpc_Kn_c a;
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

    // A real Jump Strike contact removes either double, never the main Shade,
    // a whiff, a projectile or an unrelated actor hitting during Link's jump.
    daNpc_Kn_c clone;
    player.cut=daPy_py_c::CUT_TYPE_LARGE_JUMP;
    clone.mCylCc.hit=true;
    assert(!defeat_double_on_jump_strike(&clone,clone.entry));
    clone.entry.divide=1; clone.mCylCc.hit=false;
    assert(!defeat_double_on_jump_strike(&clone,clone.entry));
    clone.mCylCc.hit=true; player.cut=0;
    assert(!defeat_double_on_jump_strike(&clone,clone.entry));
    player.cut=daPy_py_c::CUT_TYPE_LARGE_JUMP;
    clone.mCylCc.source=&a;
    assert(!defeat_double_on_jump_strike(&clone,clone.entry));
    clone.mCylCc.source=&player; clone.mCylCc.attack.type=4;
    assert(!defeat_double_on_jump_strike(&clone,clone.entry));
    clone.mCylCc.attack.type=AT_TYPE_NORMAL_SWORD; clone.mCylCc.object=nullptr;
    assert(!defeat_double_on_jump_strike(&clone,clone.entry));
    clone.mCylCc.object=&clone.mCylCc.attack;
    for (int cut : {daPy_py_c::CUT_TYPE_LARGE_JUMP_INIT,
                    daPy_py_c::CUT_TYPE_LARGE_JUMP,daPy_py_c::CUT_TYPE_LARGE_JUMP_FINISH}) {
        for (int divide : {1,2}) {
            sBattle={}; sBattle.phase=divide==1 ? 6 : 7;
            clone.entry.deleting=false; clone.entry.divide=divide;
            clone.mNoDraw=false; clone.field_0x15af=1;
            for (auto& sphere:clone.mSphCc) sphere.OnAtSetBit();
            for (auto& sweep:clone.entry.bladeSweeps) sweep.OnAtSetBit();
            player.cut=cut;
            assert(defeat_double_on_jump_strike(&clone,clone.entry));
            assert(sBattle.defeated_doubles==(1u<<divide));
            assert(sBattle.health==8 && sBattle.recovery==0); // no boss damage/event
            assert(clone.mNoDraw && clone.field_0x15af==0 && clone.entry.deleting);
            assert(!clone.mCylCc.targetEnabled && !clone.mCylCc.coEnabled);
            for (auto& sphere:clone.mSphCc) assert(!sphere.enabled);
            for (auto& sweep:clone.entry.bladeSweeps) assert(!sweep.enabled);
            assert(!defeat_double_on_jump_strike(&clone,clone.entry));
            clone.entry={}; // native Delete clears the slot; suppression survives
            assert(sBattle.defeated_doubles==(1u<<divide));
        }
    }
    assert(removed==6);

    a.owned=false;
    assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_CONTINUE);
}
'''

start = encounter.index("void stop_blade_sweeps(")
end = encounter.index("\n}\n",start)+3
source = fixture + encounter[start:end] + function("accessory_motion") + function("sword_collision") + function("after_jump_pose", "void") + function("finish_helm_splitter", "void") + function("defeat_double_on_jump_strike", "bool") + function("after_approach", "void") + checks
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "native_hooks.cpp"
    exe = Path(tmp) / "native_hooks"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade missing-sheath guard and native blade-collider hooks: passed")
