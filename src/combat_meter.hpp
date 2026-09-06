#pragma once

class dMeter2Draw_c;

namespace dawnlight {

enum class CombatMeterStyle {
    Stamina,
    FierceDeity,
};

void draw_combat_meter(
    dMeter2Draw_c* meter, float percentage, CombatMeterStyle style, int row);

}  // namespace dawnlight
