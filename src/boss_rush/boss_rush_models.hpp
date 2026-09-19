// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#pragma once

#include "mods/svc/log.h"
#include "SSystem/SComponent/c_xyz.h"

void reset_boss_rush_models();
void draw_boss_rush_models(float floorY);

void unload_boss_rush_models();


bool boss_rush_get_ganondorf_cape_anchors(cXyz& outA, cXyz& outB);
