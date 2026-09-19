// Adapted from F1mmel/dusklight-twilit-essentials, commit 1e7fb0afb828f924c165e8c57b7337304ed6cf4c.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#include "boss_rush_models.hpp"
#include "boss_rush_common.hpp"
#include "boss_rush.hpp"
#include "gallery_resources.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "SSystem/SComponent/c_lib.h"
#include "SSystem/SComponent/c_math.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "JSystem/J3DGraphBase/J3DSys.h"
#include "m_Do/m_Do_mtx.h"

#include <cstdio>
#include <cstring>
#include <map>

namespace {
std::map<J3DJoint*, J3DJointCallBack> s_originalJointCallbacks;

struct MorphTentacle {
    s16 phaseX = 0;
    s16 phaseY = 0;
    s16 phaseT = 0;
    s16 frame = 0;
    s16 waveOff = 450;
    s16 speed = 0x800;
    f32 recoil = 0.0f;
    f32 emerge = 1.0f;
    f32 thickAmp = 0.0f;
    f32 length = 0.0f;
    s16 pitch = -0x3448;
    s16 bendAx[30]; s16 bendAy[30];
    s16 bendBx[30]; s16 bendBy[30];
    f32 thick[30];
    f32 thickDelta[30];
    s16 tgtAx[30]; s16 tgtAy[30];
    s16 tgtBx[30]; s16 tgtBy[30];
    f32 tgtThick[30];
};
static MorphTentacle s_morphTent[8];
static f32 s_morphSink[8];

static void update_morph_tentacle(MorphTentacle& t, f32& sink);

static constexpr f32 kTentacleSpread = 1.0f;

static s16 morph_pitch_target() {
    s16 target = static_cast<s16>(-0xF2C / kTentacleSpread);
    if (target > 0) target = 0;
    return target;
}

static void init_morph_tentacle(MorphTentacle& t, f32& sink) {
    t.frame = static_cast<s16>(cM_rndF(65536.0f));
    t.phaseT = static_cast<s16>(cM_rndF(65536.0f));
    t.phaseX = static_cast<s16>(cM_rndF(65536.0f));
    t.phaseY = static_cast<s16>(cM_rndF(65536.0f));
    t.waveOff = static_cast<s16>(cM_rndF(100.0f) + 400.0f);
    t.speed = 0x800;
    t.recoil = 0.0f;
    t.emerge = 1.0f;
    t.thickAmp = 0.2f;
    t.length = 70.0f;
    t.pitch = morph_pitch_target();
    for (int i = 0; i < 30; ++i) {
        t.bendAx[i] = 0; t.bendAy[i] = 0;
        t.bendBx[i] = 0; t.bendBy[i] = 0;
        t.thick[i] = 1.0f;
        t.thickDelta[i] = 0.0f;
        t.tgtAx[i] = 0; t.tgtAy[i] = 0;
        t.tgtBx[i] = 0; t.tgtBy[i] = 0;
        t.tgtThick[i] = 1.0f;
    }
    sink = 0.0f;

    for (int n = 0; n < 45; ++n) {
        update_morph_tentacle(t, sink);
    }
}

static void update_morph_tentacle(MorphTentacle& t, f32& sink) {
    cLib_addCalc0(&sink, 0.1f, 30.0f);

    const f32 ampX = t.emerge * 500.0f;
    const f32 ampY = t.emerge * 1500.0f;
    s16 tail = static_cast<s16>(2000.0f / kTentacleSpread);
    for (int i = 0; i < 30; i++) {
        f32 taper = 1.0f;
        if (i < 5) {
            taper = i * 0.2f;
        } else if (i >= 20) {
            taper = (i - 20) * 0.3f + 1.0f;
        }

        t.tgtAx[i] = static_cast<s16>(taper * (ampX * cM_ssin(static_cast<s16>(t.phaseX + i * 1800))));
        t.tgtAy[i] = static_cast<s16>(taper * (ampY * cM_ssin(static_cast<s16>(t.phaseY + i * 1800))));

        t.tgtBx[i] = tail + static_cast<s16>(taper * (ampX * cM_ssin(static_cast<s16>(t.phaseY + i * 7000)) * 0.5f));
        t.tgtBy[i] = static_cast<s16>(taper * (ampY * cM_ssin(static_cast<s16>(t.phaseX + i * 7000)) * 0.5f));

        tail -= static_cast<s16>(200.0f / kTentacleSpread);
        if (tail < 0) tail = 0;

        t.tgtThick[i] = t.thickAmp + 1.0f + t.thickAmp * cM_ssin(static_cast<s16>(t.phaseT + i * -10000));
    }

    const f32 sway = cM_ssin(static_cast<s16>(t.frame * 200)) * 100.0f;
    t.phaseX += static_cast<s16>((-t.waveOff - t.recoil) + sway);
    t.phaseY += static_cast<s16>(((100 - t.waveOff) - t.recoil) + sway);
    t.phaseT += static_cast<s16>(t.recoil + 2000.0f);
    t.frame++;

    cLib_addCalc0(&t.recoil, 0.1f, 50.0f);
    cLib_addCalc2(&t.thickAmp, 0.2f, 0.1f, 0.01f);
    if (t.length < 70.0f) {
        cLib_addCalc2(&t.length, 70.0f, 0.1f, 0.5f);
    }
    cLib_addCalcAngleS2(&t.speed, 0x800, 1, 0x10);

    cLib_addCalcAngleS2(&t.pitch, morph_pitch_target(), 4, 100);

    for (int i = 0; i < 30; i++) {
        cLib_addCalcAngleS2(&t.bendAx[i], t.tgtAx[i], 2, t.speed);
        cLib_addCalcAngleS2(&t.bendAy[i], t.tgtAy[i], 2, t.speed);
        cLib_addCalcAngleS2(&t.bendBx[i], t.tgtBx[i], 2, t.speed);
        cLib_addCalcAngleS2(&t.bendBy[i], t.tgtBy[i], 2, t.speed);
        cLib_addCalc2(&t.thick[i], t.tgtThick[i], 0.5f, 0.2f);
        cLib_addCalc0(&t.thickDelta[i], 0.1f, 0.2f);
    }
}

static int morph_tent_jointCB(J3DJoint* joint, int op) {
    if (op != 0) return 1;
    const int j = joint->getJntNo();
    J3DModel* m = j3dSys.getModel();
    if (m == nullptr || j < 0 || j > 29) return 1;
    MorphTentacle* t = reinterpret_cast<MorphTentacle*>(m->getUserArea());
    if (t == nullptr) return 1;

    MTXCopy(m->getAnmMtx(j), *calc_mtx);
    mDoMtx_YrotM(*calc_mtx, t->bendAy[j] + t->bendBy[j]);
    mDoMtx_ZrotM(*calc_mtx, t->bendAx[j] + t->bendBx[j]);
    MtxTrans(t->length + -100.0f, 1.0f, 1.0f, 1);
    MTXCopy(*calc_mtx, J3DSys::mCurrentMtx);
    MtxScale(1.0f, t->thick[j] + t->thickDelta[j], t->thick[j] + t->thickDelta[j], 1);
    m->setAnmMtx(j, *calc_mtx);
    return 1;
}

struct MgnBloodDrop {
    u8   mode  = 0;
    cXyz pos;
    f32  velY  = 0.0f;
    f32  sizeX = 0.0f;
    f32  sizeY = 0.0f;
    f32  base  = 0.0f;
    f32  alpha = 0.0f;
};

struct RuntimeSlot {
    J3DModel* model = nullptr;
    mDoExt_bckAnm* bck = nullptr;
    J3DModel* subModel = nullptr;
    mDoExt_bckAnm* subBck = nullptr;
    J3DModel* subHandModel = nullptr;
    J3DModel* darkhammerBall = nullptr;
    J3DModel* ookBoomerang   = nullptr;
    static constexpr size_t kDarkhammerChainLinkCount = 14;
    J3DModel* darkhammerChainLinks[kDarkhammerChainLinkCount] = {};
    bool darkhammerBallCaptured = false;
    cXyz darkhammerBallPos, darkhammerRingPos;
    static constexpr size_t kBlizzetaIceBlockCount = 5;
    J3DModel* blizzetaIceBlocks[kBlizzetaIceBlockCount] = {};
    static constexpr size_t kMgnDropCount = 16;
    J3DModel*    mgnDropModels[kMgnDropCount] = {};
    MgnBloodDrop mgnDrops[kMgnDropCount];
    int          mgnDropTimer = 0;
    J3DModel* partModels[kMaxBossPartsPerEntry] = {};
    mDoExt_bckAnm* partBcks[kMaxBossPartsPerEntry] = {};
    mDoExt_brkAnm* partBrks[kMaxBossPartsPerEntry] = {};
    mDoExt_btkAnm* partBtks[kMaxBossPartsPerEntry] = {};
    mDoExt_brkAnm* brk = nullptr;
    mDoExt_btkAnm* btk = nullptr;
    bool resolved = false;
    int retryFrames = 0;
    mDoExt_invisibleModel deathSwordInvisModel;
    bool hasInvisModel = false;
};

static RuntimeSlot s_slots[kMaxBossGalleryEntries];

static size_t s_loadOrder[kMaxBossGalleryEntries];
static bool s_loadOrderBuilt = false;

static void build_boss_rush_load_order() {
    const size_t count = boss_rush_get_active_gallery_count();
    for (size_t i = 0; i < count; ++i) {
        s_loadOrder[i] = i;
    }
    for (size_t i = 1; i < count; ++i) {
        const size_t candidate = s_loadOrder[i];
        const f32 candidateDist = boss_rush_load_priority_distance(candidate, count);
        size_t j = i;
        while (j > 0 && boss_rush_load_priority_distance(s_loadOrder[j - 1], count) > candidateDist) {
            s_loadOrder[j] = s_loadOrder[j - 1];
            --j;
        }
        s_loadOrder[j] = candidate;
    }
    s_loadOrderBuilt = true;
}

void apply_deathsword_ghost_material(J3DModel* model) {
    if (model == nullptr || model->getModelData() == nullptr) {
        return;
    }
    J3DModelData* mData = model->getModelData();
    const u16 matNum = mData->getMaterialNum();
    for (u16 m = 0; m < matNum; ++m) {
        J3DMaterial* mat = mData->getMaterialNodePointer(m);
        if (mat == nullptr) {
            continue;
        }

        mat->getZMode()->setUpdateEnable(1);
        mat->setZCompLoc(1);
        mat->setMaterialMode(4);

        if (m == 0) {
            J3DGXColor* kcol3 = mat->getTevKColor(3);
            if (kcol3 != nullptr) {
                kcol3->a = 0xFF;
            }
        } else {
            J3DGXColor* kcol0 = mat->getTevKColor(0);
            if (kcol0 != nullptr) {
                kcol0->r = 0; kcol0->g = 0; kcol0->b = 0;
            }
            auto* col0 = mat->getTevColor(0);
            if (col0 != nullptr) {
                col0->r = 0; col0->g = 0; col0->b = 0;
            }
            J3DGXColor* kcol3 = mat->getTevKColor(3);
            if (kcol3 != nullptr) {
                kcol3->a = 0xFF;
            }
        }

        J3DBlend* blend = mat->getBlend();
        if (blend != nullptr) {
            blend->setDstFactor(5);
        }
    }
}

void render_darkhammer_ball_and_chain(RuntimeSlot& slot, const BossGalleryEntry& boss,
                                      float floorY, const cXyz& pos, const csXyz& angle, const cXyz& scale) {
    if (slot.model == nullptr || slot.model->getModelData() == nullptr ||
        slot.darkhammerBall == nullptr) {
        return;
    }
    const u16 jointNum = slot.model->getModelData()->getJointNum();
    if (jointNum <= 0x11) {
        return;
    }

    cXyz handR1, handL1;
    mDoMtx_stack_c::copy(slot.model->getAnmMtx(0x11));
    mDoMtx_stack_c::transM(20.0f, -20.0f, 5.0f);
    mDoMtx_stack_c::multVecZero(&handR1);

    mDoMtx_stack_c::copy(slot.model->getAnmMtx(0xC));
    mDoMtx_stack_c::transM(15.0f, -20.0f, -20.0f);
    mDoMtx_stack_c::multVecZero(&handL1);

    const f32 rad = cM_s2rad(angle.y);
    const f32 sinYaw = std::sin(rad);
    const f32 cosYaw = std::cos(rad);
    const cXyz fwd(sinYaw, 0.0f, cosYaw);
    const cXyz right(cosYaw, 0.0f, -sinYaw);

    if (!slot.darkhammerBallCaptured) {
        cXyz bp = handR1 + right * (18.0f * boss.scale) + fwd * (22.0f * boss.scale);
        bp.y = floorY + 47.0f * boss.scale;
        slot.darkhammerBallPos = bp;

        mDoMtx_stack_c::transS(bp.x, bp.y, bp.z);
        mDoMtx_stack_c::YrotM(angle.y);
        mDoMtx_stack_c::XrotM(-0x4000);
        mDoMtx_stack_c::transM(0.0f, 55.0f, 0.0f);
        mDoMtx_stack_c::multVecZero(&slot.darkhammerRingPos);

        slot.darkhammerBallCaptured = true;
    }
    const cXyz ballPos = slot.darkhammerBallPos;
    const cXyz ringPos = slot.darkhammerRingPos;

    static constexpr f32 kBallLift = 14.0f;
    const f32 ballLift = kBallLift * boss.scale;

    static constexpr s16 kBallYawSpin = 0x0000;
    static constexpr s16 kBallTilt    = 0x0CCC;
    mDoMtx_stack_c::transS(ballPos.x, ballPos.y + ballLift, ballPos.z);
    mDoMtx_stack_c::YrotM(angle.y);
    mDoMtx_stack_c::YrotM(kBallYawSpin);
    mDoMtx_stack_c::XrotM(-0x4000);
    mDoMtx_stack_c::transM(0.0f, 55.0f, 0.0f);
    mDoMtx_stack_c::XrotM(kBallTilt);
    mDoMtx_stack_c::scaleM(scale.x, scale.y, scale.z);
    renderModelAtMtx(slot.darkhammerBall, mDoMtx_stack_c::get());

    const f32 linkSpacing = 25.0f * boss.scale;
    const f32 floorContactY = floorY + 6.0f * boss.scale;

    auto drawLink = [&](J3DModel* linkModel, const cXyz& p0, const cXyz& p1, int linkIndex) {
        if (linkModel == nullptr) return;
        cXyz delta = p1 - p0;
        const f32 xzDist = std::sqrt(delta.x * delta.x + delta.z * delta.z);
        const s16 rot_y = cM_atan2s(delta.x, delta.z);
        const s16 rot_x = -cM_atan2s(delta.y, xzDist);

        s16 rot_z = static_cast<s16>(linkIndex * 3000);
        if (linkIndex & 1) {
            rot_z += 0x4000;
        }

        mDoMtx_stack_c::transS(p0.x, p0.y, p0.z);
        mDoMtx_stack_c::YrotM(rot_y);
        mDoMtx_stack_c::XrotM(rot_x);
        mDoMtx_stack_c::ZrotM(rot_z);
        mDoMtx_stack_c::scaleM(scale.x, scale.y, scale.z);
        mDoMtx_stack_c::transM(0.0f, 0.0f, 12.0f);
        renderModelAtMtx(linkModel, mDoMtx_stack_c::get());
    };

    constexpr int kSeg1 = 3;
    constexpr int kSeg2 = 8;
    constexpr int kSeg3 = 3;
    constexpr int kChainLinks = kSeg1 + kSeg2 + kSeg3;
    constexpr f32 kPi = 3.14159265f;

    cXyz node[kChainLinks + 1];

    const cXyz ringLifted(ringPos.x, ringPos.y + ballLift, ringPos.z);
    for (int i = 0; i <= kSeg1; ++i) {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(kSeg1);
        node[i] = ringLifted * (1.0f - t) + handR1 * t;
        node[i].y -= 6.0f * boss.scale * std::sin(t * kPi);
    }
    for (int i = 1; i <= kSeg2; ++i) {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(kSeg2);
        node[kSeg1 + i] = handR1 * (1.0f - t) + handL1 * t;
        if (i >= 2 && i <= kSeg2 - 2) {
            const f32 tt = static_cast<f32>(i - 1) / static_cast<f32>(kSeg2 - 2);
            node[kSeg1 + i].y -= 18.0f * boss.scale * std::sin(tt * kPi);
        }
    }
    for (int i = 1; i <= kSeg3; ++i) {
        node[kSeg1 + kSeg2 + i] =
            node[kSeg1 + kSeg2 + i - 1] + cXyz(0.0f, -linkSpacing, 0.0f);
    }

    for (int i = 0; i <= kChainLinks; ++i) {
        if (node[i].y < floorContactY) node[i].y = floorContactY;
    }

    for (int i = 0; i < kChainLinks; ++i) {
        drawLink(slot.darkhammerChainLinks[i], node[i], node[i + 1], i);
    }
}

void render_blizzeta_ice_blocks(RuntimeSlot& slot, const BossGalleryEntry& boss,
                                float floorY, const cXyz& pos, const csXyz& angle) {
    const f32 radius = 165.0f;
    const f32 iceScale = boss.scale * 0.95f;
    const cXyz scale(iceScale, iceScale, iceScale);

    static f32 s_bobTime = 0.0f;
    s_bobTime += 1.0f;

    for (size_t k = 0; k < RuntimeSlot::kBlizzetaIceBlockCount; ++k) {
        J3DModel* iceModel = slot.blizzetaIceBlocks[k];
        if (iceModel == nullptr) continue;

        const s16 angleOffset = static_cast<s16>(k * 0x3333);
        const s16 blockYaw = angle.y + angleOffset;
        const f32 rad = cM_s2rad(blockYaw);

        cXyz blockPos;
        blockPos.x = pos.x + radius * std::sin(rad);
        blockPos.z = pos.z + radius * std::cos(rad);
        blockPos.y = floorY + 85.0f * boss.scale;

        const f32 bobSpeed = 0.04f + 0.006f * static_cast<f32>(k);
        const f32 bobPhase = s_bobTime * bobSpeed + static_cast<f32>(k) * 1.9f;
        const f32 bobAmp   = (13.0f + 4.0f * static_cast<f32>(k % 3)) * boss.scale;
        blockPos.y += bobAmp * std::sin(bobPhase);

        const s16 facingYaw = blockYaw + 0x4000 + static_cast<s16>(k * 0x1800);

        mDoMtx_stack_c::transS(blockPos.x, blockPos.y, blockPos.z);
        mDoMtx_stack_c::YrotM(facingYaw);
        mDoMtx_stack_c::scaleM(scale.x, scale.y, scale.z);
        renderModelAtMtx(iceModel, mDoMtx_stack_c::get());
    }
}

void render_beastganon_blood_drops(RuntimeSlot& slot, const BossGalleryEntry& boss,
                                   const cXyz& pos) {
    if (!kBeastGanonBloodDropsEnabled) return;
    if (slot.model == nullptr || slot.model->getModelData() == nullptr) return;
    if (slot.model->getModelData()->getJointNum() <= 1) return;
    if (slot.mgnDropModels[0] == nullptr) return;

    const f32 sc = boss.scale;
    const f32 groundY = pos.y + 1.0f;

    if (slot.mgnDropTimer > 0) {
        --slot.mgnDropTimer;
    } else {
        for (auto& d : slot.mgnDrops) {
            if (d.mode == 0) { d.mode = 1; break; }
        }
        slot.mgnDropTimer = (cM_rnd() < 0.3f)
            ? static_cast<int>(2.0f * (cM_rndF(3.0f) + 3.0f))
            : static_cast<int>(2.0f * (cM_rndFX(10.0f) + 15.0f));
        if (slot.mgnDropTimer < 1) slot.mgnDropTimer = 1;
    }

    for (size_t i = 0; i < RuntimeSlot::kMgnDropCount; ++i) {
        MgnBloodDrop& d = slot.mgnDrops[i];
        if (d.mode == 0) continue;

        switch (d.mode) {
        case 1:
            mDoMtx_stack_c::copy(slot.model->getAnmMtx(1));
            mDoMtx_stack_c::transM(cM_rndFX(kBeastGanonBloodDropSpread) + 200.0f,
                                   cM_rndFX(kBeastGanonBloodDropSpread * 0.7f) - 150.0f,
                                   cM_rndFX(kBeastGanonBloodDropSpread));
            mDoMtx_stack_c::multVecZero(&d.pos);
            d.velY  = 0.0f;
            d.alpha = 255.0f;
            d.base  = (cM_rnd() + 0.5f) * kBeastGanonBloodDropSizeScale * sc;
            d.sizeX = d.sizeY = d.base;
            d.mode  = 2;
        case 2:
            if (d.velY > -60.0f * sc) d.velY -= 3.0f * sc;
            d.pos.y += d.velY;
            if (d.pos.y <= groundY) {
                d.pos.y = groundY;
                d.velY  = 0.0f;
                d.mode  = 3;
            }
            break;
        case 3:
            if (d.sizeY > d.base * 0.2f) d.sizeY *= 0.6f;
            if (d.sizeX < d.base * 2.0f) d.sizeX *= 1.1f;
            if (d.sizeY < d.base * 0.2f && d.sizeX > d.base * 2.0f) d.mode = 4;
            break;
        case 4:
            d.sizeY *= 0.7f;
            d.sizeX *= 1.01f;
            d.alpha -= 20.0f;
            if (d.alpha <= 0.0f) {
                d.alpha = 0.0f;
                d.mode = 0;
            }
            break;
        default:
            break;
        }
        if (d.mode == 0) continue;

        J3DModel* m = slot.mgnDropModels[i];
        if (m == nullptr || m->getModelData() == nullptr) continue;

        J3DMaterial* mat = m->getModelData()->getMaterialNodePointer(0);
        if (mat != nullptr) {
            J3DGXColor* k = mat->getTevKColor(3);
            if (k != nullptr) k->a = static_cast<u8>(d.alpha);
        }

        mDoMtx_stack_c::transS(d.pos.x, d.pos.y, d.pos.z);
        mDoMtx_stack_c::scaleM(d.sizeX, d.sizeY, d.sizeX);
        mDoMtx_stack_c::transM(0.0f, 17.0f, 0.0f);
        renderModelAtMtx(m, mDoMtx_stack_c::get());
    }
}

}

