#pragma once
#include "general_modes.hpp"
#include <array>
#include <algorithm>

namespace dawnlight::survivors {
enum class Talent : unsigned { MaxHealth, CritChance, Sprint, SprintSpeed, Magnet, ManualJump, Glider, Gale, GaleCharges, Rerolls, Banishes, Dodge, StartingItem, AutoRupees, BonusDamage, MeleeHaste, CritDamage, LegendaryChance, LifeRegen, EquipmentDropRate, EquipmentRarity, Count };
inline constexpr std::array talent_names{"Max Health", "Crit Chance", "Sprint", "Sprint Speed", "Magnet", "Manual Jump", "Glider", "Revali's Gale", "Gale Charges", "Rerolls", "Banishes", "Dodge", "Starting Item", "Auto Rupees", "Bonus Damage", "Melee Haste", "Crit Damage", "Legendary Chance", "Life Regen", "Equipment Drop Rate", "Equipment Rarity"};
inline constexpr std::array<unsigned,unsigned(Talent::Count)> talent_caps{10,10,1,2,20,1,1,1,2,5,5,5,1,5,10,5,5,5,5,10,10};
inline constexpr std::array talent_help{
    "+1 starting heart per rank (up to 13). Run Heart Containers remain separate.",
    "+1 percentage point base critical chance per rank, up to +10. Luck multiplies it; final chance caps at 100%.",
    "Hold the normal Sprint input to run at 125% speed, including automatic combat.",
    "Requires Sprint. +12.5 percentage points sprint speed per rank, up to 150%.",
    "+10% pickup range per rank, up to +200%, applied after run Magnet upgrades. Includes XP, rupees and hearts.",
    "Unlock Manual Jump using your configured Jump button or touch button.",
    "Requires Manual Jump. Press Jump in the air to deploy the Glider.",
    "Requires Glider. Hold Jump through landing to charge Revali's Gale. One charge; 120-second recovery.",
    "Requires Revali's Gale. +1 charge per rank, up to three. Charges refill for each new run.",
    "+1 free reroll per rank each run, up to six including the base reroll. Shared by level-ups and chests.",
    "+1 banishment per rank each run, up to six including the base banishment. Shared by level-ups and chests.",
    "Avoid enemy hits: 5% / 6.25% / 7.5% / 8.75% / 10%. Independent of Luck and shield blocks.",
    "Start each run by choosing one free item from three chest rewards. Uses normal rarity and shared rerolls/banishes.",
    "Gain 1 rupee per rank every 5 seconds of active play, up to 5. Pauses with the run; respects wallet capacity.",
    "+1 weapon damage per rank, up to +10, before critical hits and Dark Link. Jump Strike scales from melee damage.",
    "+5% normal sword attack speed per rank, up to +25%. Adds to Dual Wield speed; combines with Haste cooldowns.",
    "+10 percentage points critical damage per rank, up to +50%. Base x2 becomes x2.5; stacks with run Crit Damage upgrades.",
    "+0.6 percentage points base Legendary rarity chance per rank, up to +3 (1% to 4%) before Luck. Applies to item and power-up rarities.",
    "Regenerate 1/4 heart every 20 seconds of active play. Each rank shortens the interval by 1 second, down to 15 seconds. Pauses with the run.",
    "+5 percentage points equipment drop chance per rank from timed and stationary bosses: 20% base, up to 70% at Twilit Rank 5. Twilit Rank raises this toward 100% at Rank 25. Independent of Luck. Applies from the next run.",
    "Improves newly dropped equipment stats each rank. At rank 10, stat tiers 1/2/3/4/5 have 20%/25%/25%/20%/10% chances (base 50%/25%/15%/8%/2%). +2 piercing/hop rolls rise from 10% to 30%. Twilit Rank increasingly replaces these rolls with 75% Epic / 25% Legendary, guaranteed at Rank 25. This talent also weights stats within those rarities; existing gear is unchanged. Independent of Luck and Legendary Chance."};
inline constexpr Talent talent_parent(Talent t) {
    switch(t) {
    case Talent::SprintSpeed: return Talent::Sprint;
    case Talent::Glider: return Talent::ManualJump;
    case Talent::Gale: return Talent::Glider;
    case Talent::GaleCharges: return Talent::Gale;
    default: return Talent::Count;
    }
}
struct Talents {
    std::array<unsigned,unsigned(Talent::Count)> ranks{};
    unsigned rank(Talent t) const { return t<Talent::Count ? ranks[unsigned(t)] : 0; }
    unsigned spent() const { unsigned n=0;for(auto r:ranks)n+=r;return n; }
    bool valid() const {
        for(unsigned i=0;i<ranks.size();++i)
            if(ranks[i]>talent_caps[i] || (ranks[i] && talent_parent(Talent(i))!=Talent::Count && !rank(talent_parent(Talent(i)))))return false;
        return true;
    }
    bool can_buy(Talent t) const {
        return t<Talent::Count && rank(t)<talent_caps[unsigned(t)] &&
            (talent_parent(t)==Talent::Count || rank(talent_parent(t))>0);
    }
    bool refund(Talent t) {
        if(!rank(t))return false;
        --ranks[unsigned(t)];
        // Refund dependents recursively when their prerequisite reaches zero.
        bool changed;
        do { changed=false;
            for(unsigned i=0;i<ranks.size();++i)if(ranks[i] && talent_parent(Talent(i))!=Talent::Count && !rank(talent_parent(Talent(i)))) {
                ranks[i]=0;changed=true;
            }
        } while(changed);
        return true;
    }
    unsigned dodge_basis_points() const { const auto r=rank(Talent::Dodge); return r ? 500+125*(std::min(5u,r)-1) : 0; }
    unsigned equipment_drop_basis_points() const { return 2000+500*std::min(10u,rank(Talent::EquipmentDropRate)); }
    unsigned starting_hearts() const { return 3+rank(Talent::MaxHealth); }
    float sprint_multiplier() const { return rank(Talent::Sprint) ? 1.25f+0.125f*rank(Talent::SprintSpeed) : 1.f; }
};
inline bool talent_mode_override(const Talents& talents, ModeSetting setting, int64_t& value) {
    switch(setting) {
    case ModeSetting::Sprint: value=talents.rank(Talent::Sprint)>0;return true;
    case ModeSetting::SprintSpeed: value=int64_t(talents.sprint_multiplier()*100+0.5f);return true; // Config UI rounds; gameplay keeps the exact fraction.
    case ModeSetting::Jump: value=talents.rank(Talent::ManualJump)>0;return true;
    case ModeSetting::Glide: value=talents.rank(Talent::Glider)>0;return true;
    case ModeSetting::GlideItem: value=1;return true;
    case ModeSetting::Gale: case ModeSetting::GaleCounter: value=talents.rank(Talent::Gale)>0;return true;
    case ModeSetting::GaleCharges: value=1+talents.rank(Talent::GaleCharges);return true;
    case ModeSetting::GaleRecovery: value=120;return true;
    case ModeSetting::GaleHeight: value=500;return true;
    default:return false;
    }
}
// Total XP required for n completed meta levels; each grants one Unlock Point.
inline constexpr uint64_t meta_threshold(unsigned n) { return uint64_t(n)*2000+uint64_t(n)*(n ? n-1 : 0)*250; }
inline constexpr uint64_t max_meta_xp=1000000000000ULL;
inline unsigned meta_levels(uint64_t xp) {
    unsigned low=0,high=100000;
    while(low<high){const unsigned mid=(low+high+1)/2;if(meta_threshold(mid)<=xp)low=mid;else high=mid-1;}
    return low;
}
} // namespace dawnlight::survivors
