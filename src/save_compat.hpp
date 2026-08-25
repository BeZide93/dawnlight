#pragma once

#include "global.h"
#include "helpers/string.hpp"
#include "d/d_save.h"
#include "mods/api.h"

namespace dawnlight {

bool is_new_game_plus(const dSv_save_c* save);
bool is_intro_skipped(const dSv_save_c* save);
unsigned new_game_plus_count(const dSv_save_c* save);
ModResult install_save_compat_hooks(ModError* error);

}  // namespace dawnlight
