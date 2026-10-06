// Included inside fierce_deity_visual.cpp's private namespace.
using OutfitModels = std::array<J3DModel*, 6>;
OutfitModels outfit_models(daAlink_c* link) {
    return {link->mpLinkModel, link->mpLinkFaceModel, link->mpLinkHatModel,
            link->mpLinkHandModel, link->mpLinkBootModels[0], link->mpLinkBootModels[1]};
}

struct WarpLayer {
    bool active = false;
    FierceDeityTint tint = FierceDeityTint::None;
    bool inverse = false;
};

struct WarpTransition {
    daAlink_c* owner = nullptr;
    fpc_ProcID ownerId = fpcM_ERROR_PROCESS_ID_e;
    OutfitModels outgoing{};
    JKRSolidHeap* modelHeap = nullptr;
    // Tint copies borrow immutable geometry from the still-live outfit, but
    // own their packets, display lists and pose matrices in a separate heap.
    OutfitModels sources{};
    std::array<J3DModelData*, 6> sourceData{};
    std::unique_ptr<u8[]> cloneStorage;
    JKRExpHeap* cloneHeap = nullptr;
    JKRExpHeap* archiveHeap = nullptr;
    JKRExpHeap* scratchArchiveHeap = nullptr;
    const char* archive = nullptr;
    FierceDeityTint fromTint = FierceDeityTint::None;
    FierceDeityTint toTint = FierceDeityTint::None;
    bool committed = false;
    u32 particleKey = 0;
    bool particleFailureReported = false;
    FierceWarpWipe wipe;
};
WarpTransition s_transition;
bool s_extraWarpParticleStep = false;
using ForgetWarpMatrices = void (*)(J3DModel*);
ForgetWarpMatrices s_forgetWarpMatrices = nullptr;

void warp_warning(const char* message) {
    if (svc_log != nullptr) svc_log->warn(mod_ctx, message);
}

JPABaseEmitter* warp_particle_emitter() {
    if (s_transition.particleKey == 0 || g_dComIfG_gameInfo.play.getParticle() == nullptr ||
        dPa_control_c::getEmitterManager() == nullptr) return nullptr;
    return dComIfGp_particle_getEmitter(s_transition.particleKey);
}

void stop_warp_particles() {
    if (auto* emitter = warp_particle_emitter()) {
        // The level-emitter table owns this emitter. Stop drawing/creating and
        // return live particles now; its normal cleanup releases the emitter.
        // Never free it behind that table or touch Link's native warp handles.
        emitter->stopCreateParticle();
        emitter->stopDrawParticle();
        emitter->deleteAllParticle();
    }
    s_transition.particleKey = 0;
}

void update_warp_particles(daAlink_c* link) {
    if (g_dComIfG_gameInfo.play.getParticle() == nullptr ||
        dPa_control_c::getEmitterManager() == nullptr) return;
    // These are Link's COMMON warp resources, not a scene/boss archive.
    // Assemble the selected appearance on entry, dissolve it on exit.
    // APP is authored at the feet; DISAPP is anchored to the moving boundary.
    cXyz position = link->current.pos;
    const u16 effect = s_transition.wipe.entering ?
        ID_ZI_J_LK_WARP_APP_A : ID_ZI_J_LK_WARP_DISAPP_A;
    if (!s_transition.wipe.entering) {
        position.y += (s_transition.wipe.height() - FierceWarpWipe::low) * 30.0f;
    }
    auto* emitter = link->setEmitter(&s_transition.particleKey, effect,
                                     &position, &link->shape_angle);
    if (emitter == nullptr && !s_transition.particleFailureReported) {
        warp_warning("Dark Link warp: native flying-particle emitter unavailable");
        s_transition.particleFailureReported = true;
    }
}

void after_warp_particle_calc(ModContext*, void* args, void* retval, void*) {
    if (s_extraWarpParticleStep || !s_transition.committed || retval == nullptr ||
        *static_cast<bool*>(retval)) return;
    auto* emitter = mods::arg<JPABaseEmitter*>(args, 2);
    if (emitter == nullptr || emitter != warp_particle_emitter() ||
        emitter->checkStatus(JPAEmtrStts_StopCalc)) return;
    auto* resource = mods::arg<JPAResource*>(args, 0);
    auto* work = mods::arg<JPAEmitterWorkData*>(args, 1);
    // Advance only our emitter by one additional native step. This doubles
    // particle motion, lifetimes and all resource keyframes together, instead
    // of just doubling launch speed. The manager retains deletion ownership.
    s_extraWarpParticleStep = true;
    *static_cast<bool*>(retval) = resource->calc(work, emitter);
    s_extraWarpParticleStep = false;
}

bool transition_owner(daAlink_c* link) {
    return link != nullptr && link == s_transition.owner &&
        fopAcM_GetID(link) == s_transition.ownerId;
}

