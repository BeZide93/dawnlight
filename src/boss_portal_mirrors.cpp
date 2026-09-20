#include "boss_portal_mirrors.hpp"
#include "boss_mirror_geometry.hpp"
#include "service_imports.hpp"

#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "m_Do/m_Do_ext.h"
#include "mods/service.hpp"
#include "res/Object/MR-Table.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dawnlight {
namespace {
constexpr char kArchive[] = "MR-Table";
constexpr float kDiameter = 440.0f;
constexpr float kCenterHeight = 240.0f;
ProfileName sProfile = -1;
ActorHandle sRegistration = 0;

class BossMirror : public fopAc_ac_c {
public:
    request_of_phase_process_class phase{};
    J3DModel* model = nullptr;
    BossPortalSymbolSurface surface{};
    bool ready = false;

    ~BossMirror() { dComIfG_resDelete(&phase, kArchive); }

    static int make_heap(fopAc_ac_c* actor) {
        auto* self = static_cast<BossMirror*>(actor);
        auto* data = static_cast<J3DModelData*>(dComIfG_getObjectRes(
            kArchive, dRes_INDEX_MR_TABLE_BMD_U_MR_MIRROR_e));
        if (!data || data->getJointNum() != 1 || !data->getShapeNum()) return 0;
        // The engine owns the per-actor heap; archive data is shared/refcounted.
        self->model = mDoExt_J3DModel__create(data, 0, 0x11000084);
        return self->model != nullptr;
    }

    bool place_model() {
        using mirror_geometry::Vec;
        auto* data = model->getModelData();
        const float inf = std::numeric_limits<float>::infinity();
        Vec min = {inf,inf,inf}, max = {-inf,-inf,-inf};
        for (unsigned i = 0; i < data->getShapeNum(); ++i) {
            auto* shape = data->getShapeNodePointer(i);
            if (!shape) return false;
            const auto& lo = *shape->getMin();
            const auto& hi = *shape->getMax();
            const Vec a = {lo.x,lo.y,lo.z}, b = {hi.x,hi.y,hi.z};
            for (unsigned axis = 0; axis < 3; ++axis) {
                if (!std::isfinite(a[axis]) || !std::isfinite(b[axis])) return false;
                min[axis] = std::min(min[axis],a[axis]);
                max[axis] = std::max(max[axis],b[axis]);
            }
        }
        mirror_geometry::Fit fit;
        const Vec center = {current.pos.x, current.pos.y+kCenterHeight, current.pos.z};
        const float yaw = shape_angle.y*(6.2831853071795864769f/65536.0f);
        if (!mirror_geometry::fit(min,max,center,yaw,kDiameter,fit)) return false;

        // Cancel the mesh's bind-pose root transform before applying our fit.
        // This keeps the emblem aligned even when the archive uses an offset pivot.
        Mtx identity, inverseRoot, meshToWorld, base;
        MTXIdentity(identity);
        model->setBaseTRMtx(identity);
        model->setBaseScale(cXyz(1,1,1));
        model->calc();
        if (!MTXInverse(model->getAnmMtx(0),inverseRoot)) return false;
        for (unsigned r = 0; r < 3; ++r)
            for (unsigned c = 0; c < 4; ++c) meshToWorld[r][c] = fit.meshToWorld[r][c];
        MTXConcat(meshToWorld,inverseRoot,base);
        model->setBaseTRMtx(base);
        model->calc();
        surface.center.set(fit.center[0],fit.center[1],fit.center[2]);
        surface.right.set(fit.right[0],fit.right[1],fit.right[2]);
        surface.up.set(fit.up[0],fit.up[1],fit.up[2]);
        surface.normal.set(fit.normal[0],fit.normal[1],fit.normal[2]);
        surface.halfDepth = fit.halfDepth;
        attention_info.flags = 0;
        eyePos = attention_info.position = surface.center;
        // Only 18 small static models; avoid culling against the archive's old pivot.
        fopAcM_SetMtx(this, model->getBaseTRMtx());
        ready = true;
        return true;
    }
};

int create_mirror(void* actor) {
    auto* self = static_cast<BossMirror*>(actor);
    fopAcM_ct(self, BossMirror);
    if (self->ready) return cPhs_COMPLEATE_e;
    const auto phase = dComIfG_resLoad(&self->phase,kArchive);
    if (phase != cPhs_COMPLEATE_e) return phase;
    if (!fopAcM_entrySolidHeap(self,BossMirror::make_heap,0x10000) || !self->place_model()) {
        svc_log->warn(mod_ctx,"Boss Rush mirror: could not initialize the complete Mirror Chamber disc");
        return cPhs_ERROR_e;
    }
    return cPhs_COMPLEATE_e;
}
int delete_mirror(void* actor) {
    static_cast<BossMirror*>(actor)->~BossMirror();
    return 1;
}
int execute_mirror(void*) { return 1; }
int can_delete_mirror(void*) { return 1; }
int draw_mirror(void* actor) {
    auto* self = static_cast<BossMirror*>(actor);
    if (!self->ready) return 1;
    g_env_light.settingTevStruct(0x10,&self->current.pos,&self->tevStr);
    g_env_light.setLightTevColorType_MAJI(self->model,&self->tevStr);
    mDoExt_modelUpdateDL(self->model);
    return 1;
}
} // namespace

