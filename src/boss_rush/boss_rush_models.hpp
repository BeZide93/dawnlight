// Adapted from F1mmel/dusklight-twilit-essentials, commit 1e7fb0afb828f924c165e8c57b7337304ed6cf4c.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#pragma once

#include "mods/svc/log.h"
#include "SSystem/SComponent/c_xyz.h"

void reset_boss_rush_models();
void draw_boss_rush_models(float floorY);

void unload_boss_rush_models();


bool boss_rush_get_ganondorf_cape_anchors(cXyz& outA, cXyz& outB);
