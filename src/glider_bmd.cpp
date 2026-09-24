#include "glider_bmd.hpp"
#include "glider_bmd_format.hpp"
#include "service_imports.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/J3DGraphLoader/J3DModelLoader.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "d/d_kankyo.h"
#include <dvd.h>
#include "mods/svc/resource.h"
#include <memory>
#include <new>

namespace dawnlight {
namespace {
JKRExpHeap* s_heap = nullptr;
std::unique_ptr<u8[]> s_heapStorage;
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
    s_heapStorage.reset();
}

bool create_model_heap(u32 size) {
    if (size < 32 || size > kGliderBmdMaxBytes) return false;
    // J3D retains pointers into the BMD. Keep both outside room heaps, backed
    // by host memory so loading the default model cannot exhaust the game heap.
    const u32 heapSize = 8 * 1024 * 1024 + size * 4;
    s_heapStorage.reset(new (std::nothrow) u8[heapSize + 31]);
    if (!s_heapStorage) return false;
    auto* memory = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(s_heapStorage.get()) + 31) & ~uintptr_t{31});
    s_heap = JKRExpHeap::create(memory, heapSize, JKRHeap::getRootHeap(), false);
    if (!s_heap) s_heapStorage.reset();
    return s_heap != nullptr;
}

bool load_model(void* bytes, u32 size) {
    if (!valid_glider_bmd(bytes, size)) return false;
    auto* data = J3DModelLoaderDataBase::load(bytes, 0x59020010);
    if (!data || !data->getJointNum() || !data->getShapeNum() || !data->getMaterialNum())
        return false;
    auto* model = JKR_NEW J3DModel();
    if (!model || model->entryModelData(data, 0, 1) != kJ3DError_Success) return false;
    s_opaque = JKR_NEW J3DDrawBuffer();
    s_translucent = JKR_NEW J3DDrawBuffer();
    if (!s_opaque || !s_translucent ||
        s_opaque->allocBuffer(32) != kJ3DError_Success ||
        s_translucent->allocBuffer(32) != kJ3DError_Success) return false;
    s_translucent->setZSort();
    s_model = model;
    return true;
}

bool load_overlay_model(DVDFileInfo& file) {
    const u32 size = file.length;
    if (!create_model_heap(size)) return false;
    CurrentHeap scope(s_heap);
    const auto paddedSize = (size + 31) & ~31u;
    auto* bytes = s_heap->alloc(paddedSize, 32);
    return bytes && DVDReadPrio(&file, bytes, paddedSize, 0, 2) >= static_cast<s32>(size) &&
           load_model(bytes, size);
}

bool load_bundled_model() {
    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    if (svc_resource->load(mod_ctx, "DawnlightGlider.bmd", &buffer) != MOD_OK) return false;
    bool loaded = false;
    if (valid_glider_bmd(buffer.data, buffer.size) &&
        create_model_heap(static_cast<u32>(buffer.size))) {
        CurrentHeap scope(s_heap);
        auto* bytes = s_heap->alloc(static_cast<u32>(buffer.size), 32);
        if (bytes) {
            std::memcpy(bytes, buffer.data, buffer.size);
            loaded = load_model(bytes, static_cast<u32>(buffer.size));
        }
    }
    // J3D uses the aligned heap copy, never the resource service's allocation.
    svc_resource->free(mod_ctx, &buffer);
    return loaded;
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
    // Overlay packs take priority; the bundled BMD is private to Dawnlight's
    // resources and cannot shadow a pack through DVD overlay ordering.
    DVDFileInfo file{};
    if (DVDOpen(kGliderBmdPath, &file)) {
        const bool loaded = load_overlay_model(file);
        DVDClose(&file);
        if (loaded) {
            if (svc_log) svc_log->info(mod_ctx, "Glider BMD: loaded /res/Object/DawnlightGlider.bmd");
            return;
        }
        release_model();
        if (svc_log) svc_log->warn(mod_ctx, "Glider BMD: overlay failed; trying bundled model");
    }
    if (load_bundled_model()) {
        if (svc_log) svc_log->info(mod_ctx, "Glider BMD: loaded bundled DawnlightCustomGlider model");
    } else {
        release_model();
        if (svc_log) svc_log->warn(mod_ctx, "Glider BMD: bundled model failed; using emergency mesh");
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
