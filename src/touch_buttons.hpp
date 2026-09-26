#pragma once

#include "mods/api.h"
#include <string>

namespace dawnlight {
ModResult register_touch_button_config(ModError* error);
ModResult install_touch_button_hooks(ModError* error);
void shutdown_touch_buttons();
void open_touch_buttons(ModContext* ctx, void*);
bool midna_touch_button_available();
std::string midna_touch_button_icon();
bool consume_midna_touch_press();
}  // namespace dawnlight
