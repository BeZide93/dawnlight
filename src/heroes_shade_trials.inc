// Arena-owned intermissions. Included inside heroes_shade_encounter.cpp's
// private namespace so actor ownership and cancellation use the same helpers.
// Game models are resolved from the player's archives; none are bundled here.
class TrialEyeModel final : public J3DModel {
public:
    void calc() override {
        // A separately spawned native Beamos may have installed callbacks on
        // this archive's joints. They dereference a daObjLv6Bm_c user pointer,
        // which our head-only model does not have. Suppress only during this
        // synchronous calculation and restore before another model can run.
        std::array<J3DJointCallBack,6> callbacks{};
        for (unsigned i=0;i<callbacks.size();++i) {
            auto* joint=getModelData()->getJointNodePointer(i);
            callbacks[i]=joint->getCallBack(); joint->setCallBack(nullptr);
        }
        J3DModel::calc();
        for (unsigned i=0;i<callbacks.size();++i)
            getModelData()->getJointNodePointer(i)->setCallBack(callbacks[i]);
    }
    // Submit only the native Beamos head. Never toggle shared shape flags: that
    // would also affect other Beamos and models already queued for this frame.
    void entry() override {
        j3dSys.setModel(this);
        j3dSys.offFlag(J3DSysFlag_SkinPosCpu);
        j3dSys.offFlag(J3DSysFlag_SkinNrmCpu);
        getModelData()->syncJ3DSysFlags();
        j3dSys.setTexture(getModelData()->getTexture());
        getModelData()->getJointNodePointer(BM6_JNT_HEAD_e)->entryIn();
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
    void play(float rate) {
        bck.setPlaySpeed(rate); bck.play(); btk.play(); if (colored) brk.play();
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

class TrialPacket : public J3DPacket {
public:
    shade::TrialClock clock;
    cXyz center{0,0,0}, boss{0,0,0};
    std::array<cXyz,2> eyes{}, ends{};
    float radius=1000;
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
        if (!clock.active()) return;
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
        } else if (clock.kind==shade::Trial::Wind) {
            // Outward-moving ribbons make the direction and remaining gust visible.
            for (unsigned band=0;band<6;++band) {
                const float t=std::fmod(clock.ticks/45.0f+band/6.0f,1.0f);
                ring(center+cXyz(0,30+band*24,0),t*radius,8,{180,220,230,static_cast<u8>((1-t)*95)});
            }
        } else if (clock.kind==shade::Trial::Eyes) {
            for (unsigned i=0;i<2;++i) if (clock.eyes&(1u<<i)) {
                // Persistent beam geometry and capsule use the same endpoints.
                if (!clock.beam_live()) continue;
                const cXyz delta=ends[i]-eyes[i];
                const float xz=std::max(1.0f,delta.absXZ());
                const cXyz right(delta.z/xz,0,-delta.x/xz);
                for (unsigned shell=0;shell<2;++shell) {
                    const float r=shell ? 15 : 4;
                    GXBegin(GX_QUADS,GX_VTXFMT0,8*4);
                    for (unsigned side=0;side<8;++side) for (unsigned k=0;k<4;++k) {
                        const float a=(side+(k==1 || k==2))*6.2831853072f/8;
                        vertex((k>=2 ? ends[i] : eyes[i])+right*(r*std::cos(a))+cXyz(0,r*std::sin(a),0),
                            {100,220,255,static_cast<u8>(shell ? 65 : 240)});
                    }
                    GXEnd();
                }
            }
        }
        j3dSys.reinitGX();
    }
};

struct ShadeTrials {
    request_of_phase_process_class fireArchive{},eyeArchive{};
    std::array<TrialFireModel,2> fire;
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
    TrialPacket packet;
    shade::TrialClock clock;
    fopAc_ac_c* owner=nullptr;
    cXyz center{0,0,0}, origin{0,0,0};
    float radius=0, fireScale=2.5f;
    bool placed=false;
    bool visible=false;
    MessageId hint=0;
    bool hintOpened=false;