bool boss_rush_get_ganondorf_cape_anchors(cXyz& outA, cXyz& outB) {
    const size_t count = g_bossGalleryCount < kMaxBossGalleryEntries ? g_bossGalleryCount
                                                                      : kMaxBossGalleryEntries;
    for (size_t i = 0; i < count; ++i) {
        if (std::strcmp(g_bossGalleryTable[i].displayName, "Ganondorf") != 0) continue;
        const RuntimeSlot& slot = s_slots[i];
        if (slot.model == nullptr || slot.model->getModelData() == nullptr ||
            slot.model->getModelData()->getJointNum() <= 34) {
            return false;
        }
        mDoMtx_stack_c::copy(slot.model->getAnmMtx(34));
        mDoMtx_stack_c::transM(10.0f, 5.0f, -17.0f);
        mDoMtx_stack_c::multVecZero(&outA);
        mDoMtx_stack_c::copy(slot.model->getAnmMtx(25));
        mDoMtx_stack_c::transM(10.0f, 5.0f, 17.0f);
        mDoMtx_stack_c::multVecZero(&outB);
        return true;
    }
    return false;
}

void reset_boss_rush_models() {
    for (auto& slot : s_slots) {        slot.model = nullptr;
        slot.bck = nullptr;
        slot.subModel = nullptr;
        slot.subBck = nullptr;
        slot.subHandModel = nullptr;
        slot.darkhammerBall = nullptr;
        slot.ookBoomerang = nullptr;
        for (auto& link : slot.darkhammerChainLinks) {
            link = nullptr;
        }
        slot.darkhammerBallCaptured = false;
        for (auto& ice : slot.blizzetaIceBlocks) {
            ice = nullptr;
        }
        for (auto& dropModel : slot.mgnDropModels) {
            dropModel = nullptr;
        }
        for (auto& drop : slot.mgnDrops) {
            drop = MgnBloodDrop{};
        }
        slot.mgnDropTimer = 0;
        for (auto& partModel : slot.partModels) {
            partModel = nullptr;
        }
        for (auto& partBck : slot.partBcks) {
            partBck = nullptr;
        }
        for (auto& partBrk : slot.partBrks) {
            partBrk = nullptr;
        }
        for (auto& partBtk : slot.partBtks) {
            partBtk = nullptr;
        }
        slot.brk = nullptr;
        slot.btk = nullptr;
        slot.deathSwordInvisModel = mDoExt_invisibleModel{};
        slot.hasInvisModel = false;
        slot.resolved = false;
        slot.retryFrames = 0;
    }
    for (int ti = 0; ti < 8; ++ti) {
        init_morph_tentacle(s_morphTent[ti], s_morphSink[ti]);
    }
    s_loadOrderBuilt = false;
}