ModResult initialize_boss_portal_mirrors(ModError* error) {
    if (sRegistration) return MOD_OK;
    const ActorProfileDesc desc = {
        .name = "DLBMir",
        .priority_group = 7,
        .process_size = sizeof(BossMirror),
        .draw_priority = fpcDwPi_Obj_MirrorTable_e,
        .status = fopAcStts_UNK_0x40000_e,
        .group = fopAc_ACTOR_e,
        .cull_type = fopAc_CULLBOX_CUSTOM_e,
        .create_function = create_mirror,
        .delete_function = delete_mirror,
        .execute_function = execute_mirror,
        .is_delete_function = can_delete_mirror,
        .draw_function = draw_mirror,
    };
    const auto result = svc_actor->register_actor(mod_ctx,&desc,&sProfile,&sRegistration);
    if (result != MOD_OK) return mods::set_error(error,result,"failed to register Boss Rush mirror actor");
    return MOD_OK;
}

void shutdown_boss_portal_mirrors() {
    if (!sRegistration) return;
    // ActorService drains this profile's live actors before releasing callbacks.
    if (svc_actor->unregister_actor(mod_ctx,sRegistration) == MOD_OK) {
        sRegistration = 0;
        sProfile = -1;
    } else {
        svc_log->warn(mod_ctx,"Boss Rush mirrors: actor cleanup deferred to host shutdown");
    }
}

ActorId create_boss_portal_mirror(unsigned index, const cXyz& position, s16 facing, s8 room) {
    if (!sRegistration || index >= kBossPortalSymbolCount) return fpcM_ERROR_PROCESS_ID_e;
    ActorSpawnParams params{};
    params.parameters = index;
    params.argument = -1;
    params.room_num = room;
    params.position = {position.x,position.y,position.z};
    params.angle = {0,facing,0};
    params.scale = {1,1,1};
    ActorId id = fpcM_ERROR_PROCESS_ID_e;
    if (svc_actor->create_actor(mod_ctx,sProfile,&params,&id) != MOD_OK) return fpcM_ERROR_PROCESS_ID_e;
    return id;
}

bool boss_portal_mirror_surface(ActorId id, BossPortalSymbolSurface& surface) {
    if (!sRegistration || id == fpcM_ERROR_PROCESS_ID_e || fpcM_IsCreating(id)) return false;
    auto* actor = fopAcM_SearchByID(id);
    if (!actor || fopAcM_GetName(actor) != sProfile) return false;
    const auto* mirror = static_cast<const BossMirror*>(actor);
    if (!mirror->ready) return false;
    surface = mirror->surface;
    return true;
}
} // namespace dawnlight
