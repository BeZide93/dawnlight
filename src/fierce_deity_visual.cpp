#include "fierce_deity.hpp"
#include "service_imports.hpp"

#include "d/actor/d_a_alink.h"
#include "JSystem/J3DGraphBase/J3DMaterial.h"
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DSys.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"

#include <array>
#include <cstring>

namespace dawnlight {
namespace {

// Independent implementation of the dark silhouette/red-eye appearance. Do not
// patch shared BMD materials: another actor (including Dark Link) can use them.
// Apply GX state only after the native material/differed display lists, and replay
// those lists immediately afterward, including between batched shared materials.
DEFINE_HOOK(&J3DShapePacket::draw, FierceShapePacketDrawHook);
DEFINE_HOOK(&J3DShapePacket::drawFast, FierceShapePacketFastHook);
DEFINE_HOOK(&J3DShape::drawFast, FierceShapeDrawHook);

struct DrawScope {
    J3DShapePacket* packet = nullptr;
    bool applied = false;
};
std::array<DrawScope, 16> s_drawScopes{};
size_t s_drawDepth = 0;

bool player_model(daAlink_c* link, const J3DModel* model) {
    return model != nullptr && (model == link->mpLinkModel ||
        model == link->mpLinkFaceModel || model == link->mpLinkHatModel ||
        model == link->mpLinkHandModel || model == link->mpDemoFCBlendModel ||
        model == link->mpDemoFCTongueModel || model == link->mpDemoHLTmpModel ||
        model == link->mpDemoHRTmpModel || model == link->mpLinkBootModels[0] ||
        model == link->mpLinkBootModels[1]);
}

HookAction before_packet_draw(ModContext*, void* args, void*, void*) {
    auto* packet = mods::arg<J3DShapePacket*>(args, 0);
    if (s_drawDepth < s_drawScopes.size()) {
        auto& scope = s_drawScopes[s_drawDepth];
        scope = {};
        if (fierce_deity_dark_visual_active() && packet != nullptr &&
            player_model(daAlink_getAlinkActorClass(), packet->getModel())) {
            scope.packet = packet;
        }
    }
    ++s_drawDepth;
    return HOOK_CONTINUE;
}

void restore_draw_state(DrawScope& scope) {
    if (!scope.applied) return;
    auto* packet = scope.packet;
    auto* material = packet->getShape()->getMaterial();
    auto* matPacket = packet->getModel()->getMatPacket(material->getIndex());
    matPacket->callDL();
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
    alignas(32) std::array<u8, 256> bytes{};
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
    static u32 rgba(GXColor color) {
        return (u32(color.r) << 24) | (u32(color.g) << 16) |
               (u32(color.b) << 8) | color.a;
    }

public:
    void lighting(bool eye, u8 ambientAlpha, u8 materialAlpha) {
        xf(0x1009, 2); // XF number of color channels
        xf(0x100B, rgba({7, 7, 7, ambientAlpha}));
        xf(0x100D, rgba(eye ? GXColor{230, 8, 5, materialAlpha}
                           : GXColor{96, 104, 116, materialAlpha}));
        // COLOR1 only: register sources, clamp diffuse, no attenuation, light0
        // on the body; unlit eyes. Preserve the native ALPHA1 control.
        xf(0x100F, (1u << 10) | (u32(GX_DF_CLAMP) << 7) |
                     (eye ? 0u : (1u << 1) | (1u << 2)));
    }
    void swap(unsigned table, unsigned r, unsigned g, unsigned b, unsigned a) {
        // KSEL's upper 20 bits belong to native stage konst selections.
        masked_bp(0xF6 + 2 * table, 0xF, r | (g << 2));
        masked_bp(0xF7 + 2 * table, 0xF, b | (a << 2));
    }
    void stage(unsigned index, GXTexCoordID coord, GXTexMapID map,
               unsigned identity, unsigned textureSwap,
               GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d,
               GXTevOp op = GX_TEV_ADD, GXTevScale scale = GX_CS_SCALE_1) {
        const unsigned shift = (index & 1) * 12;
        const bool textured = coord != GX_TEXCOORD_NULL && map != GX_TEXMAP_NULL;
        const u32 order = (1u << 7) | (textured ?
            (u32(map) | (u32(coord) << 3) | (1u << 6)) : 0u); // COLOR1A1
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
    bool apply(unsigned stageCount) {
        // Only chan/stage counts; preserve texgens, culling and indirect stages.
        masked_bp(0x00, 0x3C70, (2u << 4) | ((stageCount - 1) << 10));
        while (size % 32 != 0 && valid) byte(0); // GX NOP padding
        if (!valid) return false;
        GXCallDisplayList(bytes.data(), static_cast<u32>(size));
        return true;
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
    if (material == nullptr || material->getTevBlock() == nullptr) return HOOK_CONTINUE;
    auto* matPacket = model->getMatPacket(material->getIndex());
    if (matPacket->getDisplayListObj() == nullptr) return HOOK_CONTINUE;
    auto* tev = material->getTevBlock();
    const unsigned count = tev->getTevStageNum();
    // Never index stage 16 or resize a foreign material's TEV allocation.
    if (count == 0 || count >= 16) return HOOK_CONTINUE;
    const auto swaps = find_swap_tables(tev, count);
    if (swaps.identity < 0) return HOOK_CONTINUE;
    const auto identity = static_cast<GXTevSwapSel>(swaps.identity);
    const auto* eyeOrder = eye_texture_order(model->getModelData(), material);
    const bool eye = eyeOrder != nullptr && count + 3 <= 16 && swaps.red >= 0 && swaps.blue >= 0;

    DarkDisplayList effect;
    effect.swap(identity, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    // XF color registers contain RGBA. Retain alpha for the native stages.
    const auto* ambient = material->getColorBlock()->getAmbColor(1);
    const auto* color = material->getMatColor(1);
    const u8 ambientAlpha = ambient != nullptr ? ambient->a : 255;
    const u8 materialAlpha = color != nullptr ? color->a : 255;
    effect.lighting(eye, ambientAlpha, materialAlpha);

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
        scope.applied = effect.apply(count + 3);
    } else {
        effect.stage(count, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, identity, identity,
                     GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
        scope.applied = effect.apply(count + 1);
    }
    return HOOK_CONTINUE;
}

void after_shape_draw(ModContext*, void*, void*, void*) {
    if (s_drawDepth != 0 && s_drawDepth <= s_drawScopes.size()) {
        restore_draw_state(s_drawScopes[s_drawDepth - 1]);
    }
}

} // namespace

ModResult initialize_fierce_deity_visual(ModError* error) {
    ModResult result;
    if ((result = mods::hook::add_pre<FierceShapePacketDrawHook>(svc_hook, before_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceShapePacketDrawHook>(svc_hook, after_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_pre<FierceShapePacketFastHook>(svc_hook, before_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceShapePacketFastHook>(svc_hook, after_packet_draw)) != MOD_OK ||
        (result = mods::hook::add_pre<FierceShapeDrawHook>(svc_hook, before_shape_draw)) != MOD_OK ||
        (result = mods::hook::add_post<FierceShapeDrawHook>(svc_hook, after_shape_draw)) != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Fierce Deity visual hooks");
    }
    return MOD_OK;
}

} // namespace dawnlight
