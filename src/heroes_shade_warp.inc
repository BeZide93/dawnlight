// Link's CoWarp visual path, applied to private Shade models. Never enter a
// player procedure: that would alter Link's equipment, controls and destination.
struct ShadeWarp {
    JKRSolidHeap* heap=nullptr;
    JKRMemArchive* archive=nullptr;
    std::array<J3DModel*,2> models{};
    mDoExt_invisibleModel ghost;
    bool ready=false,failed=false,active=false,arrival=false;
    u32 emitter=0;
    float height=180,scroll=0,cutoff=-0.5f;

    void stop() {
        if (auto* effect=dComIfGp_particle_getEmitter(emitter)) {
            effect->becomeInvalidEmitter(); effect->deleteAllParticle();
        }
        emitter=0;active=false;
    }
    void finish() {
        active=false;
        if (auto* effect=dComIfGp_particle_getEmitter(emitter)) effect->stopCreateParticle();
    }
    void destroy() {
        stop();
        // Unlink the private archive's volume before its backing heap disappears.
        if (archive) { delete archive;archive=nullptr; }
        if (heap) { mDoExt_destroySolidHeap(heap);heap=nullptr; }
        models={};ready=false;failed=false;
    }
    bool load(daNpc_Kn_c* actor) {
        // addWarpMaterial requires room for one extra TEV/texgen stage.
        for (unsigned i=0;i<2;++i) {
            auto* source=actor->mpModelMorf[i]->getModel()->getModelData();
            if (!source->getMaterialNum()) return false;
            for (u16 j=0;j<source->getMaterialNum();++j) {
                auto* material=source->getMaterialNodePointer(j);
                if (material->getTevBlock()->getTevStageNum()>=4 ||
                    material->getTexGenBlock()->getTexGenNum()>=4 ||
                    material->getTevBlock()->getTexNo(3)!=0xffff) return false;
            }
        }
        // The mounted KN_a resources have already been endian-converted in
        // place by J3D. Re-parsing those bytes (even a memcpy) corrupts geometry.
        // Read a fresh archive into this heap, keeping all vertices/textures private.
        u32 size=0;
        void* bytes=JKRDvdRipper::loadToMainRAM("/res/Object/KN_a.arc",nullptr,
            EXPAND_SWITCH_UNKNOWN1,0,heap,JKRDvdRipper::ALLOC_DIRECTION_FORWARD,0,nullptr,&size);
        if (!bytes) return false;
        archive=JKR_NEW JKRMemArchive(bytes,size,JKRMEMBREAK_FLAG_UNKNOWN0);
        if (!archive || !archive->isMounted()) return false;
        height=180;
        for (unsigned i=0;i<2;++i) {
            const unsigned resource=i==0 ? 48 : 47; // same KN_a bodies as native CreateHeap
            if (resource>=static_cast<unsigned>(archive->countFile()) || !archive->isFileEntry(resource)) return false;
            auto* raw=archive->getIdxResource(resource);
            if (!raw) return false;
            // BMWE reserves and installs the game's Always/warp_tex material.
            auto* data=dRes_info_c::loaderBasicBmd(0x424d5745,raw);
            if (!data || data->getJointNum()!=actor->mpModelMorf[i]->getModel()->getModelData()->getJointNum()) return false;
            models[i]=mDoExt_J3DModel__create(data,0x80000,0x13020684);
            if (!models[i]) return false;
            for (u16 j=0;j<data->getShapeNum();++j)
                height=std::max(height,data->getShapeNodePointer(j)->getMax()->y*actor->scale.y+20);
        }
        return ghost.create(models[0],1)!=0;
    }
    void prepare(daNpc_Kn_c* actor) {
        if (ready || failed) return;
        heap=mDoExt_createSolidHeapFromGame(0x800000,0x20);
        if (heap) {
            auto* previous=mDoExt_setCurrentHeap(heap);
            ready=load(actor);
            mDoExt_setCurrentHeap(previous);
            if (ready) mDoExt_adjustSolidHeap(heap);
        }
        if (!ready) {
            destroy();failed=true;
            svc_log->warn(mod_ctx,"Hero's Shade: warp model unavailable; using native warp particles without model dissolve");
        }
    }
    void begin(daNpc_Kn_c* actor,bool arriving) {
        stop();prepare(actor);
        active=true;arrival=arriving;scroll=0;
        cutoff=arrival ? -0.5f : height/30.0f-0.5f;
        // Keep native skeleton proportions; the old squash/teleport state is off.
        actor->field_0x16f4.set(1,1,1);actor->field_0x170c=0;actor->field_0x170d=0;
        actor->mNoDraw=arrival;
        mDoAud_seStart(arrival ? Z2SE_AL_WARP_OUT : Z2SE_AL_WARP_IN_TATE,
            &actor->current.pos,0,dComIfGp_getReverb(51));
    }
    void tick(daNpc_Kn_c* actor,int ticks) {
        if (!active) return;
        scroll+=0.15f;if (scroll>=1) scroll-=1;
        const float progress=std::clamp((ticks-(arrival ? shade::warp_arrival_hold_ticks : 0))/
            static_cast<float>(shade::warp_travel_ticks),0.0f,1.0f);
        const float visible=arrival ? progress : 1-progress;
        cutoff=-0.5f+visible*height/30.0f;
        actor->mNoDraw=ready || visible<=0 || (!arrival && progress>=0.5f);
        cXyz pos=actor->current.pos;
        if (!arrival) pos.y+=visible*height; // Link's descending dissolve edge
        emitter=dComIfGp_particle_set(emitter,arrival ? ID_ZI_J_LK_WARP_APP_A : ID_ZI_J_LK_WARP_DISAPP_A,
            &pos,&actor->tevStr,&actor->shape_angle,nullptr,0xff,nullptr,0xff,nullptr,nullptr,nullptr);
        dComIfGp_particle_levelEmitterOnEventMove(emitter);
    }
    bool draw(daNpc_Kn_c* actor) {
        if (!active || !ready) return false;
        for (unsigned i=0;i<2;++i) {
            auto* model=models[i];auto* source=actor->mpModelMorf[i]->getModel();
            model->setBaseTRMtx(source->getBaseTRMtx());model->setBaseScale(*source->getBaseScale());
            for (u16 joint=0;joint<model->getModelData()->getJointNum();++joint) {
                model->setAnmMtx(joint,source->getAnmMtx(joint));
                model->getMtxBuffer()->setScaleFlag(joint,source->getMtxBuffer()->getScaleFlag(joint));
            }
            model->calcWeightEnvelopeMtx();
            dRes_info_c::setWarpSRT(model->getModelData(),actor->current.pos,scroll,cutoff);
            g_env_light.settingTevStruct(i==0 ? 7 : 0,&actor->current.pos,&actor->tevStr);
            g_env_light.setLightTevColorType_MAJI(model,&actor->tevStr);
            if (i==0) ghost.entryDL(0);
            else mDoExt_modelEntryDL(model);
        }
        return true;
    }
} sShadeWarp;

void begin_shade_warp(daNpc_Kn_c* actor,bool arrival) { sShadeWarp.begin(actor,arrival); }
void tick_shade_warp(daNpc_Kn_c* actor,int ticks) { sShadeWarp.tick(actor,ticks); }
void finish_shade_warp() { sShadeWarp.finish(); }
void end_shade_warp() { sShadeWarp.stop(); }
