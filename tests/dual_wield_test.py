"""Exercise real Dual Wield code with a small native-API fixture; no game files."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/dual_wield.cpp").read_text()


def function(name, result="void"):
    start = source.index(f"{result} {name}(")
    return source[start:source.index("\n}\n", start) + 3]


animation_api = r'''
#pragma once
#include <cstdint>
using u16=std::uint16_t;using s16=std::int16_t;
struct J3DTransformInfo {
    struct {float x=1,y=1,z=1;} mScale;
    struct {s16 x=0,y=0,z=0;} mRotation;
    struct {float x=0,y=0,z=0;} mTranslate;
};
struct J3DAnmTransform {
    int field_0x1e=35,kind=8;float frame=0;
    virtual ~J3DAnmTransform()=default;
    int getKind() const{return kind;}
    float getFrame() const{return frame;}
    virtual void getTransform(u16,J3DTransformInfo*) const=0;
};
struct J3DAnmTransformKey : J3DAnmTransform {
    void calcTransform(float t,u16 joint,J3DTransformInfo* out) const {
        *out={};out->mTranslate={float(joint),t,float(100+joint)};
        out->mRotation={s16(joint*30),s16(joint*20),s16(joint*10)};
    }
    void getTransform(u16 joint,J3DTransformInfo* out) const override {calcTransform(frame,joint,out);}
};
struct mDoExt_AnmRatioPack {
    J3DAnmTransform* value=nullptr;
    auto getAnmTransform(){return value;}
    void setAnmTransform(J3DAnmTransform* p){value=p;}
};
'''

fixture = r'''
#include "dual_wield_animation.hpp"
#include "dual_wield_math.hpp"
#include <array>
#include <cassert>
#include <optional>
using namespace dawnlight;
using dual::Pose;using dual::Vec;
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
constexpr int dItemNo_NONE_e=0xff;
struct Args {void* link;void* model=nullptr;};
namespace mods {template<class T>T arg(void* p,int i){auto* a=static_cast<Args*>(p);return static_cast<T>(i?a->model:a->link);}}
struct J3DModel {};
struct cXyz {float x=0,y=0,z=0;void set(float a,float b,float c){x=a;y=b;z=c;}};
struct Morph {int calls=0;void initOldFrameMorf(int,int,int){++calls;}};
struct daPy_py_c {enum {CUT_TYPE_FINISH_LEFT,CUT_TYPE_FINISH_RIGHT,CUT_TYPE_FINISH_VERTICAL,CUT_TYPE_FINISH_STAB};};
struct daAlink_c {
    enum {PROC_CUT_NORMAL=1,PROC_CUT_FINISH=2,PROC_WAIT=3};
    int mProcID=PROC_WAIT,cut=0,mEquipItem=0x103;
    bool event=false,guard=false,human=true;
    Morph morph;Morph* field_0x2060=&morph;
    J3DModel model;J3DModel* mpLinkModel=&model;
    std::array<mDoExt_AnmRatioPack,3> mNowAnmPackUnder,mNowAnmPackUpper;
    cXyz field_0x3498,mSwordTopPos,field_0x3720,field_0x34a4,field_0x34b0,field_0x34bc;
    int getCutType(){return cut;}
    bool checkEventRun(){return event;}
    bool checkPlayerGuardAndAttack(){return guard;}
};
struct State {
    daAlink_c* owner=nullptr;
    bool enabled=false,active=false,mirror=false,seedBlade=false;
    float guard=0,draw=0;unsigned tick=0,poseTick=~0u;
    dual::Alternation attacks;
} s;
struct Borrow {mDoExt_AnmRatioPack* pack=nullptr;J3DAnmTransform* original=nullptr;std::optional<DualWieldAnimation> wrapper;};
std::array<Borrow,6> s_borrow;bool s_calculating=false,setting=true;
bool dual_wield_enabled(){return setting;}
bool human(daAlink_c* l){return l->human;}
bool active(daAlink_c* l){return s.owner==l && s.active;}
Pose sword_at_hand(daAlink_c*,bool right){assert(right);return {{},{20,80,40}};}
'''

fixture += "\n".join(function(name, result) for name, result in [
    ("detach", "void"), ("ordinary", "bool"), ("after_matrix", "void"),
    ("after_cut", "void"), ("before_model_calc", "HookAction"),
    ("after_model_calc", "void"), ("after_sword_pos", "void"),
    ("after_shield_draw", "void"),
])
fixture += r'''
int main() {
    J3DAnmTransformKey original;original.frame=12.5f;
    DualWieldAnimation mirrored(original);
    for(int joint=0;joint<35;++joint) {
        const bool left=joint>=6 && joint<=9,right=joint>=11 && joint<=14;
        const bool reflect=left || right || (joint>=1 && joint<=4);
        J3DTransformInfo a,b;mirrored.getTransform(joint,&a);
        original.getTransform(left?joint+5:right?joint-5:joint,&b);
        assert(a.mTranslate.x==b.mTranslate.x && a.mTranslate.y==12.5f);
        assert(a.mTranslate.z==(reflect?-b.mTranslate.z:b.mTranslate.z));
        assert(a.mRotation.x==(reflect?-b.mRotation.x:b.mRotation.x));
        assert(a.mRotation.y==(reflect?-b.mRotation.y:b.mRotation.y));
        assert(a.mRotation.z==b.mRotation.z);
        assert(a.mScale.x==1);
    }
    assert(original.frame==12.5f); // no shared BCK/frame writes

    daAlink_c link,other;s.owner=&link;Args args{&link,link.mpLinkModel};
    after_matrix(nullptr,&args,nullptr,nullptr);assert(s.active && !s.mirror);
    const float draw=s.draw;
    after_matrix(nullptr,&args,nullptr,nullptr);assert(s.draw==draw); // one advance per tick
    link.mProcID=daAlink_c::PROC_CUT_NORMAL;int success=1,failed=0;
    after_cut(nullptr,&args,&success,nullptr);assert(!s.attacks.right);
    after_cut(nullptr,&args,&failed,nullptr);assert(!s.attacks.right);
    after_cut(nullptr,&args,&success,nullptr);assert(s.attacks.right);
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);assert(s.mirror);
    for(auto& p:link.mNowAnmPackUnder)p.value=&original;
    for(auto& p:link.mNowAnmPackUpper)p.value=&original;
    before_model_calc(nullptr,&args,nullptr,nullptr);assert(s_calculating);
    for(auto& p:link.mNowAnmPackUnder)assert(p.value!=&original);
    // Calculating an accessory must not detach body animation packs prematurely.
    J3DModel accessory;Args itemArgs{&link,&accessory};
    after_model_calc(nullptr,&itemArgs,nullptr,nullptr);assert(s_calculating);
    after_model_calc(nullptr,&args,nullptr,nullptr);assert(!s_calculating);
    for(auto& p:link.mNowAnmPackUnder)assert(p.value==&original);
    for(auto& p:link.mNowAnmPackUpper)assert(p.value==&original);

    // Right contact uses the 100-unit Ordon blade. First sample must not sweep
    // across from the preceding left-hand strike; later samples stay untouched.
    s.seedBlade=true;after_sword_pos(nullptr,&args,nullptr,nullptr);
    assert(link.mSwordTopPos.x==120 && link.mSwordTopPos.y==80);
    assert(link.field_0x34b0.x==120 && link.field_0x34bc.x==20 && !s.seedBlade);
    link.field_0x34b0.x=99;after_sword_pos(nullptr,&args,nullptr,nullptr);
    assert(link.field_0x34b0.x==99);
    bool shield=true;after_shield_draw(nullptr,&args,&shield,nullptr);assert(!shield);
    Args otherArgs{&other};shield=true;after_shield_draw(nullptr,&otherArgs,&shield,nullptr);assert(shield);

    // Unsupported mixed animation rigs must not use right-hand hit positions.
    original.field_0x1e=30;before_model_calc(nullptr,&args,nullptr,nullptr);
    assert(!s_calculating && !s.mirror);original.field_0x1e=35;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);assert(s.mirror);
    link.event=true;++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);assert(!s.active && !s.mirror);
    link.event=false;setting=false;++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(!s.enabled && !s.active && !s.attacks.right);
    shield=true;after_shield_draw(nullptr,&args,&shield,nullptr);assert(shield);
    setting=true;++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    after_cut(nullptr,&args,&success,nullptr);assert(!s.attacks.right);
    // Event/wolf boundaries never modify another rig.
    link.human=false;++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);assert(!s.active);
}
'''

with tempfile.TemporaryDirectory() as folder:
    temp = Path(folder)
    for name, content in [
        ("JSystem/J3DGraphAnimator/J3DAnimation.h", animation_api),
        ("JSystem/J3DGraphBase/J3DTransform.h", "#pragma once\n"),
    ]:
        path = temp / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    (temp / "native.cpp").write_text(fixture)
    for name, path in [("math", root / "tests/dual_wield_math_test.cpp"), ("native", temp / "native.cpp")]:
        executable = temp / name
        subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-I", str(temp),
                        "-I", str(root / "src"), str(path), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
print("Dual Wield math, animation borrowing, hand contacts and lifecycle: OK")
