#include "glider_bmd.hpp"
#include "glider_bmd_format.hpp"
#include "service_imports.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/J3DGraphLoader/J3DModelLoader.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "d/d_kankyo.h"
#include <dvd.h>

namespace dawnlight {
namespace {
JKRExpHeap* s_heap = nullptr;
J3DModel* s_model = nullptr;
J3DDrawBuffer* s_opaque = nullptr;
J3DDrawBuffer* s_translucent = nullptr;
bool s_attempted = false;
using ForgetModelMatrices = void (*)(J3DModel*);
ForgetModelMatrices s_forgetMatrices = nullptr;

struct CurrentHeap {
    JKRHeap* previous;
    explicit CurrentHeap(JKRHeap* heap) : previous(heap->becomeCurrentHeap()) {}
    ~CurrentHeap() { previous->becomeCurrentHeap(); }
};

void release_model() {
    if (s_model && s_forgetMatrices) s_forgetMatrices(s_model);
    s_model = nullptr;
    s_opaque = s_translucent = nullptr;
    if (s_heap) s_heap->destroy();
    s_heap = nullptr;
}
}

void prepare_glider_bmd() {
    if (s_attempted) return;
    s_attempted = true;
    // PC-only interpolation helpers are resolved through the host manifest,
    // rather than referenced through platform link stubs.
    void* address = nullptr;
    if (svc_hook && svc_hook->resolve &&
        svc_hook->resolve(mod_ctx, "J3DModel::forgetMtx", &address, nullptr) == MOD_OK)
        s_forgetMatrices = reinterpret_cast<ForgetModelMatrices>(address);
    // Called from the first actor queue, after overlays and the DVD are ready.
    DVDFileInfo file{};
    if (!DVDOpen(kGliderBmdPath, &file)) return;
    const auto size = file.length;
    if (size >= 32 && size <= kGliderBmdMaxBytes) {
        // Neither the file nor J3D's pointers into it may live in a room heap.
        s_heap = JKRExpHeap::create(8 * 1024 * 1024 + size * 4, JKRHeap::getRootHeap(), false);
        if (s_heap) {
            CurrentHeap scope(s_heap);
            const auto paddedSize = (size + 31) & ~31u;
            auto* bytes = s_heap->alloc(paddedSize, 32);
            if (bytes && DVDReadPrio(&file, bytes, paddedSize, 0, 2) >= static_cast<s32>(size) &&
                valid_glider_bmd(bytes, size)) {
                auto* data = J3DModelLoaderDataBase::load(bytes, 0x59020010);
                if (data && data->getJointNum() && data->getShapeNum() && data->getMaterialNum()) {
                    auto* model = JKR_NEW J3DModel();
                    if (model && model->entryModelData(data, 0, 1) == kJ3DError_Success) {
                        s_opaque = JKR_NEW J3DDrawBuffer();
                        s_translucent = JKR_NEW J3DDrawBuffer();
                        if (s_opaque && s_translucent &&
                            s_opaque->allocBuffer(32) == kJ3DError_Success &&
                            s_translucent->allocBuffer(32) == kJ3DError_Success) {
                            s_translucent->setZSort();
                            s_model = model;
                        }
                    }
                }
            }
        }
    }
    DVDClose(&file);
    if (!s_model) {
        release_model();
        if (svc_log) svc_log->warn(mod_ctx, "Glider BMD: failed to load; using the built-in model");
    } else if (svc_log) {
        svc_log->info(mod_ctx, "Glider BMD: loaded /res/Object/DawnlightGlider.bmd");
    }
}

bool draw_glider_bmd(Mtx transform, dKy_tevstr_c* lighting) {
    if (!s_model) return false;
    CurrentHeap scope(s_heap);
    // The outer GliderPacket is already in a retained world list. Never enter
    // this model into that list from draw(): it would mutate the active traversal.
    // Restore all J3D state, including the camera, for the next world packet.
    J3DSys saved = j3dSys;
    s_opaque->frameInit();
    s_translucent->frameInit();
    j3dSys.setDrawBuffer(s_opaque, 0);
    j3dSys.setDrawBuffer(s_translucent, 1);
    // Pose was sampled from Link's presented hands. Keep BMD joints in their
    // local rest pose and apply the attachment through the view matrix, so the
    // host does not interpolate the world transform for a second time.
    MTXConcat(saved.mViewMtx, transform, j3dSys.getViewMtx());
    s_translucent->setZMtx(s_model->getBaseTRMtx());
    j3dSys.reinitGX();
    if (lighting) g_env_light.setLightTevColorType_MAJI(s_model, lighting);
    s_model->unlock();
    s_model->update();
    s_model->lock();
    s_model->viewCalc();
    j3dSys.setDrawModeOpaTexEdge();
    s_opaque->draw();
    j3dSys.setDrawModeXlu();
    s_translucent->draw();
    j3dSys = saved;
    j3dSys.reinitGX();
    return true;
}

void shutdown_glider_bmd() {
    release_model();
    s_attempted = false;
}
}
