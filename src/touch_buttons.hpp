#pragma once

#include "mods/api.h"

namespace dawnlight {
ModResult register_touch_button_config(ModError* error);
ModResult install_touch_button_hooks(ModError* error);
void shutdown_touch_buttons();
void open_touch_buttons(ModContext* ctx, void*);
}  // namespace dawnlight
