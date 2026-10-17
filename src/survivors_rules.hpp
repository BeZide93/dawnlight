#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace dawnlight::survivors {
// All durations are simulation ticks (TP runs its gameplay at 30 Hz).
inline constexpr unsigned ticks_per_second = 30;
inline constexpr unsigned duration = 10 * 60 * ticks_per_second;
inline constexpr unsigned enemy_capacity = 24;
inline constexpr unsigned projectile_capacity = 24;
inline constexpr unsigned orb_capacity = 64;
enum class Phase { Loading, Ready, Running, Upgrade, Treasure, Victory, Defeat };
enum class Upgrade : unsigned { Wave, Spin, Aura, Power, Haste, Reach, Magnet, Heart, Bow, Boomerang, Bombs, Ball, Rupees, Count };
enum class ChestReward : unsigned { Bow, Boomerang, Bombs, Ball, MasterSword, RedPotion, Count };
inline constexpr Upgrade chest_weapon(ChestReward reward) {
    return reward <= ChestReward::Ball ? static_cast<Upgrade>(unsigned(Upgrade::Bow) + unsigned(reward)) : Upgrade::Count;
}
inline constexpr std::array names{
    "Sword Wave", "Whirling Blade", "Light Aura", "Power", "Haste", "Reach", "Magnet", "Heart Container", "Hero's Bow", "Boomerang", "Bombs", "Ball and Chain", "Rupees"};
inline constexpr std::array descriptions{
    "Automatic piercing sword waves. Higher levels add damage and extra waves.",
    "Unlock or improve a moving spin attack.",
    "Unlock or improve a short-range pulse of light around Link.",
    "Increase automatic weapon damage.", "Reduce automatic weapon cooldowns.",
    "Increase the size of automatic attacks.", "Collect experience from farther away.",
    "Gain one maximum heart (does not heal).",
    "Faster, stronger arrows. Ranks 3 and 5 add spread shots.",
    "A damaging returning boomerang. Upgrades increase damage, size and firing rate.",
    "Lob bombs at enemies. Upgrades increase blast size, damage and firing rate.",
    "Launch a heavy ball and retract its chain. Upgrades increase damage, size and firing rate.",
    "Receive 20 rupees."};

