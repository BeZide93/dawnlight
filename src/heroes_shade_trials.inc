// Arena-owned intermissions. Included inside heroes_shade_encounter.cpp's
// private namespace so actor ownership and cancellation use the same helpers.
// Game models are resolved from the player's archives; none are bundled here.
class TrialEyeModel final : public J3DModel {
public:
    J3DJointCallBack* callbacks=nullptr;
    unsigned jointCount=0;
    J3DMaterial* eyeMaterial=nullptr;
    bool create(J3DModelData* data) {
        if (!data || !data->getJointNum() || !data->getMaterialNum()) return false;
        auto* names=data->getMaterialName();
        if (names) for (unsigned i=0;i<data->getMaterialNum();++i)
            if (std::strcmp(names->getName(i),"bm6_eye")==0) eyeMaterial=data->getMaterialNodePointer(i);
        if (!eyeMaterial || !eyeMaterial->getShape() || !eyeMaterial->getJoint()) return false;
        // Native Beamos starts with flags=0. DifferedDLBuffer is legal only
        // when the archive supplied a shared display list for this model.
        const bool shared=data->getMaterialNodePointer(0)->getSharedDisplayListObj()!=nullptr;
        const u32 flags=!shared ? 0 : data->isLocked() ? J3DMdlFlag_UseSharedDL : J3DMdlFlag_DifferedDLBuffer;
        if (entryModelData(data,flags,1)!=kJ3DError_Success ||
            (flags==J3DMdlFlag_DifferedDLBuffer && newDifferedDisplayList(0x11000284)!=kJ3DError_Success)) return false;
        jointCount=data->getJointNum();
        callbacks=JKR_NEW_ARRAY(J3DJointCallBack,jointCount);
        if (!callbacks) return false;
        lock();
        return true;
    }
    void calc() override {
        // A separately spawned native Beamos may have installed callbacks on
        // this archive's joints. They dereference a daObjLv6Bm_c user pointer,
        // which our head-only model does not have. Suppress only during this
        // synchronous calculation and restore before another model can run.
        for (unsigned i=0;i<jointCount;++i) {
            auto* joint=getModelData()->getJointNodePointer(i);
            callbacks[i]=joint->getCallBack(); joint->setCallBack(nullptr);
        }
        J3DModel::calc();
        for (unsigned i=0;i<jointCount;++i)
            getModelData()->getJointNodePointer(i)->setCallBack(callbacks[i]);
    }
    // Select the named eye material, as the native Beamos does. A skeletal
    // head joint need not own a mesh: rigid/skinned shapes can be attached to
    // another joint. Submit our model's eye packet without hiding shared shapes.
    void entry() override {
        j3dSys.setModel(this);
        j3dSys.offFlag(J3DSysFlag_SkinPosCpu);
        j3dSys.offFlag(J3DSysFlag_SkinNrmCpu);
        getModelData()->syncJ3DSysFlags();
        j3dSys.setTexture(getModelData()->getTexture());
        auto* mesh=eyeMaterial;
        if (!mesh) return;
        auto* matrix=getAnmMtx(mesh->getJoint()->getJntNo());
        for (unsigned i=0;i<2;++i) j3dSys.getDrawBuffer(i)->setZMtx(matrix);
        auto* materialPacket=getMatPacket(mesh->getIndex());
        if (!materialPacket->isLocked()) {
            if (auto* animation=mesh->getMaterialAnm()) animation->calc(mesh);
            mesh->calc(matrix);
        }
        mesh->setCurrentMtx();
        materialPacket->setMaterialAnmID(mesh->getMaterialAnm());
        materialPacket->setShapePacket(getShapePacket(mesh->getShape()->getIndex()));
        if (materialPacket->entry(j3dSys.getDrawBuffer(!mesh->isDrawModeOpaTexEdge()))) {
            j3dSys.setMatPacket(materialPacket);
            ++J3DDrawBuffer::entryNum;
            mesh->makeDisplayList();
        }
    }
};
struct TrialFireModel {
    J3DModel* model=nullptr;
    mDoExt_bckAnm bck;
    mDoExt_btkAnm btk;
    mDoExt_brkAnm brk;
    bool colored=false;
    bool create(unsigned index) {
        constexpr int models[]={dRes_INDEX_E_FM_BMD_EF_FMATTACK_A_e,dRes_INDEX_E_FM_BMD_EF_FMATTACK_B_e};
        constexpr int motions[]={dRes_INDEX_E_FM_BCK_EF_FMATTACK_A_e,dRes_INDEX_E_FM_BCK_EF_FMATTACK_B_e};
        constexpr int textures[]={dRes_INDEX_E_FM_BTK_EF_FMATTACK_A_e,dRes_INDEX_E_FM_BTK_EF_FMATTACK_B_e};
        auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes("E_fm",models[index]));
        auto* motion=static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("E_fm",motions[index]));
        auto* texture=static_cast<J3DAnmTextureSRTKey*>(dComIfG_getObjectRes("E_fm",textures[index]));
        if (!data || !motion || !texture) return false;
        model=mDoExt_J3DModel__create(data,0x80000,0x11000284);
        if (!model || !bck.init(motion,TRUE,J3DFrameCtrl::EMode_NONE,1,0,-1,false) ||
            !btk.init(data,texture,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1)) return false;
        colored=index==1;
        if (!colored) return true;
        auto* color=static_cast<J3DAnmTevRegKey*>(dComIfG_getObjectRes("E_fm",dRes_INDEX_E_FM_BRK_EF_FMATTACK_B_e));
        return color && brk.init(data,color,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1);
    }
    void reset() { bck.setFrame(0); btk.setFrame(0); if (colored) brk.setFrame(0); }
    void sample(float frame) {
        bck.setFrame(std::min(frame,bck.getEndFrame()));
        btk.play(); if (colored) brk.play();
    }
    void draw(const cXyz& origin,float scale,dKy_tevstr_c& light) {
        mDoMtx_stack_c::transS(origin.x,origin.y+3,origin.z);
        // Enlarge the radius, not the height: hanging Link must remain safe.
        mDoMtx_stack_c::scaleM(scale,2.5f,scale);
        model->setBaseTRMtx(mDoMtx_stack_c::get());
        g_env_light.setLightTevColorType_MAJI(model,&light);
        auto* data=model->getModelData();
        bck.entry(data); btk.entry(data); if (colored) brk.entry(data);
        mDoExt_modelUpdateDL(model);
        bck.remove(data); btk.remove(data); if (colored) brk.remove(data);
    }
};

