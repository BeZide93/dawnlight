// Typed member pointers expose the NPC base's protected runtime fields without
// changing host headers, hard-coded offsets or casting to a fictitious subclass.
struct ShadeWolfFields : daNpc_GWolf_c {
    static constexpr auto mAnm_p_member=&ShadeWolfFields::mAnm_p;
    static constexpr auto mHide_member=&ShadeWolfFields::mHide;
    static constexpr auto mCurAngle_member=&ShadeWolfFields::mCurAngle;
    static constexpr auto mTwilight_member=&ShadeWolfFields::mTwilight;
    static constexpr auto mOrderEvtNo_member=&ShadeWolfFields::mOrderEvtNo;
    static constexpr auto mCcStts_member=&ShadeWolfFields::mCcStts;
    static constexpr auto mBtkAnm_member=&ShadeWolfFields::mBtkAnm;
    static constexpr auto mBrkAnm_member=&ShadeWolfFields::mBrkAnm;
};
// The same type-2 Golden Wolf and MOT_HOWLB used by KN's second encounter.
// Keep the actor hidden and loaded during combat for an immediate outro swap.
struct ShadeWolf {
    ActorId id=kNone;
    bool active=false,arrival=false,swapped=false,howling=false,ownsFlash=false;
    int flashStart=-1,howlStart=0;
    float opacity=0,flashRate=0;

    daNpc_GWolf_c* actor() const { return static_cast<daNpc_GWolf_c*>(actor_by_id(id)); }
    bool ready() const { auto* wolf=actor(); return wolf && (wolf->*ShadeWolfFields::mAnm_p_member); }
    bool visible() const { return active && opacity>0; }
    bool hides_shade() const { return active && (arrival ? !swapped : swapped); }
    bool flash_owned() const {
        const auto& color=mDoGph_gInf_c::getFadeColor();
        return ownsFlash && mDoGph_gInf_c::isFade() &&
            mDoGph_gInf_c::getFadeSpeed()==0 && color.r==255 && color.g==255 && color.b==255 &&
            mDoGph_gInf_c::getFadeRate()==flashRate;
    }
    void flash(float rate) {
        if (!ownsFlash) {
            GXColor white{255,255,255,255};
            mDoGph_gInf_c::fadeOut(0,white);
            ownsFlash=true;
        } else if (!flash_owned()) return; // a scene transition replaced our fade
        flashRate=rate;
        mDoGph_gInf_c::setFadeRate(rate);
    }
    void end_flash() {
        if (flash_owned()) mDoGph_gInf_c::offFade();
        ownsFlash=false;flashRate=0;
    }
    void end_visual() {
        end_flash(); active=false;opacity=0;
        if (auto* wolf=actor()) {
            (wolf->*ShadeWolfFields::mHide_member)=true;
            fopAcM_OffStatus(wolf,fopAcStts_NOPAUSE_e);
        }
    }
    void clear() {
        end_visual();
        // Keep the identity while a queued deletion is pending: hooks must still
        // suppress its story AI/draw. A new prepare cancels/removes it first.
        remove_actor(id);
    }
    bool prepare(const cXyz& pos,s16 yaw) {
        remove_actor(id);id=kNone;
        return spawn(fpcNm_NPC_GWOLF_e,pos,yaw,0xffffff02,id)==MOD_OK;
    }
    void begin(daNpc_Kn_c* shade,bool arriving) {
        end_flash();active=true;arrival=arriving;swapped=false;howling=false;
        flashStart=arriving ? -1 : 0;howlStart=0;opacity=arriving ? 1.0f : 0.0f;
        shade->field_0x16f4.set(1,1,1);
        shade->field_0x170c=0;shade->field_0x170d=0;
        shade->mNoDraw=arriving;
        if (auto* wolf=actor()) {
            fopAcM_OnStatus(wolf,fopAcStts_NOPAUSE_e);
            wolf->current.pos=shade->current.pos;wolf->old.pos=wolf->current.pos;
            wolf->home.pos=wolf->current.pos;
            (wolf->*ShadeWolfFields::mCurAngle_member).set(0,shade->shape_angle.y,0);
            wolf->current.angle=wolf->shape_angle=(wolf->*ShadeWolfFields::mCurAngle_member);
            wolf->setMotion(0,0,1); // already seated before the camera moves
            (wolf->*ShadeWolfFields::mHide_member)=!arriving;
        }
    }
    bool tick(daNpc_Kn_c* shade,int elapsed) {
        if (!active) return true;
        auto* wolf=actor();
        if (arrival && flashStart<0) {
            if (elapsed>=shade::wolf_pan_ticks && !howling) {
                howling=true;howlStart=elapsed;
                if (wolf) wolf->setMotion(4,3,1); // WL_HOWLB; native voice at frame 1
            }
            // Follow the loaded clip rather than assuming a fixed asset length.
            // Freeze at its final frame to avoid a second howl before the flash.
            const bool done=wolf && wolf->mAnm==7 && (wolf->*ShadeWolfFields::mAnm_p_member) &&
                (wolf->*ShadeWolfFields::mAnm_p_member)->getEndFrame()>1 &&
                (wolf->*ShadeWolfFields::mAnm_p_member)->getFrame()>=(wolf->*ShadeWolfFields::mAnm_p_member)->getEndFrame()-2;
            if (howling && (done || elapsed-howlStart>=180)) {
                flashStart=elapsed;
                if (wolf && (wolf->*ShadeWolfFields::mAnm_p_member)) (wolf->*ShadeWolfFields::mAnm_p_member)->setPlaySpeed(0);
            }
        }
        if (flashStart>=0) {
            const int t=elapsed-flashStart;
            const int swapTick=shade::flash_rise_ticks+shade::flash_hold_ticks;
            if (t<=shade::flash_total_ticks) {
                const float white=t<shade::flash_rise_ticks ? t/float(shade::flash_rise_ticks) :
                    t<=swapTick ? 1.0f : 1-(t-swapTick)/float(shade::flash_fall_ticks);
                flash(std::clamp(white,0.0f,1.0f));
                if (t>=swapTick && !swapped) {
                    swapped=true;opacity=arrival ? 0.0f : 1.0f;
                    if (wolf) (wolf->*ShadeWolfFields::mHide_member)=arrival;
                }
            } else {
                end_flash();
                if (arrival) return true;
                const int fade=t-shade::flash_total_ticks-shade::wolf_hold_ticks;
                const float fraction=std::clamp(fade/float(shade::wolf_fade_ticks),0.0f,1.0f);
                opacity=1-fraction*fraction*(3-2*fraction);
                if (fraction>=1) { if (wolf) (wolf->*ShadeWolfFields::mHide_member)=true; return true; }
            }
        }
        shade->mNoDraw=hides_shade();
        return false;
    }
} sShadeWolf;

