// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#include "boss_rush_texts.hpp"
#include "boss_rush_common.hpp"
#include "boss_rush.hpp"
#include "../save_state.hpp"

#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_camera_mng.h"
#include "d/d_meter2_info.h"
#include "m_Do/m_Do_lib.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include "JSystem/J2DGraph/J2DOrthoGraph.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/JUtility/TColor.h"
#include "JSystem/JUtility/JUTFont.h"

#include <cstdio>
#include <cstring>

namespace {

f32 s_labelFade[kMaxBossGalleryEntries] = {};

void draw_world_label(const char* text, f32 x, f32 y, f32 charW, f32 charH,
                      JUtility::TColor top, JUtility::TColor bottom, u8 alpha) {
    JUTFont* font = mDoExt_getSubFont();
    if (!font) font = mDoExt_getMesgFont();
    if (!font) return;

    font->setGX();

    const f32 c = 1.6f;
    const f32 d = 1.1f;
    const f32 kOff[8][2] = {
        { c, 0.0f}, {-c, 0.0f}, {0.0f,  c}, {0.0f, -c},
        { d, d}, {d, -d}, {-d, d}, {-d, -d},
    };
    font->setCharColor(JUtility::TColor(0, 0, 0, alpha));
    for (const auto& o : kOff) {
        font->drawString_scale(x + o[0], y + o[1], charW, charH, text, true);
    }

    JUtility::TColor t = top;
    JUtility::TColor b = bottom;
    t.a = alpha;
    b.a = alpha;
    font->setGradColor(t, b);
    font->drawString_scale(x, y, charW, charH, text, true);
}

f32 measure_text_width(const char* text, f32 charW) {
    JUTFont* font = mDoExt_getSubFont();
    if (!font) font = mDoExt_getMesgFont();
    if (!font) return static_cast<f32>(std::strlen(text)) * charW;

    f32 total = 0.0f;
    for (size_t i = 0; text[i] != '\0'; i++) {
        f32 w = static_cast<f32>(font->getWidth(text[i]));
        if (w <= 0.0f) w = static_cast<f32>(font->getWidth());
        total += w * (charW / static_cast<f32>(font->getWidth()));
    }
    return total;
}

}

void boss_rush_texts_reset_fade() {
    for (auto& f : s_labelFade) f = 0.0f;
}

void draw_boss_rush_texts(float floorY) {
    if (!boss_rush_scene_load_stable()) {
        return;
    }

    J2DFillBox(0.0f, 0.0f, 0.0f, 0.0f, JUtility::TColor(0, 0, 0, 0));

    const size_t count = boss_rush_get_active_gallery_count();

    const daAlink_c* link = daAlink_getAlinkActorClass();

    for (size_t circleSlot = 0; circleSlot < count; ++circleSlot) {
        const size_t tableIdx = boss_rush_get_active_gallery_table_index(circleSlot);
        const BossGalleryEntry& boss = g_bossGalleryTable[tableIdx];

        cXyz pos;
        csXyz angle;
        boss_rush_get_slot_transform(circleSlot, count, floorY, pos, angle);

        f32 target = 0.0f;
        if (link != nullptr) {
            const f32 dx = link->current.pos.x - pos.x;
            const f32 dz = link->current.pos.z - pos.z;
            if (dx * dx + dz * dz < kBossInteractRadius * kBossInteractRadius) target = 1.0f;
        }
        s_labelFade[tableIdx] += (target - s_labelFade[tableIdx]) * 0.12f;
        if (s_labelFade[tableIdx] < 0.004f) { s_labelFade[tableIdx] = 0.0f; continue; }

        pos.y += boss.labelYOffset;

        Vec screenPos;
        mDoLib_project(&pos, &screenPos);

        if (screenPos.z >= 400000.0f || screenPos.x < -80.0f || screenPos.x > 720.0f ||
            screenPos.y < -80.0f || screenPos.y > 500.0f) {
            continue;
        }

        const f32 nameCharW = 22.0f, nameCharH = 26.0f;
        const f32 locCharW = 14.0f, locCharH = 17.0f;

        f32 nameW = measure_text_width(boss.displayName, nameCharW);
        f32 locW = boss.location ? measure_text_width(boss.location, locCharW) : 0.0f;

        const u8 nameA = static_cast<u8>(255.0f * s_labelFade[tableIdx]);
        const u8 locA = static_cast<u8>(220.0f * s_labelFade[tableIdx]);

        const bool defeated = dawnlight::save_state_boss_defeated(static_cast<u8>(tableIdx));
        draw_world_label(boss.displayName, screenPos.x - nameW * 0.5f, screenPos.y - nameCharH - locCharH,
                         nameCharW, nameCharH,
                         defeated ? JUtility::TColor(255, 160, 150, 255) : JUtility::TColor(255, 236, 170, 255),
                         defeated ? JUtility::TColor(210, 70, 60, 255) : JUtility::TColor(255, 190, 60, 255), nameA);

        if (boss.location != nullptr && boss.location[0] != '\0') {
            draw_world_label(boss.location, screenPos.x - locW * 0.5f, screenPos.y - locCharH,
                             locCharW, locCharH,
                             JUtility::TColor(230, 230, 230, 255), JUtility::TColor(180, 180, 180, 255), locA);
        }


    }

    J2DGrafContext* port = dComIfGp_getCurrentGrafPort();
    if (port) port->setup2D();
}
