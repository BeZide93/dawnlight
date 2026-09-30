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

void append_stage(unsigned index, GXTexCoordID coord, GXTexMapID map,
                  GXTevSwapSel identity, GXTevSwapSel textureSwap) {
    const auto stage = static_cast<GXTevStageID>(index);
    GXSetTevOrder(stage, coord, map, GX_COLOR1A1);
    GXSetTevDirect(stage);
    GXSetTevSwapMode(stage, identity, textureSwap);
    GXSetTevColorOp(stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    // Preserve the entire native alpha chain, including animated face/hair masks.
    GXSetTevAlphaIn(stage, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
    GXSetTevAlphaOp(stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

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

    scope.applied = true;
    GXSetTevSwapModeTable(identity, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    GXSetNumChans(2);
    // Leave ALPHA1 and the original stages intact. Only their final RGB is
    // replaced. Native scene lighting provides shape definition on the body.
    // Aurora writes the complete RGBA register even for GX_COLOR1. Carry the
    // existing alpha through so original stages using ALPHA1 remain correct.
    const auto* ambient = material->getColorBlock()->getAmbColor(1);
    const auto* color = material->getMatColor(1);
    const u8 ambientAlpha = ambient != nullptr ? ambient->a : 255;
    const u8 materialAlpha = color != nullptr ? color->a : 255;
    GXSetChanAmbColor(GX_COLOR1, GXColor{7, 7, 7, ambientAlpha});
    GXSetChanMatColor(GX_COLOR1, eye ? GXColor{230, 8, 5, materialAlpha}
                                      : GXColor{96, 104, 116, materialAlpha});
    GXSetChanCtrl(GX_COLOR1, eye ? GX_FALSE : GX_TRUE, GX_SRC_REG, GX_SRC_REG,
                  eye ? GX_LIGHT_NULL : GX_LIGHT0, GX_DF_CLAMP, GX_AF_NONE);

    if (eye) {
        const auto red = static_cast<GXTevSwapSel>(swaps.red);
        const auto blue = static_cast<GXTevSwapSel>(swaps.blue);
        GXSetTevSwapModeTable(red, GX_CH_RED, GX_CH_RED, GX_CH_RED, GX_CH_ALPHA);
        GXSetTevSwapModeTable(blue, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE, GX_CH_ALPHA);
        const auto coord = static_cast<GXTexCoordID>(eyeOrder->mTexCoord);
        const auto map = static_cast<GXTexMapID>(eyeOrder->getTexMap());
        // Native eyeball texture channels separate the iris from the sclera.
        // A red-minus-blue mask keeps the whites dark instead of painting the
        // entire eye red, using its existing animated texture coordinates.
        append_stage(count, coord, map, identity, red);
        GXSetTevColorIn(static_cast<GXTevStageID>(count), GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
        append_stage(count + 1, coord, map, identity, blue);
        GXSetTevColorIn(static_cast<GXTevStageID>(count + 1), GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
        GXSetTevColorOp(static_cast<GXTevStageID>(count + 1), GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
        append_stage(count + 2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, identity, identity);
        GXSetTevColorIn(static_cast<GXTevStageID>(count + 2), GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV, GX_CC_ZERO);
        GXSetNumTevStages(count + 3);
    } else {
        append_stage(count, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, identity, identity);
        GXSetTevColorIn(static_cast<GXTevStageID>(count), GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
        GXSetNumTevStages(count + 1);
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
