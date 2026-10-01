// Included inside fierce_deity_visual.cpp's private namespace.
using OutfitModels = std::array<J3DModel*, 6>;
OutfitModels outfit_models(daAlink_c* link) {
    return {link->mpLinkModel, link->mpLinkFaceModel, link->mpLinkHatModel,
            link->mpLinkHandModel, link->mpLinkBootModels[0], link->mpLinkBootModels[1]};
}

struct WarpLayer {
    bool active = false;
    bool dark = false;
    bool inverse = false;
};

struct WarpTransition {
    daAlink_c* owner = nullptr;
    fpc_ProcID ownerId = fpcM_ERROR_PROCESS_ID_e;
    OutfitModels outgoing{};
    JKRSolidHeap* modelHeap = nullptr;
    JKRExpHeap* archiveHeap = nullptr;
    JKRExpHeap* scratchArchiveHeap = nullptr;
    const char* archive = nullptr;
    bool fromDark = false;
    bool toDark = false;
    bool committed = false;
    FierceWarpWipe wipe;
};
WarpTransition s_transition;

bool transition_owner(daAlink_c* link) {
    return link != nullptr && link == s_transition.owner &&
        fopAcM_GetID(link) == s_transition.ownerId;
}

// A native BMWR model already contains a dormant warp texgen and texture-matrix
// vertex indices. Never guess/append vertex attributes to a foreign replacement.
int native_warp_coord(J3DModel* model, J3DMaterial* material, daAlink_c* link) {
    if (model == nullptr || material == nullptr || link == nullptr) return -1;
    auto* data = model->getModelData();
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
    for (auto* model : models) {
        if (model == nullptr) continue;
        // CPU-deformed vertex buffers cannot be retargeted by joint matrices.
        if (model->getSkinDeform() != nullptr || model->mDeformData != nullptr) return false;
        auto* data = model->getModelData();
        for (u16 i = 0; i < data->getMaterialNum(); ++i) {
            if (native_warp_coord(model, data->getMaterialNodePointer(i), link) < 0) return false;
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
    restore_archive_heap(link);
    // The replacement model heap belongs to Link. Only dispose the OUTGOING
    // instances, then their archive reference. Link keeps its original archive
    // heap: same-archive Dark transitions still store their live BMDs there.
    if (s_transition.modelHeap != nullptr) s_transition.modelHeap->destroy();
    if (s_transition.archive != nullptr) dComIfG_deleteObjectResMain(s_transition.archive);
    s_transition = {};
}

void prepare_transition(daAlink_c* link, bool fromDark, bool toDark, bool entering) {
    if (s_transition.owner != nullptr) return;
    const auto models = outfit_models(link);
    if (!warp_compatible(models, link)) return;
    auto* oldModels = link->mAnmHeap3.mAnimeHeap;
    auto* oldArchive = link->mpArcHeap;
    if (oldModels == nullptr || oldArchive == nullptr) return;

    // The outgoing pose must remain alive through the wipe. Let changeLink
    // build the replacement in a fresh native model heap. A temporary empty
    // archive heap shields the outgoing BMDs from loadModelDVD's freeAll.
    // The replacement archive has already been preloaded/pinned by the caller.
    auto* newModels = JKRSolidHeap::create(oldModels->getHeapSize() + 0x100,
                                         oldModels->getParent(), false);
    if (newModels == nullptr) return;
    auto* scratch = JKRExpHeap::create(oldArchive->getHeapSize() + 0x100,
                                      oldArchive->getParent(), false);
    if (scratch == nullptr) { newModels->destroy(); return; }
    if (!dComIfG_setObjectRes(link->mArcName, u8{0}, nullptr)) {
        scratch->destroy(); newModels->destroy(); return;
    }
    s_transition.owner = link;
    s_transition.ownerId = fopAcM_GetID(link);
    s_transition.outgoing = models;
    s_transition.modelHeap = oldModels;
    s_transition.archiveHeap = oldArchive;
    s_transition.scratchArchiveHeap = scratch;
    s_transition.archive = link->mArcName;
    s_transition.fromDark = fromDark;
    s_transition.toDark = toDark;
    s_transition.wipe.entering = entering;
    link->mAnmHeap3.mAnimeHeap = newModels;
    link->mpArcHeap = scratch;
}

WarpLayer warp_layer(J3DModel* model) {
    auto* link = daAlink_getAlinkActorClass();
    if (!s_transition.committed || !transition_owner(link)) return {};
    for (auto* outgoing : s_transition.outgoing) {
        if (model == outgoing) return {true, s_transition.fromDark, s_transition.wipe.inverse(false)};
    }
    if (player_model(link, model)) return {true, s_transition.toDark, s_transition.wipe.inverse(true)};
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
    const auto incoming = outfit_models(link);
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
