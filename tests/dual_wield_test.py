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
        if(joint==0) out->mRotation={s16(t*80),s16(t*100),s16(t*180)};
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
struct J3DModel {dual::Pose base;std::array<dual::Pose,35> joints;auto getBaseTRMtx(){return base;}auto getAnmMtx(int i){return joints[i];}};
struct cXyz {float x=0,y=0,z=0;void set(float a,float b,float c){x=a;y=b;z=c;}};
struct Morph {
    int calls=0;bool valid=true;
    std::array<J3DTransformInfo,35> transforms;
    std::array<dual::Quat,35> quaternions;
    void initOldFrameMorf(int,int,int){++calls;}
    bool getOldFrameFlg(){return valid;}
    auto getOldFrameTransInfo(int i){return &transforms[i];}
    auto getOldFrameQuaternion(int i){return &quaternions[i];}
};
struct daPy_py_c {enum {CUT_TYPE_FINISH_LEFT,CUT_TYPE_FINISH_RIGHT,CUT_TYPE_FINISH_VERTICAL,CUT_TYPE_FINISH_STAB};};
struct daAlink_c {
    enum {PROC_CUT_NORMAL=1,PROC_CUT_FINISH=2,PROC_WAIT=3,PROC_GUARD_ATTACK=4,PROC_SWORD_UNEQUIP_SP=5};
    int mProcID=PROC_WAIT,cut=0,mEquipItem=0x103;
    bool event=false,guard=false,human=true,equipping=false;
    int field_0x3198=0;
    Morph morph;Morph* field_0x2060=&morph;
    J3DModel model;J3DModel* mpLinkModel=&model;
    std::array<mDoExt_AnmRatioPack,3> mNowAnmPackUnder,mNowAnmPackUpper;
    struct Frame {float frame=10,rate=1;float getFrame() const{return frame;}float getRate() const{return rate;}float getStart() const{return 0;}float getEnd() const{return 22;}};
    std::array<Frame,3> mUnderFrameCtrl,mUpperFrameCtrl;
    float field_0x3478=6,field_0x347c=14;
    cXyz field_0x3498,mSwordTopPos,field_0x3720,field_0x34a4,field_0x34b0,field_0x34bc;
    int getCutType(){return cut;}
    bool checkEventRun(){return event;}
    bool checkPlayerGuardAndAttack(){return guard;}
    bool checkSwordEquipAnime(){return equipping;}
};
struct State {
    daAlink_c* owner=nullptr;
    bool enabled=false,active=false,mirror=false,seedBlade=false;
    float guard=0,draw=0;unsigned tick=0,poseTick=~0u;
    dual::Alternation attacks;
    DualGuardBodyPose guardBody;bool haveGuardBody=false;
    dual::StowMotion stow;Pose rightSword;
} s;
struct Borrow {mDoExt_AnmRatioPack* pack=nullptr;J3DAnmTransform* original=nullptr;std::optional<DualWieldAnimation> wrapper;};
std::array<Borrow,6> s_borrow;bool s_calculating=false,setting=true;
bool dual_wield_enabled(){return setting;}
bool human(daAlink_c* l){return l->human;}
bool active(daAlink_c* l){return s.owner==l && s.active;}
Pose pose(Pose p){return p;}
void put(J3DModel* model,int joint,Pose p){model->joints[joint]=p;}
Pose sword_at_hand(daAlink_c*,bool right){assert(right);return {{},{20,80,40}};}
'''

fixture += "\n".join(function(name, result) for name, result in [
    ("hand_mount", "Pose"), ("solve_arm", "void"), ("detach", "void"), ("ordinary", "bool"), ("start_stow", "void"), ("start_draw", "void"), ("after_equip", "void"), ("update_stow", "void"), ("after_matrix", "void"),
    ("after_cut", "void"), ("before_guard_attack", "HookAction"),
    ("guard_thrust", "float"), ("before_model_calc", "HookAction"),
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

    // Root/pelvis/legs must remain one native motion. Chest orientation stays
    // stable despite the root rotation; only a 3-unit forward translation is added.
    link.human=true;link.mProcID=daAlink_c::PROC_WAIT;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    link.morph.transforms[0].mTranslate.y=102;
    link.morph.quaternions[0]=dual::between({1,0,0},{0,1,0});
    before_guard_attack(nullptr,&args,nullptr,nullptr);assert(s.haveGuardBody);
    assert(std::abs(s.guardBody[0].mRotation.z-16384)<=1);
    link.mProcID=daAlink_c::PROC_GUARD_ATTACK;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    for(float frame:{2.0f,10.0f,18.0f}) {
        link.mUnderFrameCtrl[0].frame=frame;original.frame=frame;
        before_model_calc(nullptr,&args,nullptr,nullptr);assert(s_calculating);
        auto* anm=link.mNowAnmPackUnder[0].value;
        J3DTransformInfo root,spine,leg;
        anm->getTransform(0,&root);anm->getTransform(1,&spine);anm->getTransform(17,&leg);
        for(int joint=0;joint<=26;++joint) {
            if(joint>0 && joint<16)continue;
            J3DTransformInfo actual,native;
            anm->getTransform(joint,&actual);original.getTransform(joint,&native);
            assert(actual.mTranslate.x==native.mTranslate.x && actual.mTranslate.y==native.mTranslate.y && actual.mTranslate.z==native.mTranslate.z);
            assert(actual.mRotation.x==native.mRotation.x && actual.mRotation.y==native.mRotation.y && actual.mRotation.z==native.mRotation.z);
        }
        auto rotation=[](const J3DTransformInfo& t) {
            constexpr float radians=3.14159265358979323846f/32768;
            return dual::from_euler({t.mRotation.x*radians,t.mRotation.y*radians,t.mRotation.z*radians});
        };
        const auto world=dual::multiply(rotation(root),rotation(spine));
        assert(dual::length(dual::rotate(world,{1,0,0})-Vec{0,1,0})<.001f);
        assert(dual::length(dual::rotate(world,{0,0,1})-Vec{0,0,1})<.001f);
        const auto shift=dual::rotate(rotation(root),{spine.mTranslate.x,spine.mTranslate.y,spine.mTranslate.z});
        assert(dual::length(shift-Vec{0,0,frame==10 ? 3.0f : 0.0f})<.001f);
        assert(leg.mTranslate.y==frame && leg.mRotation.x==17*30);
        after_model_calc(nullptr,&args,nullptr,nullptr);
        assert(link.mNowAnmPackUnder[0].value==&original);
    }
    link.mProcID=daAlink_c::PROC_WAIT;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);assert(!s.haveGuardBody);

    // A morph can have stale Euler angles and a different blended quaternion.
    // Both the hand solver and rendered primary item must use the latter.
    for(int sample=0;sample<=20;++sample) {
        const float t=sample/20.0f;
        auto& grip=link.morph.transforms[10];
        grip.mRotation={0,16384,0}; // deliberately differs from the rendered rotation
        grip.mTranslate={9.664279f,2.136274f,3.018636f};
        link.morph.quaternions[10]=dual::blend(dual::Quat{-.579250f,-.405549f,.405549f,.579250f},
                                             dual::Quat{0,0,0,1},t);
        const Pose mount=hand_mount(&link,false);
        assert(dual::length(dual::rotate(mount.q,{1,0,0})-
                           dual::rotate(link.morph.quaternions[10],{1,0,0}))<.001f);
        link.model.joints[7]={{},{18,130,-10}};
        link.model.joints[8]={{},{32,105,0}};
        link.model.joints[9]={{},{24,93,22}};
        link.model.joints[10]={{},{-30,100,0}}; // native item matrix that would fold the blade
        const Pose blade=dual::cross_guard_blade(false,t);
        solve_arm(&link,false,blade,1);
        const Pose rendered=link.model.joints[10];
        const Pose attached=dual::compose(link.model.joints[9],mount);
        assert(dual::length(rendered.p-attached.p)<.001f);
        for(Vec axis:{Vec{1,0,0},Vec{0,1,0},Vec{0,0,1}})
            assert(dual::length(dual::rotate(rendered.q,axis)-dual::rotate(blade.q,axis))<.001f);
    }

    // Begin while the sword is still equipped; keep the offhand override
    // through the native inventory switch and the late shield-back frames.
    link.equipping=true;link.mEquipItem=0x103;
    s.stow={};s.stow.active=true;
    link.mUpperFrameCtrl[2].frame=5;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(s.stow.active && s.stow.progress>0 && s.draw==1);
    link.mEquipItem=dItemNo_NONE_e;link.mUpperFrameCtrl[2].frame=11;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(s.stow.active && s.draw==1);
    link.mUpperFrameCtrl[2].frame=18;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(s.stow.active && s.draw==0);
    link.equipping=false;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(!s.stow.active && s.stow.release==1);
    for(int i=0;i<7;++i){++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);}
    assert(s.stow.release==0);
    // Draw starts before the inventory swap, using the reversed equip clock.
    s.draw=0;link.equipping=true;link.mEquipItem=dItemNo_NONE_e;
    link.mUpperFrameCtrl[2].rate=-1;link.mUpperFrameCtrl[2].frame=22;
    after_equip(nullptr,&args,nullptr,nullptr);
    assert(s.stow.active && s.stow.drawing && s.stow.duration==22);
    link.mUpperFrameCtrl[2].frame=18;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(s.draw==0); // empty hand approaches; sword remains in its sheath
    link.mUpperFrameCtrl[2].frame=14;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);assert(s.draw==1);
    link.mEquipItem=0x103;link.mUpperFrameCtrl[2].frame=5;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(s.stow.active && s.stow.drawing && s.draw==1);
    link.equipping=false;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(!s.stow.active && s.stow.release==1);
    // Raising/lowering guard while holstered uses both halves of the same
    // route even when there is no native equip animation controller.
    s.stow={};s.draw=0;link.mEquipItem=dItemNo_NONE_e;link.guard=true;
    for(int i=0;i<21;++i){++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);}
    assert(s.draw==1 && !s.stow.active);
    for(int i=0;i<6;++i){++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);}
    link.guard=false;++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(s.stow.active && !s.stow.drawing && !s.stow.nativeClock);
    for(int i=0;i<25;++i){++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);}
    assert(s.draw==0 && !s.stow.active);
    // An attack interruption immediately returns ownership to combat.
    s.stow.active=true;link.mProcID=daAlink_c::PROC_CUT_NORMAL;
    ++s.tick;after_matrix(nullptr,&args,nullptr,nullptr);
    assert(!s.stow.active && s.stow.release==0);
}
'''

