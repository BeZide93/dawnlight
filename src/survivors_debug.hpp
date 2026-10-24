#pragma once
#include <algorithm>
#include <cstdint>

// Development switch: set to 0 before merging a development PR or releasing.
// Disabled builds hide the tab and forcibly neutralize every debug setting.
#ifndef DAWNLIGHT_SURVIVORS_DEBUG
#define DAWNLIGHT_SURVIVORS_DEBUG 1
#endif
namespace dawnlight::survivors {
inline constexpr bool debug_enabled = DAWNLIGHT_SURVIVORS_DEBUG != 0;
// Process-local only: never register with ConfigService or serialize to a save.
inline bool debug_session_enabled = false;
struct DebugOptions {
    unsigned minimum_rarity=0, player_damage=1, infinite_rerolls=0, infinite_banishes=0, run_minutes=20;
    bool unlock_all_maps=false;
    unsigned max_equipment_drop_rate=0, always_legendary_equipment=0;
    void clamp() {
        if (!debug_enabled) { *this={}; return; }
        minimum_rarity=std::min(4u,minimum_rarity);
        player_damage=std::clamp(player_damage,1u,10u);
        infinite_rerolls=std::min(1u,infinite_rerolls);infinite_banishes=std::min(1u,infinite_banishes);
        max_equipment_drop_rate=std::min(1u,max_equipment_drop_rate);
        always_legendary_equipment=std::min(1u,always_legendary_equipment);
        if (run_minutes!=5 && run_minutes!=10) run_minutes=20;
    }
};
}