void unload_boss_rush_models() {
    for (const auto& [joint, callback] : s_originalJointCallbacks) joint->setCallBack(callback);
    s_originalJointCallbacks.clear();
    const size_t count = g_bossGalleryCount < kMaxBossGalleryEntries ? g_bossGalleryCount
                                                                      : kMaxBossGalleryEntries;
    for (size_t i = 0; i < count; ++i) {
        RuntimeSlot& slot = s_slots[i];

        if (slot.hasInvisModel) {
            if (slot.deathSwordInvisModel.mpPackets != nullptr) {
                JKR_DELETE_ARRAY(slot.deathSwordInvisModel.mpPackets);
                slot.deathSwordInvisModel.mpPackets = nullptr;
            }
            slot.deathSwordInvisModel.mModel = nullptr;
            slot.hasInvisModel = false;
        }

        if (slot.bck != nullptr) {
            JKR_DELETE(slot.bck);
            slot.bck = nullptr;
        }
        if (slot.model != nullptr) {
            JKR_DELETE(slot.model);
            slot.model = nullptr;
        }
        if (slot.subBck != nullptr) {
            JKR_DELETE(slot.subBck);
            slot.subBck = nullptr;
        }
        if (slot.subHandModel != nullptr) {
            JKR_DELETE(slot.subHandModel);
            slot.subHandModel = nullptr;
        }
        if (slot.darkhammerBall != nullptr) {
            JKR_DELETE(slot.darkhammerBall);
            slot.darkhammerBall = nullptr;
        }
        if (slot.ookBoomerang != nullptr) {
            JKR_DELETE(slot.ookBoomerang);
            slot.ookBoomerang = nullptr;
        }
        for (auto& link : slot.darkhammerChainLinks) {
            if (link != nullptr) {
                JKR_DELETE(link);
                link = nullptr;
            }
        }
        slot.darkhammerBallCaptured = false;
        for (auto& ice : slot.blizzetaIceBlocks) {
            if (ice != nullptr) {
                JKR_DELETE(ice);
                ice = nullptr;
            }
        }
        for (auto& dropModel : slot.mgnDropModels) {
            if (dropModel != nullptr) {
                JKR_DELETE(dropModel);
                dropModel = nullptr;
            }
        }
        for (auto& drop : slot.mgnDrops) {
            drop = MgnBloodDrop{};
        }
        slot.mgnDropTimer = 0;
        if (slot.subModel != nullptr) {
            JKR_DELETE(slot.subModel);
            slot.subModel = nullptr;
        }
        for (auto& partModel : slot.partModels) {
            if (partModel != nullptr) {
                JKR_DELETE(partModel);
                partModel = nullptr;
            }
        }
        for (auto& partBck : slot.partBcks) {
            if (partBck != nullptr) {
                JKR_DELETE(partBck);
                partBck = nullptr;
            }
        }
        for (auto& partBrk : slot.partBrks) {
            if (partBrk != nullptr) {
                JKR_DELETE(partBrk);
                partBrk = nullptr;
            }
        }
        for (auto& partBtk : slot.partBtks) {
            if (partBtk != nullptr) {
                JKR_DELETE(partBtk);
                partBtk = nullptr;
            }
        }
        if (slot.brk != nullptr) {
            JKR_DELETE(slot.brk);
            slot.brk = nullptr;
        }
        if (slot.btk != nullptr) {
            JKR_DELETE(slot.btk);
            slot.btk = nullptr;
        }

        if (slot.resolved) {
            const BossGalleryEntry& boss = g_bossGalleryTable[i];
            unloadObjectArchive(boss.arcName);
            if (boss.animArcName != nullptr && boss.animArcName[0] != '\0') {
                unloadObjectArchive(boss.animArcName);
            }
            if (boss.subArcName != nullptr && boss.subArcName[0] != '\0') {
                unloadObjectArchive(boss.subArcName);
            }
            if (boss.subAnimArcName != nullptr && boss.subAnimArcName[0] != '\0') {
                unloadObjectArchive(boss.subAnimArcName);
            }
            if (boss.partsArcName != nullptr && boss.partsArcName[0] != '\0') {
                unloadObjectArchive(boss.partsArcName);
            }
            if (boss.brkArcName != nullptr && boss.brkArcName[0] != '\0') {
                unloadObjectArchive(boss.brkArcName);
            }
            if (boss.btkArcName != nullptr && boss.btkArcName[0] != '\0') {
                unloadObjectArchive(boss.btkArcName);
            }
        }

        slot.resolved = false;
        slot.retryFrames = 0;
    }
}