loader_fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <array>
using u32=std::uint32_t;
struct ModelData {int getJointNum(){return 1;}int getMaterialNum(){return 2;}} data;
struct J3DModel {} model;
struct Heap {
    std::array<char,64> bytes{};bool fail=false;
    void* alloc(u32 size,int alignment){assert(size==64 && alignment==32);return fail?nullptr:bytes.data();}
} heap;
struct {Heap* heap;} s{&heap};
struct JKRArchive {
    std::map<std::string,std::array<char,64>> files;
    std::string directory;
    void* getResource(const char* path) {
        auto it=files.find(directory+path);
        return it==files.end()?nullptr:it->second.data();
    }
    void* getResource(u32 type,const char* name) {
        assert(type==0);
        for(auto& [path,bytes]:files) {
            if(path.substr(path.find_last_of('/')+1)==name)return bytes.data();
        }
        return nullptr;
    }
    u32 getExpandedResSize(void*){return 64;}
};
std::string warning;
struct Log {void warn(void*,const char* message){warning=message;}} logService;
Log* svc_log=&logService;void* mod_ctx=nullptr;
struct dRes_info_c {
    static ModelData* loaderBasicBmd(u32 tag,void* raw) {
        assert(tag==0x424D5752 && raw==heap.bytes.data());
        assert(std::memcmp(raw,"J3D2bmd3",8)==0);
        static_cast<char*>(raw)[8]=42; // Native loader modifies only the private copy.
        return &data;
    }
};
J3DModel* mDoExt_J3DModel__create(ModelData* d,u32 flags,u32 diff) {
    assert(d==&data && flags==0x80000 && diff==0x11000284);return &model;
}
'''
loader_fixture += function("model_failure") + function("copy_model", "J3DModel*")
loader_fixture += r'''
int main() {
    JKRArchive archive;
    for(const char* name:{"al_swa.bmd","al_poda.bmd"}) {
        auto& bytes=archive.files[std::string("bmwr/")+name];
        std::memcpy(bytes.data(),"J3D2bmd3",8);
    }
    for(const char* directory:{"","bck/","bmwr/"}) {
        archive.directory=directory;
        for(const char* name:{"al_swa.bmd","al_poda.bmd"}) {
            assert(copy_model(&archive,name)==&model);
            assert(archive.files.at(std::string("bmwr/")+name)[8]==0);
        }
    }
    assert(warning.empty());
    assert(!copy_model(&archive,"missing.bmd"));
    assert(warning.find("missing.bmd: resource not found")!=std::string::npos);
    heap.fail=true;assert(!copy_model(&archive,"al_swa.bmd"));
    assert(warning.find("allocation failed")!=std::string::npos);heap.fail=false;
    archive.files.at("bmwr/al_swa.bmd")[0]='X';
    assert(!copy_model(&archive,"al_swa.bmd"));
    assert(warning.find("invalid BMD header")!=std::string::npos);
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
    (temp / "loader.cpp").write_text(loader_fixture)
    for name, path in [("math", root / "tests/dual_wield_math_test.cpp"),
                       ("native", temp / "native.cpp"), ("loader", temp / "loader.cpp")]:
        executable = temp / name
        subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-I", str(temp),
                        "-I", str(root / "src"), str(path), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
print("Dual Wield math, animation borrowing, hand contacts, lifecycle and archive loading: OK")