    int load() {
        int result=dComIfG_resLoad(&fireArchive,"E_fm");
        if (result!=cPhs_COMPLEATE_e) return result;
        return dComIfG_resLoad(&eyeArchive,"E_bm6");
    }
    bool heap() {
        for (unsigned i=0;i<2;++i) if (!fire[i].create(i)) return false;
        auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BMD_BM6_e));
        if (!data || data->getJointNum()!=6 || !data->getJointNodePointer(BM6_JNT_HEAD_e)->getMesh()) return false;
        auto* pose=static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BCK_BM6_UP_e));
        auto* color=static_cast<J3DAnmTevRegKey*>(dComIfG_getObjectRes("E_bm6",dRes_INDEX_E_BM6_BRK_BM6_TURN_e));
        if (!pose || !color || !eyePose.init(pose,TRUE,J3DFrameCtrl::EMode_NONE,0,0,-1,false) ||
            !eyeColor.init(data,color,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1)) return false;
        eyePose.setFrame(eyePose.getEndFrame());
        const u32 flags=data->getMaterialNodePointer(0)->getSharedDisplayListObj() && data->isLocked() ?
            J3DMdlFlag_UseSharedDL : J3DMdlFlag_DifferedDLBuffer;
        for (auto*& eye:eyes) {
            eye=JKR_NEW TrialEyeModel;
            if (!eye || eye->entryModelData(data,flags,1)!=kJ3DError_Success ||
                (flags==J3DMdlFlag_DifferedDLBuffer && eye->newDifferedDisplayList(0x11000284)!=kJ3DError_Success)) return false;
            eye->lock();
        }
        return true;
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
        pos=check.GetCross()-direction*inset;
        return (pos-start).absXZ()>300;
    }
    bool place() {
        if (!placed) {
            // Avoid the doors: four diagonal wall targets, two side-wall eyes.
            for (unsigned i=0;i<4;++i)
                if (!wall(static_cast<s16>(0x2000+i*0x4000),650,25,anchorPos[i])) return false;
            for (unsigned i=0;i<2;++i)
                if (!wall(static_cast<s16>(0x4000+i*0x8000),400,100,eyePos[i])) return false;
            for (unsigned i=0;i<16;++i) {
                cXyz point;
                if (wall(static_cast<s16>(i*0x1000),100,0,point)) radius=std::max(radius,(point-center).absXZ());
            }
            radius+=150;
            placed=true;
        }
        bool ready=true;
        for (unsigned i=0;i<4;++i) {
            if (!pending_or_live(anchors[i])) {
                const auto& p=anchorPos[i];
                const ActorSpawnParams params{.parameters=0,.argument=-1,.room_num=51,
                    .position={p.x,p.y,p.z},.angle={0,cLib_targetAngleY(&p,&center),0},
                    .scale={1,1,1},.create_function=nullptr};
                create_standalone_actor(fpcNm_Obj_HsTarget_e,params,anchors[i]);
            }
            ready=actor_by_id(anchors[i]) && ready;
        }
        return ready;
    }
    void close_hint() {
        auto* msg=dMsgObject_getMsgObjectClass();
        if (hint && msg && msg->msg_idx==hint) {
            if (msg->getStatusLocal()==1 && !hintOpened) msg->setStatusLocal(19);
            if (!hintOpened || msg->getStatusLocal()!=1) msg->onKillMessageFlagLocal();
        }
        hint=0; hintOpened=false;
    }
    void suspend() {
        visible=false;
        shield.OffTgSetBit(); shield.ClrTgHit(); flame.OffAtSetBit(); flame.ClrAtHit();
        for (unsigned i=0;i<2;++i) {
            eyeTargets[i].OffTgSetBit(); eyeTargets[i].ClrTgHit();
            beams[i].OffAtSetBit(); beams[i].ClrAtHit();
        }
    }
    void cancel() {
        suspend(); clock={}; packet.clock={}; close_hint();
    }
    ~ShadeTrials() {
        cancel();
        for (const auto id:anchors) remove_actor(id);
        dComIfG_resDelete(&eyeArchive,"E_bm6");
        dComIfG_resDelete(&fireArchive,"E_fm");
    }
    void begin(daNpc_Kn_c* boss) {
        cancel(); clock.begin(sBattle.trial);
        origin=boss->current.pos;
        origin.y=center.y;
        // The native deceleration (2.5 -> 0.5, step 0.08) reaches frame 47.5
        // on the last damaging tick. Scale to cover the farthest wall even
        // when Shade stood off-center, and respect a shorter archive clip.
        const float nativeRadius=31.5f*std::max(1.0f,std::min(47.5f,fire[1].bck.getEndFrame()));
        fireScale=2.5f*std::max(1.0f,(radius+(origin-center).absXZ())/nativeRadius);
        for (auto& effect:fire) effect.reset();
        auto* player=daAlink_getAlinkActorClass();
        for (unsigned i=0;i<2;++i) aim[i]=player->current.pos+cXyz(0,70,0);
        const auto message=sTrialLines[static_cast<unsigned>(clock.kind)-1].id();
        const auto id=fopMsgM_messageSetDemo(message);
        if (id && id!=kNone) hint=message;
    }
    bool tick(daNpc_Kn_c* boss) {
        auto* player=daAlink_getAlinkActorClass();
        if (!clock.active() || !player) return false;
        if (auto* msg=dMsgObject_getMsgObjectClass(); hint && msg && msg->msg_idx==hint && msg->getStatusLocal()>1) hintOpened=true;
        visible=true;
        status.Move();
        if (clock.kind==shade::Trial::Shield) {
            if (!clock.broken && shield.ChkTgHit()) {
                auto* hit=shield.GetTgHitObj();
                clock.broken=hit && hit->ChkAtType(AT_TYPE_BOMB|AT_TYPE_IRON_BALL);
                if (clock.broken) mDoAud_seStart(Z2SE_EN_BM_EYE_BREAK,&origin,0,dComIfGp_getReverb(51));
            }
            shield.ClrTgHit();
            if (clock.broken) shield.OffTgSetBit();
            else {
                shield.OnTgSetBit(); shield.SetC(boss->current.pos+cXyz(0,110,0));
                dComIfG_Ccsp()->Set(&shield);
            }
        } else if (clock.kind==shade::Trial::Fire) {
            if (clock.ticks==shade::fire_warning_ticks) {
                cinema_pose(boss,25);
                mDoAud_seStart(Z2SE_EN_FM_BLAST,&origin,0,dComIfGp_getReverb(51));
            }
            if (clock.ticks>=shade::fire_warning_ticks) {
                const int frame=clock.ticks-shade::fire_warning_ticks;
                fire[0].play(1); fire[1].play(std::max(0.5f,2.5f-0.08f*frame));
                flame.ClrAtHit();
                if (clock.fire_live()) {
                    flame.OnAtSetBit(); flame.SetC(origin);
                    flame.SetR(31.5f*fire[1].bck.getFrame()*fireScale/2.5f);
                    dComIfG_Ccsp()->Set(&flame);
                } else flame.OffAtSetBit();
            }
        } else if (clock.kind==shade::Trial::Eyes) {
            eyeColor.play();
            for (unsigned i=0;i<2;++i) if (clock.eyes&(1u<<i)) {
                auto& target=eyeTargets[i];
                if (target.ChkTgHit()) {
                    auto* hit=target.GetTgHitObj();
                    if (hit && hit->ChkAtType(AT_TYPE_ARROW)) {
                        clock.hit_eye(i); target.OffTgSetBit(); target.ClrTgHit();
                        beams[i].OffAtSetBit(); beams[i].ClrAtHit();
                        mDoAud_seStart(Z2SE_EN_BM_EYE_BREAK,&eyePos[i],0,dComIfGp_getReverb(51));
                        continue;
                    }
                }
                target.ClrTgHit(); target.OnTgSetBit(); target.SetC(eyePos[i]);
                dComIfG_Ccsp()->Set(&target);
                const cXyz destination=player->current.pos+cXyz(0,70,0);
                const cXyz delta=destination-aim[i];
                const float distance=delta.abs();
                if (distance>0.001f) aim[i]+=delta*(std::min(15.0f,distance)/distance);
                cXyz end=aim[i];
                dBgS_LinChk check; check.Set(&eyePos[i],&end,owner);
                if (dComIfG_Bgsp().LineCross(&check)) end=check.GetCross();
                packet.ends[i]=end;
                if (clock.beam_live()) {
                    beams[i].ClrAtHit(); beams[i].OnAtSetBit();
                    static_cast<cM3dGCps*>(&beams[i])->Set(eyePos[i],end,18);
                    cXyz direction=end-eyePos[i]; beams[i].SetAtVec(direction);
                    dComIfG_Ccsp()->Set(&beams[i]);
                    mDoAud_seStartLevel(Z2SE_EN_BM_BEAM2,&eyePos[i],0,dComIfGp_getReverb(51));
                }
            }
        } else if (clock.kind==shade::Trial::Wind && !player->checkEquipHeavyBoots()) {
            player->setOutPower(18.0f,cLib_targetAngleY(&center,&player->current.pos),0);
        }
        packet.clock=clock; packet.center=center; packet.boss=boss->current.pos;
        packet.eyes=eyePos; packet.radius=radius;
        return clock.tick();
    }
    void draw(dKy_tevstr_c& light) {
        if (!clock.active() || !visible) return;
        if (clock.kind==shade::Trial::Fire && clock.ticks>shade::fire_warning_ticks)
            for (auto& effect:fire) effect.draw(origin,fireScale,light);
        if (clock.kind==shade::Trial::Eyes) for (unsigned i=0;i<2;++i) if (clock.eyes&(1u<<i)) {
            const auto yaw=cLib_targetAngleY(&eyePos[i],&aim[i]);
            // The native eye is at (0,430,80). Anchor that point to the wall;
            // only the head joint is submitted, not the tower underneath it.
            mDoMtx_stack_c::transS(eyePos[i]); mDoMtx_stack_c::YrotM(yaw);
            mDoMtx_stack_c::transM(0,-430,-80);
            eyes[i]->setBaseTRMtx(mDoMtx_stack_c::get());
            g_env_light.setLightTevColorType_MAJI(eyes[i],&light);
            auto* data=eyes[i]->getModelData();
            eyePose.entry(data); eyeColor.entry(data);
            mDoExt_modelUpdateDL(eyes[i]);
            eyePose.remove(data); eyeColor.remove(data);
        }
        dComIfGd_setList(); j3dSys.getDrawBuffer(1)->entryImm(&packet,0);
    }
};
ShadeTrials* sTrials=nullptr;
void suspend_trial() { if (sTrials) sTrials->suspend(); }
void cancel_trial() { if (sTrials) sTrials->cancel(); }
void begin_trial(daNpc_Kn_c* boss) { if (sTrials) sTrials->begin(boss); }
bool tick_trial(daNpc_Kn_c* boss) { return sTrials && sTrials->tick(boss); }
