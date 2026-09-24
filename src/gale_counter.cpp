#include "gale_counter.hpp"

#include "combat_meter.hpp"
#include "config.hpp"
#include "gale_charges.hpp"
#include "hud_layout.hpp"
#include "service_imports.hpp"
#include "stamina.hpp"

#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "d/d_pane_class.h"
#include "m_Do/m_Do_ext.h"
#include "mods/service.hpp"
#include "mods/svc/hook.hpp"

#include <chrono>
#include <algorithm>
#include <limits>

namespace dawnlight {
namespace {
DEFINE_HOOK(&dMeter2Draw_c::draw, GaleCounterDraw);
DEFINE_HOOK(&dMeter2_c::_delete, GaleCounterMeterDelete);
GaleCharges s_charges;

void update_charges() {
    const double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    s_charges.update(now, gale_counter_capacity(), gale_recovery_seconds());
}

void include_icon_left(CPaneMgr* manager, J2DPane* pane, float& left) {
    if (!pane || !pane->isVisible()) return;
    // haku_n/haku_b_n are layout containers, not the left edge of the icon.
    // Their picture children extend left of that anchor. Align the artwork.
    if (pane->getTypeID() == 18) { // J2DPicture (including J2DPictureEx)
        Mtx matrix;
        for (u8 corner = 0; corner < 4; ++corner)
            left = std::min(left, manager->getGlobalVtx(pane, &matrix, corner, false, 0).x);
    }
    for (auto* child = pane->getFirstChildPane(); child; child = child->getNextChildPane())
        include_icon_left(manager, child, left);
}

float icon_left(CPaneMgr* pane, float fallback) {
    float left = std::numeric_limits<float>::max();
    include_icon_left(pane, pane->getPanePtr(), left);
    return left == std::numeric_limits<float>::max() ? fallback : left;
}

struct CounterVisual {
    JKRSolidHeap* heap = nullptr;
    J2DScreen* screen = nullptr;
    CPaneMgr* full = nullptr;
    CPaneMgr* empty = nullptr;
    bool failed = false;

    void release() {
        JKR_DELETE(full);
        JKR_DELETE(empty);
        JKR_DELETE(screen);
        full = empty = nullptr;
        screen = nullptr;
        if (heap) mDoExt_destroySolidHeap(heap);
        heap = nullptr;
        failed = false;
    }
    bool prepare() {
        if (screen) return true;
        if (failed || !dComIfGp_getMain2DArchive()) return false;
        heap = mDoExt_createSolidHeapFromGame(0x20000, 0x20);
        if (!heap) { failed = true; return false; }
        auto* previous = mDoExt_setCurrentHeap(heap);
        screen = JKR_NEW J2DScreen();
        bool ready = screen && screen->setPriority("zelda_game_image_hakusha_parts.blo", 0x20000,
                                                   dComIfGp_getMain2DArchive());
        if (ready) {
            dPaneClass_showNullPane(screen);
            ready = screen->search(MULTI_CHAR('haku_n')) && screen->search(MULTI_CHAR('haku_b_n'));
        }
        if (ready) {
            full = JKR_NEW CPaneMgr(screen, MULTI_CHAR('haku_n'), 2, nullptr);
            empty = JKR_NEW CPaneMgr(screen, MULTI_CHAR('haku_b_n'), 2, nullptr);
            ready = full && empty;
        }
        mDoExt_setCurrentHeap(previous);
        if (!ready) { release(); failed = true; return false; }
        mDoExt_adjustSolidHeap(heap);
        return true;
    }
    void draw(float x, float y, float scale, int available, int capacity) {
        if (!prepare()) return;
        auto* graf = dComIfGp_getCurrentGrafPort();
        if (!graf) return;
        graf->setup2D();
        full->setAlphaRate(1.0f);
        empty->setAlphaRate(1.0f);
        full->scale(scale, scale);
        empty->scale(scale, scale);
        for (int i = 0; i < capacity; ++i) {
            full->hide();
            empty->hide();
            auto* pane = i < available ? full : empty;
            pane->show();
            pane->translate(0, 0);
            Mtx matrix;
            const Vec corner = pane->getGlobalVtx(&matrix, 0, false, 0);
            pane->translate(x + i * 32.0f * scale - icon_left(pane, corner.x), y - corner.y);
            screen->draw(0, 0, graf);
        }
    }
};
CounterVisual s_visual;

void after_draw(ModContext*, void* args, void*, void*) {
    if (!revalis_gale_enabled() || !gale_counter_visible()) return;
    auto* link = daAlink_getAlinkActorClass();
    if (!link || link->checkWolf() || dComIfGp_event_runCheck() || dComIfGp_isEnableNextStage() ||
        dComIfGp_isPauseFlag() || dMeter2Info_getWindowStatus() || dMeter2Info_getPauseStatus()) return;
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    float x, y, scale;
    if (!combat_meter_next_row_anchor(meter, stamina_meter_visible() ? 1 : 0, x, y, scale)) return;
    // The default follows the Fierce Deity bar. These independent controls add
    // offsets and scale only to the Gale Counter, including imported presets.
    // Keep the built-in X offset in sync with the preset defaults in config.cpp.
    const auto transform = custom_hud_layout_enabled() ?
        hud_custom_element_transform(HudElement::GaleCounter) :
        DuskModHudTransform{.offset_x = 25.0f};
    update_charges();
    s_visual.draw(x + transform.offset_x, y + transform.offset_y,
        0.65f * scale * transform.scale, s_charges.available(), s_charges.capacity);
}

HookAction before_meter_delete(ModContext*, void*, void*, void*) {
    // Release archive-backed artwork before the HUD/scene heap disappears.
    // Intentionally leave the session resource and its clock untouched.
    s_visual.release();
    return HOOK_CONTINUE;
}
}  // namespace

bool gale_charge_available() {
    update_charges();
    return s_charges.available() > 0;
}

void consume_gale_charge() {
    update_charges();
    s_charges.consume();
}

ModResult initialize_gale_counter(ModError* error) {
    auto result = mods::hook::add_post<GaleCounterDraw>(svc_hook, after_draw);
    if (result == MOD_OK) result = mods::hook::add_pre<GaleCounterMeterDelete>(svc_hook, before_meter_delete);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result, "failed to install Gale Counter HUD hooks");
}

void shutdown_gale_counter() {
    s_visual.release();
    s_charges = {};
}
}  // namespace dawnlight
