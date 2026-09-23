// Native Gale Boomerang artwork, owned by the current Link scene. No boomerang
// actor is spawned: its weapon targeting, singleton and attack collision stay separate.
struct GaleVisual {
    JKRSolidHeap* heap = nullptr;
    J3DModel* model = nullptr;
    mDoExt_bckAnm motion;
    mDoExt_btkAnm texture;
    cXyz origin{0, 0, 0};
    int ticks = 0;
    bool failed = false;

    bool prepare() {
        if (model) return true;
        if (failed) return false;
        auto* data = static_cast<J3DModelData*>(dComIfG_getObjectRes(daAlink_c::getAlinkArcName(), 0x34));
        auto* bck = static_cast<J3DAnmTransform*>(dComIfG_getObjectRes(daAlink_c::getAlinkArcName(), 0x13));
        auto* btk = static_cast<J3DAnmTextureSRTKey*>(dComIfG_getObjectRes(daAlink_c::getAlinkArcName(), 0x48));
        if (!data || !bck || !btk || data->getJointNum() > 32) return false;
        heap = mDoExt_createSolidHeapFromGame(0x20000, 0x20);
        if (heap) {
            auto* previous = mDoExt_setCurrentHeap(heap);
            model = mDoExt_J3DModel__create(data, 0, 0x11000284);
            const bool ready = model && motion.init(bck, TRUE, J3DFrameCtrl::EMode_LOOP, 1, 0, -1, false) &&
                texture.init(data, btk, TRUE, J3DFrameCtrl::EMode_LOOP, 1, 0, -1);
            mDoExt_setCurrentHeap(previous);
            if (ready) {
                mDoExt_adjustSolidHeap(heap);
                return true;
            }
            mDoExt_destroySolidHeap(heap);
            heap = nullptr;
            model = nullptr;
        }
        failed = true;
        if (svc_log) svc_log->warn(mod_ctx, "Revali's Gale: could not prepare the native wind model");
        return false;
    }
    void launch(const cXyz& position) {
        origin = position;
        if (prepare()) {
            ticks = 60;
            motion.setFrame(0);
            texture.setFrame(0);
        }
    }
    void update() {
        if (ticks <= 0) return;
        --ticks;
        motion.play();
        texture.play();
        Z2GetAudioMgr()->seStartLevel(Z2SE_BOOM_TORNADO, &origin, 0, 0, 1, 1, -1, -1, 0);
    }
    void draw(daAlink_c* link) {
        if (!model || ticks <= 0) return;
        const float fade = std::min(1.0f, ticks / 15.0f);
        const cXyz scale(2.0f * fade, 2.5f * fade, 2.0f * fade);
        model->setBaseScale(scale);
        mDoMtx_stack_c::transS(origin.x, origin.y + 150.0f * fade, origin.z);
        model->setBaseTRMtx(mDoMtx_stack_c::get());
        auto* data = model->getModelData();
        // The real boomerang installs an actor-dependent joint callback on
        // shared archive data. Suppress it only during our synchronous calc.
        J3DJointCallBack callbacks[32]{};
        for (unsigned i = 0; i < data->getJointNum(); ++i) {
            auto* joint = data->getJointNodePointer(i);
            callbacks[i] = joint->getCallBack();
            joint->setCallBack(nullptr);
        }
        auto* root = data->getJointNodePointer(0);
        auto* previousCalc = root->getMtxCalc();
        const float previousBckFrame = motion.getBckAnm()->getFrame();
        const float previousBtkFrame = texture.getBtkAnm()->getFrame();
        motion.entry(data);
        texture.entry(data);
        g_env_light.setLightTevColorType_MAJI(model, &link->tevStr);
        mDoExt_modelUpdateDL(model);
        root->setMtxCalc(previousCalc);
        motion.getBckAnm()->setFrame(previousBckFrame);
        texture.getBtkAnm()->setFrame(previousBtkFrame);
        for (unsigned i = 0; i < data->getJointNum(); ++i)
            data->getJointNodePointer(i)->setCallBack(callbacks[i]);
    }
    void release() {
        ticks = 0;
        model = nullptr;
        if (heap) mDoExt_destroySolidHeap(heap);
        heap = nullptr;
        failed = false;
    }
};
GaleVisual s_galeVisual;