// Native Beamos beam geometry and both texture animations, loaded from E_bm6.
// Each eye owns animation state; no procedural substitute or shared actor callbacks.
struct TrialBeamModel {
    J3DModel* model=nullptr;
    mDoExt_bckAnm bck;
    mDoExt_btkAnm scroll,on;
    bool create() {
        auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BMD_EF_BIMOL6_e));
        auto* motion=static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BCK_EF_BIMOL6_e));
        auto* texture=static_cast<J3DAnmTextureSRTKey*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BTK_EF_BIMOL6_e));
        auto* start=static_cast<J3DAnmTextureSRTKey*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BTK_EF_BIMOL6_ON_e));
        if (!data || !motion || !texture || !start) return false;
        model=mDoExt_J3DModel__create(data,0x80000,0x11000284);
        return model && bck.init(motion,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1,false) &&
            scroll.init(data,texture,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1) &&
            on.init(data,start,TRUE,J3DFrameCtrl::EMode_NONE,1,0,-1);
    }
    void reset() {
        bck.setFrame(0); scroll.setFrame(0); on.setFrame(0);
        // EMode_NONE stops playback at the end. Rewinding the frame alone
        // does not restart the startup BTK after a hit/pause.
        bck.setPlaySpeed(1); scroll.setPlaySpeed(1); on.setPlaySpeed(1);
    }
    void play() { bck.play(); scroll.play(); on.play(); }
    float reach() { return std::min(1.0f,static_cast<int>(on.getFrame()+0.5f)*0.3f); }
    void draw(const cXyz& start,const cXyz& end,dKy_tevstr_c& light) {
        const cXyz delta=end-start;
        mDoMtx_stack_c::transS(start);
        mDoMtx_stack_c::YrotM(cM_atan2s(delta.x,delta.z));
        mDoMtx_stack_c::XrotM(cM_atan2s(-delta.y,delta.absXZ()));
        mDoMtx_stack_c::scaleM(0.5f,0.5f,delta.abs()/1200.0f);
        model->setBaseTRMtx(mDoMtx_stack_c::get());
        g_env_light.setLightTevColorType_MAJI(model,&light);
        auto* data=model->getModelData();
        bck.entry(data); scroll.entry(data); on.entry(data);
        mDoExt_modelUpdateDL(model);
        bck.remove(data); scroll.remove(data); on.remove(data);
    }
};

