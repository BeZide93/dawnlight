#include "glider_visual.hpp"
#include "glider_bmd.hpp"
#include "generated/glider_art.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_demo_item.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_name.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include <algorithm>
#include <cmath>
#include <iterator>

namespace dawnlight {
namespace {
// Resolve through the host manifest: private interpolation functions are not
// part of the cross-platform mod link stubs.
using LookupPresentationMatrix = bool (*)(const void*, Mtx);
LookupPresentationMatrix s_lookupPresentationMatrix = nullptr;

void presented_matrix(MtxP source, Mtx out) {
    if (!s_lookupPresentationMatrix || !s_lookupPresentationMatrix(source, out))
        MTXCopy(source, out);
}

bool prepare_glider_draw();
bool prepare_glider_reward_draw();

// Keep only an actor ID across frames; resolve the live pose at packet draw time.
// Queued through the same native opaque list as the custom Shade pedestal.
class GliderPacket : public J3DPacket {
public:
    ActorId ownerId = fpcM_ERROR_PROCESS_ID_e;
    cXyz origin{0,0,0};
    float sine=0, cosine=1;
    float modelScale=1.0f;
    ActorId rewardItem=fpcM_ERROR_PROCESS_ID_e;
    bool reward=false;
    GXColor color{255,255,255,255};
    GXTexObj texture{};
    bool ready=false, active=false;

    void draw() override {
        if (reward ? !prepare_glider_reward_draw() : !prepare_glider_draw()) return;
        // Both presentations share the same optional model and attachment pose.
        Mtx transform = {
            {modelScale*cosine, 0, modelScale*sine, origin.x},
            {0, modelScale, 0, origin.y},
            {-modelScale*sine, 0, modelScale*cosine, origin.z}
        };
        auto* link = static_cast<daAlink_c*>(fopAcM_SearchByID(ownerId));
        // A zero appear scale would make BMD normal matrices singular.
        if (modelScale <= 0.0001f) return;
        if (draw_glider_bmd(transform, &link->tevStr)) return;
        if (!ready) {
            GXInitTexObj(&texture,glider_art::kPixels,glider_art::kSize,glider_art::kSize,
                GX_TF_RGB565,GX_CLAMP,GX_CLAMP,GX_TRUE);
            GXInitTexObjLOD(&texture,GX_LIN_MIP_LIN,GX_LINEAR,0,glider_art::kLastMip,
                0,GX_FALSE,GX_FALSE,GX_ANISO_1);
            ready=true;
        }
        j3dSys.reinitGX();
        GXLoadPosMtxImm(j3dSys.getViewMtx(),GX_PNMTX0);
        GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);
        GXLoadTexObj(&texture,GX_TEXMAP0);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_MODULATE);
        GXSetZMode(GX_TRUE,GX_LEQUAL,GX_TRUE);
        GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
        // The canvas has a visible underside, including when viewed from below.
        GXSetCullMode(GX_CULL_NONE);
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,std::size(glider_art::kTriangles)*3);
        for (const auto& triangle : glider_art::kTriangles) {
            for (const auto index : triangle) {
                const auto& v=glider_art::kVertices[index];
                GXPosition3f32(origin.x+modelScale*(cosine*v.x+sine*v.z),origin.y+modelScale*v.y,
                    origin.z+modelScale*(-sine*v.x+cosine*v.z));
                GXColor4u8(color.r,color.g,color.b,255);
                GXTexCoord2f32(v.u,v.v);
            }
        }
        GXEnd();
        j3dSys.reinitGX();
    }
};
GliderPacket s_glider;
GliderPacket s_rewardGlider;

void queue_glider_packet(J3DPacket* packet) {
    auto* list = dComIfGd_getOpaList();
    if (!list || !list->mpBuffer || list->getEntryTableSize() == 0) return;
    // entryImm is an intrusive prepend, not a set. Re-inserting a retained
    // packet creates a cycle and draws until Aurora's vertex buffer overflows.
    // Check actual membership: draw lists survive presentation-only frames,
    // while frameInit clears their heads without clearing packet next pointers.
    for (auto* queued = list->mpBuffer[0]; queued; queued = queued->getNextPacket()) {
        if (queued == packet) return;
    }
    list->entryImm(packet, 0);
}

