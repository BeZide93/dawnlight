#include "kh2_hud_compat.hpp"
#include "fierce_deity_hud.hpp"
#include "dawnlight/kh2_hud_drive.h"

#include <algorithm>
#include <cmath>

namespace dawnlight {
namespace {
bool s_reported = false;
}

void clear_kh2_drive() {
    if (s_reported && svc_kh2hud_drive && svc_kh2hud_drive->clear_drive)
        svc_kh2hud_drive->clear_drive(mod_ctx);
    s_reported = false;
}

bool update_kh2_drive() {
    const auto* service = svc_kh2hud_drive;
    if (!service || !service->set_drive || !service->clear_drive) {
        clear_kh2_drive();
        return false;
    }
    // An explicitly registered replacement owns the bar. Do not also leave a
    // persistent KH2 gauge behind when a consumer of our public API takes over.
    const auto state = fierce_deity_hud_state();
    if (fierce_deity_hud_renderer_registered() || !state.enabled || !state.visible ||
        !std::isfinite(state.percentage)) {
        clear_kh2_drive();
        return false;
    }
    Kh2HudDriveState drive = KH2HUD_DRIVE_STATE_INIT;
    const float charge = std::clamp(state.percentage, 0.0f, 100.0f);
    if (state.active) {
        drive.type = KH2HUD_DRIVE_FORM;
        drive.form_time = charge;
        drive.form_time_max = 100.0f;
        drive.form_div = 1;
    } else {
        drive.type = KH2HUD_DRIVE_GAUGE;
        drive.max_level = 1;
        drive.level = charge >= 100.0f ? 1u : 0u;
        drive.gauge = drive.level ? 0u : static_cast<uint32_t>(charge);
    }
    if (service->set_drive(mod_ctx, &drive) != MOD_OK) {
        clear_kh2_drive();
        return false;
    }
    s_reported = true;
    return true;
}
}