// A native BMWR model already contains a dormant warp texgen and texture-matrix
// vertex indices. Never guess/append vertex attributes to a foreign replacement.
int native_warp_coord(J3DModel* model, J3DMaterial* material, daAlink_c* link) {
    if (model == nullptr || material == nullptr || link == nullptr) return -1;
    auto* data = model->getModelData();
    if (data == nullptr) return -1;
    auto* texture = data->getTexture();
    auto* tev = material->getTevBlock();
    auto* gen = material->getTexGenBlock();
    if (texture == nullptr || tev == nullptr || gen == nullptr) return -1;
    // Slot 3 must exist: never index a one/two-slot block. Patched and
    // sixteen-stage replacements may also retain the native warp layout.
    const u32 type = tev->getType();
    if (type != 'TVB4' && type != 'TVPT' && type != 'TV16') return -1;
    const unsigned coord = gen->getTexGenNum();
    const unsigned count = tev->getTevStageNum();
    const u16 texNo = tev->getTexNo(3);
    if (coord >= 4 || count == 0 || count > 3 || texNo >= texture->getNum() ||
        texture->getImgDataPtr(texNo) != link->mpWarpTexData) return -1;
    auto* matrix = gen->getTexMtx(coord);
    auto* tc = gen->getTexCoord(coord);
    if (matrix == nullptr || tc == nullptr || tc->getTexGenSrc() != GX_TG_POS ||
        (matrix->getTexMtxInfo().mInfo & 0x3F) != 8) return -1;
    return static_cast<int>(coord);
}

bool warp_compatible(const OutfitModels& models, daAlink_c* link) {
    for (unsigned part = 0; part < models.size(); ++part) {
        auto* model = models[part];
        if (model == nullptr) continue;
        // CPU-deformed vertex buffers cannot be retargeted by joint matrices.
        if (model->getSkinDeform() != nullptr || model->mDeformData != nullptr) {
            return false;
        }
        auto* data = model->getModelData();
        if (data == nullptr || data->getMaterialNum() == 0) return false;
        for (u16 i = 0; i < data->getMaterialNum(); ++i) {
            auto* material = data->getMaterialNodePointer(i);
            if (native_warp_coord(model, material, link) < 0) return false;
        }
    }
    return models[0] != nullptr;
}

void restore_archive_heap(daAlink_c* link) {
    if (s_transition.scratchArchiveHeap == nullptr) return;
    if (!transition_owner(link)) return;
    link->mpArcHeap = s_transition.archiveHeap;
    s_transition.scratchArchiveHeap->destroy();
    s_transition.scratchArchiveHeap = nullptr;
}

void discard_transition(daAlink_c* link) {
    stop_warp_particles();
    restore_archive_heap(link);
    // The replacement model heap belongs to Link. Only dispose the OUTGOING
    // instances, then their archive reference. Link keeps its original archive
    // heap: same-archive Dark transitions still store their live BMDs there.
    if (s_transition.cloneHeap != nullptr) {
        // Host interpolation also retains per-instance matrix history.
        for (auto* model : s_transition.outgoing)
            if (model != nullptr && s_forgetWarpMatrices != nullptr) s_forgetWarpMatrices(model);
        s_transition.cloneHeap->destroy();
    }
    if (s_transition.modelHeap != nullptr) s_transition.modelHeap->destroy();
    if (s_transition.archive != nullptr) dComIfG_deleteObjectResMain(s_transition.archive);
    s_transition = {};
}

bool tint_sources_live(daAlink_c* link) {
    if (s_transition.cloneHeap == nullptr) return true;
    if (!transition_owner(link) || link->mClothesChangeWaitTimer != 0) return false;
    const auto live = outfit_models(link);
    for (unsigned i = 0; i < live.size(); ++i) {
        // Compare identity before reading any borrowed data. A replacement
        // mod can retire its previous model without going through changeLink.
        if (live[i] != s_transition.sources[i]) return false;
        if (live[i] != nullptr && live[i]->getModelData() != s_transition.sourceData[i]) return false;
    }
    return true;
}

bool separate_outgoing(const OutfitModels& incoming) {
    for (unsigned i = 0; i < s_transition.outgoing.size(); ++i) {
        auto* model = s_transition.outgoing[i];
        if (model == nullptr) continue;
        for (auto* live : incoming) if (model == live) return false;
        for (unsigned j = 0; j < i; ++j)
            if (model == s_transition.outgoing[j]) return false;
    }
    return true;
}