struct Run {
    Phase phase = Phase::Loading;
    unsigned ticks = 0, kills = 0, level = 1, xp = 0;
    uint32_t random = 1;
    bool elite_spawned = false, master_sword = false;
    std::array<unsigned, 12> ranks{};
    unsigned chests_opened = 0;
    std::array<Upgrade, 3> offers{};
    uint32_t next_random() {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        return random;
    }
    void start(uint32_t seed) { *this = {}; random = seed ? seed : 1; phase = Phase::Running; }
    unsigned required_xp() const { return 5 + level * 3; }
    unsigned rank(Upgrade u) const { const auto i = static_cast<unsigned>(u); return i < ranks.size() ? ranks[i] : 0; }
    bool eligible(Upgrade u) const {
        if (u == Upgrade::Rupees) return true;
        if (rank(u) >= 5) return false;
        if (u >= Upgrade::Bow && u <= Upgrade::Ball && rank(u) == 0) return false;
        if (u >= Upgrade::Power && u <= Upgrade::Heart && rank(u) == 0) {
            unsigned occupied = 0;
            for (unsigned i = 3; i < static_cast<unsigned>(Upgrade::Bow); ++i) occupied += ranks[i] != 0;
            if (occupied >= 4) return false;
        }
        return true;
    }
    bool level_up() {
        if (phase != Phase::Running || xp < required_xp()) return false;
        xp -= required_xp(); ++level;
        std::array<Upgrade, static_cast<unsigned>(Upgrade::Count)> pool{};
        unsigned count = 0;
        for (unsigned i = 0; i < static_cast<unsigned>(Upgrade::Count); ++i)
            if (eligible(static_cast<Upgrade>(i))) pool[count++] = static_cast<Upgrade>(i);
        for (auto& offer : offers) {
            if (!count) { offer = Upgrade::Rupees; continue; }
            const unsigned choice = next_random() % count;
            offer = pool[choice]; pool[choice] = pool[--count];
        }
        phase = Phase::Upgrade;
        return true;
    }
    bool choose(unsigned choice) {
        if (phase != Phase::Upgrade || choice >= offers.size()) return false;
        const auto u = offers[choice];
        if (!eligible(u)) return false;
        if (u != Upgrade::Rupees) ++ranks[static_cast<unsigned>(u)];
        phase = Phase::Running;
        return true;
    }
    unsigned chest_cost() const { return 20 * (chests_opened + 1); }
    bool chest_rewards_left(bool bottle_available = false) const {
        if (!master_sword || bottle_available) return true;
        for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i)
            if (ranks[i] < 5) return true;
        return false;
    }
    // Atomic, run-local purchase. Unaffordable/all-maxed attempts do not roll,
    // consume currency, advance prices or change upgrades.
    bool buy_chest(unsigned& rupees, ChestReward& reward, bool bottle_available = false) {
        if (phase != Phase::Treasure || rupees < chest_cost()) return false;
        std::array<ChestReward, 6> pool{};
        unsigned count = 0;
        for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i)
            if (ranks[i] < 5) pool[count++] = static_cast<ChestReward>(i - unsigned(Upgrade::Bow));
        if (!master_sword) pool[count++] = ChestReward::MasterSword;
        if (bottle_available) pool[count++] = ChestReward::RedPotion;
        if (!count) return false;
        reward = pool[next_random() % count];
        rupees -= chest_cost(); ++chests_opened;
        if (reward == ChestReward::MasterSword) master_sword = true;
        else if (reward <= ChestReward::Ball) ++ranks[unsigned(chest_weapon(reward))];
        return true;
    }
    unsigned item_cooldown(Upgrade weapon) const {
        const unsigned base = weapon == Upgrade::Bow ? 50 : weapon == Upgrade::Boomerang ? 85 :
            weapon == Upgrade::Bombs ? 110 : 100;
        return cooldown(base - 5 * (std::max(1u, rank(weapon)) - 1));
    }
    bool tick(bool playable) {
        if (phase != Phase::Running || !playable) return false;
        ++ticks; return true;
    }
    unsigned target_population() const { return std::min(6u + ticks / (45 * ticks_per_second), enemy_capacity - 1); }
    unsigned spawn_interval() const { return std::max(15u, 75u - std::min(60u, ticks / (10 * ticks_per_second))); }
    unsigned cooldown(unsigned base) const { return std::max(6u, base * (100 - rank(Upgrade::Haste) * 10) / 100); }
    // One native collectible per confirmed kill: 5% heart, 5% blue, 40% green.
    enum class Drop { None, Heart, GreenRupee, BlueRupee };
    Drop roll_drop() {
        const unsigned roll = next_random() % 100;
        return roll < 5 ? Drop::Heart : roll < 10 ? Drop::BlueRupee :
            roll < 50 ? Drop::GreenRupee : Drop::None;
    }
    unsigned sword_bonus() const { return master_sword ? 3 : 0; }
    unsigned attack_power(unsigned weapon) const { return 2 + ranks[weapon] + rank(Upgrade::Power) + (weapon < 2 ? sword_bonus() : 0); }
    float radius(float base) const { return base * (1.0f + 0.15f * rank(Upgrade::Reach)); }
    void end(bool won) { if (phase == Phase::Running || phase == Phase::Upgrade || phase == Phase::Treasure) phase = won ? Phase::Victory : Phase::Defeat; }
};

// Explicit little-endian encoding, independent of ABI padding and game version.
struct Records {
    uint32_t runs = 0, wins = 0, best_ticks = 0, best_kills = 0, best_level = 0;
    void finish(const Run& run) {
        if (run.phase != Phase::Victory && run.phase != Phase::Defeat) return;
        ++runs; wins += run.phase == Phase::Victory;
        best_ticks = std::max(best_ticks, run.ticks);
        best_kills = std::max(best_kills, run.kills);
        best_level = std::max(best_level, run.level);
    }
    std::array<uint8_t, 28> encode() const {
        const std::array<uint32_t, 7> words{0x56525348, 1, runs, wins, best_ticks, best_kills, best_level};
        std::array<uint8_t, 28> bytes{};
        for (unsigned i = 0; i < words.size(); ++i)
            for (unsigned b = 0; b < 4; ++b) bytes[i * 4 + b] = uint8_t(words[i] >> (8 * b));
        return bytes;
    }
    bool decode(std::span<const uint8_t> bytes) {
        if (bytes.size() != 28) return false;
        std::array<uint32_t, 7> words{};
        for (unsigned i = 0; i < words.size(); ++i)
            for (unsigned b = 0; b < 4; ++b) words[i] |= uint32_t(bytes[i * 4 + b]) << (8 * b);
        if (words[0] != 0x56525348 || words[1] != 1 || words[3] > words[2]) return false;
        *this = {words[2], words[3], words[4], words[5], words[6]};
        return true;
    }
};
} // namespace dawnlight::survivors