void draw_boss_rush_models(float floorY) {
    const size_t count = boss_rush_get_active_gallery_count();

    if (!s_loadOrderBuilt) {
        build_boss_rush_load_order();
    }

    if (boss_rush_scene_load_stable()) {
        for (size_t orderIdx = 0; orderIdx < count; ++orderIdx) {
            const size_t circleSlot = s_loadOrder[orderIdx];
            const size_t tableIdx = boss_rush_get_active_gallery_table_index(circleSlot);
            RuntimeSlot& slot = s_slots[tableIdx];
            if (slot.retryFrames > 0) {
                --slot.retryFrames;
                continue;
            }
            if (slot.resolved) {
                continue;
            }
            const BossGalleryEntry& boss = g_bossGalleryTable[tableIdx];

            const int archiveStatus = loadObjectArchive(boss.arcName);
            const int animArchiveStatus = (kBossGalleryAnimEnabled && boss.animArcName != nullptr && boss.animArcName[0] != '\0')
                                              ? loadObjectArchive(boss.animArcName)
                                              : 0;
            const int subArchiveStatus = (boss.subArcName != nullptr && boss.subArcName[0] != '\0')
                                             ? loadObjectArchive(boss.subArcName)
                                             : 0;
            const int subAnimArchiveStatus = (kBossGalleryAnimEnabled && boss.subAnimArcName != nullptr && boss.subAnimArcName[0] != '\0')
                                                 ? loadObjectArchive(boss.subAnimArcName)
                                                 : 0;
            const int partsArchiveStatus = (boss.partsArcName != nullptr && boss.partsArcName[0] != '\0')
                                                ? loadObjectArchive(boss.partsArcName)
                                                : 0;
            const int brkArchiveStatus = (boss.brkArcName != nullptr && boss.brkArcName[0] != '\0')
                                              ? loadObjectArchive(boss.brkArcName)
                                              : 0;
            const int btkArchiveStatus = (boss.btkArcName != nullptr && boss.btkArcName[0] != '\0')
                                              ? loadObjectArchive(boss.btkArcName)
                                              : 0;

            // Failed first-load requests must not permanently resolve an empty
            // slot. Back off so one missing archive does not block the gallery.
            if (archiveStatus < 0 || animArchiveStatus < 0 || subArchiveStatus < 0 ||
                subAnimArchiveStatus < 0 || partsArchiveStatus < 0 ||
                brkArchiveStatus < 0 || btkArchiveStatus < 0) {
                slot.retryFrames = 60;
                break;
            }

            if (archiveStatus == 1 || animArchiveStatus == 1 || subArchiveStatus == 1 || subAnimArchiveStatus == 1 ||
                partsArchiveStatus == 1 || brkArchiveStatus == 1 || btkArchiveStatus == 1) {
            } else {
                if (archiveStatus == 0 && animArchiveStatus == 0) {
                    slot.model = loadBmdFromArc(boss.arcName, boss.bmdName);
                    if (!slot.model) {
                        slot.retryFrames = 60;
                        break;
                    }
                    slot.resolved = true;

                    if (slot.model != nullptr && slot.model->getModelData() != nullptr &&
                        std::strcmp(boss.displayName, "Death Sword") == 0) {
                        slot.deathSwordInvisModel.create(slot.model, 1);
                        slot.hasInvisModel = true;
                    }

                    if ((kBossGalleryAnimEnabled || boss.forceAnim) && slot.model != nullptr &&
                        boss.bckName != nullptr && boss.bckName[0] != '\0') {
                        const char* animArc = (boss.animArcName != nullptr && boss.animArcName[0] != '\0')
                                                  ? boss.animArcName
                                                  : boss.arcName;
                        slot.bck = loadBckFromArc(animArc, boss.bckName);
                    }

                    if (slot.model != nullptr && std::strcmp(boss.displayName, "Morpheel") == 0 &&
                        slot.bck == nullptr) {
                        slot.bck = loadBckFromArcIdx(boss.arcName, 0x1E);
                    }

                    if (brkArchiveStatus == 0 && slot.model != nullptr &&
                        boss.brkName != nullptr && boss.brkName[0] != '\0') {
                        const char* brkArc = (boss.brkArcName != nullptr && boss.brkArcName[0] != '\0')
                                                 ? boss.brkArcName
                                                 : boss.arcName;
                        slot.brk = loadBrkFromArc(brkArc, boss.brkName, slot.model->getModelData());
                    }
                    if (btkArchiveStatus == 0 && slot.model != nullptr &&
                        boss.btkName != nullptr && boss.btkName[0] != '\0') {
                        const char* btkArc = (boss.btkArcName != nullptr && boss.btkArcName[0] != '\0')
                                                 ? boss.btkArcName
                                                 : boss.arcName;
                        slot.btk = loadBtkFromArc(btkArc, boss.btkName, slot.model->getModelData());
                    }
                }

                if (subArchiveStatus == 0 && subAnimArchiveStatus == 0 &&
                    boss.subArcName != nullptr && boss.subBmdName != nullptr) {
                    slot.subModel = loadBmdFromArc(boss.subArcName, boss.subBmdName);
                    if (kBossGalleryAnimEnabled && slot.subModel != nullptr &&
                        boss.subBckName != nullptr && boss.subBckName[0] != '\0') {
                        const char* subAnimArc = (boss.subAnimArcName != nullptr && boss.subAnimArcName[0] != '\0')
                                                     ? boss.subAnimArcName
                                                     : boss.subArcName;
                        slot.subBck = loadBckFromArc(subAnimArc, boss.subBckName);
                    }
                    if (slot.subModel != nullptr && std::strcmp(boss.subArcName, "HoZelda") == 0) {
                        slot.subHandModel = loadBmdFromArc("HoZelda", "o_zg_bow.bmd");
                    }
                }

                if (partsArchiveStatus == 0 && slot.model != nullptr &&
                    std::strcmp(boss.displayName, "Darkhammer") == 0) {
                    slot.darkhammerBall = loadBmdFromArc("E_th_ball", "ib.bmd");
                    for (size_t l = 0; l < RuntimeSlot::kDarkhammerChainLinkCount; ++l) {
                        slot.darkhammerChainLinks[l] = loadBmdFromArc("E_th_ball", "tc.bmd");
                    }
                }

                if (archiveStatus == 0 && slot.model != nullptr &&
                    std::strcmp(boss.displayName, "Blizzeta") == 0) {
                    for (size_t k = 0; k < RuntimeSlot::kBlizzetaIceBlockCount; ++k) {
                        slot.blizzetaIceBlocks[k] = loadBmdFromArc("B_yo", "yo_ice.bmd");
                    }
                }

                if (archiveStatus == 0 && slot.model != nullptr &&
                    std::strcmp(boss.displayName, "Ook") == 0) {
                    slot.ookBoomerang = loadBmdFromArcIdx(boss.arcName, 0x2E);
                }

                if (archiveStatus == 0 && slot.model != nullptr &&
                    kBeastGanonBloodDropsEnabled &&
                    std::strcmp(boss.displayName, "Beast Ganon") == 0) {
                    for (auto& dropModel : slot.mgnDropModels) {
                        dropModel = loadBmdFromArcIdx(boss.arcName, 0x30);
                    }
                }

                if (partsArchiveStatus == 0 && boss.parts != nullptr && boss.partCount > 0) {
                    const char* partsArc = (boss.partsArcName != nullptr && boss.partsArcName[0] != '\0')
                                               ? boss.partsArcName
                                               : boss.arcName;
                    const u8 partCount = boss.partCount < kMaxBossPartsPerEntry ? boss.partCount
                                                                                 : static_cast<u8>(kMaxBossPartsPerEntry);
                    for (u8 p = 0; p < partCount; ++p) {
                        slot.partModels[p] = loadBmdFromArc(partsArc, boss.parts[p].bmdName);
                        if (slot.partModels[p] != nullptr &&
                            std::strcmp(boss.displayName, "Morpheel") == 0 &&
                            std::strcmp(boss.parts[p].bmdName, "oh.bmd") == 0) {
                            J3DModelData* tmd = slot.partModels[p]->getModelData();
                            if (tmd != nullptr) {
                                for (u16 jc = 0; jc < tmd->getJointNum(); jc++) {
                                    auto* joint = tmd->getJointNodePointer(jc);
                                    s_originalJointCallbacks.emplace(joint, joint->getCallBack());
                                    joint->setCallBack(morph_tent_jointCB);
                                }
                            }
                            const int ti = (p >= 1 && p <= 8) ? (p - 1) : 0;
                            init_morph_tentacle(s_morphTent[ti], s_morphSink[ti]);
                            slot.partModels[p]->setUserArea(reinterpret_cast<uintptr_t>(&s_morphTent[ti]));
                        }

                        const BossPartAttachment& part = boss.parts[p];
                        if (slot.partModels[p] != nullptr && part.bckName != nullptr && part.bckName[0] != '\0') {
                            const char* partBckArc = (part.bckArcName != nullptr && part.bckArcName[0] != '\0')
                                                          ? part.bckArcName
                                                          : partsArc;
                            slot.partBcks[p] = loadBckFromArc(partBckArc, part.bckName);
                        }
                        if (slot.partModels[p] != nullptr && part.brkName != nullptr && part.brkName[0] != '\0') {
                            slot.partBrks[p] = loadBrkFromArc(partsArc, part.brkName, slot.partModels[p]->getModelData());
                        }
                        if (slot.partModels[p] != nullptr && part.btkName != nullptr && part.btkName[0] != '\0') {
                            slot.partBtks[p] = loadBtkFromArc(partsArc, part.btkName, slot.partModels[p]->getModelData());
                        }

                        if (slot.partModels[p] != nullptr &&
                            std::strcmp(boss.displayName, "Morpheel") == 0 &&
                            std::strcmp(boss.parts[p].bmdName, "oh_core.bmd") == 0) {
                            slot.partBcks[p] = loadBckFromArcIdx(partsArc, 0x11);
                        }
                    }
                }

            }

            break;
        }
    }

    for (size_t circleSlot = 0; circleSlot < count; ++circleSlot) {
        const size_t tableIdx = boss_rush_get_active_gallery_table_index(circleSlot);
        const BossGalleryEntry& boss = g_bossGalleryTable[tableIdx];
        RuntimeSlot& slot = s_slots[tableIdx];

        if (slot.model == nullptr && slot.subModel == nullptr) {
            continue;
        }

        cXyz pos;
        csXyz angle;
        boss_rush_get_slot_transform(circleSlot, count, floorY, pos, angle);
        pos.y += boss.yOffset;

        angle.x += boss.modelRot.x;
        angle.y += boss.modelRot.y;
        angle.z += boss.modelRot.z;

        if (boss.radialOffset != 0.0f) {
            pos.x += boss.radialOffset * (pos.x / kChamberCircleRadius);
            pos.z += boss.radialOffset * (pos.z / kChamberCircleRadius);
        }

        cXyz scale(boss.scale, boss.scale, boss.scale);
        if (slot.model != nullptr) {
            if (std::strcmp(boss.displayName, "Death Sword") == 0) {
                if (slot.bck != nullptr) {
                    slot.bck->play();
                    slot.bck->entry(slot.model->getModelData());
                }
                mDoMtx_stack_c::transS(pos.x, pos.y, pos.z);
                mDoMtx_stack_c::ZXYrotM(angle.x, angle.y, angle.z);
                slot.model->setBaseScale(scale);
                slot.model->setBaseTRMtx(mDoMtx_stack_c::get());
                slot.model->calc();

                daAlink_c* alink = daAlink_getAlinkActorClass();
                if (alink != nullptr) {
                    g_env_light.settingTevStruct_colget_player(&alink->tevStr);
                    g_env_light.setLightTevColorType_MAJI(slot.model, &alink->tevStr);
                }

                apply_deathsword_ghost_material(slot.model);

                if (slot.hasInvisModel) {
                    slot.deathSwordInvisModel.entryDL(nullptr);
                } else {
                    mDoExt_modelUpdateDL(slot.model);
                }
            } else {
                if (slot.brk != nullptr) {
                    slot.brk->play();
                    slot.brk->entry(slot.model->getModelData());
                }
                if (slot.btk != nullptr) {
                    slot.btk->play();
                    slot.btk->entry(slot.model->getModelData());
                }
                renderModelAt(slot.model, pos, angle, scale, slot.bck);
            }
        }
        if (slot.subModel != nullptr) {
            cXyz subPos = pos;
            if (slot.model != nullptr && slot.model->getModelData() != nullptr &&
                slot.model->getModelData()->getJointNum() > 21) {
                mDoMtx_multVecZero(slot.model->getAnmMtx(21), &subPos);
                subPos.y -= 185.0f * boss.scale;
            }
            renderModelAt(slot.subModel, subPos, angle, scale, slot.subBck);

            if (slot.subHandModel != nullptr && slot.subModel->getModelData() != nullptr &&
                slot.subModel->getModelData()->getJointNum() > 0x11) {
                mDoMtx_stack_c::copy(slot.subModel->getAnmMtx(0x11));
                mDoMtx_stack_c::transM(10.0f, -2.0f, 0.0f);
                mDoMtx_stack_c::XYZrotM(cM_deg2s(95.0f), 0, cM_deg2s(10.0f));
                Mtx bowMtx;
                MTXCopy(mDoMtx_stack_c::get(), bowMtx);
                renderModelAtMtx(slot.subHandModel, bowMtx);
            }
        }

        if (slot.model != nullptr && slot.darkhammerBall != nullptr &&
            std::strcmp(boss.displayName, "Darkhammer") == 0) {
            render_darkhammer_ball_and_chain(slot, boss, floorY, pos, angle, scale);
        }

        if (slot.model != nullptr && std::strcmp(boss.displayName, "Blizzeta") == 0) {
            render_blizzeta_ice_blocks(slot, boss, floorY, pos, angle);
        }

        if (slot.model != nullptr && std::strcmp(boss.displayName, "Beast Ganon") == 0) {
            render_beastganon_blood_drops(slot, boss, pos);
        }

        if (slot.model != nullptr && slot.ookBoomerang != nullptr &&
            std::strcmp(boss.displayName, "Ook") == 0 &&
            slot.model->getModelData() != nullptr &&
            slot.model->getModelData()->getJointNum() > 0x17) {
            mDoMtx_stack_c::copy(slot.model->getAnmMtx(0x17));
            mDoMtx_stack_c::transM(15.0f, 70.0f, 20.0f);
            mDoMtx_stack_c::YrotM(-0x652C);
            mDoMtx_stack_c::XrotM(-0x2219);
            mDoMtx_stack_c::ZrotM(0x38D8);
            mDoMtx_stack_c::scaleM(0.75f, 0.75f, 0.75f);
            renderModelAtMtx(slot.ookBoomerang, mDoMtx_stack_c::get());
        }

        if (boss.parts != nullptr && boss.partCount > 0 && slot.model != nullptr &&
            slot.model->getModelData() != nullptr) {
            Mtx partWorldMtx[kMaxBossPartsPerEntry];
            const u8 partCount = boss.partCount < kMaxBossPartsPerEntry ? boss.partCount
                                                                         : static_cast<u8>(kMaxBossPartsPerEntry);
            const u16 jointNum = slot.model->getModelData()->getJointNum();

            if (std::strcmp(boss.displayName, "Morpheel") == 0) {
                for (int ti = 0; ti < 8; ++ti) {
                    update_morph_tentacle(s_morphTent[ti], s_morphSink[ti]);
                }
            }

            for (u8 p = 0; p < partCount; ++p) {
                J3DModel* partModel = slot.partModels[p];
                if (partModel == nullptr) {
                    continue;
                }

                const BossPartAttachment& part = boss.parts[p];

                if (std::strcmp(boss.displayName, "Morpheel") == 0 &&
                    std::strcmp(part.bmdName, "oh.bmd") == 0) {
                    const int ti = p - 1;
                    if (ti < 0 || ti >= 8) continue;

                    cXyz rootPos;
                    mDoMtx_multVecZero(slot.model->getAnmMtx(part.jointIndex), &rootPos);
                    rootPos.x = pos.x + (rootPos.x - pos.x) / kTentacleSpread;
                    rootPos.z = pos.z + (rootPos.z - pos.z) / kTentacleSpread;
                    const s16 outYaw = cM_atan2s(rootPos.x - pos.x, rootPos.z - pos.z);

                    if (slot.partBrks[p] != nullptr) {
                        slot.partBrks[p]->play();
                        slot.partBrks[p]->entry(partModel->getModelData());
                    }
                    if (slot.partBtks[p] != nullptr) {
                        slot.partBtks[p]->play();
                        slot.partBtks[p]->entry(partModel->getModelData());
                    }

                    mDoMtx_stack_c::transS(rootPos.x, rootPos.y + s_morphSink[ti], rootPos.z);
                    mDoMtx_stack_c::YrotM(outYaw);
                    mDoMtx_stack_c::XrotM(s_morphTent[ti].pitch);
                    mDoMtx_stack_c::scaleM(boss.scale, boss.scale, boss.scale);
                    renderModelAtMtx(partModel, mDoMtx_stack_c::get());
                    continue;
                }

                MtxP baseMtx = nullptr;
                if (part.parentPart < 0) {
                    if (part.jointIndex < 0 || jointNum <= static_cast<u16>(part.jointIndex)) {
                        continue;
                    }
                    baseMtx = slot.model->getAnmMtx(part.jointIndex);
                } else if (part.parentPart < static_cast<s8>(p)) {
                    baseMtx = partWorldMtx[part.parentPart];
                } else {
                    continue;
                }

                mDoMtx_stack_c::copy(baseMtx);
                mDoMtx_stack_c::transM(part.offset.x, part.offset.y, part.offset.z);
                mDoMtx_stack_c::ZXYrotM(part.rot.x, part.rot.y, part.rot.z);
                MTXCopy(mDoMtx_stack_c::get(), partWorldMtx[p]);

                if (slot.partBcks[p] != nullptr) {
                    if (std::strcmp(boss.displayName, "Morpheel") == 0 &&
                        std::strcmp(part.bmdName, "oh_core.bmd") == 0) {
                        slot.partBcks[p]->play();
                        slot.partBcks[p]->entry(partModel->getModelData());
                    } else {
                        slot.partBcks[p]->entry(partModel->getModelData(), part.bckFrame);
                    }
                }
                if (slot.partBrks[p] != nullptr) {
                    slot.partBrks[p]->play();
                    slot.partBrks[p]->entry(partModel->getModelData());
                }
                if (slot.partBtks[p] != nullptr) {
                    slot.partBtks[p]->play();
                    slot.partBtks[p]->entry(partModel->getModelData());
                }
                renderModelAtMtx(partModel, partWorldMtx[p]);
            }
        }
    }
}
