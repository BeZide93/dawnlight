#include "config.hpp"
#include "service_imports.hpp"

#include "d/actor/d_a_alink.h"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_matAnm_c::calc, LinkEyeMaterialCalcHook);

void after_link_eye_material_calc(ModContext*, void* args, void*, void*) {
    auto* animation = mods::arg<const daAlink_matAnm_c*>(args, 0);
    auto* material = mods::arg<J3DMaterial*>(args, 1);
    if (animation == nullptr || material == nullptr) {
        return;
    }

    const int percent = eye_movement_range_percent();
    if (percent == 100) {
        return;
    }

    const f32 factor = static_cast<f32>(percent) / 100.0f;
    for (u32 i = 0; i < 8; ++i) {
        if (!animation->getTexMtxAnm(i).getAnmFlag()) {
            continue;
        }

        J3DTexMtx* texMtx = material->getTexGenBlock()->getTexMtx(i);
        if (texMtx == nullptr) {
            continue;
        }

        J3DTextureSRTInfo& srt = texMtx->getTexMtxInfo().mSRT;
        srt.mTranslationX *= factor;
        srt.mTranslationY *= factor;
    }
}

}  // namespace

ModResult install_eye_movement_hooks(ModError* error) {
    const ModResult result =
        mods::hook::add_post<LinkEyeMaterialCalcHook>(svc_hook, after_link_eye_material_calc);
    if (result != MOD_OK) {
        return mods::set_error(
            error, result, "failed to install Dawnlight eye movement range hook");
    }
    return MOD_OK;
}

}  // namespace dawnlight
