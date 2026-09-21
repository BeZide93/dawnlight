"""Run the actual accessory and sword hooks against a minimal native API fixture.

This reproduces the missing lesson-7 sheath and exercises collider registration,
not just a second copy of the hit-window formula. No game assets are required.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
encounter = (root / "src/heroes_shade_encounter.cpp").read_text()


def function(name):
    start = encounter.index(f"HookAction {name}(")
    end = encounter.index("\n}\n", start) + 3
    return encounter[start:end]


fixture = r'''
#include "heroes_shade_battle.hpp"
#include <array>
#include <cassert>
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
};
struct Matrix { cXyz translation; };
void MTXMultVec(const Matrix* m,const cXyz* in,cXyz* out) {
    *out={in->x+m->translation.x,in->y+m->translation.y,in->z+m->translation.z};
}
struct Model {
    Matrix blade;
    Matrix* getAnmMtx(int joint) { assert(joint==13); return &blade; }
};
struct Morf {
    Model model;
    float frame=0;
    Model* getModel() { return &model; }
    float getFrame() { return frame; }
};
struct Motion {
    int no=shade::helm_splitter, step=0;
    int getNo() { return no; }
    int getStepNo() { return step; }
};
int player;
void* daPy_getPlayerActorClass() { return &player; }
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
struct Fighter {
    int offense=shade::helm_splitter;
    bool animationStarted=true, deleting=false;
    shade::BladeMotion blade;
};
struct daNpc_Kn_c {
    bool owned=true;
    void* mpPodModel=nullptr;
    unsigned mPodAnmFlags=0x41;
    int field_0x15cd=1;
    Fighter entry;
    Motion mMotionSeqMngr;
    Morf morf;
    std::array<Morf*,1> mpModelMorf{&morf};
    std::array<Sphere,2> mSphCc;
};
Fighter* fighter(daNpc_Kn_c* a) { return a->owned ? &a->entry : nullptr; }
shade::Battle sBattle;
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
        a.morf.model.blade.translation={0,100,z};
        space.registered=0;
        assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        return space.registered;
    };
    assert(sample(0,0)==0);
    assert(sample(5,15)==2); // formerly outside the 40%-75% gate
    assert(a.mSphCc[0].center.x==60 && a.mSphCc[1].center.x==120);
    assert(a.mSphCc[0].center.y==100 && a.mSphCc[0].center.z==15);
    assert(a.mSphCc[0].radius==30);
    assert(sample(6,15)==0); // stationary blade does not cause late damage
    assert(!a.mSphCc[0].enabled && !a.mSphCc[1].enabled);
    assert(sample(55,30)==2); // actual late cut must not be discarded either
    a.mSphCc[0].shield=true; a.mSphCc[0].target=&player;
    assert(sample(56,45)==0);
    assert(sample(57,60)==0); // lowering the shield cannot revive this strike
    a.entry.blade={};
    assert(sample(0,0)==0);
    assert(sample(5,15)==2);
    a.mSphCc[1].hit=true; a.mSphCc[1].target=&player;
    assert(sample(6,30)==0); // one damaging contact per attack

    a.entry.blade={}; a.entry.offense=shade::back_slice;
    a.mMotionSeqMngr.no=shade::back_slice;
    for (int step=0;step<2;++step) {
        a.mMotionSeqMngr.step=step;
        assert(sample(0,0)==0);
        assert(sample(5,15)==0); // sidestep and roll never hurt
    }
    a.mMotionSeqMngr.step=2;
    assert(sample(0,0)==0);
    assert(sample(5,15)==2);
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

    a.owned=false;
    assert(sword_collision(nullptr,&a,nullptr,nullptr)==HOOK_CONTINUE);
}
'''

source = fixture + function("accessory_motion") + function("sword_collision") + checks
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "native_hooks.cpp"
    exe = Path(tmp) / "native_hooks"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade missing-sheath guard and native blade-collider hooks: passed")
