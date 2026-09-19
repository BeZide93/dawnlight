#include "boss_rush_hologram.hpp"
#include "../save_state.hpp"
#include "mods/svc/hook.hpp"
#include "m_Do/m_Do_ext.h"

#include <unordered_map>
#include <vector>

namespace {
std::unordered_map<J3DModel*, u8> s_galleryModels;
DEFINE_HOOK(&J3DShapePacket::drawFast, HologramShapeDrawHook);

HookAction before_shape_draw(ModContext*, void* args, void*, void*) {
    auto* packet = mods::arg<J3DShapePacket*>(args, 0);
    if (!packet || !packet->getShape()) return HOOK_CONTINUE;
    const auto found = s_galleryModels.find(packet->getModel());
    if (found == s_galleryModels.end()) return HOOK_CONTINUE;

    // Override GPU state after the original material/display lists have loaded.
    // Never recolor the archive's shared materials: the Enemy Spawner can use
    // exactly the same model data as a gallery figure in this room.
    auto* material = packet->getShape()->getMaterial();
    auto* tev = material->getTevBlock();
    auto* order = tev->getTevOrder(0);
    GXTexCoordID coord = GX_TEXCOORD_NULL;
    GXTexMapID texture = GX_TEXMAP_NULL;
    // TEV orders can name an UNUSED coordinate, even on untextured materials.
    // Sampling it would make Aurora compile the default GX_MAX_TEXGENSRC (21)
    // and persist that invalid pipeline, causing another abort on app startup.
    if (order && order->mTexCoord < material->getTexGenNum() &&
        order->mTexCoord < 8 && order->getTexMap() < 8 &&
        tev->getTexNo(order->getTexMap()) != 0xFFFF) {
        const auto* texCoord = material->getTexCoord(order->mTexCoord);
        if (texCoord) {
            const u8 source = texCoord->getTexGenSrc();
            const u8 type = texCoord->getTexGenType();
            const bool supportedSource = source <= GX_TG_TEX7 ||
                source == GX_TG_COLOR0 || source == GX_TG_COLOR1;
            // Emboss generators depend on other coordinates/lighting. Use
            // constant opacity for these and other unsupported generators.
            if (supportedSource && (type == GX_TG_MTX2x4 || type == GX_TG_MTX3x4)) {
                coord = static_cast<GXTexCoordID>(order->mTexCoord);
                texture = static_cast<GXTexMapID>(order->getTexMap());
            }
        }
    }
    apply_boss_rush_hologram_gx(boss_rush_hologram_color(found->second), coord, texture);
    return HOOK_CONTINUE;
}

void after_shape_draw(ModContext*, void* args, void*, void*) {
    auto* packet = mods::arg<J3DShapePacket*>(args, 0);
    if (!packet || !packet->getShape() || !s_galleryModels.contains(packet->getModel())) return;
    // A native actor can be the next shape in the SAME material batch.
    // Restore the original material and instance-specific registers now.
    auto* material = packet->getShape()->getMaterial();
    auto* original = packet->getModel()->getMatPacket(material->getIndex())->getDisplayListObj();
    if (original) original->callDL();
    if (packet->getDisplayListObj()) packet->getDisplayListObj()->callDL();
}
}

GXColor boss_rush_hologram_color(u8 bossIndex) {
    return dawnlight::save_state_boss_defeated(bossIndex)
        ? GXColor{255, 55, 70, 112} : GXColor{45, 155, 255, 112};
}

void apply_boss_rush_hologram_gx(GXColor color, GXTexCoordID coord, GXTexMapID texture) {
    // A luminous single-color projection, with texture alpha retained for
    // cut-out hair/wings. 44% opacity keeps the room visible through the model.
    if (texture == GX_TEXMAP_NULL) GXSetNumTexGens(0);
    GXSetNumIndStages(0);
    GXSetNumTevStages(1);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevOrder(GX_TEVSTAGE0, coord, texture, GX_COLOR_NULL);
    GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevKColor(GX_KCOLOR0, color);
    GXSetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
    GXSetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K0_A);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    if (texture != GX_TEXMAP_NULL) {
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, GX_CA_ZERO);
    } else {
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    }
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_COPY);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetZCompLoc(GX_FALSE);
}

ModResult init_boss_rush_holograms(const HookService* hooks) {
    auto result = mods::hook::add_pre<HologramShapeDrawHook>(hooks, before_shape_draw);
    if (result != MOD_OK) return result;
    result = mods::hook::add_post<HologramShapeDrawHook>(hooks, after_shape_draw);
    if (result != MOD_OK) mods::hook::uninstall<HologramShapeDrawHook>(hooks);
    return result;
}

void clear_boss_rush_holograms() { s_galleryModels.clear(); }

void shutdown_boss_rush_holograms(const HookService* hooks) {
    mods::hook::uninstall<HologramShapeDrawHook>(hooks);
    clear_boss_rush_holograms();
}

void register_boss_rush_hologram(J3DModel* model, u8 bossIndex) {
    if (model) s_galleryModels.insert_or_assign(model, bossIndex);
}

void submit_boss_rush_hologram(J3DModel* model) {
    if (!model) return;
    auto* data = model->getModelData();
    // Route these instance packets through the translucent draw buffer, then
    // restore shared material metadata before any other actor can use it.
    std::vector<u32> modes;
    modes.reserve(data->getMaterialNum());
    for (u16 i = 0; i < data->getMaterialNum(); ++i) {
        auto* material = data->getMaterialNodePointer(i);
        modes.push_back(material->getMaterialMode());
        material->setMaterialMode(4);
    }
    mDoExt_modelUpdateDL(model);
    for (u16 i = 0; i < data->getMaterialNum(); ++i)
        data->getMaterialNodePointer(i)->setMaterialMode(modes[i]);
}
