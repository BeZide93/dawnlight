#include "../src/general_modes.hpp"
#include "../src/gale_charges.hpp"
#include <array>
#include <cassert>
using namespace dawnlight;

int main() {
    // Stored heart pieces are fifths of a heart, not health-gauge quarters.
    for (int life = 15; life < 30; ++life) assert(progression_charges(life) == 1);
    assert(progression_charges(30) == 2);
    assert(progression_charges(44) == 2);
    assert(progression_charges(45) == 3);
    assert(progression_charges(100) == 6);
    assert(progression_charges(0) == 1);

    constexpr std::array settings{ModeSetting::Sprint, ModeSetting::SprintSpeed,
        ModeSetting::Jump, ModeSetting::JumpHeight, ModeSetting::FlurryRush,
        ModeSetting::BulletTime, ModeSetting::EnemyHardMode, ModeSetting::BossHardMode,
        ModeSetting::HealthScale, ModeSetting::ManualShielding, ModeSetting::GaleRecovery,
        ModeSetting::ArrowModes, ModeSetting::GreatSpin, ModeSetting::NoNormalHitInvulnerability,
        ModeSetting::Progression, ModeSetting::Glide, ModeSetting::GlideItem,
        ModeSetting::Gale, ModeSetting::GaleCounter, ModeSetting::GaleCharges,
        ModeSetting::FierceDeity, ModeSetting::GaleHeight, ModeSetting::Stamina,
        ModeSetting::WolfSprint, ModeSetting::WolfSpeed, ModeSetting::DisableAutoJump};
    constexpr std::array<int64_t, settings.size()> intended{
        1, 150, 1, 110, 1, 2, 1, 1, 300, 1, 60, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 500, 1, 1, 100, 1};
    for (size_t i = 0; i < settings.size(); ++i) {
        int64_t value = 777;
        assert(!mode_override(settings[i], false, false, {}, value));
        assert(value == 777);
        assert(mode_override(settings[i], true, false, {}, value));
        assert(value == intended[i]);
        value = 777;
        assert(!mode_override(settings[i], false, false, {}, value));
        assert(value == 777); // Off exposes the user's value, unchanged.
    }
    const ProgressionState completed{true, true, true, 6};
    for (const auto setting : {ModeSetting::Glide, ModeSetting::Gale,
            ModeSetting::GaleCounter, ModeSetting::FierceDeity}) {
        int64_t value = 0;
        assert(mode_override(setting, false, true, completed, value) && value == 1);
        assert(mode_override(setting, true, false, completed, value) && value == 1);
        assert(mode_override(setting, true, false, {}, value) && value == 0);
    }
    int64_t value = 123;
    assert(!mode_override(ModeSetting::SprintSpeed, false, true, {}, value));
    assert(!mode_override(ModeSetting::JumpHeight, false, true, {}, value));
    assert(!mode_override(ModeSetting::GaleHeight, false, true, {}, value));
    assert(!mode_override(ModeSetting::Stamina, false, true, {}, value));
    for (const auto setting : {ModeSetting::WolfSprint, ModeSetting::WolfSpeed,
            ModeSetting::DisableAutoJump}) {
        assert(!mode_override(setting, false, true, {}, value));
    }
    assert(!mode_override(ModeSetting::None, true, true, {}, value));
    assert(mode_override(ModeSetting::GaleCharges, false, true, completed, value) && value == 6);

    ProgressionNotifications notifications;
    ProgressionState state;
    assert(notifications.update(true, state) == 0);
    state.glide = true;
    assert(notifications.update(true, state) == 1);
    assert(notifications.update(true, state) == 0); // Repeated frames/scene transitions.
    state.gale = true;
    assert(notifications.update(true, state) == 2);
    state.charges = 2;
    assert(notifications.update(true, state) == 8);
    state.fierceDeity = true;
    assert(notifications.update(true, state) == 4);
    notifications = {}; // Load a completed save: no retroactive toast spam.
    assert(notifications.update(true, completed) == 0);
    notifications = {}; // Load a new save: no state inherited.
    assert(notifications.update(true, {}) == 0);
    assert(notifications.update(false, completed) == 0);
    assert(notifications.update(true, completed) == 0); // Re-enable silently.

    GaleCharges charges;
    charges.update(0, 1, 60);
    assert(charges.consume());
    charges.update(30, 2, 60); // Heart-container milestone adds a usable charge.
    assert(charges.available() == 1 && charges.elapsed == 30);
    charges.update(60, 2, 60);
    assert(charges.available() == 2);
}
