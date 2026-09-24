#pragma once
#include <mtx.h>
struct dKy_tevstr_c;
namespace dawnlight {
// Static BMD loaded on first use: optional overlay, then bundled default.
void prepare_glider_bmd();
bool draw_glider_bmd(Mtx transform, dKy_tevstr_c* lighting);
void shutdown_glider_bmd();
}
