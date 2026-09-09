#include "slow_motion/Controller.h"

// Spatial samples of the scene, feathered to zero at the center. The HUD is
// rendered later and remains sharp; no previous-frame trails on Link's pose.
static void drawSlowMotionEdges(view_class* view, float strength) {
    if (strength <= 0.001f) return;
    GXLoadTexObj(mDoGph_gInf_c::getFrameBufferTexObj(), GX_TEXMAP0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetZCompLoc(GX_TRUE);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
    GXSetFog(GX_FOG_NONE, 0, 0, 0, 0, g_clearColor);
    GXSetCullMode(GX_CULL_NONE);
    Mtx44 ortho;
    C_MTXOrtho(ortho, 0, 1, 0, 1, 0, 10);
    GXSetProjection(ortho, GX_ORTHOGRAPHIC);
    GXLoadPosMtxImm(cMtx_getIdentity(), 0);
    GXSetCurrentMtx(0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    constexpr int columns = 48;
    constexpr int rows = 24;
    for (int pass = 1; pass <= 6; ++pass) {
        auto vertex = [=](float x, float y) {
            const float edge = slow_motion::blur_weight(x, y);
            const float zoom = 1.0f + pass * 0.028f * strength * edge;
            GXPosition3f32(x, y, -1.0f);
            GXColor4u8(255, 255, 255, static_cast<u8>(edge * strength * 56.0f));
            GXTexCoord2f32(0.5f + (x - 0.5f) / zoom, 0.5f + (y - 0.5f) / zoom);
        };
        GXBegin(GX_QUADS, GX_VTXFMT0, columns * rows * 4);
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < columns; ++x) {
                vertex(float(x) / columns, float(y) / rows);
                vertex(float(x + 1) / columns, float(y) / rows);
                vertex(float(x + 1) / columns, float(y + 1) / rows);
                vertex(float(x) / columns, float(y + 1) / rows);
            }
        }
        GXEnd();
    }
    GXSetProjection(view->projMtx, GX_PERSPECTIVE);
}
