#pragma once

#include <mods/api.h>
#ifdef __cplusplus
#include <mods/service.hpp>
#endif

#define DAWNLIGHT_FIERCE_DEITY_HUD_SERVICE_ID "dev.bezide.dawnlight.fierce_deity_hud"
#define DAWNLIGHT_FIERCE_DEITY_HUD_SERVICE_MAJOR 1u
#define DAWNLIGHT_FIERCE_DEITY_HUD_SERVICE_MINOR 0u

/* All calls and callbacks run on the game thread. Fixed-width flags are 0/1. */
typedef struct DawnlightFierceDeityHudState {
    uint32_t struct_size;
    uint32_t enabled;
    /* Logical visibility (player/menu gates), before native HUD alpha and fades. */
    uint32_t visible;
    /* Transformation active, independent of the charging bar's visibility. */
    uint32_t active;
    float percentage; /* 0..100; zero without a current player */
} DawnlightFierceDeityHudState;

#define DAWNLIGHT_FIERCE_DEITY_HUD_STATE_INIT {sizeof(DawnlightFierceDeityHudState), 0, 0, 0, 0.0f}

typedef struct DawnlightFierceDeityHudFrame {
    uint32_t struct_size;
    DawnlightFierceDeityHudState state;
    /* Full bar bounds in the current J2D HUD coordinate system, not framebuffer pixels. */
    float x, y, width, height;
    float scale_x, scale_y;
    /* Final opacity, including native ancestors, HUD idle fade and fade-when-empty. */
    float alpha;
    /* Borrowed J2DGrafContext*, valid only during this callback (requires game SDK). */
    void* graf_context;
} DawnlightFierceDeityHudFrame;

/*
 * Draw synchronously here; the frame and its pointers must not be retained.
 * Return true ONLY after drawing a replacement. False draws Dawnlight's bar.
 * Apply frame->alpha exactly once; Dawnlight's own fade hooks are suspended for
 * this callback. Do not invoke the game's HUD draw recursively, throw exceptions,
 * or change gameplay. No callback occurs when the bar is logically hidden.
 */
typedef bool (*DawnlightFierceDeityHudDrawFn)(ModContext* ctx,
    const DawnlightFierceDeityHudFrame* frame, void* user_data);

typedef struct DawnlightFierceDeityHudService {
    ServiceHeader header;
    /* Initialize out_state with DAWNLIGHT_FIERCE_DEITY_HUD_STATE_INIT first. */
    ModResult (*get_state)(ModContext* ctx, DawnlightFierceDeityHudState* out_state);
    /* One owner. Another mod gets MOD_CONFLICT; the same owner may replace its callback. */
    ModResult (*register_renderer)(ModContext* ctx, DawnlightFierceDeityHudDrawFn draw, void* user_data);
    /* Only the owner may remove it. Also removed automatically when the owner detaches. */
    ModResult (*unregister_renderer)(ModContext* ctx);
} DawnlightFierceDeityHudService;

MOD_DECLARE_SERVICE(DawnlightFierceDeityHudService, svc_dawnlight_fierce_hud,
    DAWNLIGHT_FIERCE_DEITY_HUD_SERVICE_ID, DAWNLIGHT_FIERCE_DEITY_HUD_SERVICE_MAJOR,
    DAWNLIGHT_FIERCE_DEITY_HUD_SERVICE_MINOR);
