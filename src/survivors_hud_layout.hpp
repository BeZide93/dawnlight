#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace dawnlight::survivors {
enum class HudPart : unsigned { Timer, Rank, ExpBar, Info, Level, Score, ExpText, BossName, BossBar, ItemBanner, EnemyCounter, Count };
struct HudPlacement { float x, y, scale; };
constexpr std::array<const char*,unsigned(HudPart::Count)> hud_part_names{
    "Timer", "Twilit Rank", "EXP-Bar", "Info-Texts", "Level-Text", "Score text", "EXP-text", "Boss-Name", "Boss-Bar", "Item Banner", "Enemy Counter"};
constexpr std::array<const char*,unsigned(HudPart::Count)> hud_part_keys{
    "timer", "rank", "exp-bar", "info", "level", "score", "exp-text", "boss-name", "boss-bar", "item-banner", "enemy-counter"};
// X is relative to the viewport center; Y is relative to its top, in native HUD units.
constexpr std::array<std::array<int64_t,3>,unsigned(HudPart::Count)> hud_defaults{{
    {-35,40,100}, {120,40,100}, {0,82,100}, {0,96,100}, {-125,62,100},
    {-20,62,100}, {110,62,100}, {0,94,100}, {0,120,100}, {156,224,100}, {0,18,100}
}};
inline int64_t clamp_hud_value(unsigned axis,int64_t value) {
    return axis==2 ? std::clamp<int64_t>(value,10,400) : std::clamp<int64_t>(value,-9999,9999);
}
inline HudPlacement hud_placement(float center,std::array<int64_t,3> values) {
    return {center+float(clamp_hud_value(0,values[0])),float(clamp_hud_value(1,values[1])),
        float(clamp_hud_value(2,values[2]))/100.f};
}
} // namespace dawnlight::survivors
