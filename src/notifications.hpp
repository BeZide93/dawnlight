#pragma once

#include "config.hpp"
#include "mods/svc/ui.h"

namespace dawnlight {
// Gate only Progression System announcements, never the unlock itself.
// Other Dawnlight notifications use the native UI service independently.
inline void push_progression_toast(ModContext* ctx, const UiService* ui, const UiToastDesc& toast) {
    if (notifications_enabled() && ui && ui->push_toast) ui->push_toast(ctx, &toast);
}
} // namespace dawnlight
