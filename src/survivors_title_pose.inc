// Borrow animation packs only for the body calculation; never edit an archive
// BCK or leave a wrapper attached to a native actor across frames/scene changes.
struct TitlePoseAnimation final : J3DAnmTransformKey {
    std::array<J3DTransformInfo,35> joints{};
    void getTransform(u16 joint,J3DTransformInfo* out) const override { *out=joints[joint]; }
};
struct TitleModelPose {
    ActorId owner=kNone;
    J3DModel* model=nullptr;
    TitlePoseAnimation animation;
    std::array<J3DAnmTransform*,6> originals{};
    void* call=nullptr;
} s_titleModel;
bool title_model_ready(daAlink_c* link) {
    return link && link==daAlink_getAlinkActorClass() && s_titleModel.owner==fopAcM_GetID(link) &&
        s_titleModel.model==link->mpLinkModel;
}
bool title_parents(J3DJoint* joint,int parent,TitleRestPose& pose) {
    for (;joint;joint=joint->getYounger()) {
        const auto id=joint->getJntNo();
        if (id>=pose.count || pose.parent[id]!=-2) return false;
        pose.parent[id]=parent;
        if (!title_parents(joint->getChild(),id,pose)) return false;
    }
    return true;
}
HookAction before_title_model(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    auto* model=mods::arg<J3DModel*>(args,1);
    if (!survivors_rest_active() || link!=daAlink_getAlinkActorClass() || !link ||
        !model || model!=link->mpLinkModel || s_titleModel.call) return HOOK_CONTINUE;
    auto& s=s_titleModel;
    s.owner=kNone;
    if (model->getModelData()->getJointNum()!=35) return HOOK_CONTINUE;
    auto* source=link->mNowAnmPackUnder[0].getAnmTransform();
    if (!source || source->getKind()!=8 || static_cast<J3DAnmTransformKey*>(source)->field_0x1e!=35)
        return HOOK_CONTINUE;
    TitleRestPose pose;
    pose.parent.fill(-2);
    if (!title_parents(model->getModelData()->getJointNodePointer(0),-1,pose)) return HOOK_CONTINUE;
    for (unsigned i=0;i<pose.count;++i) {
        auto& t=s.animation.joints[i];
        static_cast<J3DAnmTransformKey*>(source)->calcTransform(0,i,&t);
        pose.local[i]={joint_rotation(t),{t.mTranslate.x,t.mTranslate.y,t.mTranslate.z}};
    }
    if (!pose.rest(survivors_hub_arrival_active() ? hub_arrival_progress() : 0.f)) return HOOK_CONTINUE;
    for (unsigned i=0;i<pose.count;++i) {
        auto& t=s.animation.joints[i];
        set_joint_rotation(t,pose.local[i].q);
        const auto p=pose.local[i].p;
        t.mTranslate={p.x,p.y,p.z};
    }
    for (unsigned i=0;i<6;++i) {
        auto& pack=i<3 ? link->mNowAnmPackUnder[i] : link->mNowAnmPackUpper[i-3];
        s.originals[i]=pack.getAnmTransform();
        if (s.originals[i]) pack.setAnmTransform(&s.animation);
    }
    s.call=args; s.model=model; s.owner=fopAcM_GetID(link);
    return HOOK_CONTINUE;
}
void after_title_model(ModContext*,void* args,void*,void*) {
    auto& s=s_titleModel;
    if (s.call!=args) return;
    auto* link=mods::arg<daAlink_c*>(args,0);
    for (unsigned i=0;i<6;++i) {
        auto& pack=i<3 ? link->mNowAnmPackUnder[i] : link->mNowAnmPackUpper[i-3];
        if (s.originals[i]) pack.setAnmTransform(s.originals[i]);
        s.originals[i]=nullptr;
    }
    s.call=nullptr;
}
HookAction before_title_joint(ModContext*,void* args,void* result,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!s_titleModel.call || !title_model_ready(link) || !survivors_rest_active()) return HOOK_CONTINUE;
    // Standing foot/arm IK and root clearing would undo the authored rest pose.
    // All packs supply the same complete pose, so blend-switch callbacks are
    // unnecessary for this one body calculation. Gameplay retains them.
    *static_cast<bool*>(result)=true;
    return HOOK_SKIP_ORIGINAL;
}
