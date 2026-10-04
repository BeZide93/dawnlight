#pragma once
#include <array>
#include <cstddef>

namespace dawnlight {
enum class DarkLinkSetting { Gauge, SwordGain, DamageGain, Depletion, DamageMultiplier, Count };
struct DarkLinkSettingDesc {
    const char* key;
    const char* label;
    int min, max, standard;
    const char* suffix;
    const char* help;
};
inline constexpr std::array<DarkLinkSettingDesc, static_cast<size_t>(DarkLinkSetting::Count)> kDarkLinkSettings{{
    {"dark-link-gauge", "Dark Link Gauge", 1, 10000, 100, " points",
        "Points required for a full gauge. Increasing capacity keeps your current points."},
    {"dark-link-sword-gain", "Sword Attack Gain", 0, 1000, 5, " points",
        "Points gained per damaging sword hit while Dark Link is inactive."},
    {"dark-link-damage-gain", "Damage Received Gain", 0, 1000, 0, " points",
        "Points gained per health-damage event while human and Dark Link is inactive. Blocked hits and armor-only costs do not count."},
    {"dark-link-depletion", "Depletion Rate", 0, 1000, 5, " /sec",
        "Gauge points consumed per second while active. Zero disables depletion. Pauses in menus."},
    {"dark-link-damage-multiplier", "Damage Multiplier", 0, 1000, 200, "%",
        "Sword damage while Dark Link is active. 100% is normal damage; 200% is double. Fractional damage rounds up."},
}};
} // namespace dawnlight
