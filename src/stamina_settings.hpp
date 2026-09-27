#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace dawnlight {
enum class StaminaSetting {
    Amount, Recovery, ExhaustThreshold, ExhaustRecovery, Sprint, WolfSprint, Glide, BulletTime, FlurryRush, Block, GuardBreak, GreatSpin, ShieldAttack, BackSlice, HelmSplitter, MidnaAttack, Count
};
struct StaminaSettingDesc {
    const char* key;
    const char* label;
    int min, max, standard;
    const char* suffix;
};
inline constexpr std::array<StaminaSettingDesc, static_cast<size_t>(StaminaSetting::Count)> kStaminaSettings{{
    {"stamina-amount", "Stamina Amount", 50, 500, 100, ""},
    {"stamina-recovery", "Recovery speed", 1, 100, 5, " /sec"},
    {"stamina-exhaust-threshold", "Exhaust Threshold", 0, 100, 50, ""},
    {"stamina-exhaust-recovery", "Exhaust Recovery speed", 1, 100, 5, " /sec"},
    {"stamina-sprint", "Sprint", 0, 20, 5, " /sec"},
    {"stamina-wolf-sprint", "Wolf Sprint", 0, 20, 5, " /sec"},
    {"stamina-glide", "Glide", 0, 20, 5, " /sec"},
    {"stamina-bullet-time", "Bullet Time", 0, 50, 15, " /sec"},
    {"stamina-flurry-rush", "Flurry Rush", 0, 100, 50, ""},
    {"stamina-block", "Block", 0, 100, 10, ""},
    {"stamina-guard-break", "Guard Break", 0, 100, 60, ""},
    {"stamina-great-spin", "Great Spin Projectile", 0, 100, 40, ""},
    {"stamina-shield-attack", "Shield Attack", 0, 50, 20, ""},
    {"stamina-back-slice", "Back Slice", 0, 50, 20, ""},
    {"stamina-helm-splitter", "Helm Splitter", 0, 50, 20, ""},
    {"stamina-midna-attack", "Midna Attack", 0, 100, 50, ""},
}};

// Costs, recovery rates and the exhaustion threshold use stamina points.
// Five max-life units form a heart; incomplete heart pieces add no capacity.
inline int stamina_capacity(int base, bool progression, int maxLife) {
    return base + (progression ? std::max(0, maxLife / 5 - 3) * 10 : 0);
}
} // namespace dawnlight
