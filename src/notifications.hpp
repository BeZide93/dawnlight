#pragma once

#include "config.hpp"
#include "mods/svc/ui.h"

namespace dawnlight {
// All Dawnlight toasts share this gate. Do not gate the action producing a
// notification: progression, resource changes and settings still take effect.
inline void push_dawnlight_toast(ModContext* ctx, const UiService* ui, const UiToastDesc& toast) {
    if (notifications_enabled() && ui && ui->push_toast) ui->push_toast(ctx, &toast);
}
} // namespace dawnlight
