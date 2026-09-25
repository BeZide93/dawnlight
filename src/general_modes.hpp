#pragma once

#include <algorithm>
#include <cstdint>

namespace dawnlight {
enum class ModeSetting {
    None, Progression, Sprint, SprintSpeed, Jump, JumpHeight, FlurryRush, BulletTime,
    EnemyHardMode, BossHardMode, HealthScale, ManualShielding, GaleRecovery,
    ArrowModes, GreatSpin, NoNormalHitInvulnerability, Glide, GlideItem, Gale,
    GaleCounter, GaleCharges, FierceDeity, GaleHeight, Stamina, WolfSprint, WolfSpeed, DisableAutoJump,
};

struct ProgressionState {
    bool glide = false;
    bool gale = false;
    bool fierceDeity = false;
    int charges = 1;
};

// Max life is stored in fifths of a heart (including collected heart pieces),
// independently of the current health gauge. Damage cannot remove charges.
inline int progression_charges(int maxLife) {
    return std::clamp(maxLife / 15, 1, 12);
}

// Overrides never write the user's persisted settings. Turning either mode off
// exposes those settings again, including after an application restart.
inline bool mode_override(ModeSetting setting, bool dawnlight, bool progression,
    const ProgressionState& state, int64_t& value) {
    if (dawnlight) {
        switch (setting) {
        case ModeSetting::Stamina:
        case ModeSetting::Progression:
        case ModeSetting::Jump:
        case ModeSetting::WolfSprint:
        case ModeSetting::DisableAutoJump:
        case ModeSetting::FlurryRush:
        case ModeSetting::EnemyHardMode:
        case ModeSetting::BossHardMode:
        case ModeSetting::ManualShielding:
        case ModeSetting::ArrowModes:
        case ModeSetting::GreatSpin:
        case ModeSetting::NoNormalHitInvulnerability: value = 1; return true;
        case ModeSetting::SprintSpeed: value = 150; return true;
        case ModeSetting::WolfSpeed: value = 100; return true;
        case ModeSetting::JumpHeight: value = 110; return true;
        case ModeSetting::BulletTime: value = 2; return true; // BOTW
        case ModeSetting::HealthScale: value = 300; return true;
        case ModeSetting::GaleRecovery: value = 60; return true;
        case ModeSetting::GaleHeight: value = 500; return true;
        default: break;
        }
    }
    if (dawnlight || progression) {
        switch (setting) {
        case ModeSetting::Sprint: value = 1; return true;
        case ModeSetting::Glide: value = state.glide; return true;
        case ModeSetting::GlideItem: value = 1; return true; // Glider
        case ModeSetting::Gale:
        case ModeSetting::GaleCounter: value = state.gale; return true;
        case ModeSetting::GaleCharges: value = state.charges; return true;
        case ModeSetting::FierceDeity: value = state.fierceDeity; return true;
        default: break;
        }
    }
    return false;
}

// Loading a save or enabling progression establishes a baseline silently.
// Only milestones reached afterwards produce toasts; scene changes do not reset it.
struct ProgressionNotifications {
    bool tracking = false;
    ProgressionState previous{};
    unsigned update(bool enabled, const ProgressionState& state) {
        unsigned unlocked = 0;
        if (enabled && tracking) {
            if (state.glide && !previous.glide) unlocked |= 1;
            if (state.gale && !previous.gale) unlocked |= 2;
            if (state.fierceDeity && !previous.fierceDeity) unlocked |= 4;
            if (state.gale && previous.gale && state.charges > previous.charges) unlocked |= 8;
        }
        tracking = enabled;
        previous = state;
        return unlocked;
    }
};
} // namespace dawnlight
