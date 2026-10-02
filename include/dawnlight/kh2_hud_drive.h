#pragma once
// KH2 HUD drive API 1.0, supplied by Kite for Dawnlight integration.
// set_drive persists until clear_drive or owner unload. See docs/fierce-deity-hud-api.md.

#include <mods/api.h>

#include <stdint.h>

#ifdef __cplusplus
#include <mods/service.hpp>
#endif

#define KH2HUD_DRIVE_SERVICE_ID "com.kite.kh2hud.drive"
#define KH2HUD_DRIVE_SERVICE_MAJOR 1u
#define KH2HUD_DRIVE_SERVICE_MINOR 0u

typedef enum Kh2HudDriveType {
    KH2HUD_DRIVE_NONE = 0,   // not drawn
    KH2HUD_DRIVE_GAUGE = 1,  // the drive gauge, filling
    KH2HUD_DRIVE_FORM = 2,   // in a drive form: its time left
} Kh2HudDriveType;

typedef struct Kh2HudDriveState {
    uint32_t struct_size;   // sizeof(Kh2HudDriveState)
    uint32_t type;          // Kh2HudDriveType
    // KH2HUD_DRIVE_GAUGE
    uint32_t gauge;         // the bar filling towards the next level, 0..100
    uint32_t level;         // full bars, 0..9 - the digit
    uint32_t max_level;     // bars the gauge holds, 1..9 - level at it shows MAX
    // KH2HUD_DRIVE_FORM
    float form_time;        // time left, any unit
    float form_time_max;    // the form's whole time, same unit
    uint32_t form_div;      // bars the time is shown as, 1-9 (for KH2 the levels the form cost)
} Kh2HudDriveState;

#define KH2HUD_DRIVE_STATE_INIT {sizeof(Kh2HudDriveState), 0u, 0u, 0u, 1u, 0.0f, 0.0f, 1u}

typedef struct Kh2HudDriveService {
    ServiceHeader header;

    // Show the drive gauge with this state. MOD_INVALID_ARGUMENT for a null or short.
    ModResult (*set_drive)(ModContext* ctx, const Kh2HudDriveState* state);

    // Take it away again. Only the mod that SET it can clear it.
    void (*clear_drive)(ModContext* ctx);
} Kh2HudDriveService;

MOD_DECLARE_SERVICE(Kh2HudDriveService, svc_kh2hud_drive, KH2HUD_DRIVE_SERVICE_ID,
    KH2HUD_DRIVE_SERVICE_MAJOR, KH2HUD_DRIVE_SERVICE_MINOR);
