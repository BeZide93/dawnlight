// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#include "gallery_resources.hpp"
#include <set>
#include <string>
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_alink.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_mtx.h"
#include "d/d_kankyo.h"
#include "JSystem/JKernel/JKRExpHeap.h"

#include <cstring>

static void normalizeArcName(const char* src, char* dst, size_t dstSize) {
    if (src == nullptr || dst == nullptr || dstSize == 0) return;
    std::strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
    char* dot = std::strstr(dst, ".arc");
    if (dot != nullptr) {
        *dot = '\0';
    }
}

namespace {
// One reference per archive for the entire gallery, including in-flight loads.
// Never release an archive just because another actor already had it loaded.
std::set<std::string> s_galleryArchives;
}

int loadObjectArchive(const char* arcName) {
    if (!arcName || !*arcName) return -1;
    char cleanName[64];
    normalizeArcName(arcName, cleanName, sizeof(cleanName));
    if (s_galleryArchives.find(cleanName) == s_galleryArchives.end()) {
        if (!dComIfG_setObjectRes(cleanName, 0, JKRHeap::getRootHeap())) return -1;
        s_galleryArchives.insert(cleanName);
    }
    const int sync = dComIfG_syncObjectRes(cleanName);
    return sync == 0 ? 0 : (sync < 0 ? -1 : 1);
}

void unloadObjectArchive(const char*) {
    // Shared archives outlive all of the gallery's models and animation objects.
}

void release_boss_gallery_archives() {
    for (const auto& name : s_galleryArchives) dComIfG_deleteObjectResMain(name.c_str());
    s_galleryArchives.clear();
}

