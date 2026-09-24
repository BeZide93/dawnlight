#include "glider_visual.hpp"
#include "generated/glider_art.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
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

// A persistent packet owns only a pose snapshot, never an actor/archive pointer.
// Queued through the same native opaque list as the custom Shade pedestal.
class GliderPacket : public J3DPacket {
public:
    cXyz origin{0,0,0};
    float sine=0, cosine=1;
    GXColor color{255,255,255,255};
    GXTexObj texture{};
    bool ready=false, active=false;

    void draw() override {
        if (!active) return;
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
                GXPosition3f32(origin.x+cosine*v.x+sine*v.z,origin.y+v.y,
                    origin.z-sine*v.x+cosine*v.z);
                GXColor4u8(color.r,color.g,color.b,255);
                GXTexCoord2f32(v.u,v.v);
            }
        }
        GXEnd();
        j3dSys.reinitGX();
    }
};
GliderPacket s_glider;
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

void clear_glider_visual() { s_glider.active=false; }

void queue_glider_visual(daAlink_c* link) {
    if (!link->mpLinkModel || link->checkPlayerNoDraw() || link->checkStatusWindowDraw()) return;
    // mLeft/RightHandPos only advance on simulation ticks. Sample the same
    // presented joints as Link's renderer on EVERY draw, including intermediate
    // frames, so a smoothly moving camera cannot slide past a frozen glider.
    Mtx left, right, root;
    presented_matrix(link->mpLinkModel->getAnmMtx(link->mLeftHandJntNo), left);
    presented_matrix(link->mpLinkModel->getAnmMtx(link->mRightHandJntNo), right);
    s_glider.origin.set((left[0][3]+right[0][3])*0.5f,
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
    s_glider.sine=std::sin(yaw);
    s_glider.cosine=std::cos(yaw);
    const auto& ambient=link->tevStr.AmbCol;
    // Follow the player's room lighting; retain a little fill for the woodwork.
    auto channel=[](int value) { return static_cast<u8>(std::clamp(value+48,48,255)); };
    s_glider.color={channel(ambient.r),channel(ambient.g),channel(ambient.b),255};
    s_glider.active=true;
    dComIfGd_getOpaList()->entryImm(&s_glider,0);
}
}
