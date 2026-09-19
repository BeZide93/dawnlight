// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#pragma once

#include "global.h"
#include "SSystem/SComponent/c_math.h"
#include "SSystem/SComponent/c_xyz.h"
#include "SSystem/SComponent/c_sxyz.h"

#include <cstddef>
#include <cmath>

struct BossGalleryEntry {
    const char* displayName;
    const char* location;
    const char* arcName;
    const char* bmdName;
    const char* animArcName;
    const char* bckName;
    const char* stage;
    s16 point;
    s8 room;
    s8 layer;
    f32 scale;
    f32 yOffset;
    f32 labelYOffset;
    f32 radialOffset = 0.0f;

    const char* subArcName = nullptr;
    const char* subBmdName = nullptr;
    const char* subAnimArcName = nullptr;
    const char* subBckName = nullptr;

    const char* partsArcName = nullptr;
    const struct BossPartAttachment* parts = nullptr;
    u8 partCount = 0;

    const char* brkArcName = nullptr;
    const char* brkName = nullptr;
    const char* btkArcName = nullptr;
    const char* btkName = nullptr;

    const cXyz* fightSpawnPos = nullptr;
    s16 fightSpawnAngle = 0;

    csXyz modelRot = csXyz(0, 0, 0);

    bool forceAnim = false;
};

struct BossPartAttachment {
    const char* bmdName;
    s8 jointIndex;
    s8 parentPart;
    cXyz offset;
    csXyz rot;

    const char* bckArcName = nullptr;
    const char* bckName = nullptr;
    f32 bckFrame = 0.0f;

    const char* brkName = nullptr;
    const char* btkName = nullptr;
};

extern const BossGalleryEntry g_bossGalleryTable[];
extern const size_t g_bossGalleryCount;

constexpr size_t kMaxBossPartsPerEntry = 18;

bool boss_rush_scene_load_stable();

constexpr bool kBossGalleryModelsEnabled = true;

constexpr bool kBossGalleryTextsEnabled = true;

constexpr bool kBossGalleryAnimEnabled = true;

constexpr bool kBeastGanonBloodDropsEnabled = true;
constexpr float kBeastGanonBloodDropSizeScale = 1.0f;
constexpr float kBeastGanonBloodDropSpread = 120.0f;

constexpr size_t kMaxBossGalleryEntries = 24;

constexpr const char* kBossRushChamberStage = "D_MN06B";
constexpr s8  kBossRushChamberRoom = 51;
constexpr s16 kBossRushChamberPoint = -1;
constexpr s8  kBossRushChamberLayer = 0;
constexpr f32 kChamberCircleRadius = 1350.0f;
constexpr f32 kBossChamberFloorY = -400.0f;

constexpr f32 kBossInteractRadius = 250.0f;

inline void boss_rush_get_slot_transform(size_t index, size_t total, f32 floorY,
                                          cXyz& outPos, csXyz& outAngle) {
    const f32 angleDeg = 180.0f - (static_cast<f32>(index) * 360.0f / static_cast<f32>(total));
    const f32 angleRad = (angleDeg * 3.14159265f) / 180.0f;
    outPos.x = kChamberCircleRadius * std::sin(angleRad);
    outPos.z = kChamberCircleRadius * std::cos(angleRad);
    outPos.y = floorY;

    outAngle.x = 0;
    outAngle.y = cM_deg2s(angleDeg + 180.0f);
    outAngle.z = 0;
}

inline f32 boss_rush_load_priority_distance(size_t index, size_t total) {
    f32 angleDeg = 180.0f - (static_cast<f32>(index) * 360.0f / static_cast<f32>(total));
    angleDeg = std::fmod(angleDeg, 360.0f);
    if (angleDeg < 0.0f) {
        angleDeg += 360.0f;
    }
    f32 dist = std::fabs(angleDeg - 180.0f);
    return dist > 180.0f ? 360.0f - dist : dist;
}
