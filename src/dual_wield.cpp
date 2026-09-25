#include "dual_wield.hpp"
#include "dual_wield_math.hpp"
#include "dual_wield_animation.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_leaf.h"
#include "JSystem/J3DGraphLoader/J3DModelLoader.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "JSystem/J3DGraphBase/J3DShape.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "mods/svc/hook.hpp"
#include <array>
#include <memory>
#include <optional>
#include <cstring>
#include <cstdio>
#include <new>

namespace dawnlight {
namespace {
using dual::Pose;using dual::Quat;using dual::Vec;
DEFINE_HOOK(&daAlink_c::execute, DualExecute);
DEFINE_HOOK(&daAlink_c::setMatrix, DualMatrix);
DEFINE_HOOK(&daAlink_c::modelCalc, DualModelCalc);
DEFINE_HOOK(&daAlink_c::setArmMatrix, DualArms);
DEFINE_HOOK(&daAlink_c::setItemMatrix, DualItems);
DEFINE_HOOK(&daAlink_c::setSwordPos, DualSwordPos);
DEFINE_HOOK(&daAlink_c::checkShieldDraw, DualShieldDraw);
DEFINE_HOOK(&daAlink_c::modelDraw, DualModelDraw);
DEFINE_HOOK(&daAlink_c::procCutNormalInit, DualCut);
DEFINE_HOOK(&daAlink_c::procCutFinishInit, DualFinish);
DEFINE_HOOK(&daAlink_c::setCollision, DualCollision);
DEFINE_HOOK(&fpcLf_Delete, DualDelete);

using ForgetModel=void(*)(J3DModel*);
ForgetModel s_forget=nullptr;
daAlink_c* s_failedOwner=nullptr;
struct State {
    daAlink_c* owner=nullptr;
    JKRExpHeap* heap=nullptr;
    std::unique_ptr<u8[]> storage;
    J3DModel* sword=nullptr;
    J3DModel* sheath=nullptr;
    dual::Alternation attacks;
    bool enabled=false,active=false,mirror=false,seedBlade=false,forcedBlade=false;
    float guard=0,draw=0;
    unsigned tick=0,poseTick=~0u;
    Pose rightSword,hipSword;
} s;
struct Borrow {
    mDoExt_AnmRatioPack* pack=nullptr;
    J3DAnmTransform* original=nullptr;
    std::optional<DualWieldAnimation> wrapper;
};
std::array<Borrow,6> s_borrow;
bool s_calculating=false;

Quat rotation(const Mtx m) {
    float a[3][3];
    for(int c=0;c<3;++c) {
        float len=std::sqrt(m[0][c]*m[0][c]+m[1][c]*m[1][c]+m[2][c]*m[2][c]);
        for(int r=0;r<3;++r) a[r][c]=len>1e-6f ? m[r][c]/len : (r==c ? 1.0f : 0.0f);
    }
    Quat q;float trace=a[0][0]+a[1][1]+a[2][2];
    if(trace>0) {
        float k=std::sqrt(trace+1)*2;
        q={(a[2][1]-a[1][2])/k,(a[0][2]-a[2][0])/k,(a[1][0]-a[0][1])/k,k/4};
    } else {
        int i=a[1][1]>a[0][0] ? 1 : 0;if(a[2][2]>a[i][i]) i=2;
        int j=(i+1)%3,k=(i+2)%3;float n=std::sqrt(std::max(0.0f,1+a[i][i]-a[j][j]-a[k][k]))*2;
        if(n<1e-6f) return {};
        float v[3]{};v[i]=n/4;v[j]=(a[j][i]+a[i][j])/n;v[k]=(a[k][i]+a[i][k])/n;
        q={v[0],v[1],v[2],(a[k][j]-a[j][k])/n};
    }
    return dual::normalized(q);
}
Pose pose(const Mtx m) { return {rotation(m),{m[0][3],m[1][3],m[2][3]}}; }
void matrix(Pose p,Mtx m) {
    Quaternion q{p.q.x,p.q.y,p.q.z,p.q.w};MTXQuat(m,&q);
    m[0][3]=p.p.x;m[1][3]=p.p.y;m[2][3]=p.p.z;
}
Pose local(float x,float y,float z,float rz=0,float ry=0) {
    constexpr float half=3.14159265358979323846f/360;
    return {dual::multiply({0,0,std::sin(rz*half),std::cos(rz*half)},
                          {0,std::sin(ry*half),0,std::cos(ry*half)}),{x,y,z}};
}
void put(J3DModel* model,int joint,Pose p) { matrix(p,model->getAnmMtx(joint)); }
void put(J3DModel* model,Pose p) { Mtx m;matrix(p,m);model->setBaseTRMtx(m);model->calc(); }

void detach() {
    for(auto& b:s_borrow) {
        if(b.pack && b.wrapper && b.pack->getAnmTransform()==&*b.wrapper)
            b.pack->setAnmTransform(b.original);
        b.pack=nullptr;b.original=nullptr;b.wrapper.reset();
    }
    s_calculating=false;
}
void release() {
    detach();
    if(s.forcedBlade && s.owner && s.owner->mSwordModel && s.owner->mEquipItem!=0x103)
        s.owner->offSwordModel();
    if(s_forget) { if(s.sword) s_forget(s.sword);if(s.sheath) s_forget(s.sheath); }
    if(s.heap) s.heap->destroy();
    s=State{};
}
void model_failure(const char* name,const char* reason) {
    if(!svc_log) return;
    char message[256];
    std::snprintf(message,sizeof(message),"Dual Wield: %s: %s",name,reason);
    svc_log->warn(mod_ctx,message);
}
J3DModel* copy_model(JKRArchive* archive,const char* name) {
    // The one-argument overload is a path lookup relative to the archive's
    // current directory. Alink's meshes live under bmwr/, not at its root.
    // Type 0 searches by name across the archive, independent of that directory.
    void* raw=archive->getResource(0,name);
    if(!raw) { model_failure(name,"resource not found in archive");return nullptr; }
    const u32 bytes=archive->getExpandedResSize(raw);
    if(bytes<32 || bytes>4*1024*1024 || std::memcmp(raw,"J3D2bmd",7)!=0) {
        model_failure(name,"invalid BMD header or resource size");return nullptr;
    }
    void* copy=s.heap->alloc(bytes,32);
    if(!copy) { model_failure(name,"private model allocation failed");return nullptr; }
    std::memcpy(copy,raw,bytes);
    // Alink stores both meshes as BMWR: initialize their material animators,
    // warp material and display lists exactly like native resource loading.
    auto* data=dRes_info_c::loaderBasicBmd(0x424D5752,copy);
    if(!data || !data->getJointNum() || !data->getMaterialNum()) {
        model_failure(name,"native BMD loader failed");return nullptr;
    }
    auto* model=mDoExt_J3DModel__create(data,0x80000,0x11000284);
    if(!model) model_failure(name,"native model creation failed");
    return model;
}
bool prepare(daAlink_c* link) {
    if(s.owner!=link) release();
    if(s.sword && s.sheath) return true;
    constexpr u32 size=8*1024*1024;
    s.storage.reset(new(std::nothrow) u8[size+31]);
    if(!s.storage) { model_failure("heap","backing allocation failed");return false; }
    auto* memory=reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(s.storage.get())+31)&~uintptr_t{31});
    s.heap=JKRExpHeap::create(memory,size,JKRHeap::getRootHeap(),false);
    if(!s.heap) { model_failure("heap","creation failed");release();return false; }
    auto* previous=s.heap->becomeCurrentHeap();
    // The live Alink archive has already had its vertex arrays byte-swapped
    // by J3D. A distinct heap makes mount return a fresh archive, not that
    // cached instance. Never feed the live resource through the loader twice.
    auto* archive=JKRArchive::mount("/res/Object/Alink.arc",JKRArchive::MOUNT_MEM,
                                  s.heap,JKRArchive::MOUNT_DIRECTION_HEAD);
    if(archive) {
        s.sword=copy_model(archive,"al_swa.bmd");
        s.sheath=copy_model(archive,"al_poda.bmd");
        archive->unmount();
    } else model_failure("/res/Object/Alink.arc","private archive mount failed");
    previous->becomeCurrentHeap();
    if(!s.sword || !s.sheath) { release();return false; }
    s.owner=link;
    if(svc_log) svc_log->info(mod_ctx,"Dual Wield: private Ordon sword and scabbard ready");
    return true;
}
bool human(daAlink_c* link) {
    return link && !link->checkWolf() && link->mpLinkModel && link->field_0x2060 &&
        link->mpLinkModel->getModelData()->getJointNum()==35 && link->mSwordModel &&
        link->mSheathModel && link->checkSwordGet() && !link->checkWoodSwordEquip();
}
bool ordinary(daAlink_c* link) {
    if(link->mProcID==daAlink_c::PROC_CUT_NORMAL) return true;
    if(link->mProcID!=daAlink_c::PROC_CUT_FINISH) return false;
    return link->getCutType()==daPy_py_c::CUT_TYPE_FINISH_LEFT ||
        link->getCutType()==daPy_py_c::CUT_TYPE_FINISH_RIGHT ||
        link->getCutType()==daPy_py_c::CUT_TYPE_FINISH_VERTICAL ||
        link->getCutType()==daPy_py_c::CUT_TYPE_FINISH_STAB;
}
bool active(daAlink_c* link) { return s.owner==link && s.active && s.sword && s.sheath; }
HookAction before_execute(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(s.owner && s.owner!=link) release();
    if(!dual_wield_enabled()) s_failedOwner=nullptr;
    if(dual_wield_enabled() && human(link) && s_failedOwner!=link && !prepare(link)) {
        s_failedOwner=link;
        if(svc_log) svc_log->warn(mod_ctx,"Dual Wield: could not prepare private Ordon models; keeping native equipment");
    }
    if(s.owner==link) ++s.tick;
    return HOOK_CONTINUE;
}
void after_matrix(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);if(s.owner!=link) return;
    const bool enabled=dual_wield_enabled() && human(link);
    if(enabled!=s.enabled) {
        s.attacks.reset();s.seedBlade=true;
        if(human(link)) link->field_0x2060->initOldFrameMorf(6,1,16);
    }
    s.enabled=enabled;
    s.active=enabled && !link->checkEventRun();
    const bool mirror=s.active && ordinary(link) && s.attacks.right;
    if(mirror!=s.mirror) s.seedBlade=true;
    s.mirror=mirror;
    if(s.poseTick==s.tick) return;
    s.poseTick=s.tick;
    const bool guard=s.active && (link->mEquipItem==0x103 || link->mEquipItem==dItemNo_NONE_e) &&
                     link->checkPlayerGuardAndAttack();
    const bool drawn=s.active && (link->mEquipItem==0x103 || guard);
    s.guard=dual::approach(s.guard,guard ? 1.0f : 0.0f,1.0f/5);
    s.draw=dual::approach(s.draw,drawn ? 1.0f : 0.0f,1.0f/8);
}
void after_cut(ModContext*,void* args,void* result,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!active(link) || !ordinary(link) || !*static_cast<int*>(result)) return;
    s.attacks.begin();s.seedBlade=true;
    // Native quaternion/translation morphing sees the modified pose from the
    // previous body calculation. It also blends an interrupted combination.
    link->field_0x2060->initOldFrameMorf(4,1,16);
}
HookAction before_model_calc(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    auto* model=mods::arg<J3DModel*>(args,1);
    if(!active(link) || !s.mirror || model!=link->mpLinkModel || s_calculating) return HOOK_CONTINUE;
    // Reject nonstandard tracks as a group; never partly mirror a mixed rig.
    for(int i=0;i<6;++i) {
        auto& pack=i<3 ? link->mNowAnmPackUnder[i] : link->mNowAnmPackUpper[i-3];
        auto* anm=pack.getAnmTransform();
        if(anm && (anm->getKind()!=8 || anm->field_0x1e!=35)) { s.mirror=false;return HOOK_CONTINUE; }
    }
    s_calculating=true;
    for(int i=0;i<6;++i) {
        auto& b=s_borrow[i];b.pack=i<3 ? &link->mNowAnmPackUnder[i] : &link->mNowAnmPackUpper[i-3];
        b.original=b.pack->getAnmTransform();if(!b.original) continue;
        b.wrapper.emplace(*static_cast<J3DAnmTransformKey*>(b.original));
        b.pack->setAnmTransform(&*b.wrapper);
    }
    return HOOK_CONTINUE;
}
void after_model_calc(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(s_calculating && s.owner==link && mods::arg<J3DModel*>(args,1)==link->mpLinkModel) detach();
}
Pose hand_mount(daAlink_c* link,bool right) {
    const auto& t=*link->field_0x2060->getOldFrameTransInfo(10);
    Quaternion q;JMAEulerToQuat(t.mRotation.x,t.mRotation.y,t.mRotation.z,&q);
    Pose mount{{q.x,q.y,q.z,q.w},{t.mTranslate.x,t.mTranslate.y,t.mTranslate.z}};
    if(right) { mount.q.x=-mount.q.x;mount.q.y=-mount.q.y;mount.p.z=-mount.p.z; }
    return mount;
}
Pose sword_at_hand(daAlink_c* link,bool right) {
    return dual::compose(pose(link->mpLinkModel->getAnmMtx(right ? 14 : 9)),hand_mount(link,right));
}
void solve_arm(daAlink_c* link,bool right,Pose sword,float weight) {
    auto* model=link->mpLinkModel;int first=right ? 12 : 7;
    dual::Arm arm{pose(model->getAnmMtx(first)),pose(model->getAnmMtx(first+1)),pose(model->getAnmMtx(first+2))};
    const Pose item=dual::compose(dual::inverse(arm.hand),pose(model->getAnmMtx(first+3)));
    Pose hand=dual::compose(sword,dual::inverse(hand_mount(link,right)));
    const auto solved=dual::reach(arm,hand,weight);
    put(model,first,solved.upper);put(model,first+1,solved.lower);put(model,first+2,solved.hand);
    // Both item joints remain attached to their hands after the IK pass.
    put(model,first+3,dual::compose(solved.hand,item));
}
void after_arms(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);if(!active(link)) return;
    auto* model=link->mpLinkModel;
    s.hipSword=dual::compose(pose(model->getAnmMtx(16)),local(-3,0,18,20));
    if(s.guard>0) {
        Pose base=pose(model->getBaseTRMtx());
        float push=0;
        if(link->mProcID==daAlink_c::PROC_GUARD_ATTACK) {
            const float start=link->field_0x3478-4,end=link->field_0x347c+4;
            const float t=std::clamp((link->mUnderFrameCtrl[0].getFrame()-start)/std::max(1.0f,end-start),0.0f,1.0f);
            push=16*std::sin(3.14159265358979323846f*dual::smooth(t));
        }
        for(bool right:{false,true}) {
            const Vec shoulder=dual::rotate(dual::conjugate(base.q),pose(model->getAnmMtx(right ? 12 : 7)).p-base.p);
            const float side=shoulder.x>=0 ? 1.0f : -1.0f;
            Pose cross{dual::between({1,0,0},dual::unit({-.65f*side,.75f,.12f})),{18*side,118,26+push}};
            solve_arm(link,right,dual::compose(base,cross),dual::smooth(s.guard));
        }
    }
    // Apply the draw after the guard solve too: raising the guard directly
    // from a holstered state must not teleport the offhand sword off the hip.
    if(s.draw>0 && s.draw<1 && !ordinary(link)) {
        Pose target=dual::blend(s.hipSword,sword_at_hand(link,true),dual::smooth(s.draw));
        solve_arm(link,true,target,1);
    }
    s.rightSword=s.draw>0 ? sword_at_hand(link,true) : s.hipSword;
}
void after_items(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(s.owner!=link) return;
    if(s.forcedBlade && (!active(link) || s.guard<=0)) {
        if(link->mEquipItem!=0x103) link->offSwordModel();s.forcedBlade=false;
    }
    if(!active(link)) return;
    put(s.sword,s.rightSword);
    // Native Ordon sword-in-sheath transform, inverted to place the scabbard
    // around the same hip-mounted blade rather than creating a second offset.
    const Pose mount=local(-18.5f,.14f,12.2f,0,33.1f);
    put(s.sheath,dual::compose(s.hipSword,dual::inverse(mount)));
    if(s.guard>0 && link->mEquipItem!=0x103) {
        put(link->mSwordModel,sword_at_hand(link,false));
        link->mSwordModel->getModelData()->getMaterialNodePointer(0)->getShape()->show();
        s.forcedBlade=true;
    }
}
void after_sword_pos(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);if(!active(link)) return;
    if(s.mirror) {
        const Pose blade=sword_at_hand(link,true);
        const Vec tip=blade.p+dual::rotate(blade.q,{100,0,0});
        const bool reverse=link->getCutType()==daPy_py_c::CUT_TYPE_FINISH_RIGHT;
        const Vec direction=dual::rotate(blade.q,{0,0,reverse ? -1.0f : 1.0f});
        link->field_0x3498.set(blade.p.x,blade.p.y,blade.p.z);
        link->mSwordTopPos.set(tip.x,tip.y,tip.z);link->field_0x3720=link->mSwordTopPos;
        link->field_0x34a4.set(direction.x,direction.y,direction.z);
    }
    if(s.seedBlade) {
        link->field_0x34b0=link->mSwordTopPos;link->field_0x34bc=link->field_0x3498;s.seedBlade=false;
    }
}
void after_shield_draw(ModContext*,void* args,void* result,void*) {
    if(active(mods::arg<daAlink_c*>(args,0))) *static_cast<bool*>(result)=false;
}
void after_model_draw(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!active(link) || mods::arg<J3DModel*>(args,1)!=link->mSwordModel) return;
    const int hidden=mods::arg<int>(args,2);
    link->modelDraw(s.sword,hidden);link->modelDraw(s.sheath,hidden);
}
void after_collision(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!active(link) || link->mProcID!=daAlink_c::PROC_GUARD_ATTACK || !link->mProcVar5.field_0x3012) return;
    const Pose left=sword_at_hand(link,false),right=sword_at_hand(link,true);
    const Vec a=left.p+dual::rotate(left.q,{35,0,0}),b=right.p+dual::rotate(right.q,{35,0,0});
    link->mGuardAtCps.SetStartEnd(cXyz(a.x,a.y,a.z),cXyz(b.x,b.y,b.z));
    // Keep Shield Attack's native type, timing and stun/Hidden Skill behavior.
}
HookAction before_delete(ModContext*,void* args,void*,void*) {
    if(mods::arg<void*>(args,0)==s_failedOwner) s_failedOwner=nullptr;
    if(mods::arg<void*>(args,0)==s.owner) release();return HOOK_CONTINUE;
}
} // namespace
ModResult install_dual_wield_hooks(ModError* error) {
    void* address=nullptr;
    if(svc_hook->resolve && svc_hook->resolve(mod_ctx,"J3DModel::forgetMtx",&address,nullptr)==MOD_OK)
        s_forget=reinterpret_cast<ForgetModel>(address);
    ModResult result=MOD_OK;
#define PRE(H,F) if((result=mods::hook::add_pre<H>(svc_hook,F))!=MOD_OK) return mods::set_error(error,result,"Dual Wield: " #H)
#define POST(H,F) if((result=mods::hook::add_post<H>(svc_hook,F))!=MOD_OK) return mods::set_error(error,result,"Dual Wield: " #H)
    PRE(DualExecute,before_execute);POST(DualMatrix,after_matrix);
    PRE(DualModelCalc,before_model_calc);POST(DualModelCalc,after_model_calc);
    POST(DualArms,after_arms);POST(DualItems,after_items);POST(DualSwordPos,after_sword_pos);
    POST(DualShieldDraw,after_shield_draw);POST(DualModelDraw,after_model_draw);
    POST(DualCut,after_cut);POST(DualFinish,after_cut);POST(DualCollision,after_collision);
    PRE(DualDelete,before_delete);
#undef PRE
#undef POST
    return MOD_OK;
}
void shutdown_dual_wield() { release();s_failedOwner=nullptr; }
} // namespace dawnlight
