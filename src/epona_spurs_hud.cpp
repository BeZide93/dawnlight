#include "epona_spurs_hud.hpp"
#include "hud_layout.hpp"
#include "service_imports.hpp"
#include "d/d_meter_hakusha.h"
#include "d/d_meter2_draw.h"
#include "d/d_pane_class.h"
#include "mods/svc/hook.hpp"
#include <array>
#include <algorithm>

namespace dawnlight {
namespace {
DEFINE_HOOK(&dMeterHakusha_c::draw, SpursDraw);
DEFINE_HOOK(&dMeter2Draw_c::drawPikariHakusha, SpursFlash);

struct SpurPaneState {
    J2DPane* pane = nullptr;
    float x = 0, y = 0, sx = 1, sy = 1;
};
struct SpurDrawState {
    dMeterHakusha_c* owner = nullptr;
    std::array<std::array<float, 2>, 12> positions{};
    std::array<SpurPaneState, 3> panes{};
    float buttonX = 0, buttonY = 0, scale = 1;
    int count = 0;
} s_spurs;

HookAction before_spurs_draw(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeterHakusha_c*>(args, 0);
    const auto transform = hud_layout_epona_spurs_transform();
    if (!meter || s_spurs.owner ||
        (transform.offset_x == 0 && transform.offset_y == 0 && transform.scale == 1))
        return HOOK_CONTINUE;
    const int count = std::clamp(meter->getHakushaNum(), 0, 12);
    if (!count) return HOOK_CONTINUE;

    // updateHakusha caches screen-space positions from main-HUD markers, but
    // draw renders separate screens. Moving only hakunall misses those cached
    // positions and does not scale the icons or their flashes. Borrow the draw
    // inputs instead, after other layout hooks, and restore them in the post.
    s_spurs.owner = meter;
    s_spurs.count = count;
    s_spurs.scale = transform.scale;
    s_spurs.buttonX = meter->mButtonAPosX;
    s_spurs.buttonY = meter->mButtonAPosY;
    const float anchorX = meter->mHakushaData[0].pos_x;
    const float anchorY = meter->mHakushaData[0].pos_y;
    const auto x = [&](float value) { return anchorX + (value - anchorX) * transform.scale + transform.offset_x; };
    const auto y = [&](float value) { return anchorY + (value - anchorY) * transform.scale + transform.offset_y; };
    for (int i = 0; i < count; ++i) {
        auto& data = meter->mHakushaData[i];
        s_spurs.positions[i] = {data.pos_x, data.pos_y};
        data.pos_x = x(data.pos_x);
        data.pos_y = y(data.pos_y);
    }
    meter->mButtonAPosX = x(meter->mButtonAPosX);
    meter->mButtonAPosY = y(meter->mButtonAPosY);
    const std::array<CPaneMgr*, 3> managers{meter->mpHakushaOn, meter->mpHakushaOff, meter->mpButtonA};
    for (size_t i = 0; i < managers.size(); ++i) {
        auto* pane = managers[i] ? managers[i]->getPanePtr() : nullptr;
        if (!pane) continue;
        s_spurs.panes[i] = {pane, pane->getTranslateX(), pane->getTranslateY(), pane->getScaleX(), pane->getScaleY()};
        pane->scale(pane->getScaleX() * transform.scale, pane->getScaleY() * transform.scale);
    }
    return HOOK_CONTINUE;
}

HookAction before_spurs_flash(ModContext*, void* args, void*, void*) {
    // The native flash center already comes from the transformed icon. Only
    // multiply its size; translating it again would apply the offset twice.
    if (s_spurs.owner) mods::arg_ref<float>(args, 4) *= s_spurs.scale;
    return HOOK_CONTINUE;
}

void after_spurs_draw(ModContext*, void* args, void*, void*) {
    if (!s_spurs.owner || mods::arg<dMeterHakusha_c*>(args, 0) != s_spurs.owner) return;
    auto* meter = s_spurs.owner;
    for (int i = 0; i < s_spurs.count; ++i) {
        meter->mHakushaData[i].pos_x = s_spurs.positions[i][0];
        meter->mHakushaData[i].pos_y = s_spurs.positions[i][1];
    }
    meter->mButtonAPosX = s_spurs.buttonX;
    meter->mButtonAPosY = s_spurs.buttonY;
    for (const auto& state : s_spurs.panes) {
        if (!state.pane) continue;
        state.pane->translate(state.x, state.y);
        state.pane->scale(state.sx, state.sy);
    }
    // Do not rewind flags, alpha, or animation frames advanced by native draw.
    // No pointers survive the draw, including when mounting/dismounting or
    // changing scenes creates a new sub-meter at the same address.
    s_spurs = {};
}
} // namespace

ModResult initialize_epona_spurs_hud(ModError* error) {
    HookOptions pre = HOOK_OPTIONS_INIT, post = HOOK_OPTIONS_INIT;
    pre.priority = -100;
    post.priority = 100;
    ModResult result = mods::hook::add_pre<SpursDraw>(svc_hook, before_spurs_draw, &pre);
    if (result == MOD_OK) result = mods::hook::add_post<SpursDraw>(svc_hook, after_spurs_draw, &post);
    if (result == MOD_OK) result = mods::hook::add_pre<SpursFlash>(svc_hook, before_spurs_flash, &pre);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result, "failed to install Epona spurs HUD hooks");
}
} // namespace dawnlight