void prepare_tint_transition(daAlink_c* link, FierceDeityTint fromTint, FierceDeityTint toTint, bool entering) {
    if (link == nullptr || s_transition.owner != nullptr || fromTint == toTint) return;
    const auto models = outfit_models(link);
    if (!warp_compatible(models, link)) return;
    // Host-backed, bounded storage avoids exhausting Link's or the room's
    // heaps. No geometry/texture copy and no archive reload is needed.
    constexpr u32 capacity = 2 * 1024 * 1024;
    std::unique_ptr<u8[]> storage(new (std::nothrow) u8[capacity + 31]);
    if (!storage) return;
    auto* aligned = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(storage.get()) + 31) & ~uintptr_t{31});
    auto* heap = JKRExpHeap::create(aligned, capacity, JKRHeap::getRootHeap(), false);
    if (heap == nullptr) return;
    auto* previous = heap->becomeCurrentHeap();
    OutfitModels copies{};
    bool complete = true;
    for (unsigned i = 0; i < models.size(); ++i) {
        auto* source = models[i];
        if (source == nullptr) continue;
        // Keep the small metadata header too: forgetMtx reads its joint and
        // envelope counts during cleanup, even if TE has retired the source.
        // Arrays/materials/textures remain borrowed and are never freed here.
        auto* data = JKR_NEW J3DModelData(*source->getModelData());
        if (data == nullptr) { complete = false; break; }
        auto* copy = JKR_NEW J3DModel();
        // Preserve matrix modes, but never convert a borrowed shared display
        // list to double buffering inside our short-lived heap.
        u32 flags = source->mFlags;
        if ((flags & J3DMdlFlag_UseSharedDL) || data->getModelDataType() == 1)
            flags |= J3DMdlFlag_UseSingleDL;
        if (copy == nullptr || copy->entryModelData(data, flags, 1) != kJ3DError_Success ||
            ((flags & J3DMdlFlag_DifferedDLBuffer) &&
             copy->newDifferedDisplayList(source->mDiffFlag) != kJ3DError_Success)) {
            complete = false;
            break;
        }
        copy->lock();
        copy->setBaseTRMtx(source->getBaseTRMtx());
        copy->setBaseScale(*source->getBaseScale());
        copies[i] = copy;
    }
    previous->becomeCurrentHeap();
    if (!complete) {
        // No copy has been drawn, so none has interpolation history yet.
        heap->destroy();
        warp_warning("Dark Link warp: tint instance allocation failed");
        return;
    }
    s_transition.owner = link;
    s_transition.ownerId = fopAcM_GetID(link);
    s_transition.outgoing = copies;
    s_transition.sources = models;
    for (unsigned i = 0; i < models.size(); ++i)
        if (models[i] != nullptr) s_transition.sourceData[i] = models[i]->getModelData();
    s_transition.cloneStorage = std::move(storage);
    s_transition.cloneHeap = heap;
    s_transition.fromTint = fromTint;
    s_transition.toTint = toTint;
    s_transition.wipe.entering = entering;
}

void prepare_transition(daAlink_c* link, FierceDeityTint fromTint, FierceDeityTint toTint, bool entering) {
    if (s_transition.owner != nullptr) return;
    const auto models = outfit_models(link);
    if (!warp_compatible(models, link)) return;
    auto* oldModels = link->mAnmHeap3.mAnimeHeap;
    auto* oldArchive = link->mpArcHeap;
    if (oldModels == nullptr || oldArchive == nullptr) {
        warp_warning("Dark Link warp: skipped (player heaps unavailable)");
        return;
    }

    // The outgoing pose must remain alive through the wipe. Let changeLink
    // build the replacement in a fresh native model heap. A temporary empty
    // archive heap shields the outgoing BMDs from loadModelDVD's freeAll.
    // The replacement archive has already been preloaded/pinned by the caller.
    auto* newModels = JKRSolidHeap::create(oldModels->getHeapSize() + 0x100,
                                         oldModels->getParent(), false);
    if (newModels == nullptr) {
        warp_warning("Dark Link warp: skipped (model heap allocation failed)");
        return;
    }
    // The pinned target is already loaded: resLoad only acquires its reference.
    // This heap only absorbs freeAll. Do not duplicate Link's ~13 MiB archive
    // reservation just to clear an empty heap during the synchronous reload.
    auto* scratch = JKRExpHeap::create(0x1000,
                                      oldArchive->getParent(), false);
    if (scratch == nullptr) {
        warp_warning("Dark Link warp: skipped (scratch heap allocation failed)");
        newModels->destroy(); return;
    }
    if (!dComIfG_setObjectRes(link->mArcName, u8{0}, nullptr)) {
        warp_warning("Dark Link warp: skipped (outgoing archive reference failed)");
        scratch->destroy(); newModels->destroy(); return;
    }
    s_transition.owner = link;
    s_transition.ownerId = fopAcM_GetID(link);
    s_transition.outgoing = models;
    s_transition.modelHeap = oldModels;
    s_transition.archiveHeap = oldArchive;
    s_transition.scratchArchiveHeap = scratch;
    s_transition.archive = link->mArcName;
    s_transition.fromTint = fromTint;
    s_transition.toTint = toTint;
    s_transition.wipe.entering = entering;
    link->mAnmHeap3.mAnimeHeap = newModels;
    link->mpArcHeap = scratch;
}

