#pragma once
#include <mtx.h>
struct dKy_tevstr_c;
namespace dawnlight {
// Optional static BMD from a data-only .dusk overlay; loaded on first use.
void prepare_glider_bmd();
bool draw_glider_bmd(Mtx transform, dKy_tevstr_c* lighting);
void shutdown_glider_bmd();
}
