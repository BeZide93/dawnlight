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
timing::Attempt s_caveAttempt;
unsigned variant_for(int target) {
    const unsigned hard = bossrush_hardmode_hazards_enabled() ? 1 : 0;
    return target == timing::cave ? hard | (cave_randomizer_enabled() ? 2 : 0) : hard;
}
JUtility::TColor timer_color(unsigned variant, bool gradient = false) {
    if (variant == 3) return gradient ? JUtility::TColor(125, 35, 195, 255) : JUtility::TColor(205, 110, 255, 255);
    if (variant) return gradient ? JUtility::TColor(190, 20, 20, 255) : JUtility::TColor(255, 65, 55, 255);
    return gradient ? JUtility::TColor(220, 195, 140, 255) : JUtility::TColor(255, 239, 190, 255);
}
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
    s_caveAttempt = {};
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
void text_at(const char* text, float x, float y, float size, unsigned variant = 0) {
    auto* font = mDoExt_getMesgFont();
    if (!font) return;
    J2DPrint shadow(font, JUtility::TColor(0, 0, 0, 230), JUtility::TColor(0, 0, 0, 230));
    shadow.setFontSize(size, size);
    shadow.printReturn(text, 400, size + 8, HBIND_CENTER,
                       VBIND_TOP, x - 199, y + 1, 255);
    J2DPrint label(font, timer_color(variant), timer_color(variant, true));
    label.setFontSize(size, size);
    label.printReturn(text, 400, size + 8, HBIND_CENTER,
                      VBIND_TOP, x - 200, y, 255);
}
void draw_hud(uint32_t ms, unsigned variant) {
    char text[32];
    // Zero is a valid running display; only unplayed records use dashes.
    std::snprintf(text, sizeof(text), "%02u:%02u.%02u", ms / 60000,
                  (ms / 1000) % 60, (ms / 10) % 100);
    const float center = mDoGph_gInf_c::getMinXF() + mDoGph_gInf_c::getWidthF() * 0.5f;
    const float y = mDoGph_gInf_c::getHeightF() - 58;
    text_at("Clear Time", center, y - 19, 15, variant);
    if (!s_digits.prepare()) { text_at(text, center, y, 25, variant); return; }
    float width = 0;
    for (char c : text) { if (!c) break; width += c >= '0' && c <= '9' ? 21 : 10; }
    float x = center - width * 0.5f;
    for (char c : text) {
        if (!c) break;
        if (c >= '0' && c <= '9') {
            // The same number textures used by dDlst_TimerScrnDraw_c/minigames.
            s_digits.pictures[c - '0']->setWhite(variant ? timer_color(variant) :
                                                       JUtility::TColor(255, 255, 255, 255));
            s_digits.pictures[c - '0']->draw(x, y, 21, 28, false, false, false);
            x += 21;
        } else {
            char separator[2]{c, 0};
            text_at(separator, x + 5, y + 1, 23, variant);
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
        draw_hud(timing::milliseconds(s_attempt.elapsed), s_attempt.variant);
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
        const unsigned variant = variant_for(i);
        format_time(s_records.values[i + variant * timing::count], text);
        text_at(text, screen.x, screen.y, std::clamp(18000.0f / w, 12.0f, 22.0f), variant);
    }
}
HookAction before_meter_delete(ModContext*, void*, void*, void*) {
    s_digits.release();
    // Keep run totals over scene changes, but exclude the entire load interval.
    s_attempt.wasCounting = false;
    s_caveAttempt.wasCounting = false;
    return HOOK_CONTINUE;
}
}

bool clear_timer_near(const cXyz& interactionPosition) {
    auto* player = dComIfGp_getPlayer(0);
    // Measure from the approach point on the ground, not the floating text.
    // Show the record shortly before the 150-unit portal interaction radius.
    return player && player->current.pos.absXZ(interactionPosition) <= 350.0f &&
        std::abs(player->current.pos.y - interactionPosition.y) <= 200.0f;
}

void begin_clear_timer(int target) {
    s_caveAttempt = {};
    s_attempt = {};
    if (!clear_timer_enabled() || !save_state_boss_rush_active()) return;
    load_records();
    s_attempt.begin(target, variant_for(target));
}
void begin_cave_boss_timer(int boss) {
    const auto cave = s_attempt;
    begin_clear_timer(boss);
    if (cave.target == timing::cave && !cave.completed) s_caveAttempt = cave;
    s_caveAttempt.wasCounting = false;
}
void end_cave_boss_timer() {
    s_attempt = s_caveAttempt;
    s_attempt.wasCounting = false;
    s_caveAttempt = {};
}
void finish_clear_timer(int target) {
    if (!clear_timer_enabled() || !save_state_boss_rush_active()) return;
    load_records();
    if (s_attempt.variant != variant_for(s_attempt.target)) { cancel_clear_timer(); return; }
    if (!s_attempt.finish(target, s_records)) return;
    const auto bytes = s_records.encode();
    if (svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size()) != MOD_OK)
        svc_log->warn(mod_ctx, "Dawnlight: could not store Boss Rush clear time");
}
void cancel_clear_timer(int target) {
    if (target < 0 || s_attempt.target == target) s_attempt = {};
    if (target < 0 || s_caveAttempt.target == target) s_caveAttempt = {};
}
void update_clear_timer() {
    if (!clear_timer_enabled() || !save_state_boss_rush_active()) {
        cancel_clear_timer();
        return;
    }
    const auto context = clear_timer_context();
    if (context.dead || (s_attempt.target >= 0 && s_attempt.variant != variant_for(s_attempt.target)) ||
        (s_caveAttempt.target >= 0 && s_caveAttempt.variant != variant_for(timing::cave))) {
        cancel_clear_timer();
        return;
    }
    const double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    const bool matching = s_attempt.target == timing::run
        ? context.encounter >= 0 && context.encounter <= timing::run
        : context.encounter == s_attempt.target;
    s_attempt.tick(now, context.active && context.counting && matching, context.encounter);
    // An optional fairy-room boss remains part of the enclosing Cave run.
    s_caveAttempt.tick(now, context.active && context.counting, context.encounter);
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
    s_caveAttempt = {};
    s_records = {};
    s_loaded = false;
    mods::hook_uninstall<ClearTimerDraw>(svc_hook);
    mods::hook_uninstall<ClearTimerMeterDelete>(svc_hook);
}
}
