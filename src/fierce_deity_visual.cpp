#include "fierce_deity.hpp"
#include "dual_wield.hpp"
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
#include <cstring>
#include <memory>
#include <new>
#include <utility>

namespace dawnlight {
namespace {

// Draw-local implementation of the selectable body and eye appearances. Do not
// patch shared BMD materials: another actor (including Dark Link) can use them.
// Apply GX state only after the native material/differed display lists, and replay
// those lists immediately afterward, including between batched shared materials.
DEFINE_HOOK(&J3DMatPacket::draw, FierceMaterialDrawHook);
DEFINE_HOOK(&J3DShapePacket::draw, FierceShapePacketDrawHook);
DEFINE_HOOK(&J3DShapePacket::drawFast, FierceShapePacketFastHook);
DEFINE_HOOK(&J3DShape::drawFast, FierceShapeDrawHook);
DEFINE_HOOK(&J3DShapeDraw::draw, FiercePrimitiveDrawHook);
DEFINE_HOOK(&JPAResource::calc, FierceWarpParticleCalcHook);
DEFINE_HOOK(&mDoExt_modelUpdateDL, FierceEquipmentUpdateHook);
DEFINE_HOOK(&mDoExt_modelEntryDL, FierceEquipmentEntryHook);
#if defined(__APPLE__)
DEFINE_HOOK(&fpcMtd_Method, FiercePlayerDrawHook);
#else
DEFINE_HOOK(&fpcLf_DrawMethod, FiercePlayerDrawHook);
#endif

struct DrawScope {
    J3DShapePacket* packet = nullptr;
    J3DMatPacket* materialPacket = nullptr;
    bool applied = false;
    FierceDeityTint tint = FierceDeityTint::None;
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
    return link != nullptr && model != nullptr && (model == link->mpLinkModel ||
        model == link->mpLinkFaceModel || model == link->mpLinkHatModel ||
        model == link->mpLinkHandModel || model == link->mpDemoFCBlendModel ||
        model == link->mpDemoFCTongueModel || model == link->mpDemoHLTmpModel ||
        model == link->mpDemoHRTmpModel || model == link->mpLinkBootModels[0] ||
        model == link->mpLinkBootModels[1]);
}

#include "fierce_deity_equipment.inc"

bool player_equipment(daAlink_c* link, const J3DModel* model) {
    // Compare live instances, never shared model data or material names. Keep
    // equipment separate from player_model: it has no retained outgoing layer.
    return link != nullptr && model != nullptr &&
        (model == link->mSwordModel || model == link->mSheathModel ||
         model == link->mShieldModel || model == link->mpKanteraModel ||
         dual_wield_owns_model(link, model) || observed_player_equipment(link, model));
}

#include "fierce_deity_transition.inc"
#include "fierce_deity_aura.inc"

HookAction before_packet_draw(ModContext*, void* args, void*, void*) {
    auto* packet = mods::arg<J3DShapePacket*>(args, 0);
    if (s_drawDepth < s_drawScopes.size()) {
        auto& scope = s_drawScopes[s_drawDepth];
        scope = {};
        if (packet != nullptr) {
            auto* link = daAlink_getAlinkActorClass();
            const auto layer = warp_layer(packet->getModel());
            const auto tint = layer.active ? layer.tint :
                ((player_model(link, packet->getModel()) ||
                  player_equipment(link, packet->getModel())) ?
                 fierce_deity_displayed_tint() : FierceDeityTint::None);
            if (layer.active || tint != FierceDeityTint::None) {
                scope.materialPacket = drawing_material(packet);
                if (scope.materialPacket != nullptr) {
                    scope.packet = packet;
                    scope.tint = tint;
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

// Reuse an identity table i…16381 tokens truncated…rol: a GX setter flushing GEN_MODE from a stale shadow
    // (one texgen) recreates the log's invalid generator with this native model.
    load_native(3);
    const u32 staleShadow=1|(2u<<4)|(3u<<10);
    write_bp(staleShadow);
    assert(!shader_texgens_valid());
    bp=nativeBP; liveTexGens=3;
    assert(shader_texgens_valid());
    // A full TREF/KSEL shadow write would also corrupt the paired native stage.
    write_bp((0x29u<<24)|(1u<<19));
    assert((bp[0x29]&0xFFF)!=(nativeBP[0x29]&0xFFF));
    write_bp((0xF6u<<24)|4u);
    assert((bp[0xF6]&0xFFFFF0)!=(nativeBP[0xF6]&0xFFFFF0));

    J3DMaterial mat; J3DShape shape{&mat}; J3DModel player, other;
    link.mpLinkModel=&player;
    J3DShapePacket p{&player,&shape,{}}, foreign{&other,&shape,{}};
    draw_shadow(p);
    draw_begin(p);
    assert(gpuCount==5 && gpuMaterial.r==24);
    assert(gpuMaterial.a==93 && gpuAmbient.a==61); // Aurora's full RGBA writes
    // Check actual SPEC bits, independent channels, additive composition,
    // and normal-dependent highlights rather than a constant gray silhouette.
    assert(xf[0x100F]==((1u<<9)|(1u<<1)|(1u<<2)));
    assert(((xf[0x100E]>>7)&3)==GX_DF_CLAMP);
    assert(((bp[0x29]>>19)&7)==0); // stage3: COLOR0 diffuse
    assert(((bp[0x2A]>>7)&7)==1); // stage4: COLOR1 specular
    assert((gpuStages[4].color==std::array<int,4>{GX_CC_RASC,GX_CC_ZERO,GX_CC_ZERO,GX_CC_CPREV}));
    const float hx=xf_float_value(0x60D), hy=xf_float_value(0x60E), hz=xf_float_value(0x60F);
    assert(highlight(hx,hy,hz)>0.98f);
    assert(highlight(0,0,1)>0.1f && highlight(0,0,1)<0.5f);
    assert(highlight(1,0,0)==0);
    assert((xf[0x100A]&255)==61 && (xf[0x100C]&255)==93);
    assert(xf[0x1010]==nativeXF[0x1010] && xf[0x1011]==nativeXF[0x1011]);
    draw_end();
    assert(bp==nativeBP && xf==nativeXF); // includes light registers
    assert(gpuCount==3 && restoreCalls==2); // base + per-instance animation DL
    int writes=gxWrites;
    draw_begin(foreign); draw_end(); // same material pointer, different actor
    assert(gxWrites==writes && restoreCalls==2);
    darkActive=false;
    draw_begin(p); draw_end();
    assert(gxWrites==writes);
    darkActive=true;

    // A material without a light in its own DL must not overwrite global lights.
    mat.color.hasLight=false;
    draw_begin(p); assert(gpuCount==4 && gpuMaterial.r==8);
    for(unsigned i=0x603;i<=0x60F;++i) assert(xf[i]==nativeXF[i]);
    draw_end(); mat.color.hasLight=true;
    mat.color.alpha.enabled=true;
    draw_begin(p); assert(gpuCount==4);
    for(unsigned i=0x603;i<=0x60F;++i) assert(xf[i]==nativeXF[i]);
    draw_end(); mat.color.alpha.enabled=false;

    // Entire eyes are self-lit red, independent of original iris/pupil/sclera
    // RGB and lighting. Alpha/eyelid cutouts still use the native chain.
    for(const char* name : {"al_eyeballL_m","bl_eyeballR_m","ml_eyeballL_m","zl_eyeballR_m"}) {
        player.data.materials.name=name;
        draw_begin(p);
        assert(gpuCount==4 && gpuMaterial.r==255 && gpuMaterial.g==28 && gpuMaterial.b==20);
        assert((xf[0x100F] & 2)==0); // COLOR1 lighting disabled: emissive in dark rooms
        assert(gpuMaterial.a==93 && gpuAmbient.a==61);
        for(unsigned i=0x603;i<=0x60F;++i) assert(xf[i]==nativeXF[i]);
        for(float r : {0.f,0.4f,0.6f,1.f}) for(float b : {0.f,0.4f,0.6f,1.f})
            assert(eye_coverage(3,r,b)==1); // white, black, blue and red all emit
        for(int alpha : {0,93,255}) assert(warp_alpha(3,alpha,0)==alpha);
        draw_end(); assert(bp==nativeBP && xf==nativeXF);
    }
    // Named replacement eyes no longer depend on native texture names or
    // spare red/blue swap tables; the full surface uses only one extra stage.
    player.data.textures.name="custom_eye_texture";
    mat.tev.stages[0].mTevSwapModeInfo=0;
    mat.tev.stages[1].mTevSwapModeInfo=5;
    mat.tev.stages[2].mTevSwapModeInfo=14;
    draw_begin(p);assert(gpuCount==4 && gpuMaterial.r==255);draw_end();
    for(auto& stage : mat.tev.stages) stage.mTevSwapModeInfo=0;
    player.data.textures.name="al_eyeball";
    // Cover both halves of packed TREF registers and every legal stage count.
    // Baseline live TREF values intentionally differ from the CPU material's
    // orders (as they can after per-instance material animation).
    for(int count=1;count<=15;++count) {
        mat.tev.count=count;
        for(const char* name : {"al_body", "al_eyeballL_m"}) {
            player.data.materials.name=name;
            draw_begin(p);
            assert(gpuCount==count+(std::strstr(name,"eyeball") ? 1 : count<=14 ? 2 : 1));
            draw_end();
            assert(bp==nativeBP && xf==nativeXF);
        }
    }
    // Batched materials can be owned by a different model. Only the player
    // receives the effect, and cleanup must replay the batch's actual base DL.
    mat.tev.count=3;
    player.data.materials.name="al_body";
    other.packet.material=&mat;
    other.packet.shapes=&foreign; foreign.next=&p;
    void* batchArgs[]={&other.packet};
    before_material_draw(nullptr,batchArgs,nullptr,nullptr);
    load_native(3);
    void* foreignArgs[]={&foreign}; void* playerArgs[]={&p}; void* bodyArgs[]={&shape};
    writes=gxWrites;
    before_packet_draw(nullptr,foreignArgs,nullptr,nullptr);
    before_shape_draw(nullptr,bodyArgs,nullptr,nullptr);
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(gxWrites==writes);
    const int ownCalls=player.packet.list.calls, batchCalls=other.packet.list.calls;
    before_packet_draw(nullptr,playerArgs,nullptr,nullptr);
    before_shape_draw(nullptr,bodyArgs,nullptr,nullptr);
    assert(gpuCount==5); assert_native_fields(3);
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(player.packet.list.calls==ownCalls && other.packet.list.calls==batchCalls+1);
    after_material_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_materialDepth==0 && s_materialScopes[0]==nullptr);
    foreign.next=nullptr;
    draw_shadow(p); // a regular draw must not leave an active material scope

    // An unrelated material draw on the stack does not authorize this packet.
    before_material_draw(nullptr,batchArgs,nullptr,nullptr);
    draw_shadow(p);
    after_material_draw(nullptr,nullptr,nullptr,nullptr);
    // Reproduce the Android fatal from the actual Aurora BP/XF semantics.
    // A valid native draw can have 3 active generators from XF while GEN_MODE's
    // cached low nibble still contains 1. A masked stage-count update then
    // decodes the entire stale BP word and drops generators 1/2 (src sentinel 21).
    player.packet.shapes=&p; player.packet.material=&mat;
    for(unsigned generators : {1u,2u,3u})
    for(bool warp : {false,true})
    for(auto tint : {FierceDeityTint::None,FierceDeityTint::Dark,FierceDeityTint::White,FierceDeityTint::Gold}) {
        if(!warp && tint==FierceDeityTint::None) continue;
        warpTest=warp;warpDark=tint!=FierceDeityTint::None;
        palette=tint;mat.texGenNum=generators;mat.tev.count=generators;
        load_native(generators);
        xf[0x103f]=liveTexGens=generators;
        bp[0]=(bp[0]&~15u)|(generators-1); // stale BP cache after a later XF write
        nativeBP=bp;nativeXF=xf;
        assert(shader_texgens_valid());
        void* ownerArgs[]={&player.packet};void* packetArgs[]={&p};void* shapeArgs[]={&shape};
        before_material_draw(nullptr,ownerArgs,nullptr,nullptr);
        before_packet_draw(nullptr,packetArgs,nullptr,nullptr);
        before_shape_draw(nullptr,shapeArgs,nullptr,nullptr);
        const unsigned expected=generators+(warp?1:0);
        assert(liveTexGens==expected && (bp[0]&15)==expected && xf[0x103f]==expected);
        assert(shader_texgens_valid());
        before_primitive_draw(nullptr,nullptr,nullptr,nullptr);
        assert(shader_texgens_valid());
        draw_end();assert(liveTexGens==generators && shader_texgens_valid());
    }
    palette=FierceDeityTint::Dark;warpTest=warpDark=false;mat.texGenNum=mat.tev.count=3;
    // Skipped originals and bounded recursion unwind without stale pointers.
    player.packet.shapes=&p;
    void* ownArgs[]={&player.packet};
    for(int i=0;i<20;++i) before_material_draw(nullptr,ownArgs,nullptr,nullptr);
    draw_shadow(p);
    for(int i=0;i<20;++i) after_material_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_materialDepth==0);
    for(auto* material : s_materialScopes) assert(material==nullptr);

    // Warp-only and dark+warp passes use the same native generator slot. Check
    // complements for EVERY mask byte: no gap at the threshold, no double draw,
    // and native cutout/translucent alpha remains unchanged on its chosen side.
    warpTest=true;
    for(bool dark : {false,true}) {
        warpDark=dark;
        std::array<int,256> normal{}, inverse{};
        for(int alpha : {0,93,255}) {
            for(bool invert : {false,true}) {
                warpInverse=invert;
                draw_begin(p);
                assert((bp[0]&15)==4 && xf[0x103F]==4 && xf[0x1043]==6);
                assert(xf[0x1053]==9 && shader_texgens_valid());
                assert((bp[0x36]&0x3ffff)==31 && (bp[0x37]&0x3ffff)==63);
                assert((bp[0x43]&(1u<<6))==0); // no hidden-half depth writes
                before_primitive_draw(nullptr,nullptr,nullptr,nullptr);
                for(unsigned element=0;element<12;++element)
                    assert(xf_float_value(0x500+36+element)==float(element));
                for(int mask=0;mask<256;++mask)
                    (invert?inverse:normal)[mask]=warp_alpha(3,alpha,mask);
                draw_end();
                assert(bp==nativeBP && xf==nativeXF);
            }
            for(int mask=0;mask<256;++mask) {
                assert(normal[mask]+inverse[mask]==alpha);
                assert(normal[mask]==0 || inverse[mask]==0);
            }
        }
    }
    draw_shadow(p); // a transition still never enters the shadow-image pass
    warpTest=false;

    // Whole-eye emission must preserve complementary warp alpha and transparency.
    player.data.materials.name="al_eyeballL_m";
    warpTest=warpDark=true;
    for(bool invert : {false,true}) {
        warpInverse=invert;
        draw_begin(p);
        assert(gpuCount==4+(invert ? 3 : 1));
        assert(eye_coverage(3,0,0)==1 && eye_coverage(3,1,1)==1);
        for(int alpha : {0,93,255}) for(int mask=0;mask<256;++mask)
            assert(warp_alpha(3,alpha,mask)==((invert ? mask!=255 : mask==255) ? alpha : 0));
        draw_end();assert(bp==nativeBP && xf==nativeXF);
    }
    warpTest=false;
    player.data.materials.name="al_body";

    // Every held/stowed equipment instance gets the same body shading, even
    // when its material is shared with an unrelated actor. No retained warp
    // layer exists for equipment, and shadow draws must remain native.
    J3DModel sword, sheath, shield, second, secondSheath, lantern, teGear;
    link.mSwordModel=&sword;link.mSheathModel=&sheath;link.mShieldModel=&shield;
    s.owner=&link;s.sword=&second;s.sheath=&secondSheath;
    link.mpKanteraModel=&lantern;observedModel=&teGear;
    for(auto tint : {FierceDeityTint::Dark,FierceDeityTint::White,FierceDeityTint::Gold})
    for(auto* model : {&sword,&sheath,&shield,&second,&secondSheath,&lantern,&teGear}) {
        palette=tint;
        J3DShapePacket gear{model,&shape,{}};
        assert(!player_model(&link,model) && player_equipment(&link,model));
        draw_begin(gear);assert(gpuCount==(tint==FierceDeityTint::White?4:5));
        assert(gpuMaterial.r==(tint==FierceDeityTint::White?240:tint==FierceDeityTint::Gold?255:24));
        if(tint==FierceDeityTint::White) {
            assert(gpuMaterial.r==gpuMaterial.g && gpuMaterial.g==gpuMaterial.b);
            assert((gpuStages[3].color==std::array<int,4>{GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC}));
            assert(((bp[0x29]>>12)&(1u<<6))==0); // no texture RGB can turn white areas black
            assert(!(xf[0x100F]&2)); // uniform white in every lighting condition
            for(int alpha : {0,93,255})assert(warp_alpha(3,alpha,0)==alpha);
        }
        draw_end();assert(bp==nativeBP && xf==nativeXF);
        draw_shadow(gear);
        darkActive=false;
        writes=gxWrites;draw_begin(gear);draw_end();assert(gxWrites==writes);
        darkActive=true;
    }
    palette=FierceDeityTint::Dark;
    assert(!player_equipment(nullptr,&sword) && !player_equipment(&link,nullptr));
    assert(!player_equipment(&link,&other));
    daAlink_c otherLink;s.owner=&otherLink;
    assert(!player_equipment(&link,&second));
    s.owner=&link;s.sword=nullptr;s.sheath=nullptr; // released Dual Wield models
    assert(!player_equipment(&link,&second) && !player_equipment(&link,&secondSheath));
    link.mSwordModel=nullptr;link.mSheathModel=nullptr;link.mShieldModel=nullptr;
    link.mpKanteraModel=nullptr;observedModel=nullptr;
    assert(!player_equipment(&link,&teGear) && !player_equipment(&link,&lantern));
    assert(!player_equipment(&link,&sword) && !player_equipment(&link,&shield));

    // White classifies texture color BEFORE lighting, using all RGB channels.
    // Saturated yellow/magenta/cyan must not be mistaken for white.
    palette=FierceDeityTint::White;
    for(bool eye : {false,true}) {
        player.data.materials.name=eye ? "al_eyeballL_m" : "al_body";
        draw_begin(p);assert(gpuCount==6 && !(xf[0x100F]&2));
        const std::array<float,3> glow{gpuMaterial.r/255.f,gpuMaterial.g/255.f,gpuMaterial.b/255.f};
        assert((eye && gpuMaterial.r==255 && gpuMaterial.g==176 && gpuMaterial.b==24) ||
               (!eye && gpuMaterial.r==240 && gpuMaterial.g==240 && gpuMaterial.b==240));
        assert((monochrome_color(3,{1,1,1})==std::array<float,3>{0,0,0}));
        for(auto color : {std::array<float,3>{0,0,0},{1,0,0},{0,1,0},{0,0,1},{1,1,0},{1,0,1},{0,1,1},{.5f,.5f,.5f}})
            assert(monochrome_color(3,color)==glow);
        // Native white/gray boundary, including each independent RGB channel.
        const int threshold=eye?191:223;
        for(int component=0;component<3;++component) for(int value=0;value<256;++value) {
            std::array<float,3> color{1,1,1};color[component]=value/255.f;
            assert(monochrome_color(3,color)==(value>threshold ? std::array<float,3>{0,0,0}:glow));
        }
        for(int alpha : {0,93,255})assert(warp_alpha(3,alpha,0)==alpha);
        draw_end();assert(bp==nativeBP && xf==nativeXF);
        // Both wipe directions preserve the same classification/alpha.
        warpTest=warpDark=true;
        for(bool inverse : {false,true}) {
            warpInverse=inverse;draw_begin(p);
            assert(gpuCount==6+(inverse?3:1));
            assert((monochrome_color(3,{1,1,1})==std::array<float,3>{0,0,0}));
            assert(monochrome_color(3,{0,0,0})==glow);
            for(int alpha : {0,93,255})for(int mask=0;mask<256;++mask)
                assert(warp_alpha(3,alpha,mask)==((inverse?mask!=255:mask==255)?alpha:0));
            draw_end();assert(bp==nativeBP && xf==nativeXF);
        }
        warpTest=false;
    }
    // White eyes must reuse their actual animated UV/map, not assume slot 0.
    mat.tev.orders[0]={2,2};
    draw_begin(p);
    assert(((bp[0x29]>>12)&0x7f)==(2u|(2u<<3)|(1u<<6)));
    draw_end();mat.tev.orders[0]={0,0};
    // Gold uses warm normal-dependent highlights and full white emissive eyes.
    palette=FierceDeityTint::Gold;player.data.materials.name="al_body";
    draw_begin(p);assert(gpuCount==5 && gpuMaterial.r==255 && gpuMaterial.g==221 && gpuMaterial.b==120);
    assert((xf[0x100C]>>8)==((190u<<16)|(115u<<8)|18u));
    assert(highlight(-0.212703f,0.265879f,0.940248f)>highlight(0,0,1));
    draw_end();assert(bp==nativeBP && xf==nativeXF);
    player.data.materials.name="al_eyeballR_m";
    draw_begin(p);assert(gpuCount==4 && gpuMaterial.r==255 && gpuMaterial.g==255 && gpuMaterial.b==255);
    assert(!(xf[0x100F]&2) && eye_coverage(3,0,0)==1);draw_end();
    // White's three-stage path fits through stage 16, then safely falls back.
    palette=FierceDeityTint::White;
    for(int count=1;count<=15;++count) {
        mat.tev.count=count;draw_begin(p);
        assert(gpuCount==count+(count<=13?3:1));draw_end();
    }
    mat.tev.count=3;palette=FierceDeityTint::Dark;player.data.materials.name="al_body";

    // Different native materials use different dormant generator slots. Every
    // coefficient, including the changing vertical translation, must reach the
    // selected post matrix as a complete block without touching its neighbors.
    for(unsigned coord=0;coord<4;++coord) {
        for(unsigned frame=0;frame<50;++frame) {
            Mtx matrix;
            for(unsigned row=0;row<3;++row) for(unsigned col=0;col<4;++col)
                matrix[row][col]=float(row*4+col)+float(frame)*0.12f;
            const auto previous=xf;
            DarkDisplayList upload;
            upload.post_matrix(coord,matrix);
            assert(upload.submit());
            const unsigned base=0x500+coord*12;
            for(unsigned reg=0;reg<xf.size();++reg) {
                if(reg>=base && reg<base+12) {
                    const unsigned element=reg-base;
                    assert(xf_float_value(reg)==matrix[element/4][element%4]);
                } else assert(xf[reg]==previous[reg]);
            }
        }
    }

    // A malformed/future expanded effect must fail without submitting partial
    // commands or writing past the fixed command buffer.
    DarkDisplayList oversized;
    for(int i=0;i<80;++i) oversized.swap(0,0,1,2,3);
    writes=gxWrites; assert(!oversized.apply(4,3)); assert(gxWrites==writes);

    // Capacity limits and foreign layouts never overflow the 16-stage pipeline.
    mat.tev.count=16;
    writes=gxWrites; draw_begin(p); draw_end(); assert(gxWrites==writes);
    mat.tev.count=14;
    draw_begin(p); assert(gpuCount==16); draw_end(); // two body stages fit
    mat.tev.count=3;
    player.data.textures.name="custom_without_native_mask";
    draw_begin(p); assert(gpuCount==5 && gpuMaterial.r==24); draw_end();
    player.data.textures.name="al_eyeball";
    mat.tev.stages[0].mTevSwapModeInfo=0;
    mat.tev.stages[1].mTevSwapModeInfo=5;
    mat.tev.stages[2].mTevSwapModeInfo=14; // all swap tables in use
    draw_begin(p); assert(gpuCount==5); draw_end();
    // Nested unrelated packets must not inherit the player's effect; cancellation
    // before the shape draw must not leave any state or pointers behind.
    void* args[]={&p}; before_packet_draw(nullptr,args,nullptr,nullptr);
    draw_begin(foreign);
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    after_material_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==1);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==0 && s_drawScopes[0].packet==nullptr);
    for(int i=0;i<20;++i) before_packet_draw(nullptr,args,nullptr,nullptr);
    void* shapeArgs[]={&shape}; writes=gxWrites;
    before_shape_draw(nullptr,shapeArgs,nullptr,nullptr);
    assert(gxWrites==writes); // bounded recursion guard
    for(int i=0;i<20;++i) after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==0);
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp = Path(temp) / 'visual.cpp'
    exe = Path(temp) / 'visual'
    cpp.write_text(fixture + dual_owner + callbacks + checks)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Fierce Deity warp masks, late depth, specular response, shadow isolation, Dark/White/Gold palettes, eyes, equipment and alpha: passed')
