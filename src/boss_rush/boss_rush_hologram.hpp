#pragma once

#include "mods/svc/hook.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"

ModResult init_boss_rush_holograms(const HookService* hooks);
void shutdown_boss_rush_holograms(const HookService* hooks);
void clear_boss_rush_holograms();
void register_boss_rush_hologram(J3DModel* model, u8 bossIndex);
void submit_boss_rush_hologram(J3DModel* model);
GXColor boss_rush_hologram_color(u8 bossIndex);
// Shared by the model packets and Ganondorf's separately simulated cape.
void apply_boss_rush_hologram_gx(GXColor color, GXTexCoordID coord = GX_TEXCOORD_NULL,
                                GXTexMapID texture = GX_TEXMAP_NULL);