class TrialPacket : public J3DPacket {
public:
    shade::TrialClock clock;
    cXyz center{0,0,0}, boss{0,0,0};
    std::array<cXyz,2> ends{};
    float radius=1000;
    static constexpr unsigned rimSegments=64;
    std::array<cXyz,rimSegments> rimOuter{},rimInner{};
    std::array<bool,rimSegments> rimValid{};
    bool lava=false;
    int lavaTicks=0;
    void vertex(const cXyz& p,GXColor c) {
        GXPosition3f32(p.x,p.y,p.z); GXColor4u8(c.r,c.g,c.b,c.a);
    }
    void ring(const cXyz& p,float radius,float width,GXColor color) {
        GXBegin(GX_QUADS,GX_VTXFMT0,96*4);
        for (unsigned i=0;i<96;++i) for (unsigned k=0;k<4;++k) {
            const float a=(i+(k==1 || k==2))*6.2831853072f/96;
            const float r=radius+(k>=2 ? width : 0);
            vertex(p+cXyz(std::sin(a)*r,0,std::cos(a)*r),color);
        }
        GXEnd();
    }
    void draw() override {
        if (!clock.active() && !lava) return;
        j3dSys.reinitGX();
        GXLoadPosMtxImm(j3dSys.getViewMtx(),GX_PNMTX0); GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc(); GXSetVtxDesc(GX_VA_POS,GX_DIRECT); GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(0); GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
        GXSetZMode(GX_TRUE,GX_LEQUAL,GX_FALSE);
        GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_ONE,GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0); GXSetCullMode(GX_CULL_NONE);
        if (lava) {
            // Original molten ribbons follow the sampled room edge. The dark
            // margins and moving bright seams stay inside the same narrow strip.
            for (unsigned i=0;i<rimSegments;++i) {
                const unsigned next=(i+1)%rimSegments;
                if (!rimValid[i] || !rimValid[next]) continue;
                GXBegin(GX_QUADS,GX_VTXFMT0,4*4);
                for (unsigned band=0;band<4;++band) for (unsigned k=0;k<4;++k) {
                    const unsigned edge=(k==1 || k==2) ? next : i;
                    const float across=(band+(k>=2))/4.0f;
                    const float pulse=0.5f+0.5f*std::sin(edge*1.7f+lavaTicks*0.08f+band);
                    const bool crust=across==0 || across==1;
                    vertex(rimOuter[edge]+(rimInner[edge]-rimOuter[edge])*across,
                        {static_cast<u8>(crust ? 85 : 255),static_cast<u8>(crust ? 12 : 55+110*pulse),
                         static_cast<u8>(crust ? 3 : 8),255});
                }
                GXEnd();
            }
        }
        if (clock.kind==shade::Trial::Shield) {
            // Smooth translucent shell with a moving latitude highlight.
            GXBegin(GX_QUADS,GX_VTXFMT0,24*48*4);
            for (unsigned band=0;band<24;++band) for (unsigned side=0;side<48;++side)
                for (unsigned k=0;k<4;++k) {
                    const float lat=-1.5707963268f+(band+(k>=2))*3.1415926536f/24;
                    const float lon=(side+(k==1 || k==2))*6.2831853072f/48;
                    const float r=165+clock.burst*12;
                    const float pulse=std::pow(std::max(0.0f,std::cos(lat*4-clock.ticks*0.07f)),12);
                    vertex(boss+cXyz(r*std::cos(lat)*std::sin(lon),110+r*std::sin(lat),r*std::cos(lat)*std::cos(lon)),
                        {95,170,255,static_cast<u8>((24+70*pulse)*(1-clock.burst/12.0f))});
                }
            GXEnd();
        } else if (clock.kind==shade::Trial::Fire && clock.ticks<shade::fire_warning_ticks) {
            const float charge=static_cast<float>(clock.ticks)/shade::fire_warning_ticks;
            for (unsigned i=0;i<3;++i) ring(boss+cXyz(0,5+i*7,0),100+charge*180+i*32,12,{255,85,20,180});
        } else if (clock.kind==shade::Trial::Wind && !clock.windReleased) {
            // Outward-moving ribbons make the direction and remaining gust visible.
            for (unsigned band=0;band<6;++band) {
                const float t=std::fmod(clock.ticks/45.0f+band/6.0f,1.0f);
                ring(center+cXyz(0,30+band*24,0),t*radius,8,{180,220,230,static_cast<u8>((1-t)*95)});
            }

        }
        j3dSys.reinitGX();
    }
};

#include "heroes_shade_trial_audio.inc"

struct ShadeTrials {
    ShadeTrialAudio audio;
    request_of_phase_process_class fireArchive{},eyeArchive{};
    std::array<TrialFireModel,2> fire;
    std::array<TrialBeamModel,2> beamModels;
    std::array<TrialEyeModel*,2> eyes{};
    mDoExt_bckAnm eyePose;
    mDoExt_brkAnm eyeColor;
    std::array<ActorId,4> anchors{kNone,kNone,kNone,kNone};
    std::array<cXyz,4> anchorPos{};
    std::array<cXyz,2> eyePos{},aim{};
    dCcD_Stts status;
    dCcD_Sph shield;
    std::array<dCcD_Sph,2> eyeTargets;
    std::array<dCcD_Cps,2> beams;
    dCcD_Cyl flame;
    dCcD_Cps lavaContact;
    bool lavaActive=false;
    int lavaRecovery=0;
    TrialPacket packet;
    shade::TrialClock clock;
    fopAc_ac_c* owner=nullptr;
    cXyz center{0,0,0}, origin{0,0,0};
    float radius=0, fireScale=2.5f;
    std::array<bool,4> anchorPlaced{},anchorAligned{};
    std::array<s16,4> anchorYaw{};
    s16 lastWallYaw=0;
    std::array<bool,2> eyePlaced{};
    bool radiusReady=false;
    JKRSolidHeap* effectHeap=nullptr;
    bool effectsReady=false;
    bool effectsFailed=false;
    const char* failure="unknown effect initialization step";
    bool visible=false;
    int beamRecovery=0;
    bool beamVisible=false;