WarpLayer warp_layer(J3DModel* model) {
    auto* link = daAlink_getAlinkActorClass();
    if (!s_transition.committed || !transition_owner(link) || !tint_sources_live(link)) return {};
    for (auto* outgoing : s_transition.outgoing) {
        if (model == outgoing) return {true, s_transition.fromTint, s_transition.wipe.inverse(false)};
    }
    if (player_model(link, model)) return {true, s_transition.toTint, s_transition.wipe.inverse(true)};
    return {};
}

void copy_outgoing_pose(J3DModel* outgoing, J3DModel* incoming) {
    if (outgoing == nullptr || incoming == nullptr) return;
    auto* oldData = outgoing->getModelData();
    auto* newData = incoming->getModelData();
    Mtx inverse, delta;
    MTXInverse(outgoing->getBaseTRMtx(), inverse);
    MTXConcat(incoming->getBaseTRMtx(), inverse, delta);
    auto* oldNames = oldData->getJointName();
    auto* newNames = newData->getJointName();
    for (u16 i = 0; i < oldData->getJointNum(); ++i) {
        int index = -1;
        if (oldNames != nullptr && newNames != nullptr) {
            const char* name = oldNames->getName(i);
            if (name != nullptr) index = newNames->getIndex(name);
        }
        if (index < 0 && oldData->getJointNum() == newData->getJointNum()) index = i;
        if (index >= 0 && index < newData->getJointNum()) {
            outgoing->setAnmMtx(i, incoming->getAnmMtx(index));
            outgoing->setScaleFlag(i, incoming->getMtxBuffer()->getScaleFlag(index));
        } else {
            // Custom extra bones keep their local pose, following the moving
            // root instead of collapsing onto an unrelated destination joint.
            Mtx matrix;
            MTXConcat(delta, outgoing->getAnmMtx(i), matrix);
            outgoing->setAnmMtx(i, matrix);
        }
    }
    outgoing->setBaseTRMtx(incoming->getBaseTRMtx());
    outgoing->setBaseScale(*incoming->getBaseScale());
    outgoing->calcWeightEnvelopeMtx();
}

void draw_outgoing(daAlink_c* link) {
    if (!s_transition.committed || !transition_owner(link) || link->checkPlayerNoDraw() ||
        link->checkWolf() || link->mClothesChangeWaitTimer != 0) return;
    // Do not free packets during a draw traversal. tick retires a stale copy
    // on the next update; until then neither layer uses its borrowed data.
    if (!tint_sources_live(link)) return;
    const auto incoming = outfit_models(link);
    if (!separate_outgoing(incoming)) return;
    for (unsigned i = 0; i < incoming.size(); ++i) {
        auto* model = s_transition.outgoing[i];
        if (model == nullptr || incoming[i] == nullptr) continue;
        // Boots are separate heavy-boots models, not the regular outfit boots.
        if (i >= 4 && !link->checkEquipHeavyBoots()) continue;
        copy_outgoing_pose(model, incoming[i]);
        g_env_light.setLightTevColorType_MAJI(model, &link->tevStr);
        mDoExt_modelEntryDL(model);
    }
}

// Same SRT and projection as dRes_info_c::setWarpSRT / Link's native warp.
// This is a draw-local matrix; shared model materials are never modified.
void warp_texture_matrix(Mtx result) {
    J3DTexMtxInfo info = {
        0, 8, 0, 0, {0.5f, 0.5f, 0.0f},
        {0.1f, 0.1f, 0, s_transition.wipe.scroll(), s_transition.wipe.height()},
        {{0.5f, 0, 0, 0.5f}, {0, 0.5f, 0, 0.5f}, {0, 0, 0, 1}, {0, 0, 0, 1}}
    };
    auto* link = s_transition.owner;
    Mtx placement, inverseView;
    MTXTrans(placement, -link->current.pos.x, -link->current.pos.y, -link->current.pos.z);
    auto* camera = dComIfGp_getCamera(dComIfGp_getPlayerCameraID(0));
    mDoMtx_YrotM(placement, fopCamM_GetAngleY(camera));
    Mtx44 effect;
    std::memcpy(effect, info.mEffectMtx, sizeof(effect));
    cMtx_concat(effect, placement, info.mEffectMtx);
    MTXInverse(j3dSys.getViewMtx(), inverseView);
    J3DTexMtx matrix(info);
    matrix.calcPostTexMtx(inverseView);
    MTXCopy(matrix.getMtx(), result);
}
