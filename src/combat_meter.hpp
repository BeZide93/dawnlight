#pragma once

class dMeter2Draw_c;

namespace dawnlight {

enum class CombatMeterStyle {
    Stamina,
    StaminaExhausted,
    FierceDeity,
};

// Shared scene/menu visibility, independent of whether Stamina Bar is enabled.
bool combat_meter_hud_visible();
// Alpha inherited by the bars from their containing HUD screen/panes.
float combat_meter_screen_alpha(dMeter2Draw_c* meter);

void draw_combat_meter(
    dMeter2Draw_c* meter, float percentage, CombatMeterStyle style, int row);

// Anchor below the Fierce Deity row, including its HUD transform, even when empty.
bool combat_meter_next_row_anchor(dMeter2Draw_c* meter, int row, float& x, float& y, float& scale);

}  // namespace dawnlight