DEFINE_HOOK(&daNpc_GWolf_c::main, ShadeWolfMainHook);
DEFINE_HOOK(&daNpc_GWolf_c::Execute, ShadeWolfExecuteHook);
DEFINE_HOOK(&daNpc_GWolf_c::Draw, ShadeWolfDrawHook);

HookAction wolf_main(ModContext*,void* args,void* result,void*) {
    auto* wolf=mods::arg<daNpc_GWolf_c*>(args,0);
    if (fopAcM_GetID(wolf)!=sShadeWolf.id) return HOOK_CONTINUE;
    // Retain native skeletal/texture animation and the howl's frame-synced
    // sound; never run story events, warps, switches or proximity attacks.
    (wolf->*ShadeWolfFields::mHide_member)=!sShadeWolf.visible();(wolf->*ShadeWolfFields::mTwilight_member)=false;
    wolf->attention_info.flags=0;(wolf->*ShadeWolfFields::mOrderEvtNo_member)=0;
    wolf->speedF=0;wolf->speed.zero();wolf->gravity=0;
    (wolf->*ShadeWolfFields::mCcStts_member).ClrCcMove();
    wolf->setLookMode(0);
    wolf->playMotion();
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction wolf_execute(ModContext*,void* args,void* result,void*) {
    auto* wolf=mods::arg<daNpc_GWolf_c*>(args,0);
    if (fopAcM_GetID(wolf)!=sShadeWolf.id) return HOOK_CONTINUE;
    // Hidden wolves still need one native Execute during create to pose the
    // sitting model. Pause all later animation together with the cutscene.
    if (dComIfGp_isPauseFlag() || ui_document_visible() || sStopping) {
        *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

// Fade the native textured/skeletal model in the translucent draw list. Append
// one GPU stage which multiplies the existing alpha by a constant texture;
// no edits to shared materials, BRK registers, archives or display-list sizes.
struct ShadeWolfFadePacket : J3DPacket {
    J3DModel* model=nullptr;
    float alpha=1;
    alignas(32) std::array<u8,64> texels{};
    void draw() override {
        if (!model || alpha<=0) return;
        for (int i=0;i<16;++i) {
            texels[i*2]=static_cast<u8>(255*alpha);texels[i*2+1]=255;
            texels[32+i*2]=texels[33+i*2]=255;
        }
        DCStoreRange(texels.data(),texels.size());
        GXTexObj texture;
        GXInitTexObj(&texture,texels.data(),4,4,GX_TF_RGBA8,GX_CLAMP,GX_CLAMP,GX_FALSE);
        GXInitTexObjLOD(&texture,GX_NEAR,GX_NEAR,0,0,0,GX_FALSE,GX_FALSE,GX_ANISO_1);
        GXInvalidateTexAll();
        auto* data=model->getModelData();
        j3dSys.setTexture(data->getTexture());
        for (u16 i=0;i<data->getMaterialNum();++i) {
            auto* material=data->getMaterialNodePointer(i);
            auto* mat=model->getMatPacket(i);
            auto* shape=model->getShapePacket(material->getShape()->getIndex());
            auto* tev=material->getTevBlock();
            const u8 count=tev->getTevStageNum();
            std::array<bool,8> used{};
            for (u8 j=0;j<count;++j) {
                const auto map=tev->getTevOrder(j)->getTexMap();
                if (map<used.size()) used[map]=true;
            }
            int freeMap=7;while (freeMap>=0 && used[freeMap]) --freeMap;
            if (count>=16 || freeMap<0) continue; // GWolf uses far fewer stages/maps
            material->load();
            mat->callDL();
            model->getVertexBuffer()->setArray();
            shape->getShape()->loadPreDrawSetting();
            if (shape->getDisplayListObj()) shape->getDisplayListObj()->callDL();
            GXLoadTexObj(&texture,static_cast<GXTexMapID>(freeMap));
            const auto stage=static_cast<GXTevStageID>(count);
            GXSetNumTevStages(count+1);
            GXSetTevOrder(stage,GX_TEXCOORD0,static_cast<GXTexMapID>(freeMap),GX_COLOR_NULL);
            GXSetTevDirect(stage);
            GXSetTevSwapMode(stage,GX_TEV_SWAP0,GX_TEV_SWAP0);
            GXSetTevColorIn(stage,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_CPREV);
            GXSetTevColorOp(stage,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
            GXSetTevAlphaIn(stage,GX_CA_ZERO,GX_CA_APREV,GX_CA_TEXA,GX_CA_ZERO);
            GXSetTevAlphaOp(stage,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
            GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_COPY);
            GXSetZMode(GX_TRUE,GX_LEQUAL,GX_FALSE);
            GXSetZCompLoc(GX_FALSE);
            GXSetAlphaCompare(GX_GREATER,0,GX_AOP_AND,GX_ALWAYS,0);
            shape->drawFast();
            J3DShape::resetVcdVatCache();
        }
    }
} sShadeWolfFade;

HookAction wolf_draw(ModContext*,void* args,void* result,void*) {
    auto* wolf=mods::arg<daNpc_GWolf_c*>(args,0);
    if (fopAcM_GetID(wolf)!=sShadeWolf.id) return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;
    if (!sShadeWolf.visible()) return HOOK_SKIP_ORIGINAL;
    (wolf->*ShadeWolfFields::mHide_member)=false;
    if (sShadeWolf.opacity>=1) return HOOK_CONTINUE;
    auto* model=(wolf->*ShadeWolfFields::mAnm_p_member)->getModel();auto* data=model->getModelData();
    g_env_light.settingTevStruct(5,&wolf->current.pos,&wolf->tevStr);
    g_env_light.setLightTevColorType_MAJI(model,&wolf->tevStr);
    (wolf->*ShadeWolfFields::mBtkAnm_member).entry(data);(wolf->*ShadeWolfFields::mBrkAnm_member).entry(data);
    model->calcMaterial();model->diff();model->viewCalc();
    (wolf->*ShadeWolfFields::mBtkAnm_member).remove(data);(wolf->*ShadeWolfFields::mBrkAnm_member).remove(data);
    sShadeWolfFade.model=model;sShadeWolfFade.alpha=sShadeWolf.opacity;
    dComIfGd_setList();
    j3dSys.getDrawBuffer(1)->entryImm(&sShadeWolfFade,0);
    return HOOK_SKIP_ORIGINAL;
}