bool prepare_glider_pose(GliderPacket& packet) {
    if (!packet.active) return false;
    auto* actor = fopAcM_SearchByID(packet.ownerId);
    if (!actor || fopAcM_GetName(actor) != fpcNm_ALINK_e) return false;
    auto* link = static_cast<daAlink_c*>(actor);
    if (!link->mpLinkModel || link->checkPlayerNoDraw() || link->checkStatusWindowDraw()) return false;
    // fpcLf_Draw skips Link on presentation-only frames. Interpolation lookup
    // must happen here in J3DPacket::draw, which is replayed on every render,
    // rather than when Link first queues this packet during the simulation tick.
    // Hand joints are the wrists. Use the item attachment joints instead: these
    // are the native sword/shield grip points inside the closed hands. Sample
    // their presented matrices on EVERY draw, including intermediate frames,
    // so the grip points keep the same interpolation as Link's renderer.
    Mtx left, right, root;
    presented_matrix(link->mpLinkModel->getAnmMtx(link->mLeftItemJntNo), left);
    presented_matrix(link->mpLinkModel->getAnmMtx(link->mRightItemJntNo), right);
    packet.origin.set((left[0][3]+right[0][3])*0.5f,
        (left[1][3]+right[1][3])*0.5f, (left[2][3]+right[2][3])*0.5f);
    // Apply the presented root's yaw delta as well. Using shape_angle alone
    // would still snap the canopy on turns; a matrix delta also crosses +/-pi.
    MtxP currentRoot = link->mpLinkModel->getAnmMtx(0);
    presented_matrix(currentRoot, root);
    float turnSine = 0, turnCosine = 0;
    for (int axis = 0; axis < 3; ++axis) {
        turnSine += root[0][axis]*currentRoot[2][axis] - root[2][axis]*currentRoot[0][axis];
        turnCosine += root[0][axis]*currentRoot[0][axis] + root[2][axis]*currentRoot[2][axis];
    }
    const float yaw=link->shape_angle.y*(3.14159265358979323846f/32768.0f) +
        std::atan2(turnSine, turnCosine);
    packet.sine=std::sin(yaw);
    packet.cosine=std::cos(yaw);
    const auto& ambient=link->tevStr.AmbCol;
    // Follow the player's room lighting; retain a little fill for the woodwork.
    // Keep the canvas/runes readable in shadow while retaining ambient tint.
    auto channel=[](int value) { return static_cast<u8>(std::clamp(160+value*95/255,160,255)); };
    packet.color={channel(ambient.r),channel(ambient.g),channel(ambient.b),255};
    return true;
}

bool prepare_glider_draw() {
    return prepare_glider_pose(s_glider);
}

bool prepare_glider_reward_draw() {
    auto* actor = fopAcM_SearchByID(s_rewardGlider.rewardItem);
    if (!s_rewardGlider.active || !actor || fopAcM_GetName(actor) != fpcNm_Demo_Item_e) return false;
    auto* item = static_cast<daDitem_c*>(actor);
    auto* link = daAlink_getAlinkActorClass();
    if (!item->chkDraw() || item->chkDead() || !link ||
        fopAcM_GetID(link) != s_rewardGlider.ownerId || link->mProcID != daAlink_c::PROC_GET_ITEM)
        return false;
    // A compact presentation copy fits the native reward camera. Keep the
    // model origin at Link's two item-grip joints and the native appear scale.
    s_rewardGlider.modelScale = 0.45f * std::clamp(item->scale.x, 0.0f, 1.0f);
    return prepare_glider_pose(s_rewardGlider);
}

}

void init_glider_visual() {
    s_lookupPresentationMatrix = nullptr;
    void* address = nullptr;
    if (svc_hook && svc_hook->resolve &&
        svc_hook->resolve(mod_ctx, "dusk::interp::lookup_replacement", &address, nullptr) == MOD_OK)
        s_lookupPresentationMatrix = reinterpret_cast<LookupPresentationMatrix>(address);
    if (!s_lookupPresentationMatrix && svc_log)
        svc_log->warn(mod_ctx, "Glider: presentation matrix lookup unavailable; using simulation pose");
}

void clear_glider_visual() {
    s_glider.active=false;
    s_glider.ownerId=fpcM_ERROR_PROCESS_ID_e;
}

void queue_glider_visual(daAlink_c* link) {
    if (!link->mpLinkModel || link->checkPlayerNoDraw() || link->checkStatusWindowDraw()) return;
    prepare_glider_bmd();
    s_glider.ownerId=fopAcM_GetID(link);
    s_glider.active=true;
    queue_glider_packet(&s_glider);
}

void clear_glider_reward_visual() {
    s_rewardGlider.active = false;
    s_rewardGlider.ownerId = fpcM_ERROR_PROCESS_ID_e;
    s_rewardGlider.rewardItem = fpcM_ERROR_PROCESS_ID_e;
}

void queue_glider_reward_visual(daAlink_c* link, ActorId item) {
    if (!link || !link->mpLinkModel || link->checkPlayerNoDraw() || link->checkStatusWindowDraw()) return;
    prepare_glider_bmd();
    s_rewardGlider.ownerId = fopAcM_GetID(link);
    s_rewardGlider.rewardItem = item;
    s_rewardGlider.reward = true;
    s_rewardGlider.active = true;
    queue_glider_packet(&s_rewardGlider);
}

}