    int load() {
        int result=dComIfG_resLoad(&fireArchive,"E_fm");
        if (result!=cPhs_COMPLEATE_e) return result;
        return dComIfG_resLoad(&eyeArchive,"E_bm6");
    }
    bool heap() {
        failure="Fyrus fire models/animations";
        for (unsigned i=0;i<2;++i) if (!fire[i].create(i)) return false;
        failure="Beamos model resource";
        auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BMD_BM6_e));
        if (!data) return false;
        failure="Beamos pose/color animations";
        auto* pose=static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BCK_BM6_UP_e));
        auto* color=static_cast<J3DAnmTevRegKey*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BRK_BM6_TURN_e));
        if (!pose || !color || !eyePose.init(pose,TRUE,J3DFrameCtrl::EMode_NONE,0,0,-1,false) ||
            !eyeColor.init(data,color,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1)) return false;
        eyePose.setFrame(eyePose.getEndFrame());
        failure="Beamos bm6_eye material/model packets";
        for (auto*& eye:eyes) {
            eye=JKR_NEW TrialEyeModel;
            if (!eye || !eye->create(data)) return false;
        }
        failure="Beamos beam models/animations";
        for (auto& beam:beamModels) if (!beam.create()) return false;
        return true;
    }
    bool prepare_effects() {
        if (effectsReady || effectsFailed) return effectsReady;
        const int phase=load();
        if (phase!=cPhs_COMPLEATE_e) {
            if (phase==cPhs_ERROR_e) {
                effectsFailed=true;
                svc_log->warn(mod_ctx,"Hero's Shade: failed to load E_fm/E_bm6; pedestal and wall targets remain available");
            }
            return false;
        }
        // Keep effect allocation out of the sword's create/heap transaction.
        // The original pedestal can draw while the two archives load.
        effectHeap=mDoExt_createSolidHeapFromGame(0x80000,0x20);
        failure="effect heap allocation";
        if (effectHeap) {
            auto* previous=mDoExt_setCurrentHeap(effectHeap);
            effectsReady=heap();
            mDoExt_setCurrentHeap(previous);
            if (effectsReady) mDoExt_adjustSolidHeap(effectHeap);
            else { mDoExt_destroySolidHeap(effectHeap); effectHeap=nullptr; }
        }
        if (!effectsReady) {
            effectsFailed=true;
            svc_log->warn(mod_ctx,"Hero's Shade: effect initialization failed");
            svc_log->warn(mod_ctx,failure);
        }
        return effectsReady;
    }
    void init(fopAc_ac_c* actor) {
        owner=actor; center=actor->current.pos;
        status.Init(255,0,actor);
        shield.SetStts(&status); shield.SetTgType(AT_TYPE_BOMB|AT_TYPE_IRON_BALL);
        shield.SetTgSPrm(0x11); shield.SetR(165); shield.SetTgSe(dCcD_SE_HARD_BODY);
        // Native Fyrus fire: special 0xE passes ChkAtNoGuard, including shields.
        flame.SetStts(&status); flame.SetAtType(AT_TYPE_CSTATUE_SWING);
        flame.SetAtAtp(shade::special_attack_power); flame.SetAtSPrm(0x1f);
        flame.SetAtSpl(static_cast<dCcG_At_Spl>(0xe)); flame.SetAtMtrl(dCcD_MTRL_FIRE);
        flame.SetH(300);
        lavaContact.SetStts(&status); lavaContact.SetAtType(AT_TYPE_CSTATUE_SWING);
        lavaContact.SetAtSPrm(0x1f); lavaContact.SetAtAtp(shade::normal_attack_power);
        lavaContact.SetAtMtrl(dCcD_MTRL_FIRE); lavaContact.SetAtSpl(static_cast<dCcG_At_Spl>(0xe));
        for (unsigned i=0;i<2;++i) {
            eyeTargets[i].SetStts(&status); eyeTargets[i].SetTgType(AT_TYPE_ARROW);
            eyeTargets[i].SetTgSPrm(0x11); eyeTargets[i].SetR(55);
            beams[i].SetStts(&status); beams[i].SetAtType(AT_TYPE_100);
            beams[i].SetAtSPrm(0x1d); beams[i].SetAtAtp(shade::special_attack_power);
            beams[i].SetAtSe(dCcD_SE_SWORD);
        }
        cancel();
    }
    bool wall(s16 angle,float height,float inset,cXyz& pos) {
        cXyz start=center+cXyz(0,height,0);
        const cXyz direction(cM_ssin(angle),0,cM_scos(angle));
        cXyz end=start+direction*10000;
        dBgS_LinChk check; check.Set(&start,&end,owner);
        if (!dComIfG_Bgsp().LineCross(&check)) return false;
        cM3dGPla plane;
        if (!dComIfG_Bgsp().GetTriPla(check,&plane)) return false;
        cXyz normal=plane.mNormal;
        if (normal.x*direction.x+normal.z*direction.z>0) normal*=-1;
        lastWallYaw=cM_atan2s(normal.x,normal.z);
        pos=check.GetCross()+cXyz(cM_ssin(lastWallYaw),0,cM_scos(lastWallYaw))*inset;
        return (pos-start).absXZ()>300;
    }
    bool find_wall(s16 angle,float height,float inset,cXyz& pos) {
        // A ray through a doorway or a corner must not block every target.
        constexpr int offsets[]={0,-0x800,0x800,-0x1000,0x1000};
        for (const int offset:offsets)
            if (wall(static_cast<s16>(angle+offset),height,inset,pos)) return true;
        return false;
    }
    bool align_anchor(daHsTarget_c* target,unsigned index) {
        auto* bg=target->mpBgW;
        if (!bg || !bg->GetBgd() || !bg->GetVtxTbl() || !target->mpModel) return false;
        // Determine the actual hookable face from DZB, not a guessed model pitch.
        // Rotate its normal to horizontal/inward so Link enters wall wait instead
        // of roof wait. Model and MoveBG receive exactly the same transform.
        cXyz normal(0,0,0);
        float largest=0;
        auto* data=bg->GetBgd();
        const auto* vertices=bg->GetVtxTbl();
        for (int i=0;i<data->m_t_num;++i) if (bg->GetPolyHSStick(i)) {
            const auto& tri=data->m_t_tbl[i];
            const cXyz a=cXyz(vertices[tri.m_vtx_idx1])-cXyz(vertices[tri.m_vtx_idx0]);
            const cXyz b=cXyz(vertices[tri.m_vtx_idx2])-cXyz(vertices[tri.m_vtx_idx0]);
            const cXyz cross(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);
            const float area=cross.abs();
            if (area>largest) { largest=area; normal=bg->pm_tri[i].m_plane.mNormal; }
        }
        if (largest<=0) return false;
        mDoMtx_stack_c::YrotS(static_cast<s16>(-target->shape_angle.y));
        mDoMtx_stack_c::multVec(&normal,&normal);
        const auto yaw=cM_atan2s(normal.x,normal.z);
        const auto pitch=cM_atan2s(normal.y,normal.absXZ());
        mDoMtx_stack_c::transS(anchorPos[index]);
        mDoMtx_stack_c::YrotM(anchorYaw[index]);
        mDoMtx_stack_c::XrotM(pitch);
        mDoMtx_stack_c::YrotM(static_cast<s16>(-yaw));
        target->mpModel->setBaseTRMtx(mDoMtx_stack_c::get());
        MTXCopy(mDoMtx_stack_c::get(),target->mBgMtx);
        bg->Move();
        return true;
    }
    bool place() {
        bool ready=true;
        for (unsigned i=0;i<4;++i) {
            if (!anchorPlaced[i]) {
                const auto angle=static_cast<s16>(0x2000+i*0x4000);
                anchorPlaced[i]=find_wall(angle,650,90,anchorPos[i]) || find_wall(angle,550,90,anchorPos[i]);
                if (anchorPlaced[i]) anchorYaw[i]=lastWallYaw;
            }
            if (!anchorPlaced[i]) { ready=false; continue; }
            if (!pending_or_live(anchors[i])) {
                const auto& p=anchorPos[i];
                const ActorSpawnParams params{.parameters=1,.argument=-1,.room_num=51,
                    .position={p.x,p.y,p.z},.angle={0,anchorYaw[i],0},
                    .scale={1,1,1},.create_function=nullptr};
                anchorAligned[i]=false;
                create_standalone_actor(fpcNm_Obj_HsTarget_e,params,anchors[i]);
            }
            auto* target=static_cast<daHsTarget_c*>(actor_by_id(anchors[i]));
            if (target && !anchorAligned[i]) anchorAligned[i]=align_anchor(target,i);
            ready=target && anchorAligned[i] && ready;
        }
        // Eye placement is independent; missing eye geometry cannot hide all
        // four targets. Recheck only missing positions while the room loads.
        for (unsigned i=0;i<2;++i) {
            if (!eyePlaced[i]) eyePlaced[i]=find_wall(static_cast<s16>(0x4000+i*0x8000),400,100,eyePos[i]);
            ready=eyePlaced[i] && ready;
        }
        if (ready && !radiusReady) {
            for (unsigned i=0;i<16;++i) {
                cXyz point;
                if (wall(static_cast<s16>(i*0x1000),100,0,point)) radius=std::max(radius,(point-center).absXZ());
            }
            for (const auto& point:anchorPos) radius=std::max(radius,(point-center).absXZ());
            radius+=150;
            float rimLimit=0;
            for (const auto& p:anchorPos) rimLimit=std::max(rimLimit,(p-center).absXZ());
            rimLimit=(rimLimit+150)*1.25f;
            for (unsigned i=0;i<TrialPacket::rimSegments;++i) {
                cXyz edge;
                packet.rimValid[i]=wall(static_cast<s16>(i*0x10000/TrialPacket::rimSegments),40,8,edge);
                packet.rimValid[i]=packet.rimValid[i] && (edge-center).absXZ()<rimLimit;
                if (!packet.rimValid[i]) continue;
                edge.y=center.y+4;
                packet.rimOuter[i]=edge;
                packet.rimInner[i]=edge+cXyz(cM_ssin(lastWallYaw)*80,0,cM_scos(lastWallYaw)*80);
            }
            radiusReady=true;
        }
        return ready;
    }
    void suspend() {
        audio.stop();
        visible=false; beamVisible=false; packet.lava=false;
        lavaContact.OffAtSetBit(); lavaContact.ClrAtHit();
        shield.OffTgSetBit(); shield.ClrTgHit(); flame.OffAtSetBit(); flame.ClrAtHit();
        for (unsigned i=0;i<2;++i) {
            eyeTargets[i].OffTgSetBit(); eyeTargets[i].ClrTgHit();
            beams[i].OffAtSetBit(); beams[i].ClrAtHit();
        }
    }
    void cancel(bool keepLava=false) {
        suspend(); clock={}; packet.clock={}; beamRecovery=0;
        if (!keepLava) { lavaActive=false; lavaRecovery=0; packet.lavaTicks=0; }
    }
    void tick_lava() {
        auto* player=daAlink_getAlinkActorClass();
        if (!lavaActive || !player) return;
        packet.lava=true; ++packet.lavaTicks;
        if (lavaRecovery>0) --lavaRecovery;
        if (lavaContact.ChkAtHit() && lavaContact.GetAtHitAc()==player) lavaRecovery=45;
        lavaContact.OffAtSetBit(); lavaContact.ClrAtHit();
        if (lavaRecovery || player->getDamageWaitTimer()) return;
        // Register only the nearest edge, avoiding dozens of global collision
        // entries and overlapping hits. Geometry and collision share the rim.
        float nearest=1.0e20f;
        cXyz start,end;
        for (unsigned i=0;i<TrialPacket::rimSegments;++i) {
            const unsigned next=(i+1)%TrialPacket::rimSegments;
            if (!packet.rimValid[i] || !packet.rimValid[next]) continue;
            const cXyz a=(packet.rimOuter[i]+packet.rimInner[i])*0.5f;
            const cXyz b=(packet.rimOuter[next]+packet.rimInner[next])*0.5f;
            const cXyz d=b-a, p=player->current.pos-a;
            const float length=d.x*d.x+d.z*d.z;
            const float t=length>0 ? std::clamp((p.x*d.x+p.z*d.z)/length,0.0f,1.0f) : 0;
            const float distance=(player->current.pos-(a+d*t)).absXZ();
            if (distance<nearest) { nearest=distance; start=a; end=b; }
        }
        if (nearest>120) return;
        static_cast<cM3dGCps*>(&lavaContact)->Set(start,end,40);
        cXyz direction=center-player->current.pos; lavaContact.SetAtVec(direction);
        lavaContact.OnAtSetBit(); dComIfG_Ccsp()->Set(&lavaContact);
    }
    void reset_beam_aim() {
        auto* player=daAlink_getAlinkActorClass();
        for (unsigned i=0;i<2;++i) {
            cXyz toward=player->current.pos-eyePos[i];
            if (toward.absXZ()<1) toward=cXyz(1,0,0);
            const float length=toward.absXZ();
            const cXyz forward(toward.x/length,0,toward.z/length);
            const cXyz side(forward.z,0,-forward.x);
            float safest=-1;
            // Try both sides and both depth offsets. Near a wall, clipping a
            // single candidate could otherwise put the starting spot on Link.
            for (unsigned candidate=0;candidate<4;++candidate) {
                const unsigned pick=(candidate+i*2)%4;
                cXyz target=player->current.pos+(pick<2 ? side : forward)*(pick%2 ? -600 : 600);
                target.y=center.y+40;
                cXyz start=center+cXyz(0,40,0);
                dBgS_LinChk check; check.Set(&start,&target,owner);
                if (dComIfG_Bgsp().LineCross(&check)) {
                    target=check.GetCross();
                    const cXyz inward=start-target;
                    target+=inward*(80.0f/std::max(80.0f,inward.absXZ()));
                }
                target.y=center.y+4;
                const cXyz ray=target-eyePos[i], toPlayer=player->current.pos-eyePos[i];
                const float squared=ray.x*ray.x+ray.z*ray.z;
                const float t=squared>0 ? std::clamp((ray.x*toPlayer.x+ray.z*toPlayer.z)/squared,0.0f,1.0f) : 0;
                const float clearance=(player->current.pos-(eyePos[i]+ray*t)).absXZ();
                if (clearance>safest) { safest=clearance; aim[i]=target; }
            }
            beamModels[i].reset();
        }
    }
    ~ShadeTrials() {
        cancel();
        for (const auto id:anchors) remove_actor(id);
        if (effectHeap) mDoExt_destroySolidHeap(effectHeap);
        dComIfG_resDelete(&eyeArchive,"E_bm6");
        dComIfG_resDelete(&fireArchive,"E_fm");
    }
    void begin(daNpc_Kn_c* boss) {
        cancel(true); clock.begin(sBattle.trial);
        origin=boss->current.pos;
        origin.y=center.y;
        audio.begin(clock.kind,boss->current.pos+cXyz(0,110,0),eyePos);
        // Sample the native motion over six seconds. The outer edge reaches
        // even the farthest wall when Shade casts from an off-center position.
        const float nativeRadius=31.5f*std::max(1.0f,std::min(47.5f,fire[1].bck.getEndFrame()));
        fireScale=2.5f*(radius+(origin-center).absXZ())/nativeRadius;
        for (auto& effect:fire) effect.reset();
        reset_beam_aim();
    }
    bool tick(daNpc_Kn_c* boss) {
        auto* player=daAlink_getAlinkActorClass();
        if (!clock.active() || !player) return false;
        visible=true;
        status.Move();
        if (clock.kind==shade::Trial::Shield) {
            if (!clock.broken && shield.ChkTgHit()) {
                auto* hit=shield.GetTgHitObj();
                clock.broken=hit && hit->ChkAtType(AT_TYPE_BOMB|AT_TYPE_IRON_BALL);
            }
            audio.update_shield(clock.broken,boss->current.pos+cXyz(0,110,0));
            shield.ClrTgHit();
            if (clock.broken) shield.OffTgSetBit();
            else {
                shield.OnTgSetBit(); shield.SetC(boss->current.pos+cXyz(0,110,0));
                dComIfG_Ccsp()->Set(&shield);
            }
        } else if (clock.kind==shade::Trial::Fire) {
            if (clock.ticks==shade::fire_warning_ticks) {
                cinema_pose(boss,25);
            }
            audio.update_fire(clock.ticks>=shade::fire_warning_ticks,origin);
            if (clock.ticks>=shade::fire_warning_ticks) {
                const float progress=shade::fire_progress(clock.ticks);
                const float fade=std::max(0,clock.ticks-shade::fire_warning_ticks-shade::fire_travel_ticks+1);
                fire[0].sample(42*progress+fade);
                fire[1].sample(std::min(47.5f,fire[1].bck.getEndFrame())*progress+fade*0.5f);
                flame.ClrAtHit();
                if (clock.fire_live()) {
                    flame.OnAtSetBit(); flame.SetC(origin);
                    flame.SetR(31.5f*fire[1].bck.getFrame()*fireScale/2.5f);
                    dComIfG_Ccsp()->Set(&flame);
                } else flame.OffAtSetBit();
            }
        } else if (clock.kind==shade::Trial::Eyes) {
            eyeColor.play();
            if (beamRecovery>0) --beamRecovery;
            bool contacted=false;
            for (auto& beam:beams) {
                contacted|=beam.ChkAtHit() && beam.GetAtHitAc()==player;
                beam.OffAtSetBit(); beam.ClrAtHit();
            }
            if (contacted) { beamRecovery=shade::beam_recovery_ticks; reset_beam_aim(); }
            beamVisible=clock.beam_live();
            const bool damaging=beamVisible && !beamRecovery && !player->getDamageWaitTimer();
            // One capsule per simulation step prevents two eyes delivering a
            // simultaneous hit. Recovery disarms damage, not rendering; restart
            // from an offset floor point, then visibly sweep back toward Link.
            const unsigned firingEye=clock.eyes==3 ? (clock.ticks&1u) : clock.eyes==1 ? 0 : 1;
            for (unsigned i=0;i<2;++i) if (clock.eyes&(1u<<i)) {
                auto& target=eyeTargets[i];
                if (target.ChkTgHit()) {
                    auto* hit=target.GetTgHitObj();
                    if (hit && hit->ChkAtType(AT_TYPE_ARROW)) {
                        clock.hit_eye(i);
                        audio.update_beam(i,false,eyePos[i],packet.ends[i],0,player->current.pos);
                        target.OffTgSetBit(); target.ClrTgHit();
                        beams[i].OffAtSetBit(); beams[i].ClrAtHit();
                        mDoAud_seStart(Z2SE_EN_BM_EYE_BREAK,&eyePos[i],0,dComIfGp_getReverb(51));
                        continue;
                    }
                }
                target.ClrTgHit(); target.OnTgSetBit(); target.SetC(eyePos[i]);
                dComIfG_Ccsp()->Set(&target);
                const cXyz destination(player->current.pos.x,center.y+4,player->current.pos.z);
                const cXyz delta=destination-aim[i];
                const float distance=delta.abs();
                if (beamVisible && !beamRecovery && distance>0.001f) aim[i]+=delta*(std::min(10.0f,distance)/distance);
                cXyz end=aim[i];
                dBgS_LinChk check; check.Set(&eyePos[i],&end,owner);
                if (dComIfG_Bgsp().LineCross(&check)) end=check.GetCross();
                packet.ends[i]=end;
                if (beamVisible) {
                    beamModels[i].play();
                    if (damaging && i==firingEye) {
                        const cXyz hitEnd=eyePos[i]+(end-eyePos[i])*beamModels[i].reach();
                        beams[i].OnAtSetBit();
                        static_cast<cM3dGCps*>(&beams[i])->Set(eyePos[i],hitEnd,20);
                        cXyz direction=hitEnd-eyePos[i]; beams[i].SetAtVec(direction);
                        dComIfG_Ccsp()->Set(&beams[i]);
                    }
                }
                audio.update_beam(i,beamVisible,eyePos[i],end,beamModels[i].reach(),player->current.pos);
            }
        } else if (clock.kind==shade::Trial::Wind) {
            audio.update_wind(!clock.windReleased && clock.ticks>=shade::wind_warning_ticks,player->current.pos);
            if (!clock.windReleased && !player->checkEquipHeavyBoots()) {
                const float power=shade::wind_strength(clock.ticks);
                if (power>0) player->setOutPower(power,cLib_targetAngleY(&center,&player->current.pos),1);
            }
        }
        packet.clock=clock; packet.center=center; packet.boss=boss->current.pos;
        packet.radius=radius;
        const bool done=clock.tick();
        if (done && clock.kind==shade::Trial::Fire) { lavaActive=true; tick_lava(); }
        return done;
    }
    void draw(dKy_tevstr_c& light) {
        if (packet.lava) { dComIfGd_setList(); j3dSys.getDrawBuffer(1)->entryImm(&packet,0); }
        if (!clock.active() || !visible) return;
        if (clock.kind==shade::Trial::Fire && clock.ticks>shade::fire_warning_ticks)
            for (auto& effect:fire) effect.draw(origin,fireScale,light);
        if (clock.kind==shade::Trial::Eyes) for (unsigned i=0;i<2;++i) if (clock.eyes&(1u<<i)) {
            const auto yaw=cLib_targetAngleY(&eyePos[i],&aim[i]);
            // The raised native eye is at (0,430,80). Anchor it to the wall;
            // submit its named material regardless of mesh-to-joint layout.
            mDoMtx_stack_c::transS(eyePos[i]); mDoMtx_stack_c::YrotM(yaw);
            mDoMtx_stack_c::transM(0,-430,-80);
            eyes[i]->setBaseTRMtx(mDoMtx_stack_c::get());
            g_env_light.setLightTevColorType_MAJI(eyes[i],&light);
            auto* data=eyes[i]->getModelData();
            eyePose.entry(data); eyeColor.entry(data);
            mDoExt_modelUpdateDL(eyes[i]);
            eyePose.remove(data); eyeColor.remove(data);
            if (beamVisible) beamModels[i].draw(eyePos[i],packet.ends[i],light);
        }
        if (!packet.lava) { dComIfGd_setList(); j3dSys.getDrawBuffer(1)->entryImm(&packet,0); }
    }
};
ShadeTrials* sTrials=nullptr;
void suspend_trial() { if (sTrials) sTrials->suspend(); }
void cancel_trial() { if (sTrials) sTrials->cancel(); }
void finish_trial() { if (sTrials) sTrials->cancel(true); }
void tick_arena_hazards() { if (sTrials) sTrials->tick_lava(); }
void begin_trial(daNpc_Kn_c* boss) { if (sTrials) sTrials->begin(boss); }
bool tick_trial(daNpc_Kn_c* boss) { return sTrials && sTrials->tick(boss); }

void sync_trial_position(daNpc_Kn_c* boss) {
    if (sTrials && sTrials->visible && sTrials->clock.kind==shade::Trial::Shield) {
        sTrials->shield.SetC(boss->current.pos+cXyz(0,110,0));
        sTrials->packet.boss=boss->current.pos;
    }
}
