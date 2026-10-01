#include "clear_timer.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "save_state.hpp"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DPrint.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_lib.h"
#include "mods/service.hpp"
#include "mods/hook.hpp"
#include <chrono>
#include <cstdio>

namespace dawnlight {
namespace {
DEFINE_HOOK(&dMeter2Draw_c::draw, ClearTimerDraw);
DEFINE_HOOK(&dMeter2_c::_delete, ClearTimerMeterDelete);
constexpr char kBlob[] = "bossrush-clear-times-v1";
timing::Records s_records;
timing::Attempt s_attempt;
SaveObserverHandle s_observer = 0;
bool s_loaded = false;

struct Digits {
    JKRSolidHeap* heap = nullptr;
    std::array<J2DPicture*, 10> pictures{};
    void release() {
        for (auto*& picture : pictures) { JKR_DELETE(picture); picture = nullptr; }
        if (heap) mDoExt_destroySolidHeap(heap);
        heap = nullptr;
    }
    bool prepare() {
        if (heap) return true;
        auto* archive = dComIfGp_getMain2DArchive();
        if (!archive) return false;
        std::array<ResTIMG*, 10> resources{};
        for (int i = 0; i < 10; ++i) {
            resources[i] = static_cast<ResTIMG*>(archive->getResource(
                'TIMG', dMeter2Info_getNumberTextureName(i)));
            if (!resources[i]) return false;
        }
        heap = mDoExt_createSolidHeapFromGame(0x18000, 0x20);
        if (!heap) return false;
        auto* previous = mDoExt_setCurrentHeap(heap);
        bool ready = true;
        for (int i = 0; i < 10; ++i) {
            pictures[i] = JKR_NEW J2DPicture(resources[i]);
            ready &= pictures[i] != nullptr;
        }
        mDoExt_setCurrentHeap(previous);
        if (!ready) { release(); return false; }
        mDoExt_adjustSolidHeap(heap);
        return true;
    }
} s_digits;

void reset_save(ModContext*, uint32_t, void*) {
    s_records = {};
    s_attempt = {};
    s_loaded = false;
}
void load_save(ModContext*, uint32_t slot, void* data) {
    reset_save(nullptr, slot, data);
    // Load lazily after all save observers and mode callbacks have run.
}
void load_records() {
    if (s_loaded || !save_state_boss_rush_active()) return;
    auto bytes = s_records.encode();
    size_t size = bytes.size();
    if (svc_save->get_blob(mod_ctx, kBlob, bytes.data(), &size) == MOD_OK)
        s_records.decode(std::span(bytes.data(), std::min(size, bytes.size())));
    s_loaded = true;
}

void format_time(uint32_t ms, char (&text)[32]) {
    if (!ms) { std::snprintf(text, sizeof(text), "--:--.--"); return; }
    // No 99-minute wraparound on long Cave runs.
    std::snprintf(text, sizeof(text), "%02u:%02u.%02u", ms / 60000,
                  (ms / 1000) % 60, (ms / 10) % 100);
}
void text_at(const char* text, float x, float y, float size) {
    auto* font = mDoExt_getMesgFont();
    if (!font) return;
    J2DPrint shadow(font, JUtility::TColor(0, 0, 0, 230), JUtility::TColor(0, 0, 0, 230));
    shadow.setFontSize(size, size);
    shadow.printReturn(text, 400, size + 8, HBIND_CENTER,
                       VBIND_TOP, x - 199, y + 1, 255);
    J2DPrint label(font, JUtility::TColor(255, 239, 190, 255), JUtility::TColor(220, 195, 140, 255));
    label.setFontSize(size, size);
    label.printReturn(text, 400, size + 8, HBIND_CENTER,
                      VBIND_TOP, x - 200, y, 255);
}
void draw_hud(uint32_t ms) {
    char text[32];
    // Zero is a valid running display; only unplayed records use dashes.
    std::snprintf(text, sizeof(text), "%02u:%02u.%02u", ms / 60000,
                  (ms / 1000) % 60, (ms / 10) % 100);
    const float center = mDoGph_gInf_c::getMinXF() + mDoGph_gInf_c::getWidthF() * 0.5f;
    const float y = mDoGph_gInf_c::getHeightF() - 58;
    text_at("Clear Time", center, y - 19, 15);
    if (!s_digits.prepare()) { text_at(text, center, y, 25); return; }
    float width = 0;
    for (char c : text) { if (!c) break; width += c >= '0' && c <= '9' ? 21 : 10; }
    float x = center - width * 0.5f;
    for (char c : text) {
        if (!c) break;
        if (c >= '0' && c <= '9') {
            // The same number textures used by dDlst_TimerScrnDraw_c/minigames.
            s_digits.pictures[c - '0']->draw(x, y, 21, 28, false, false, false);
            x += 21;
        } else {
            char separator[2]{c, 0};
            text_at(separator, x + 5, y + 1, 23);
            x += 10;
        }
    }
}
void after_draw(ModContext*, void*, void*, void*) {
    if (!clear_timer_enabled() || !save_state_boss_rush_active()) return;
    const auto context = clear_timer_context();
    bool ui = false;
    svc_ui->is_any_document_visible(mod_ctx, &ui);
    if (!context.active || ui || dComIfGp_isPauseFlag() || dComIfGp_isEnableNextStage() ||
        dComIfGp_event_runCheck()) return;
    auto* graf = dComIfGp_getCurrentGrafPort();
    if (!graf || !dComIfGd_getView()) return;
    graf->setup2D();
    load_records();
    if (s_attempt.target >= 0 && context.encounter >= 0)
        draw_hud(timing::milliseconds(s_attempt.elapsed));
    for (unsigned i = 0; i < timing::count; ++i) {
        cXyz world;
        if (!clear_timer_anchor(i, world)) continue;
        const auto& matrix = *dComIfGd_getProjViewMtx();
        const float w = matrix[3][0]*world.x + matrix[3][1]*world.y +
                        matrix[3][2]*world.z + matrix[3][3];
        if (w <= 1) continue;
        cXyz screen;
        mDoLib_project(&world, &screen);
        if (!std::isfinite(screen.x) || !std::isfinite(screen.y) ||
            screen.x < mDoGph_gInf_c::getMinXF() || screen.x > mDoGph_gInf_c::getMaxXF() ||
            screen.y < 0 || screen.y > mDoGph_gInf_c::getHeightF()) continue;
        char text[32];
        format_time(s_records.values[i], text);
        text_at(text, screen.x, screen.y, std::clamp(18000.0f / w, 12.0f, 22.0f));
    }
}
HookAction before_meter_delete(ModContext*, void*, void*, void*) {
    s_digits.release();
    // Keep run totals over scene changes, but exclude the entire load interval.
    s_attempt.wasCounting = false;
    return HOOK_CONTINUE;
}
}

void begin_clear_timer(int target) {
    s_attempt = {};
    if (!clear_timer_enabled() || !save_state_boss_rush_active()) return;
    load_records();
    s_attempt.begin(target);
}
void finish_clear_timer(int target) {
    if (!clear_timer_enabled() || !save_state_boss_rush_active()) return;
    load_records();
    if (!s_attempt.finish(target, s_records)) return;
    const auto bytes = s_records.encode();
    if (svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size()) != MOD_OK)
        svc_log->warn(mod_ctx, "Dawnlight: could not store Boss Rush clear time");
}
void cancel_clear_timer(int target) {
    if (target < 0 || s_attempt.target == target) s_attempt = {};
}
void update_clear_timer() {
    const auto context = clear_timer_context();
    if (!clear_timer_enabled() || !save_state_boss_rush_active() || context.dead) {
        cancel_clear_timer();
        return;
    }
    const double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    const bool matching = s_attempt.target == timing::run
        ? context.encounter >= 0 && context.encounter <= timing::run
        : context.encounter == s_attempt.target;
    s_attempt.tick(now, context.active && context.counting && matching, context.encounter);
}
ModResult initialize_clear_timer(ModError* error) {
    auto result = svc_save->observe_saves(mod_ctx, reset_save, load_save, nullptr, nullptr, &s_observer);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to observe clear-time saves");
    result = mods::hook_add_post<ClearTimerDraw>(svc_hook, after_draw);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to install clear-time HUD");
    result = mods::hook_add_pre<ClearTimerMeterDelete>(svc_hook, before_meter_delete);
    return result == MOD_OK ? result : mods::set_error(error, result, "failed to install clear-time HUD cleanup");
}
void shutdown_clear_timer() {
    if (s_observer) svc_save->unobserve_saves(mod_ctx, s_observer);
    s_observer = 0;
    s_digits.release();
    s_attempt = {};
    s_records = {};
    s_loaded = false;
    mods::hook_uninstall<ClearTimerDraw>(svc_hook);
    mods::hook_uninstall<ClearTimerMeterDelete>(svc_hook);
}
}
