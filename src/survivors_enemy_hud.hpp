#pragma once
#include "mods/svc/ui.h"
#include <cstddef>
struct fopAc_ac_c;
namespace dawnlight::enemy_hud {
ModResult initialize();
ModResult build(UiElementHandle pane);
void refresh();
void observe(size_t slot,const fopAc_ac_c* actor,bool eligible);
void forget(size_t slot);
void tick();
void draw();
void reset();
void shutdown();
}
