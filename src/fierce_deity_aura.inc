// Included inside fierce_deity_visual.cpp's private namespace.
// Own a level-emitter key; Link's native metamorphosis slots remain untouched.
struct DarkAura {
    daAlink_c* owner = nullptr;
    fpc_ProcID ownerId = fpcM_ERROR_PROCESS_ID_e;
    u32 particleKey = 0;
    bool failureReported = false;
};
DarkAura s_darkAura;

JPABaseEmitter* dark_aura_emitter() {
    if (s_darkAura.particleKey == 0 || g_dComIfG_gameInfo.play.getParticle() == nullptr ||
        dPa_control_c::getEmitterManager() == nullptr) return nullptr;
    return dComIfGp_particle_getEmitter(s_darkAura.particleKey);
}

void stop_dark_aura() {
    if (auto* emitter = dark_aura_emitter()) {
        emitter->stopCreateParticle();
        emitter->stopDrawParticle();
        emitter->deleteAllParticle();
    }
    // The native level table releases the emitter when we stop refreshing it.
    s_darkAura = {};
}

void update_dark_aura(daAlink_c* link) {
    if (link == nullptr || !fierce_deity_active() ||
        fierce_deity_displayed_tint() != FierceDeityTint::Dark ||
        fierce_deity_transition_busy() || link->checkWolf() || link->checkDeadHP() ||
        link->checkSceneChangeAreaStart() || link->checkEventRun() ||
        link->checkPlayerNoDraw() ||
        link->mpLinkModel == nullptr || link->mpLinkModel->getModelData()->getJointNum() <= 1) {
        stop_dark_aura();
        return;
    }
    if (s_darkAura.owner != link || s_darkAura.ownerId != fopAcM_GetID(link)) {
        stop_dark_aura();
        s_darkAura.owner = link;
        s_darkAura.ownerId = fopAcM_GetID(link);
    }
    if (g_dComIfG_gameInfo.play.getParticle() == nullptr ||
        dPa_control_c::getEmitterManager() == nullptr) {
        s_darkAura.particleKey = 0;
        return;
    }
    // WTOA_B is the black rising effect attached to wolf joint 2 before the
    // wolf-to-human swap. Use the human body's central joint instead.
    auto* emitter = link->setEmitter(&s_darkAura.particleKey, ID_ZI_J_WTOA_B,
                                     &link->current.pos, nullptr);
    if (emitter != nullptr) {
        emitter->setGlobalRTMatrix(link->mpLinkModel->getAnmMtx(1));
    } else if (!s_darkAura.failureReported) {
        warp_warning("Dark Link aura: native metamorphosis emitter unavailable");
        s_darkAura.failureReported = true;
    }
}

HookAction before_dark_aura_calc(ModContext*, void* args, void*, void*) {
    auto* emitter = mods::arg<JPABaseEmitter*>(args, 2);
    if (emitter == nullptr || emitter != dark_aura_emitter() ||
        emitter->checkStatus(JPAEmtrStts_StopCalc)) return HOOK_CONTINUE;
    // Replay finite resource timelines before native termination. This also
    // loops emission keyframes, without resetting living particles or allocating
    // an emitter every frame. Resources with maxFrame == 0 already run forever.
    if (emitter->mMaxFrame > 0 && emitter->getAge() >= static_cast<u32>(emitter->mMaxFrame)) {
        emitter->mTick = 0;
        emitter->playCreateParticle();
    }
    return HOOK_CONTINUE;
}
