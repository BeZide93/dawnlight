#include "fierce_deity.hpp"
#include "service_imports.hpp"
#include "fierce_deity_wipe.hpp"

#include "d/actor/d_a_alink.h"
#include "JSystem/J3DGraphBase/J3DMaterial.h"
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DSys.h"
#include "JSystem/J3DGraphBase/J3DShapeDraw.h"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRSolidHeap.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JParticle/JPAResource.h"
#include "d/d_com_inf_game.h"
#include "d/d_particle_name.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_camera_mng.h"
#include "f_pc/f_pc_leaf.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_mtx.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"

#include <array>
#include <cstdio>
#include <cstring>

namespace dawnlight {
namespace {

// Independent implementation of the dark glossy/red-eye appearance. Do not
// patch shared BMD materials: another actor (including Dark Link) can use them.
// Apply GX state only after the native material/differed display lists, and replay
// those lists immediately afterward, including between batched shared materials.
DEFINE_HOOK(&J3DMatPacket::draw, FierceMaterialDrawHook);
DEFINE_HOOK(&J3DShapePacket::draw, FierceShapePacketDrawHook);
DEFINE_HOOK(&J3DShapePacket::drawFast, FierceShapePacketFastHook);
DEFINE_HOOK(&J3DShape::drawFast, FierceShapeDrawHook);
DEFINE_HOOK(&J3DShapeDraw::draw, FiercePrimitiveDrawHook);
DEFINE_HOOK(&JPAResource::calc, FierceWarpParticleCalcHook);
#if defined(__APPLE__)
DEFINE_HOOK(&fpcMtd_Method, FiercePlayerDrawHook);
#else
DEFINE_HOOK(&fpcLf_DrawMethod, FiercePlayerDrawHook);
#endif

struct DrawScope {
    J3DShapePacket* packet = nullptr;
    J3DMatPacket* materialPacket = nullptr;
    bool applied = false;
    bool dark = false;
    bool warp = false;
    bool inverse = false;
    int warpCoord = -1;
};
std::array<DrawScope, 16> s_drawScopes{};
size_t s_drawDepth = 0;

// Shadow image rendering also calls ShapePacket::drawFast, but deliberately
// loads one untextured stage and zero texgens instead of the model's material.
// Applying model TEV stages there activates stale texture references (tcg src
// 21); replaying the model DL afterward also corrupts the remaining shadows.
// Only shade packets in the current regular material draw's batch.
std::array<J3DMatPacket*, 16> s_materialScopes{};
size_t s_materialDepth = 0;

HookAction before_material_draw(ModContext*, void* args, void*, void*) {
    if (s_materialDepth < s_materialScopes.size()) {
        s_materialScopes[s_materialDepth] = mods::arg<J3DMatPacket*>(args, 0);
    }
    ++s_materialDepth;
    return HOOK_CONTINUE;
}

void after_material_draw(ModContext*, void*, void*, void*) {
    if (s_materialDepth == 0) return;
    --s_materialDepth;
    if (s_materialDepth < s_materialScopes.size()) s_materialScopes[s_materialDepth] = nullptr;
}

J3DMatPacket* drawing_material(J3DShapePacket* packet) {
    if (s_materialDepth == 0 || s_materialDepth > s_materialScopes.size()) return nullptr;
    auto* material = s_materialScopes[s_materialDepth - 1];
    if (material == nullptr) return nullptr;
    // A material batch may contain several actors/models with shared materials.
    for (auto* shape = material->getShapePacket(); shape != nullptr;
         shape = static_cast<J3DShapePacket*>(shape->getNextPacket())) {
        if (shape == packet) return material;
    }
    return nullptr;
}

bool player_model(daAlink_c* link, const J3DModel* model) {
    return model != nullptr && (model == link->mpLinkModel ||
        model == link->mpLinkFaceModel || model == link->mpLinkHatModel ||
        model == link->mpLinkHandModel || model == link->mpDemoFCBlendModel ||
        model == link->mpDemoFCTongueModel || model == link->mpDemoHLTmpModel ||
        model == link->mpDemoHRTmpModel || model == link->mpLinkBootModels[0] ||
        model == link->mpLinkBootModels[1]);
}

#include "fierce_deity_transition.inc"

HookAction before_packet_draw(ModContext*, void* args, void*, void*) {
    auto* packet = mods::arg<J3DShapePacket*>(args, 0);
    if (s_drawDepth < s_drawScopes.size()) {
        auto& scope = s_drawScopes[s_drawDepth];
        scope = {};
        if (packet != nullptr) {
            const auto layer = warp_layer(packet->getModel());
            const bool dark = layer.active ? layer.dark :
                (fierce_deity_dark_visual_active() &&
                 player_model(daAlink_getAlinkActorClass(), packet->getModel()));
            if (layer.active || dark) {
                scope.materialPacket = drawing_material(packet);
                if (scope.materialPacket != nullptr) {
                    scope.packet = packet;
                    scope.dark = dark;
                    scope.warp = layer.active;
                    scope.inverse = layer.inverse;
                }
            }
        }
    }
    ++s_drawDepth;
    return HOOK_CONTINUE;
}

void restore_draw_state(DrawScope& scope) {
    if (!scope.applied) return;
    auto* packet = scope.packet;
    // Replay the actual batch material, which can belong to another model.
    scope.materialPacket->callDL();
    if (packet->getDisplayListObj() != nullptr) packet->getDisplayListObj()->callDL();
    scope.applied = false;
}

void after_packet_draw(ModContext*, void*, void*, void*) {
    if (s_drawDepth == 0) return;
    --s_drawDepth;
    if (s_drawDepth < s_drawScopes.size()) {
        restore_draw_state(s_drawScopes[s_drawDepth]);
        s_drawScopes[s_drawDepth] = {};
    }
}

// Reuse an identity table if present. Only unused tables may be repurposed, since
// the original stages still compute texture alpha (hair, eyelashes, cutouts).
struct SwapTables {
    int identity = -1;
    int red = -1;
    int blue = -1;
};

SwapTables find_swap_tables(J3DTevBlock* tev, unsigned count) {
    std::array<bool, 4> used{};
    SwapTables result;
    for (unsigned i = 0; i < count; ++i) {
        const auto* stage = tev->getTevStage(i);
        if (stage == nullptr) return result;
        used[stage->mTevSwapModeInfo & 3] = true;
        used[(stage->mTevSwapModeInfo >> 2) & 3] = true;
    }
    for (int i = 0; i < 4; ++i) {
        const auto* table = tev->getTevSwapModeTable(i);
        if (table != nullptr && table->getR() == GX_CH_RED &&
            table->getG() == GX_CH_GREEN && table->getB() == GX_CH_BLUE &&
            table->getA() == GX_CH_ALPHA) {
            result.identity = i;
            break;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (used[i] || i == result.identity) continue;
        if (result.identity < 0) result.identity = i;
        else if (result.red < 0) result.red = i;
        else if (result.blue < 0) result.blue = i;
    }
    return result;
}

const J3DTevOrder* eye_texture_order(J3DModelData* data, J3DMaterial* material) {
    const auto* names = data->getMaterialName();
    if (names == nullptr) return nullptr;
    const char* name = names->getName(material->getIndex());
    // The al/bl/ml/zl outfits and compatible replacements use these suffixes.
    if (name == nullptr || (std::strstr(name, "eyeballL") == nullptr &&
                            std::strstr(name, "eyeballR") == nullptr)) return nullptr;
    const auto* textureNames = data->getTextureName();
    const auto* texture = data->getTexture();
    if (textureNames == nullptr || texture == nullptr) return nullptr;
    auto* tev = material->getTevBlock();
    for (unsigned i = 0; i < tev->getTevStageNum(); ++i) {
        const auto* order = tev->getTevOrder(i);
        if (order == nullptr || order->mTexCoord >= 8 || order->getTexMap() >= 8) continue;
        // Tev blocks have different texture-slot capacities. Only query a map
        // used by an existing stage, never an arbitrary assumed slot.
        const u16 texNo = tev->getTexNo(order->getTexMap());
        if (texNo >= texture->getNum()) continue;
        const char* texName = textureNames->getName(texNo);
        if (texName != nullptr && std::strstr(texName, "eyeball") != nullptr) return order;
    }
    return nullptr;
}

// J3D material display lists update live registers without updating GX's CPU
// shadow state. GXSetNumChans/GXSetNumTevStages would flush a stale GEN_MODE,
// losing numTexGens (Aurora then sees GX_MAX_TEXGENSRC / "tcg src 21").
// TREF and KSEL also pack fields belonging to existing stages. Emit a small
// display list with BP masks against the *live* state, as J3D itself does.
// No GX shadow-state setters or changes to shared material data are needed.
class DarkDisplayList {
    alignas(32) std::array<u8, 1024> bytes{};
    size_t size = 0;
    bool valid = true;

    void byte(u8 value) {
        if (size < bytes.size()) bytes[size++] = value;
        else valid = false;
    }
    void word(u32 value) {
        byte(value >> 24); byte(value >> 16); byte(value >> 8); byte(value);
    }
    void bp(u8 reg, u32 value) {
        byte(0x61); word((u32(reg) << 24) | (value & 0xFFFFFF));
    }
    void masked_bp(u8 reg, u32 mask, u32 value) {
        bp(0xFE, mask); bp(reg, value);
    }
    void xf(u16 reg, u32 value) {
        byte(0x10); word(reg); word(value); // one XF word, big-endian
    }
    void xf_float(u16 reg, float value) {
        u32 bits;
        std::memcpy(&bits, &value, sizeof(bits));
        xf(reg, bits);
    }
    static u32 rgba(GXColor color) {
        return (u32(color.r) << 24) | (u32(color.g) << 16) |
               (u32(color.b) << 8) | color.a;
    }

public:
    void lighting(bool eye, u8 ambientAlpha, u8 materialAlpha) {
        xf(0x1009, 2); // XF number of color channels
        xf(0x100B, rgba({64, 64, 64, ambientAlpha}));
        xf(0x100D, rgba(eye ? GXColor{230, 8, 5, materialAlpha}
                           : GXColor{32, 36, 42, materialAlpha}));
        // COLOR1 only: register sources, clamp diffuse, no attenuation, light0
        // on the body; unlit eyes. Preserve the native ALPHA1 control.
        xf(0x100F, (1u << 10) | (u32(GX_DF_CLAMP) << 7) |
                     (eye ? 0u : (1u << 1) | (1u << 2)));
    }
    void specular_lighting(u8 ambientAlpha0, u8 materialAlpha0,
                           u8 ambientAlpha1, u8 materialAlpha1) {
        xf(0x1009, 2);
        // Dark diffuse base on COLOR0; preserve both native alpha channels.
        xf(0x100A, rgba({64, 64, 64, ambientAlpha0}));
        xf(0x100C, rgba({32, 36, 42, materialAlpha0}));
        xf(0x100E, (1u << 10) | (u32(GX_DF_CLAMP) << 7) | (1u << 1) | (1u << 2));
        // COLOR1 uses GX_AF_SPEC: attenuation enabled, diffuse disabled.
        xf(0x100B, rgba({0, 0, 0, ambientAlpha1}));
        xf(0x100D, rgba({100, 112, 128, materialAlpha1}));
        xf(0x100F, (1u << 9) | (1u << 1) | (1u << 2));

        // A soft camera-space key light keeps the black surface readable in
        // dark rooms. Both channels use light0, with different attenuation.
        // L = (-0.4, 0.5, sqrt(0.59)); H = normalize(L + view direction).
        // Use Aurora's finite distance to avoid overflow on mobile GPUs.
        xf(0x0603, rgba({255, 255, 255, 255}));
        xf_float(0x0604, 0.0f); xf_float(0x0605, 0.0f); xf_float(0x0606, 1.0f);
        xf_float(0x0607, 16.0f); xf_float(0x0608, 0.0f); xf_float(0x0609, -15.0f);
        xf_float(0x060A, -419430.4f);
        xf_float(0x060B, 524288.0f);
        xf_float(0x060C, 805414.91f);
        xf_float(0x060D, -0.212703f);
        xf_float(0x060E, 0.265879f);
        xf_float(0x060F, 0.940248f);
    }
    void swap(unsigned table, unsigned r, unsigned g, unsigned b, unsigned a) {
        // KSEL's upper 20 bits belong to native stage konst selections.
        masked_bp(0xF6 + 2 * table, 0xF, r | (g << 2));
        masked_bp(0xF7 + 2 * table, 0xF, b | (a << 2));
    }
    void stage(unsigned index, GXTexCoordID coord, GXTexMapID map,
               unsigned identity, unsigned textureSwap,
               GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d,
               GXTevOp op = GX_TEV_ADD, GXTevScale scale = GX_CS_SCALE_1,
               unsigned rasterChannel = 1) {
        const unsigned shift = (index & 1) * 12;
        const bool textured = coord != GX_TEXCOORD_NULL && map != GX_TEXMAP_NULL;
        const u32 order = (rasterChannel << 7) | (textured ?
            (u32(map) | (u32(coord) << 3) | (1u << 6)) : 0u); // COLOR0A0 or COLOR1A1
        // Preserve the other half, even when it is an animated native stage.
        masked_bp(0x28 + index / 2, 0xFFFu << shift, order << shift);
        bp(0x10 + index, 0); // direct TEV stage (no indirect lookup)
        bp(0xC0 + 2 * index, u32(d) | (u32(c) << 4) | (u32(b) << 8) |
            (u32(a) << 12) | (u32(op) << 18) | (1u << 19) | (u32(scale) << 20));
        // Preserve the native alpha chain: (0 * (1 - 0) + 0 * 0) + APREV.
        bp(0xC1 + 2 * index, identity | (textureSwap << 2) |
            (u32(GX_CA_APREV) << 4) | (u32(GX_CA_ZERO) << 7) |
            (u32(GX_CA_ZERO) << 10) | (u32(GX_CA_ZERO) << 13) | (1u << 19));
    }
    void alpha(unsigned index, unsigned swap, unsigned a, unsigned b,
               unsigned c, unsigned d, unsigned op = 0, unsigned dest = 0) {
        const u32 operation = op < 2 ? (op << 18) :
            (3u << 16) | ((op & 1u) << 18) | (((op >> 1) & 3u) << 20);
        bp(0xC1 + 2 * index, swap | (swap << 2) | (d << 4) | (c << 7) |
            (b << 10) | (a << 13) | operation | (1u << 19) | (dest << 22));
    }
    unsigned warp(unsigned count, unsigned coord, unsigned swap, bool inverse,
                  unsigned width, unsigned height) {
        masked_bp(0x00, 0xF, coord + 1);
        xf(0x103F, coord + 1);
        xf(0x1040 + coord, (1u << 1) | (1u << 2)); // MTX3x4, POS, ABC1
        xf(0x1050 + coord, coord * 3); // native post-matrix slot, no normalization
        masked_bp(0x30 + coord * 2, 0x3FFFF, width - 1);
        masked_bp(0x31 + coord * 2, 0x3FFFF, height - 1);
        masked_bp(0x43, 1u << 6, 0); // late depth test: clipped pixels write no depth
        // Save native alpha before building the complementary half of the mask.
        if (inverse) {
            stage(count, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, swap, swap,
                  GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
            alpha(count++, swap, 7, 7, 7, 0, 0, 3); // A2 = APREV
        }
        stage(count, static_cast<GXTexCoordID>(coord), GX_TEXMAP3, swap, swap,
              GX_CC_ZERO, inverse ? GX_CC_ZERO : GX_CC_TEXC,
              inverse ? GX_CC_ZERO : GX_CC_CPREV,
              inverse ? GX_CC_CPREV : GX_CC_ZERO);
        const unsigned shift = (count & 1) ? 19 : 9;
        masked_bp(0xF6 + count / 2, 0x1Fu << shift, 0u); // K alpha = 1 (255)
        // Native warp's alpha test requires exactly 255. Keep that boundary,
        // then subtract EXACTLY the same mask for the complementary layer.
        alpha(count++, swap, 4, 6, 0, 7, 15); // A8_EQ, TEXA/KONST/APREV/ZERO
        if (inverse) {
            stage(count, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, swap, swap,
                  GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
            alpha(count++, swap, 0, 7, 7, 3, 1); // APREV = A2 - APREV
        }
        return count;
    }
    void post_matrix(unsigned coord, const Mtx matrix) {
        // Aurora's copy_xf_data only supports a full, aligned post matrix.
        // Twelve single-word XF commands silently overwrite element zero in
        // release builds instead of updating their respective matrix elements.
        byte(0x10);
        word((11u << 16) | (0x500 + coord * 12));
        for (unsigned row = 0; row < 3; ++row) {
            for (unsigned col = 0; col < 4; ++col) {
                u32 bits;
                std::memcpy(&bits, &matrix[row][col], sizeof(bits));
                word(bits);
            }
        }
    }
    bool submit() {
        while (size % 32 != 0 && valid) byte(0);
        if (!valid) return false;
        GXCallDisplayList(bytes.data(), static_cast<u32>(size));
        return true;
    }
    bool apply(unsigned stageCount, bool darkChannels = true) {
        // Warp-only passes preserve native lighting/channel counts as well.
        masked_bp(0x00, darkChannels ? 0x3C70 : 0x3C00,
                  (darkChannels ? 2u << 4 : 0) | ((stageCount - 1) << 10));
        return submit();
    }
};

HookAction before_shape_draw(ModContext*, void* args, void*, void*) {
    if (s_drawDepth == 0 || s_drawDepth > s_drawScopes.size()) return HOOK_CONTINUE;
    auto& scope = s_drawScopes[s_drawDepth - 1];
    auto* shape = mods::arg<const J3DShape*>(args, 0);
    if (scope.packet == nullptr || shape == nullptr || scope.applied ||
        scope.packet->getShape() != shape) return HOOK_CONTINUE;
    auto* material = shape->getMaterial();
    auto* model = scope.packet->getModel();
    if (material == nullptr || material->getTevBlock() == nullptr ||
        material->getColorBlock() == nullptr) return HOOK_CONTINUE;
    auto* matPacket = scope.materialPacket;
    if (matPacket->getDisplayListObj() == nullptr) return HOOK_CONTINUE;
    auto* tev = material->getTevBlock();
    const unsigned count = tev->getTevStageNum();
    // Never index stage 16 or resize a foreign material's TEV allocation.
    if (count == 0 || count >= 16) return HOOK_CONTINUE;
    const auto swaps = find_swap_tables(tev, count);
    if (swaps.identity < 0) return HOOK_CONTINUE;
    const auto identity = static_cast<GXTevSwapSel>(swaps.identity);
    unsigned next = count;
    const unsigned warpStages = scope.warp ? (scope.inverse ? 3 : 1) : 0;
    if (scope.warp) {
        scope.warpCoord = native_warp_coord(model, material, daAlink_getAlinkActorClass());
        if (scope.warpCoord < 0 || count + warpStages + (scope.dark ? 3 : 0) > 16)
            return HOOK_CONTINUE;
    }
    const auto* eyeOrder = eye_texture_order(model->getModelData(), material);
    const bool eye = eyeOrder != nullptr && count + 3 <= 16 && swaps.red >= 0 && swaps.blue >= 0;

    DarkDisplayList effect;
    effect.swap(identity, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    if (scope.dark) {
    // XF color registers contain RGBA. Retain alpha for the native stages.
    const auto* ambient = material->getColorBlock()->getAmbColor(1);
    const auto* color = material->getMatColor(1);
    const u8 ambientAlpha = ambient != nullptr ? ambient->a : 255;
    const u8 materialAlpha = color != nullptr ? color->a : 255;
    // Only borrow light0 when the actual batch material reloads it in its DL.
    // Replaying that DL in restore_draw_state restores the light as well as
    // both color channels. Materials without a restorable light use diffuse.
    auto* batchMaterial = matPacket->getMaterial();
    const auto* alpha0 = material->getColorBlock()->getColorChan(1);
    const auto* alpha1 = material->getColorBlock()->getColorChan(3);
    const bool alphaUsesLight0 =
        (alpha0 != nullptr && alpha0->getEnable() && (alpha0->getLightMask() & 1)) ||
        (alpha1 != nullptr && alpha1->getEnable() && (alpha1->getLightMask() & 1));
    const bool glossy = !alphaUsesLight0 && !eye && count + 2 <= 16 && batchMaterial != nullptr &&
        batchMaterial->getColorBlock() != nullptr &&
        batchMaterial->getColorBlock()->getLight(0) != nullptr;
    if (glossy) {
        const auto* ambient0 = material->getColorBlock()->getAmbColor(0);
        const auto* color0 = material->getMatColor(0);
        effect.specular_lighting(ambient0 != nullptr ? ambient0->a : 255,
            color0 != nullptr ? color0->a : 255, ambientAlpha, materialAlpha);
    } else {
        effect.lighting(eye, ambientAlpha, materialAlpha);
    }

    if (eye) {
        const auto red = static_cast<GXTevSwapSel>(swaps.red);
        const auto blue = static_cast<GXTevSwapSel>(swaps.blue);
        effect.swap(red, GX_CH_RED, GX_CH_RED, GX_CH_RED, GX_CH_ALPHA);
        effect.swap(blue, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE, GX_CH_ALPHA);
        const auto coord = static_cast<GXTexCoordID>(eyeOrder->mTexCoord);
        const auto map = static_cast<GXTexMapID>(eyeOrder->getTexMap());
        // Red-minus-blue isolates the iris using the existing animated UVs.
        effect.stage(count, coord, map, identity, red,
                     GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
        effect.stage(count + 1, coord, map, identity, blue,
                     GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,
                     GX_TEV_SUB, GX_CS_SCALE_2);
        effect.stage(count + 2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, identity, identity,
                     GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV, GX_CC_ZERO);
        next = count + 3;
    } else if (glossy) {
        effect.stage(count, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, identity, identity,
                     GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
                     GX_TEV_ADD, GX_CS_SCALE_1, 0);
        // Add COLOR1's highlight to the diffuse color already in PREV.
        effect.stage(count + 1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, identity, identity,
                     GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
        next = count + 2;
    } else {
        effect.stage(count, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, identity, identity,
                     GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
        next = count + 1;
    }
    }
    if (scope.warp) {
        auto* image = model->getModelData()->getTexture()->getResTIMG(tev->getTexNo(3));
        next = effect.warp(next, scope.warpCoord, identity, scope.inverse, image->width, image->height);
    }
    scope.applied = effect.apply(next, scope.dark);
    if (scope.applied && scope.warp) ++s_transition.maskedShapes;
    return HOOK_CONTINUE;
}

void after_shape_draw(ModContext*, void*, void*, void*) {
    if (s_drawDepth != 0 && s_drawDepth <= s_drawScopes.size()) {
        restore_draw_state(s_drawScopes[s_drawDepth - 1]);
    }
}

HookAction before_primitive_draw(ModContext*, void*, void*, void*) {
    if (s_drawDepth == 0 || s_drawDepth > s_drawScopes.size()) return HOOK_CONTINUE;
    const auto& scope = s_drawScopes[s_drawDepth - 1];
    if (!scope.applied || scope.warpCoord < 0) return HOOK_CONTINUE;
    // Skinning loads position/texture matrices inside drawFast, after our
    // material hook. Install the warp projection AFTER those per-group loads.
    Mtx matrix;
    warp_texture_matrix(matrix);
    DarkDisplayList projection;
    projection.post_matrix(scope.warpCoord, matrix);
    if (projection.submit()) ++s_transition.projectedGroups;
    return HOOK_CONTINUE;
}

void after_player_draw(ModContext*, void* args, void*, void*) {
    auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr || mods::arg<void*>(args, 1) != link || link->sub_method == nullptr) return;
    const auto* methods = reinterpret_cast<const leafdraw_method_class*>(link->sub_method);
#if defined(__APPLE__)
    if (mods::arg<process_method_func>(args, 0) != methods->draw_method) return;
#else
    if (mods::arg<const leafdraw_method_class*>(args, 0) != methods) return;
#endif
    draw_outgoing(link);
}

} // namespace

bool fierce_deity_transition_busy() { return s_transition.owner != nullptr; }

void fierce_deity_transition_prepare(daAlink_c* link, bool fromDark, bool toDark, bool entering) {
    prepare_transition(link, fromDark, toDark, entering);
}

void fierce_deity_transition_commit(daAlink_c* link) {
    if (!transition_owner(link) || s_transition.committed) return;
    restore_archive_heap(link);
    // Native clothes changes reuse the same model heap/address. Our retained
    // outgoing instances require rebinding the skeletal collision explicitly.
    link->field_0x2e44.mModel = link->mpLinkModel;
    if (!warp_compatible(outfit_models(link), link)) {
        discard_transition(link);
        return;
    }
    s_transition.committed = true;
    // Match procCoWarpInit's human sounds to our APP / DISAPP particles.
    // OUT is the native appearance sound; IN_TATE is vertical disappearance.
    link->seStartOnlyReverb(s_transition.wipe.entering ?
        Z2SE_AL_WARP_OUT : Z2SE_AL_WARP_IN_TATE);
    warp_log("Fierce Deity warp: started (50 ticks, native material)");
    update_warp_particles(link);
}

void fierce_deity_transition_tick(daAlink_c* link) {
    if (!transition_owner(link) || !s_transition.committed) return;
    if (link->checkWolf() || link->checkDeadHP() || link->checkSceneChangeAreaStart() ||
        link->checkEventRun() || s_transition.wipe.advance()) {
        discard_transition(link);
    } else {
        update_warp_particles(link);
    }
}

void fierce_deity_transition_cancel(daAlink_c* link) {
    if (s_transition.owner == nullptr) return;
    // reset_for_link can run after the old actor is gone; only touch a live owner.
    if (link == nullptr && transition_owner(daAlink_getAlinkActorClass()))
        link = daAlink_getAlinkActorClass();
    discard_transition(link);
}

ModResult initialize_fierce_deity_visual(ModError* error) {
    ModResult result;
    if ((result = mods::hook::add_pre<FierceMaterialDrawHook>(svc_hook, before_material_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceMaterialDrawHook>(svc_hook, after_material_draw)) != MOD_OK ||
        (result = mods::hook::add_pre<FierceShapePacketDrawHook>(svc_hook, before_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceShapePacketDrawHook>(svc_hook, after_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_pre<FierceShapePacketFastHook>(svc_hook, before_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceShapePacketFastHook>(svc_hook, after_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_pre<FierceShapeDrawHook>(svc_hook, before_shape_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceShapeDrawHook>(svc_hook, after_shape_draw)) != MOD_OK ||
        (result = mods::hook::add_pre<FiercePrimitiveDrawHook>(svc_hook, before_primitive_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceWarpParticleCalcHook>(svc_hook, after_warp_particle_calc)) != MOD_OK ||
        (result = mods::hook::add_post<FiercePlayerDrawHook>(svc_hook, after_player_draw)) != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Fierce Deity visual hooks");
    }
    return MOD_OK;
}

} // namespace dawnlight
