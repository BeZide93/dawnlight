#include "hud_fade.hpp"

#include "combat_meter.hpp"
#include "fierce_deity_hud.hpp"
#include "config.hpp"
#include "hud_fade_state.hpp"
#include "service_imports.hpp"

#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter_button.h"
#include "d/d_meter_map.h"
#include "mods/service.hpp"
#include "mods/svc/hook.hpp"

#include <chrono>
#include <vector>

namespace dawnlight {
namespace {
DEFINE_HOOK(&dMeter2Draw_c::draw, FadeMeterDraw);
DEFINE_HOOK(&dMeterMap_c::draw, FadeMapDraw);
DEFINE_HOOK(&dMeterButton_c::draw, FadeButtonDraw);
DEFINE_HOOK(&dMeter2_c::_delete, FadeMeterDelete);
DEFINE_HOOK(&J2DScreen::draw, FadeScreenDraw);
DEFINE_HOOK(static_cast<void (J2DPicture::*)(f32, f32, f32, f32, bool, bool, bool)>(
    &J2DPicture::draw), FadePictureDraw);

HudIdleFade s_idle;
HudFadeEnvelope s_stamina, s_fierceDeity;
daAlink_c* s_player = nullptr;
cXyz s_position;
s16 s_angle = 0;
float s_hudAlpha = 1.0f;
float s_barAlpha = 1.0f;
unsigned s_hudDepth = 0;
std::vector<void*> s_hudCalls;

struct PaneAlpha {
    J2DPane* pane;
    u8 alpha;
    u8 colorAlpha;
    bool changed;
};
using AlphaFrame = std::vector<PaneAlpha>;
struct DrawAlphaFrame {
    void* call;
    AlphaFrame panes;
};
std::vector<DrawAlphaFrame> s_screens;
std::vector<DrawAlphaFrame> s_pictures;

double seconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void update_idle() {
    auto* link = daAlink_getAlinkActorClass();
    const bool enabled = hud_auto_fade_enabled() && link && combat_meter_hud_visible() &&
                         !dComIfGp_isEnableNextStage();
    bool idle = false;
    if (enabled && link == s_player) {
        const cXyz delta = link->current.pos - s_position;
        const bool moved = delta.abs2() > 0.01f || link->current.angle.y != s_angle;
        // Automatic idle gestures and low-health breathing use separate procs.
        // Keep the idle timer through those transitions, but still reveal the
        // HUD for movement, attacks, aiming, damage and other active procs.
        idle = !moved && (link->mProcID == daAlink_c::PROC_WAIT ||
                         link->mProcID == daAlink_c::PROC_SERVICE_WAIT ||
                         link->mProcID == daAlink_c::PROC_TIRED_WAIT ||
                         link->mProcID == daAlink_c::PROC_WOLF_WAIT ||
                         link->mProcID == daAlink_c::PROC_WOLF_SERVICE_WAIT ||
                         link->mProcID == daAlink_c::PROC_WOLF_TIRED_WAIT);
        if (moved) s_position = link->current.pos;
    } else if (link) {
        s_position = link->current.pos;
    }
    s_player = link;
    if (link) s_angle = link->current.angle.y;
    s_hudAlpha = s_idle.update(seconds(), enabled, idle);
}

// Scale only the start of each alpha-inheritance chain. Scaling every pane
// would multiply the fade repeatedly and make nested HUD artwork vanish first.
void fade_tree(J2DPane* pane, float alpha, AlphaFrame& saved, bool root = true) {
    if (!pane) return;
    const bool change = root || !pane->isInfluencedAlpha();
    saved.push_back({pane, pane->getAlpha(), pane->mColorAlpha, change});
    if (change) pane->setAlpha(static_cast<u8>(pane->getAlpha() * alpha));
    for (auto* child = pane->getFirstChildPane(); child; child = child->getNextChildPane())
        fade_tree(child, alpha, saved, false);
}

void restore_alpha(std::vector<DrawAlphaFrame>& stack, void* call) {
    // Another mod can skip a draw before our late pre-hook runs. Its post-hook
    // still runs; do not accidentally restore/pop an enclosing draw in that case.
    if (stack.empty() || stack.back().call != call) return;
    for (auto& state : stack.back().panes) {
        if (state.changed) state.pane->setAlpha(state.alpha);
        state.pane->mColorAlpha = state.colorAlpha;
    }
    stack.pop_back();
}

HookAction before_hud(ModContext*, void* args, void*, void*) {
    s_hudCalls.push_back(args);
    if (s_hudDepth++ == 0) update_idle();
    return HOOK_CONTINUE;
}
void after_hud(ModContext*, void* args, void*, void*) {
    if (s_hudCalls.empty() || s_hudCalls.back() != args) return;
    s_hudCalls.pop_back();
    if (s_hudDepth) --s_hudDepth;
}

HookAction before_screen(ModContext*, void* args, void*, void*) {
    s_screens.push_back({args, {}});
    const float alpha = (s_hudDepth ? s_hudAlpha : 1.0f) * s_barAlpha;
    if (alpha < 1.0f)
        fade_tree(mods::arg<J2DScreen*>(args, 0), alpha, s_screens.back().panes);
    return HOOK_CONTINUE;
}
void after_screen(ModContext*, void* args, void*, void*) { restore_alpha(s_screens, args); }

HookAction before_picture(ModContext*, void* args, void*, void*) {
    s_pictures.push_back({args, {}});
    // Minimap and ammunition numbers use standalone pictures, not J2DScreens.
    if (s_hudDepth && s_screens.empty() && s_hudAlpha < 1.0f) {
        auto* pane = mods::arg<J2DPicture*>(args, 0);
        if (pane) {
            s_pictures.back().panes.push_back({pane, pane->getAlpha(), pane->mColorAlpha, true});
            pane->setAlpha(static_cast<u8>(pane->getAlpha() * s_hudAlpha));
        }
    }
    return HOOK_CONTINUE;
}
void after_picture(ModContext*, void* args, void*, void*) { restore_alpha(s_pictures, args); }

HookAction before_meter_delete(ModContext*, void*, void*, void*) {
    shutdown_hud_fade();
    return HOOK_CONTINUE;
}
} // namespace

void draw_combat_meter_screen(J2DScreen* screen, J2DGrafContext* graf, bool stamina, float percentage,
    const DawnlightFierceDeityHudFrame* replacement) {
    const bool enabled = stamina ?
        hud_custom_stamina_fade_when_full() :
        hud_custom_fierce_deity_fade_when_empty();
    auto& fade = stamina ? s_stamina : s_fierceDeity;
    const float previous = s_barAlpha;
    s_barAlpha = fade.update(seconds(), stamina ? percentage >= 100.0f : percentage <= 0.0f, enabled);
    bool handled = false;
    if (!stamina && replacement) {
        auto frame = *replacement;
        frame.alpha *= s_barAlpha * (s_hudDepth ? s_hudAlpha : 1.0f);
        // The consumer receives final opacity. Suspend our automatic hooks to
        // avoid fading its own J2D screens/pictures a second time.
        const unsigned depth = s_hudDepth;
        const float barAlpha = s_barAlpha;
        s_hudDepth = 0;
        s_barAlpha = 1.0f;
        handled = draw_external_fierce_deity_hud(frame);
        s_hudDepth = depth;
        s_barAlpha = barAlpha;
    }
    if (!handled) screen->draw(0.0f, 0.0f, graf);
    s_barAlpha = previous;
}

ModResult initialize_hud_fade(ModError* error) {
    HookOptions first = HOOK_OPTIONS_INIT;
    HookOptions last = HOOK_OPTIONS_INIT;
    first.priority = 10000;
    last.priority = -10000;
    // Keep the HUD scope alive through other mods' draw callbacks. Apply alpha
    // after their layout/alpha changes, then restore it before their post hooks.
    ModResult result = mods::hook::add_pre<FadeMeterDraw>(svc_hook, before_hud, &first);
    if (result == MOD_OK) result = mods::hook::add_post<FadeMeterDraw>(svc_hook, after_hud, &last);
    if (result == MOD_OK) result = mods::hook::add_pre<FadeMapDraw>(svc_hook, before_hud, &first);
    if (result == MOD_OK) result = mods::hook::add_post<FadeMapDraw>(svc_hook, after_hud, &last);
    if (result == MOD_OK) result = mods::hook::add_pre<FadeButtonDraw>(svc_hook, before_hud, &first);
    if (result == MOD_OK) result = mods::hook::add_post<FadeButtonDraw>(svc_hook, after_hud, &last);
    if (result == MOD_OK) result = mods::hook::add_pre<FadeScreenDraw>(svc_hook, before_screen, &last);
    if (result == MOD_OK) result = mods::hook::add_post<FadeScreenDraw>(svc_hook, after_screen, &first);
    if (result == MOD_OK) result = mods::hook::add_pre<FadePictureDraw>(svc_hook, before_picture, &last);
    if (result == MOD_OK) result = mods::hook::add_post<FadePictureDraw>(svc_hook, after_picture, &first);
    if (result == MOD_OK) result = mods::hook::add_pre<FadeMeterDelete>(svc_hook, before_meter_delete);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result, "failed to install HUD fade hooks");
}

void shutdown_hud_fade() {
    s_idle = {};
    s_stamina = {};
    s_fierceDeity = {};
    s_player = nullptr;
    s_hudAlpha = s_barAlpha = 1.0f;
    s_hudDepth = 0;
    s_hudCalls.clear();
    s_screens.clear();
    s_pictures.clear();
}
} // namespace dawnlight
