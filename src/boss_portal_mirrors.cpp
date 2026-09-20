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
#include <cstring>
#include <vector>

namespace dawnlight {
namespace {
constexpr char kArchive[] = "MR-Table";
ProfileName sProfile = -1;
ActorHandle sRegistration = 0;

// Dusklight's J3D loader converts vertex arrays to host byte order. Preserve
// the format/stride rather than treating every archive as an array of floats.
std::vector<mirror_geometry::Vec> read_vectors(const void* data, unsigned count,
                                              unsigned stride, int type, unsigned frac) {
    const unsigned bytes=type==GX_F32 ? 4 : (type==GX_S16 || type==GX_U16) ? 2 :
                         (type==GX_S8 || type==GX_U8) ? 1 : 0;
    if (!data || !bytes || stride<3*bytes || count>65536 || frac>31) return {};
    std::vector<mirror_geometry::Vec> result;
    result.reserve(count);
    const auto* input=static_cast<const unsigned char*>(data);
    for (unsigned i=0;i<count;++i) {
        mirror_geometry::Vec v{};
        bool valid=true;
        for (unsigned axis=0;axis<3;++axis) {
            const auto* p=input+i*stride+axis*bytes;
            float value=0;
            switch (type) {
            case GX_F32: std::memcpy(&value,p,4); break;
            case GX_S16: { s16 n; std::memcpy(&n,p,2); value=n; break; }
            case GX_U16: { u16 n; std::memcpy(&n,p,2); value=n; break; }
            case GX_S8: { s8 n; std::memcpy(&n,p,1); value=n; break; }
            case GX_U8: value=*p; break;
            }
            v[axis]=type==GX_F32 ? value : std::ldexp(value,-static_cast<int>(frac));
            valid=valid && std::isfinite(v[axis]);
        }
        if (valid) result.push_back(v);
    }
    return result;
}

class BossMirror : public fopAc_ac_c {
public:
    request_of_phase_process_class phase{};
    J3DModel* model = nullptr;
    J3DModel* frame = nullptr;
    mDoExt_bckAnm raisedPose;
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
        auto* frameData=static_cast<J3DModelData*>(dComIfG_getObjectRes(
            kArchive,dRes_INDEX_MR_TABLE_BMD_U_MR_TABLE_e));
        auto* pose=static_cast<J3DAnmTransform*>(dComIfG_getObjectRes(
            kArchive,dRes_INDEX_MR_TABLE_BCK_U_MR_TABLE_UP_e));
        if (!self->model || !frameData || !pose ||
            frameData->getJointNum()<=U_MR_TABLE_JNT_MIRROR_e) return 0;
        self->frame=mDoExt_J3DModel__create(frameData,0x80000,0x11000084);
        // Sample the completed vanilla raising animation explicitly. No timer,
        // room switches, stairs, sound, cutscenes or reflection actor are needed.
        return self->frame && self->raisedPose.init(pose,0,0,0,0,-1,false);
    }

    void frame_pose(bool draw) {
        auto* data=frame->getModelData();
        auto* root=data->getJointNodePointer(0);
        auto* previous=root->getMtxCalc();
        raisedPose.entry(data,raisedPose.getBckAnm()->getFrameMax());
        if (draw) mDoExt_modelUpdateDL(frame);
        else frame->calc();
        root->setMtxCalc(previous);
    }

    float frame_floor() {
        float floor=std::numeric_limits<float>::infinity();
        auto* data=frame->getModelData();
        for (unsigned joint=0;joint<data->getJointNum();++joint) {
            // Ignore empty attachment joints: their default zero bounds must
            // not pull a world-authored chamber stand away from the hub floor.
            for (auto* mesh=data->getJointNodePointer(joint)->getMesh();mesh;mesh=mesh->getNext()) {
                auto* shape=mesh->getShape();
                if (!shape) continue;
                const auto& min=*shape->getMin();
                const auto& max=*shape->getMax();
                const auto& matrix=frame->getAnmMtx(joint);
                for (unsigned corner=0;corner<8;++corner) {
                    const float x=corner&1 ? max.x : min.x;
                    const float y=corner&2 ? max.y : min.y;
                    const float z=corner&4 ? max.z : min.z;
                    floor=std::min(floor,matrix[1][0]*x+matrix[1][1]*y+matrix[1][2]*z+matrix[1][3]);
                }
            }
        }
        return floor;
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
        // First evaluate the entire assembly in the original chamber coordinates
        // at scale 1, including the frame's raised attachment bone and disc root.
        Mtx identity;
        MTXIdentity(identity);
        frame->setBaseScale(cXyz(1,1,1));
        frame->setBaseTRMtx(identity);
        frame_pose(false);
        model->setBaseScale(cXyz(1,1,1));
        model->setBaseTRMtx(frame->getAnmMtx(U_MR_TABLE_JNT_MIRROR_e));
        model->calc();
        auto read_matrix=[](const Mtx matrix) {
            mirror_geometry::Matrix result{};
            for (unsigned r=0;r<3;++r) for (unsigned c=0;c<4;++c) result[r][c]=matrix[r][c];
            return result;
        };
        mirror_geometry::Fit native,fit;
        if (!mirror_geometry::describe_disc(min,max,read_matrix(model->getAnmMtx(0)),native)) return false;
        mirror_geometry::Matrix placement;
        const float yaw=shape_angle.y*(6.2831853071795864769f/65536.0f);
        if (!mirror_geometry::place_assembly(native,frame_floor(),
                {current.pos.x,current.pos.y,current.pos.z},yaw,placement)) return false;
        Mtx base;
        for (unsigned r=0;r<3;++r) for (unsigned c=0;c<4;++c) base[r][c]=placement[r][c];
        frame->setBaseTRMtx(base);
        frame_pose(false);
        model->setBaseTRMtx(frame->getAnmMtx(U_MR_TABLE_JNT_MIRROR_e));
        model->calc();
        if (!mirror_geometry::describe_disc(min,max,read_matrix(model->getAnmMtx(0)),fit)) return false;
        const auto& vertices=data->getVertexData();
        const auto positions=read_vectors(vertices.getVtxPosArray(),
            std::min(vertices.getVtxNum(),vertices.getVtxArrNum(GX_VA_POS)),
            vertices.getVtxArrStride(GX_VA_POS),vertices.getVtxPosType(),vertices.getVtxPosFrac());
        const auto normals=read_vectors(vertices.getVtxNrmArray(),vertices.getNrmNum(),
            vertices.getVtxArrStride(GX_VA_NRM),vertices.getVtxNrmType(),vertices.getVtxNrmFrac());
        mirror_geometry::Face face;
        if (!mirror_geometry::mount_face(positions,normals,fit,face)) {
            svc_log->warn(mod_ctx,"Boss Rush mirror: no broad inward-facing surface found in the loaded mesh");
            return false;
        }
        surface.center.set(face.center[0],face.center[1],face.center[2]);
        surface.right.set(face.right[0],face.right[1],face.right[2]);
        surface.up.set(face.up[0],face.up[1],face.up[2]);
        surface.normal.set(face.normal[0],face.normal[1],face.normal[2]);
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
    if (!fopAcM_entrySolidHeap(self,BossMirror::make_heap,0x18000) || !self->place_model()) {
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
    g_env_light.setLightTevColorType_MAJI(self->frame,&self->tevStr);
    g_env_light.setLightTevColorType_MAJI(self->model,&self->tevStr);
    self->frame_pose(true);
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