J3DModel* loadBmdFromArc(const char* arcName, const char* bmdName, cXyz scale) {
    if (arcName == nullptr || bmdName == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    if (loadObjectArchive(cleanArc) != 0) {
        return nullptr;
    }

    char cleanBmd[64];
    std::strncpy(cleanBmd, bmdName, sizeof(cleanBmd) - 1);
    cleanBmd[sizeof(cleanBmd) - 1] = '\0';
    char* dot = std::strstr(cleanBmd, ".bmd");
    if (dot != nullptr) {
        *dot = '\0';
    }

    void* res = nullptr;
    dRes_info_c* info = dComIfG_getObjectResInfo(cleanArc);
    JKRArchive* archive = (info != nullptr) ? info->getArchive() : nullptr;
    if (archive != nullptr) {
        res = archive->getResource('BMD ', bmdName);
        if (res == nullptr) {
            res = archive->getResource('BMD ', cleanBmd);
        }
    }

    if (res == nullptr) {
        res = dComIfG_getObjectRes(cleanArc, bmdName);
    }
    if (res == nullptr) {
        res = dComIfG_getObjectRes(cleanArc, cleanBmd);
    }
    if (res == nullptr && std::strcmp(cleanArc, "E_fm") == 0 && std::strcmp(cleanBmd, "fm_core") == 0) {
        res = dComIfG_getObjectRes("E_fm", 0x27);
    }
    if (res == nullptr && std::strcmp(cleanArc, "E_th_ball") == 0) {
        if (std::strcmp(cleanBmd, "ib") == 0) {
            res = dComIfG_getObjectRes("E_th_ball", 4);
        } else if (std::strcmp(cleanBmd, "tc") == 0) {
            res = dComIfG_getObjectRes("E_th_ball", 7);
        }
    }
    if (res == nullptr && std::strcmp(cleanArc, "B_yo") == 0 && std::strcmp(cleanBmd, "yo_ice") == 0) {
        res = dComIfG_getObjectRes("B_yo", 0x21);
    }

    if (res == nullptr) {
        return nullptr;
    }

    J3DModelData* modelData = static_cast<J3DModelData*>(res);
    if (modelData->getMaterialNum() == 0 || modelData->getShapeTable() == nullptr ||
        modelData->getShapeTable()->getShapeNum() == 0 ||
        modelData->getMaterialNodePointer(0) == nullptr) {
        return nullptr;
    }

    J3DModel* model = mDoExt_J3DModel__create(modelData, 0x80000, 0x11000284);
    if (model != nullptr) {
        model->setBaseScale(scale);
    }
    return model;
}

#include "JSystem/J3DGraphLoader/J3DAnmLoader.h"

mDoExt_bckAnm* loadBckFromArc(const char* arcName, const char* bckName, int playMode, f32 rate) {
    if (arcName == nullptr || bckName == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    if (loadObjectArchive(cleanArc) != 0) {
        return nullptr;
    }

    char cleanBck[64];
    std::strncpy(cleanBck, bckName, sizeof(cleanBck) - 1);
    cleanBck[sizeof(cleanBck) - 1] = '\0';
    char* dot = std::strstr(cleanBck, ".bck");
    if (dot != nullptr) {
        *dot = '\0';
    }

    void* res = dComIfG_getObjectRes(cleanArc, bckName);
    if (res == nullptr) {
        res = dComIfG_getObjectRes(cleanArc, cleanBck);
    }

    if (res == nullptr) {
        dRes_info_c* info = dComIfG_getObjectResInfo(cleanArc);
        JKRArchive* archive = (info != nullptr) ? info->getArchive() : nullptr;
        if (archive != nullptr) {
            res = archive->getResource(bckName);
            if (res == nullptr) {
                res = archive->getResource(cleanBck);
            }
            if (res == nullptr) {
                res = archive->getResource('BCK ', bckName);
            }
            if (res == nullptr) {
                res = archive->getResource('BCK ', cleanBck);
            }
        }
    }

    if (res == nullptr) {
        return nullptr;
    }

    if (std::memcmp(res, "J3D1", 4) == 0) {
        res = J3DAnmLoaderDataBase::load(res);
        if (res == nullptr) {
            return nullptr;
        }
    }

    J3DAnmTransform* pbck = static_cast<J3DAnmTransform*>(res);
    mDoExt_bckAnm* bckAnm = JKR_NEW mDoExt_bckAnm();
    if (bckAnm == nullptr) {
        return nullptr;
    }

    if (!bckAnm->init(pbck, TRUE, playMode, rate, 0, -1, false)) {
        JKR_DELETE(bckAnm);
        return nullptr;
    }

    return bckAnm;
}

J3DModel* loadBmdFromArcIdx(const char* arcName, int resIndex, cXyz scale) {
    if (arcName == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    if (loadObjectArchive(cleanArc) != 0) {
        return nullptr;
    }

    void* res = dComIfG_getObjectRes(cleanArc, resIndex);
    if (res == nullptr) {
        return nullptr;
    }

    J3DModelData* modelData = static_cast<J3DModelData*>(res);
    if (modelData->getMaterialNum() == 0 || modelData->getShapeTable() == nullptr ||
        modelData->getShapeTable()->getShapeNum() == 0 ||
        modelData->getMaterialNodePointer(0) == nullptr) {
        return nullptr;
    }

    J3DModel* model = mDoExt_J3DModel__create(modelData, 0x80000, 0x11000284);
    if (model != nullptr) {
        model->setBaseScale(scale);
    }
    return model;
}

mDoExt_bckAnm* loadBckFromArcIdx(const char* arcName, int resIndex, int playMode, f32 rate) {
    if (arcName == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    if (loadObjectArchive(cleanArc) != 0) {
        return nullptr;
    }

    void* res = dComIfG_getObjectRes(cleanArc, resIndex);
    if (res == nullptr) {
        return nullptr;
    }

    if (std::memcmp(res, "J3D1", 4) == 0) {
        res = J3DAnmLoaderDataBase::load(res);
        if (res == nullptr) {
            return nullptr;
        }
    }

    J3DAnmTransform* pbck = static_cast<J3DAnmTransform*>(res);
    mDoExt_bckAnm* bckAnm = JKR_NEW mDoExt_bckAnm();
    if (bckAnm == nullptr) {
        return nullptr;
    }

    if (!bckAnm->init(pbck, TRUE, playMode, rate, 0, -1, false)) {
        JKR_DELETE(bckAnm);
        return nullptr;
    }

    return bckAnm;
}

mDoExt_brkAnm* loadBrkFromArc(const char* arcName, const char* brkName, J3DModelData* modelData, int playMode, f32 rate) {
    if (arcName == nullptr || brkName == nullptr || modelData == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    if (loadObjectArchive(cleanArc) != 0) {
        return nullptr;
    }

    char cleanBrk[64];
    std::strncpy(cleanBrk, brkName, sizeof(cleanBrk) - 1);
    cleanBrk[sizeof(cleanBrk) - 1] = '\0';
    char* dot = std::strstr(cleanBrk, ".brk");
    if (dot != nullptr) {
        *dot = '\0';
    }

    void* res = nullptr;
    dRes_info_c* info = dComIfG_getObjectResInfo(cleanArc);
    JKRArchive* archive = (info != nullptr) ? info->getArchive() : nullptr;
    if (archive != nullptr) {
        res = archive->getResource('BRK ', brkName);
        if (res == nullptr) {
            res = archive->getResource('BRK ', cleanBrk);
        }
    }

    if (res == nullptr) {
        res = dComIfG_getObjectRes(cleanArc, brkName);
    }
    if (res == nullptr) {
        res = dComIfG_getObjectRes(cleanArc, cleanBrk);
    }
    if (res == nullptr && std::strcmp(cleanArc, "E_fm") == 0) {
        if (std::strcmp(cleanBrk, "core_lighton") == 0) {
            res = dComIfG_getObjectRes("E_fm", 0x31);
        }
    }

    if (res == nullptr) {
        return nullptr;
    }

    if (std::memcmp(res, "J3D1", 4) == 0) {
        res = J3DAnmLoaderDataBase::load(res);
        if (res == nullptr) {
            return nullptr;
        }
    }

    J3DAnmTevRegKey* pbrk = static_cast<J3DAnmTevRegKey*>(res);
    mDoExt_brkAnm* brkAnm = JKR_NEW mDoExt_brkAnm();
    if (brkAnm == nullptr) {
        return nullptr;
    }

    if (!brkAnm->init(modelData, pbrk, 1, playMode, rate, 0, -1)) {
        JKR_DELETE(brkAnm);
        return nullptr;
    }

    return brkAnm;
}

mDoExt_btkAnm* loadBtkFromArc(const char* arcName, const char* btkName, J3DModelData* modelData, int playMode, f32 rate) {
    if (arcName == nullptr || btkName == nullptr || modelData == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    if (loadObjectArchive(cleanArc) != 0) {
        return nullptr;
    }

    char cleanBtk[64];
    std::strncpy(cleanBtk, btkName, sizeof(cleanBtk) - 1);
    cleanBtk[sizeof(cleanBtk) - 1] = '\0';
    char* dot = std::strstr(cleanBtk, ".btk");
    if (dot != nullptr) {
        *dot = '\0';
    }

    void* res = nullptr;
    dRes_info_c* info = dComIfG_getObjectResInfo(cleanArc);
    JKRArchive* archive = (info != nullptr) ? info->getArchive() : nullptr;
    if (archive != nullptr) {
        res = archive->getResource('BTK ', btkName);
        if (res == nullptr) {
            res = archive->getResource('BTK ', cleanBtk);
        }
        if (res == nullptr) {
            res = archive->getResource(btkName);
        }
        if (res == nullptr) {
            res = archive->getResource(cleanBtk);
        }
    }

    if (res == nullptr) {
        res = dComIfG_getObjectRes(cleanArc, btkName);
    }
    if (res == nullptr) {
        res = dComIfG_getObjectRes(cleanArc, cleanBtk);
    }
    if (res == nullptr && std::strcmp(cleanArc, "E_fm") == 0) {
        if (std::strcmp(cleanBtk, "core_beat") == 0) {
            res = dComIfG_getObjectRes("E_fm", 0x3F);
        } else {
            res = dComIfG_getObjectRes("E_fm", 0x42);
        }
    }

    if (res == nullptr) {
        return nullptr;
    }

    if (std::memcmp(res, "J3D1", 4) == 0) {
        res = J3DAnmLoaderDataBase::load(res);
        if (res == nullptr) {
            return nullptr;
        }
    }

    J3DAnmTextureSRTKey* pbtk = static_cast<J3DAnmTextureSRTKey*>(res);
    mDoExt_btkAnm* btkAnm = JKR_NEW mDoExt_btkAnm();
    if (btkAnm == nullptr) {
        return nullptr;
    }

    if (!btkAnm->init(modelData, pbtk, 1, playMode, rate, 0, -1)) {
        JKR_DELETE(btkAnm);
        return nullptr;
    }

    return btkAnm;
}

void renderModelAt(J3DModel* model, const cXyz& pos, const csXyz& angle, const cXyz& scale, mDoExt_bckAnm* bck) {
    if (model == nullptr) return;

    if (bck != nullptr) {
        bck->play();
        bck->entry(model->getModelData());
    }

    mDoMtx_stack_c::transS(pos.x, pos.y, pos.z);
    mDoMtx_stack_c::ZXYrotM(angle.x, angle.y, angle.z);
    model->setBaseScale(scale);
    model->setBaseTRMtx(mDoMtx_stack_c::get());
    model->calc();

    daAlink_c* alink = daAlink_getAlinkActorClass();
    if (alink != nullptr) {
        g_env_light.settingTevStruct_colget_player(&alink->tevStr);
        g_env_light.setLightTevColorType_MAJI(model, &alink->tevStr);
    }

    mDoExt_modelUpdateDL(model);
}

void renderModelAtMtx(J3DModel* model, MtxP mtx, mDoExt_bckAnm* bck) {
    if (model == nullptr) return;

    if (bck != nullptr) {
        bck->play();
        bck->entry(model->getModelData());
    }

    model->setBaseTRMtx(mtx);
    model->calc();

    daAlink_c* alink = daAlink_getAlinkActorClass();
    if (alink != nullptr) {
        g_env_light.settingTevStruct_colget_player(&alink->tevStr);
        g_env_light.setLightTevColorType_MAJI(model, &alink->tevStr);
    }

    mDoExt_modelUpdateDL(model);
}
