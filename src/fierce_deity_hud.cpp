#include "fierce_deity_hud.hpp"
#include "mods/svc/host.h"

namespace dawnlight {
namespace {
struct Renderer {
    ModContext* owner = nullptr;
    DawnlightFierceDeityHudDrawFn draw = nullptr;
    void* user = nullptr;
};
Renderer s_renderer;
uint64_t s_lifecycle = 0;
bool s_ready = false;
bool s_drawing = false;

ModResult get_state(ModContext* ctx, DawnlightFierceDeityHudState* out) {
    if (!ctx || !out || out->struct_size < sizeof(*out)) return MOD_INVALID_ARGUMENT;
    if (!s_ready) { *out = DAWNLIGHT_FIERCE_DEITY_HUD_STATE_INIT; return MOD_UNAVAILABLE; }
    *out = fierce_deity_hud_state();
    return MOD_OK;
}
ModResult register_renderer(ModContext* ctx, DawnlightFierceDeityHudDrawFn draw, void* user) {
    if (!ctx || !draw) return MOD_INVALID_ARGUMENT;
    if (!s_ready) return MOD_UNAVAILABLE;
    if (s_renderer.owner && s_renderer.owner != ctx) return MOD_CONFLICT;
    s_renderer = {ctx, draw, user};
    return MOD_OK;
}
ModResult unregister_renderer(ModContext* ctx) {
    if (!ctx) return MOD_INVALID_ARGUMENT;
    if (!s_ready) return MOD_UNAVAILABLE;
    if (s_renderer.owner && s_renderer.owner != ctx) return MOD_CONFLICT;
    s_renderer = {};
    return MOD_OK;
}
void on_lifecycle(ModContext*, ModContext* subject, const char*, ModLifecycleEvent event, void*) {
    if (event == MOD_LIFECYCLE_DETACHED && s_renderer.owner == subject) s_renderer = {};
}
constexpr DawnlightFierceDeityHudService s_service{
    SERVICE_HEADER(DawnlightFierceDeityHudService, 1, 0),
    get_state, register_renderer, unregister_renderer,
};
EXPORT_SERVICE(s_service);
}

ModResult initialize_fierce_deity_hud(ModError* error) {
    if (s_ready) return MOD_OK;
    const auto result = svc_host->watch_mod_lifecycle(mod_ctx, on_lifecycle, nullptr, &s_lifecycle);
    if (result != MOD_OK)
        return mods::set_error(error, result, "failed to watch Fierce Deity HUD renderer lifecycle");
    s_ready = true;
    return MOD_OK;
}
void shutdown_fierce_deity_hud() {
    s_ready = false;
    s_renderer = {};
    if (s_lifecycle && svc_host) svc_host->unwatch_mod_lifecycle(mod_ctx, s_lifecycle);
    s_lifecycle = 0;
}
bool fierce_deity_hud_renderer_registered() { return s_ready && s_renderer.draw; }

bool draw_external_fierce_deity_hud(const DawnlightFierceDeityHudFrame& frame) {
    if (!s_ready || !s_renderer.draw || s_drawing || !frame.state.visible) return false;
    const auto renderer = s_renderer; // callback may unregister/replace itself
    s_drawing = true;
    const bool handled = renderer.draw(renderer.owner, &frame, renderer.user);
    s_drawing = false;
    return handled;
}
}
