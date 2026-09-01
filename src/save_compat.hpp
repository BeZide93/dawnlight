#pragma once

#include "mods/api.h"

namespace dawnlight {

bool is_new_game_plus();
bool is_intro_skipped();
unsigned new_game_plus_count();
ModResult install_save_compat_hooks(ModError* error);

}  // namespace dawnlight
