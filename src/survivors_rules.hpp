#pragma once
#include "survivors_gear.hpp"
#include "survivors_debug.hpp"
#include "survivors_damage.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <span>
#include "survivors_progression.hpp"

namespace dawnlight::survivors {
// All durations are simulation ticks (TP runs its gameplay at 30 Hz).
inline constexpr unsigned ticks_per_second = 30;
inline constexpr unsigned duration = 20 * 60 * ticks_per_second;
inline constexpr unsigned elite_interval = 5 * 60 * ticks_per_second;
inline constexpr unsigned timed_elite_count = duration / elite_interval;
inline constexpr unsigned guard_capacity = 8;
inline constexpr unsigned elite_capacity = timed_elite_count + guard_capacity;
inline constexpr unsigned guard_respawn_interval = 3 * 60 * ticks_per_second;
// Guard respawns stop at 20 minutes; the final timed elite can be defeated later.
inline constexpr unsigned elite_reward_capacity = timed_elite_count +
    guard_capacity * (1 + (duration - 1) / guard_respawn_interval);
inline constexpr unsigned enemy_capacity = 24 * 5 * 3 + elite_capacity;
// Initial reusable storage only; player projectiles grow on demand without a cap.
inline constexpr unsigned initial_projectile_slots = 42;
inline constexpr unsigned item_projectile_begin(unsigned type) { return type ? 28 + (type - 1) * 4 : 8; }
inline constexpr unsigned item_initial_slots(unsigned type) { return type ? 4 : 20; }
inline constexpr unsigned orb_capacity = 64;
inline constexpr unsigned pot_capacity = 72;
inline constexpr unsigned pot_respawn_interval = 20 * ticks_per_second;
inline constexpr unsigned horde_duration = 15 * ticks_per_second;
inline constexpr unsigned map_chest_capacity = 18;
inline constexpr float item_target_range = 1400.f;
inline unsigned map_chest_budget(unsigned map) {
    constexpr std::array<unsigned,5> counts{3,18,18,6,4};
    return map < counts.size() ? counts[map] : 0;
}
inline unsigned pot_budget(unsigned map, float floorArea) {
    // Density follows the loaded floor area, bounded per arena for native actor
    // budgets. Outdoor maps always have substantially more pots than indoors.
    constexpr std::array<unsigned,5> minimum{6,40,40,12,8}, maximum{10,72,72,24,12};
    if (map>=minimum.size()) return 0;
    const float density=floorArea>0 ? floorArea/2000000.f : 0.f;
    return unsigned(std::clamp(density,float(minimum[map]),float(maximum[map])));
}
enum class Phase { Loading, Ready, Running, Upgrade, TreasureChoice, Treasure, Victory, Defeat };
enum class Upgrade : unsigned { Wave, Spin, ShieldAttack, Power, Haste, Reach, Magnet, Heart, Bow, Boomerang, Bombs, Ball, FireArrows, Luck, PotionHealing, CritChance, CritDamage, JumpStrike, DualWield, Knockback, Piercing, Multishot, LifeRegen, LifeSteal, Hurricane, FireBomb, BombArrow, FireEdge, DarkAura, Shockwave, Lightning, MasterSword, MultiStrike, Rupees, ChainSpin, Count };
inline constexpr DamageSource weapon_damage_source(unsigned weapon) {
    switch(Upgrade(weapon)) {
    case Upgrade::Lightning:return DamageSource::Lightning;
    case Upgrade::Wave:return DamageSource::Wave;
    case Upgrade::Spin:return DamageSource::Spin;
    case Upgrade::JumpStrike:return DamageSource::Jump;
    case Upgrade::Bow:return DamageSource::Bow;
    case Upgrade::Boomerang:return DamageSource::Boomerang;
    case Upgrade::Bombs:return DamageSource::Bombs;
    case Upgrade::Ball:return DamageSource::Ball;
    case Upgrade::ShieldAttack:return DamageSource::Shield;
    case Upgrade::Shockwave:return DamageSource::Shockwave;
    case Upgrade::BombArrow:return DamageSource::BombArrow;
    default:return DamageSource::Other;
    }
}
inline constexpr unsigned upgrade_cap(Upgrade u) {
    switch (u) {
    case Upgrade::Knockback: case Upgrade::Piercing: return 10;
    case Upgrade::Multishot: case Upgrade::MultiStrike: return 3;
    case Upgrade::LifeRegen: return 5;
    case Upgrade::LifeSteal: case Upgrade::Hurricane: case Upgrade::FireBomb: case Upgrade::BombArrow:
    case Upgrade::FireEdge: case Upgrade::DarkAura: case Upgrade::Shockwave: case Upgrade::ChainSpin: return 1;
    case Upgrade::Luck: return 20;
    default: return 30;
    }
}
enum class Rarity : unsigned { Common, Uncommon, Rare, Epic, Legendary };
inline constexpr unsigned reward_units(Rarity rarity) { return 1 + unsigned(rarity); }
inline constexpr Rarity upgrade_min_rarity(Upgrade u) {
    return ((u >= Upgrade::LifeSteal && u <= Upgrade::Shockwave) || u==Upgrade::ChainSpin) ? Rarity::Legendary : (u == Upgrade::Multishot || u == Upgrade::MultiStrike) ? Rarity::Epic :
        u == Upgrade::LifeRegen ? Rarity::Rare : Rarity::Common;
}
inline constexpr unsigned upgrade_units(Upgrade u, Rarity rarity) {
    if (rarity < upgrade_min_rarity(u)) return 0;
    return ((u >= Upgrade::LifeSteal && u <= Upgrade::Shockwave) || u==Upgrade::ChainSpin) ? 1 : (u == Upgrade::Multishot || u == Upgrade::MultiStrike) ?
        (rarity == Rarity::Legendary ? 2 : 1) : reward_units(rarity);
}
inline constexpr std::array rarity_names{"Common", "Uncommon", "Rare", "Epic", "Legendary"};
inline constexpr std::array rarity_colors{"#f4d78b", "#7bdd83", "#72b7ff", "#cb91ff", "#ffad54"};
enum class ChestReward : unsigned { Bow, Boomerang, Bombs, Ball, MasterSword, RedPotion, OrdonSword, Shield, DarkLink, Count };
inline constexpr Upgrade chest_weapon(ChestReward reward) {
    return reward <= ChestReward::Ball ? static_cast<Upgrade>(unsigned(Upgrade::Bow) + unsigned(reward)) : Upgrade::Count;
}
inline constexpr std::array names{
    "Sword Wave", "Spin Attack", "Shield Attack", "Power", "Haste", "Reach", "Magnet", "Heart Container", "Hero's Bow", "Boomerang", "Bombs", "Ball and Chain", "Fire Arrows", "Luck", "Potion Healing", "Crit Chance", "Crit Damage", "Jump Strike", "Dual Wield", "Knockback", "Piercing", "Multishot", "Life Regen", "Life Steal", "Hurricane", "Fire Bomb", "Bomb Arrow", "Fire Edge", "Dark Aura", "Shockwave", "Lightning Strike", "Master Sword", "Multi Strike", "Rupees", "Chain Spin"};
inline constexpr std::array descriptions{
    "Piercing sword waves. Great Spin projectile with a larger radius from level 15.",
    "Automatic spin attack. Great Spin with a larger radius from level 15.",
    "Unlock or improve an automatic shield bash that staggers enemies.",
    "Increase automatic weapon damage.", "Reduce automatic weapon cooldowns.",
    "Increase attack reach/range. Double effect for melee, Dual Wield splash, Spin, Jump Strike and Lightning Strike; other abilities retain normal scaling.", "Attract experience, rupees, hearts and heart pieces from farther away.",
    "Gain one maximum heart and fully heal.",
    "Faster, stronger arrows. Three arrows from level 10; five from level 20, across 45 degrees. Extra hits on one enemy per volley deal 25% damage.",
    "A damaging returning boomerang. Upgrades increase damage, hops and firing rate; hit size stays fixed.",
    "Lob bombs at enemies. Upgrades increase blast size, damage and firing rate.",
    "Launch a heavy ball and retract its chain. Upgrades increase damage, size and firing rate.",
    "Ignite enemies for periodic damage. Hits refresh the burn; upgrades improve damage and duration.",
    "Improve rarity weights by +5% and critical chance by +2.5% per level. Lightning Strike gains +2 percentage points per level, up to 60%. Heart and rupee bonuses gain +10% per level, capped at +50%.",
    "Red potions restore an extra quarter heart per level.",
    "Direct hits gain +0.75% base critical chance per level. Luck multiplies this chance.",
    "Critical hits start at x2 damage. Each level adds +0.05 to the multiplier.",
    "Master Sword: occasionally perform a quick Jump Strike. Landing splash starts at 30% melee damage; +5 percentage points per rank.",
    "Improve equipped Dual Wield: +1% melee critical chance and +2.5% melee attack speed per level and 2/3/4/5 total targets at levels 1/10/20/30. Requires Ordon Sword.",
    "Add a horizontal push to all direct attacks without interrupting enemy actions. +40 distance per level; maximum level 10. Heavy weapons start with a fixed 240-unit push. Bosses and elites are immune to knockback.",
    "Arrows pass through one additional enemy per level, damaging every target. Applies to every arrow, including Multishot; maximum level 10.",
    "Add one projectile/chain per level to Bow, Boomerang, Bombs, Ball and Chain, Sword Wave and Lightning Strike. Epic grants 1 level; Legendary grants 2; maximum level 3.",
    "Reduce passive regeneration interval by 1 second per level; maximum level 5. Rare or better. Stacks with the Life Regen talent.",
    "Sword kills restore health. Damage over time does not trigger healing.",
    "Double Boomerang size, wind effects and hit radius. It damages every enemy along its flight path.",
    "Bomb explosions ignite every hit enemy for 10% of bomb damage each second for 5 seconds.",
    "Bomb arrows explode once on their first enemy hit for 50% bow damage. Level-5 bomb radius scales with Reach; Piercing still works.",
    "All melee hits ignite enemies for 10% normal melee damage each second for 5 seconds, including Spin, Dual Wield and Jump Strike.",
    "While Dark Link is active, ignite nearby enemies with black flames. Each second deals 10% of target maximum HP; lingers 5 seconds outside the aura.",
    "Shield Attack launches a piercing shockwave for current melee damage. Level-5 Ball and Chain width scales with Reach.",
    "Melee hits can release chain lightning that jumps between enemies. Luck increases its trigger chance.",
    "Increases sword ability damage and Lightning Strike damage. Requires the Master Sword from a chest; Light Sword from level 5.",
    "Repeat Sword Attack, Spin Attack, Sword Wave and Lightning Strike. Requires Dual Wield.",
    "Receive 20 rupees.",
    "Ball and Chain spirals once around Link while retracting. Requires Ball and Chain."};

// A burn refresh extends its duration without delaying the next tick or stacking.
// Store tenths of HP so 10% effects stay proportional even below ten damage.
struct LegendaryBurn {
    unsigned remaining=0, pulse=ticks_per_second, tenths=0, carry=5;
    void ignite(unsigned damageTenths) {
        if (!remaining) { pulse=ticks_per_second; carry=5; }
        remaining=5*ticks_per_second; tenths=damageTenths;
    }
    unsigned tick() {
        if (!remaining) return 0;
        --remaining;
        if (--pulse) return 0;
        pulse=ticks_per_second; carry+=tenths;
        const unsigned damage=carry/10; carry%=10; return damage;
    }
};

enum class ManualAbility : unsigned { Sword, Spin, JumpStrike, Wave, Bow, Boomerang, Bombs, Ball, ShieldAttack, Shockwave, Count };
constexpr ManualAbility manual_ability(Upgrade weapon) {
    switch(weapon) {
    case Upgrade::Spin:return ManualAbility::Spin;
    case Upgrade::JumpStrike:return ManualAbility::JumpStrike;
    case Upgrade::Wave:return ManualAbility::Wave;
    case Upgrade::Bow:return ManualAbility::Bow;
    case Upgrade::Boomerang:return ManualAbility::Boomerang;
    case Upgrade::Bombs:return ManualAbility::Bombs;
    case Upgrade::Ball:return ManualAbility::Ball;
    case Upgrade::ShieldAttack:return ManualAbility::ShieldAttack;
    case Upgrade::Shockwave:return ManualAbility::Shockwave;
    default:return ManualAbility::Count;
    }
}
struct RunOptions {
    float enemy_hp = 1, enemy_damage = 1, enemy_speed = 1, horde = 1;
    float camera_distance = 200;
    float boss_hp = 1;
    bool legacy_difficulty = false; // Historical snapshots retain pre-Boss-HP rank/rewards.
    bool weighted_difficulty = false; // v15 five-setting weighted rewards, before the x5 cap.
    DebugOptions debug{};
    int reticle_y = 0;
    bool switch_attack_lock = false;
    bool gyro_aim = true;
    bool allow_cheats = false;
    bool enemy_collision = false;
    std::array<bool,unsigned(ManualAbility::Count)> manual_aim=[] {
        std::array<bool,unsigned(ManualAbility::Count)> enabled{};enabled.fill(true);return enabled;
    }();
    bool manual_enabled(ManualAbility ability) const {
        return unsigned(ability)<manual_aim.size()&&manual_aim[unsigned(ability)];
    }
    bool manual_enabled(Upgrade weapon) const { return manual_enabled(manual_ability(weapon)); }
    static float difficulty(float value,float maximum) {
        return std::isfinite(value)?std::clamp(std::round(value*2.f)*.5f,1.f,maximum):1.f;
    }
    float twilit_rank() const {
        if(legacy_difficulty||weighted_difficulty)
            return difficulty(enemy_hp,10)+difficulty(enemy_damage,10)+difficulty(enemy_speed,3)+difficulty(horde,5)-4+
                (legacy_difficulty?0.f:difficulty(boss_hp,10)-1.f);
        return difficulty(enemy_hp,5)+difficulty(enemy_damage,5)+difficulty(enemy_speed,5)+difficulty(horde,5)+difficulty(boss_hp,5);
    }
    float score_percent() const {
        if(legacy_difficulty||weighted_difficulty)
            return 100+10*(difficulty(enemy_hp,10)-1)+15*(difficulty(enemy_damage,10)-1)+
                25*(difficulty(enemy_speed,3)-1)+10*(difficulty(horde,5)-1)+
                (legacy_difficulty?0.f:10*(difficulty(boss_hp,10)-1));
        return twilit_rank()*20;
    }
    unsigned equipment_progress() const {
        return unsigned(std::clamp((twilit_rank()-5.f)*500.f,0.f,10000.f));
    }
    void clamp() {
        debug.clamp();
        boss_hp=difficulty(boss_hp,5);
        reticle_y=std::clamp(reticle_y,-40,40);
        enemy_hp=difficulty(enemy_hp,5); enemy_damage=difficulty(enemy_damage,5);
        enemy_speed=difficulty(enemy_speed,5); horde=difficulty(horde,5);
        camera_distance=std::isfinite(camera_distance)?std::clamp(camera_distance,100.f,500.f):200.f;
    }
};
struct Run {
    Phase phase = Phase::Loading;
    unsigned ticks = 0, kills = 0, level = 1, xp = 0;
    uint32_t random = 1;
    DamageTotals damage;
    bool modded = false; // Sticky for this run, even if the extra mod is later disabled.
    bool endless=false;
    bool debug_run=false; // Captured at start; remains excluded through Endless.
    unsigned endless_start=0, normal_score=0;
    float rank_snapshot=-1;
    RunOptions options;
    Talents talents;
    GearItem gear_weapon{}, gear_tunic{}; // Midna can equip the currently advertised drop.
    unsigned gear_hidden_hearts=0; // Tunic capacity above the 20-heart cap, for reversible swaps.
    void equip_gear(const GearInventory& gear) {
        gear_weapon=gear.equipped(GearSlot::Weapon);gear_tunic=gear.equipped(GearSlot::Tunic);
        maximum_hearts=std::min(20u,talents.starting_hearts()+gear_tunic.bonus(GearStat::Hearts));
        gear_hidden_hearts=0;
    }
    void equip_drop(GearItem item) {
        if(!item.valid() || item.kind==GearKind::Empty)return;
        if(item.is_weapon()) {gear_weapon=item;return;}
        const unsigned total=maximum_hearts+gear_hidden_hearts;
        const unsigned next=total-std::min(total,gear_tunic.bonus(GearStat::Hearts))+item.bonus(GearStat::Hearts);
        maximum_hearts=std::min(20u,next);gear_hidden_hearts=next-maximum_hearts;
        gear_tunic=item;
    }
    unsigned dodge_basis_points() const { return std::min(10000u,talents.dodge_basis_points()+gear_tunic.second*100u); }
    unsigned timed_elites = 0, guard_elites = 0;
    unsigned drop_progress = 0, spawn_progress = 0, maximum_hearts = 3;
    unsigned master_sword = 0, shield = 0, dual_wield = 0, dark_link = 0;
    bool red_potion = false, starting_item_pending = false;
    unsigned free_rerolls = 1, banishes = 1, paid_rerolls = 0;
    std::array<bool,unsigned(Upgrade::Count)> banned_upgrades{};
    std::array<bool,unsigned(ChestReward::Count)> banned_items{};
    unsigned dark_clock = 0, dark_remaining = 0;
    unsigned horde_remaining = 0;
    unsigned gibdo_scream_cooldown = 0;
    std::array<unsigned, unsigned(Upgrade::Rupees)> ranks{};
    unsigned chain_spin=0; // Appended upgrade; preserve historical rank-array offsets.
    unsigned chests_opened = 0;
    std::array<Upgrade, 3> offers{};
    std::array<Rarity, 3> offer_rarities{};
    uint32_t next_random() {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        return random;
    }
    void start(uint32_t seed, RunOptions settings = {}, Talents perks = {}) {
        *this={}; options=settings; options.legacy_difficulty=false; options.weighted_difficulty=false; options.clamp(); talents=perks;
        debug_run=debug_enabled && debug_session_enabled;
        if(!debug_run)options.debug={};
        maximum_hearts=talents.starting_hearts();
        starting_item_pending=talents.rank(Talent::StartingItem)>0;
        free_rerolls+=talents.rank(Talent::Rerolls); banishes+=talents.rank(Talent::Banishes);
        random=seed ? seed : 1; phase=Phase::Running;
    }
    DebugOptions debug_options() const { auto value=options.debug;value.clamp();return value; }
    unsigned run_duration() const { return debug_options().run_minutes*60*ticks_per_second; }
    float twilit_rank() const { return rank_snapshot>=0?rank_snapshot:options.twilit_rank(); }
    unsigned endless_minutes() const { return endless && ticks>=endless_start?(ticks-endless_start)/(60*ticks_per_second):0; }
    double endless_factor() const { return std::pow(1.1, double(std::min(1000u,endless_minutes()))); }
    bool continue_endless() {
        if(phase!=Phase::Victory || endless)return false;
        normal_score=score();endless_start=ticks;endless=true;phase=Phase::Running;return true;
    }
    unsigned incoming_damage(unsigned base,unsigned factor=1) const {
        return unsigned(std::min(32767.0,std::ceil(double(base)*options.enemy_damage*factor*endless_factor())));
    }
    unsigned boss_interval() const { return run_duration()/timed_elite_count; }
    unsigned player_damage(unsigned base) const { return unsigned(std::min<uint64_t>(32767,uint64_t(base)*debug_options().player_damage)); }
    bool infinite_rerolls() const { return debug_options().infinite_rerolls!=0; }
    bool infinite_banishes() const { return debug_options().infinite_banishes!=0; }
    bool can_banish() const { return infinite_banishes() || banishes!=0; }
    unsigned magnet_range() const { return (150+65*rank(Upgrade::Magnet))*(100+10*talents.rank(Talent::Magnet))/100; }
    unsigned normal_kills() const { return kills-std::min(kills,timed_elites+guard_elites); }
    unsigned score() const {
        const double base=2*((endless?ticks:std::min(ticks,run_duration()))/ticks_per_second)+std::floor(double(normal_kills())*10/RunOptions::difficulty(options.horde,5))+
            uint64_t(timed_elites)*500+uint64_t(guard_elites)*250+uint64_t(level ? level-1 : 0)*50+
            (phase==Phase::Victory || endless ? 3000 : 0);
        return unsigned(std::min<double>(UINT32_MAX,base*options.score_percent()/100));
    }
    unsigned equipment_drop_basis_points() const {
        const unsigned base=talents.equipment_drop_basis_points();
        return base+(10000-base)*options.equipment_progress()/10000;
    }
    unsigned earned_meta_xp() const { return debug_run?0:score()-std::min(score(),endless?normal_score:0u); }
    unsigned required_xp() const { return 5 + level * 3; }
    unsigned base_rank(Upgrade u) const { const auto i=unsigned(u);return u==Upgrade::ChainSpin?chain_spin:u==Upgrade::MasterSword?master_sword:u==Upgrade::DualWield?dual_wield:i<ranks.size()?ranks[i]:0; }
    unsigned gear_levels(Upgrade upgrade,GearFamily family) const {
        if(gear_weapon.family()!=family)return 0;
        const auto weapon=family==GearFamily::Bow?Upgrade::Bow:family==GearFamily::Boomerang?Upgrade::Boomerang:
            family==GearFamily::Bombs?Upgrade::Bombs:family==GearFamily::Ball?Upgrade::Ball:Upgrade::Count;
        if(family!=GearFamily::Sword&&!base_rank(weapon))return 0;
        GearPerk perk=GearPerk::None;
        switch(upgrade) {
        case Upgrade::Lightning:perk=GearPerk::Lightning;break;
        case Upgrade::Wave:perk=GearPerk::Wave;break;case Upgrade::Spin:perk=GearPerk::Spin;break;
        case Upgrade::MultiStrike:perk=GearPerk::MultiStrike;break;case Upgrade::FireArrows:perk=GearPerk::FireArrows;break;
        case Upgrade::Piercing:perk=GearPerk::Piercing;break;case Upgrade::Multishot:perk=GearPerk::Multishot;break;
        case Upgrade::Reach:perk=GearPerk::Reach;break;case Upgrade::Haste:perk=GearPerk::Haste;break;
        case Upgrade::FireEdge:perk=GearPerk::FireEdge;break;case Upgrade::BombArrow:perk=GearPerk::BombArrow;break;
        case Upgrade::Hurricane:perk=GearPerk::Hurricane;break;case Upgrade::FireBomb:perk=GearPerk::FireBomb;break;
        case Upgrade::ChainSpin:perk=GearPerk::ChainSpin;break;default:break;
        }
        return perk!=GearPerk::None&&gear_weapon.perk==perk?gear_weapon.perk_levels():0;
    }
    unsigned rank(Upgrade u) const {
        const auto family=u==Upgrade::Wave||u==Upgrade::Spin||u==Upgrade::MultiStrike||u==Upgrade::Lightning||u==Upgrade::FireEdge?GearFamily::Sword:
            u==Upgrade::FireArrows||u==Upgrade::Piercing||u==Upgrade::BombArrow?GearFamily::Bow:
            u==Upgrade::Hurricane?GearFamily::Boomerang:u==Upgrade::FireBomb?GearFamily::Bombs:
            u==Upgrade::ChainSpin?GearFamily::Ball:GearFamily::None;
        const auto extra=family==GearFamily::None?0:gear_levels(u,family);
        return extra?std::min(upgrade_cap(u),base_rank(u)+extra):base_rank(u);
    }
    unsigned potion_heal_quarters() const { return 4 + rank(Upgrade::PotionHealing); }
    unsigned luck_percent() const { return 100 + 10 * std::min(20u, rank(Upgrade::Luck)); }
    unsigned crit_chance_basis_points() const { return std::min(10000u, (std::min(30u,rank(Upgrade::CritChance))*75+talents.rank(Talent::CritChance)*100)*(100+5*std::min(20u,rank(Upgrade::Luck))/2)/100); }
    unsigned crit_damage_percent() const { return 200 + 5*std::min(30u,rank(Upgrade::CritDamage)) + 10*talents.rank(Talent::CritDamage); }
    bool roll_chance(unsigned chance) {
        if (!chance) return false;
        // Rejection sampling avoids modulo bias. Zero chance consumes no RNG.
        constexpr uint32_t limit=UINT32_MAX-UINT32_MAX%10000;
        uint32_t roll;do {roll=next_random()-1;} while(roll>=limit);
        return roll%10000<std::min(10000u,chance);
    }
    static GearFamily gear_family(Upgrade weapon) {
        switch(weapon) {
        case Upgrade::Bow:return GearFamily::Bow;
        case Upgrade::Boomerang:return GearFamily::Boomerang;
        case Upgrade::Bombs:return GearFamily::Bombs;
        case Upgrade::Ball:return GearFamily::Ball;
        default:return GearFamily::None;
        }
    }
    unsigned gear_bonus(GearFamily family, GearStat stat) const {
        if(gear_weapon.family()!=family)return 0;
        // Equipment specializes a build; run weapons still come from chests.
        if(family!=GearFamily::Sword) {
            const auto weapon=family==GearFamily::Bow?Upgrade::Bow:family==GearFamily::Boomerang?Upgrade::Boomerang:
                family==GearFamily::Bombs?Upgrade::Bombs:family==GearFamily::Ball?Upgrade::Ball:Upgrade::Count;
            if(weapon==Upgrade::Count || !rank(weapon))return 0;
        }
        return gear_weapon.bonus(stat);
    }
    unsigned gear_bonus(Upgrade weapon, GearStat stat) const {return gear_bonus(gear_family(weapon),stat);}
    unsigned gear_knockback_percent(DamageSource source) const {
        const auto family=sword_kill_source(source)?GearFamily::Sword:source==DamageSource::Bombs?GearFamily::Bombs:
            source==DamageSource::Ball?GearFamily::Ball:GearFamily::None;
        return gear_bonus(family,GearStat::Knockback);
    }
    float item_flight_speed(Upgrade weapon, bool returning=false) const {
        const float base=returning?55.f:weapon==Upgrade::Bow?70.f:weapon==Upgrade::Ball?40.f:42.f;
        return base*(1.f+gear_bonus(weapon,GearStat::FlightSpeed)/100.f);
    }
    float ball_range() const {return radius(900.f,Upgrade::Ball)*(1.f+gear_bonus(Upgrade::Ball,GearStat::Reach)/100.f);}
    unsigned melee_crit_bonus() const { return std::min(30u,dual_wield)*100+gear_bonus(GearFamily::Sword,GearStat::Crit)*100; }
    unsigned critical_power(unsigned base, bool* critical=nullptr, bool melee=false, bool bow=false) {
        const bool hit=base && roll_chance(std::min(10000u,crit_chance_basis_points()+(melee?melee_crit_bonus():0)+(bow?gear_bonus(Upgrade::Bow,GearStat::Crit)*100:0)));
        if (critical) *critical=hit;
        if (!hit) return base;
        // Native health/damage are integral; round once, before Dark Link.
        return unsigned(std::min<uint64_t>(65535,(uint64_t(base)*crit_damage_percent()+50)/100));
    }
    // Drop frequency/quality retain their old +50% cap independently of rarities.
    unsigned lucky_chance(unsigned basis_points) const { return basis_points * std::min(150u, luck_percent()) / 100; }
    std::array<uint32_t, 5> rarity_weights() const {
        // 60 / 25k / 10k^2 / 4k^3 / k^4. Integer weights retain half-step Luck ranks.
        const uint32_t t = 100 + 5*std::min(20u,rank(Upgrade::Luck)); // k = t / 100; t is in [100, 200].
        // Shift 0.6 percentage points per perk rank from Common to Legendary
        // in the base distribution, then apply the same Luck weights.
        const uint32_t perk=std::min(5u,talents.rank(Talent::LegendaryChance));
        return {600000000-6000000*perk, 2500000*t, 10000*t*t, 40*t*t*t,
            uint32_t(uint64_t(10+6*perk)*t*t*t*t/100)};
    }
    Rarity roll_rarity() {
        const auto weights = rarity_weights();
        uint32_t total = 0;
        for (auto weight : weights) total += weight;
        // Xorshift visits 1..UINT32_MAX. Reject the short tail before reducing.
        const uint32_t limit = UINT32_MAX - UINT32_MAX % total;
        uint32_t roll;
        do { roll = next_random() - 1; } while (roll >= limit);
        roll %= total;
        for (unsigned i = 0; i < weights.size(); ++i) {
            if (roll < weights[i]) return Rarity(std::max(i,debug_options().minimum_rarity));
            roll -= weights[i];
        }
        return Rarity::Common;
    }
    unsigned upgrade_gain(Upgrade u, Rarity rarity) const {
        unsigned remaining = (u < Upgrade::Rupees || u==Upgrade::ChainSpin) ? upgrade_cap(u) - std::min(upgrade_cap(u), base_rank(u)) : 0;
        if (u == Upgrade::Heart) remaining = std::min(remaining, 20 - std::min(20u, maximum_hearts));
        return std::min(upgrade_units(u, rarity), remaining);
    }
    unsigned upgrade_rupees(Upgrade u, Rarity rarity) const {
        return 20 * (upgrade_units(u, rarity) - upgrade_gain(u, rarity));
    }
    bool eligible(Upgrade u) const {
        if (u >= Upgrade::Count || banned_upgrades[unsigned(u)]) return false;
        if (u >= Upgrade::Bow && u <= Upgrade::Ball && banned_items[unsigned(u)-unsigned(Upgrade::Bow)]) return false;
        if (restricted_healing() && u==Upgrade::LifeSteal) return false;
        if (u == Upgrade::Rupees) return true;
        if (base_rank(u) >= upgrade_cap(u) || (upgrade_cap(u)==1&&rank(u))) return false;
        if (u == Upgrade::MultiStrike && !owns(ChestReward::OrdonSword)) return false;
        if (u == Upgrade::MasterSword && (!owns(ChestReward::MasterSword) || !chest_eligible(ChestReward::MasterSword))) return false;
        if (u == Upgrade::ShieldAttack && !owns(ChestReward::Shield)) return false;
        if (u == Upgrade::DualWield && (!owns(ChestReward::OrdonSword) || banned_items[unsigned(ChestReward::OrdonSword)])) return false;
        if (u == Upgrade::PotionHealing && !red_potion) return false;
        if (u == Upgrade::JumpStrike && !master_sword) return false;
        if ((u == Upgrade::FireArrows || u == Upgrade::Piercing) && !rank(Upgrade::Bow)) return false;
        if (u == Upgrade::ChainSpin && !base_rank(Upgrade::Ball)) return false;
        if (u == Upgrade::Hurricane && !rank(Upgrade::Boomerang)) return false;
        if (u == Upgrade::FireBomb && !rank(Upgrade::Bombs)) return false;
        if (u == Upgrade::BombArrow && !rank(Upgrade::Bow)) return false;
        if (u == Upgrade::DarkAura && !dark_link) return false;
        if (u == Upgrade::Shockwave && (!shield || !rank(Upgrade::ShieldAttack))) return false;
        if (u == Upgrade::Multishot && !rank(Upgrade::Wave) && !rank(Upgrade::Bow) &&
            !rank(Upgrade::Boomerang) && !rank(Upgrade::Bombs) && !rank(Upgrade::Ball) && !rank(Upgrade::Lightning)) return false;
        if (u >= Upgrade::Bow && u <= Upgrade::Ball && rank(u) == 0) return false;
        if (u == Upgrade::Heart && maximum_hearts >= 20) return false;
        return true;
    }
    void roll_upgrades() {
        std::array<Upgrade, static_cast<unsigned>(Upgrade::Count)> pool{};
        unsigned count = 0;
        for (unsigned i = 0; i < static_cast<unsigned>(Upgrade::Count); ++i)
            if (eligible(static_cast<Upgrade>(i))) pool[count++] = static_cast<Upgrade>(i);
        // Roll rarity first. Restricted upgrades never turn a common offer
        // into a free Epic/Legendary; Luck retains its normal rarity weights.
        for (unsigned slot = 0; slot < offers.size(); ++slot) {
            const auto rarity = offer_rarities[slot] = roll_rarity();
            std::array<unsigned, unsigned(Upgrade::Count)> candidates{};
            unsigned available = 0;
            for (unsigned i = 0; i < count; ++i)
                if (rarity >= upgrade_min_rarity(pool[i])) candidates[available++] = i;
            if (!available) { offers[slot] = eligible(Upgrade::Rupees) ? Upgrade::Rupees : Upgrade::Count; continue; }
            const unsigned choice = candidates[next_random() % available];
            offers[slot] = pool[choice]; pool[choice] = pool[--count];
        }
    }

    bool level_up() {
        if (phase != Phase::Running || xp < required_xp()) return false;
        xp -= required_xp(); ++level; roll_upgrades(); phase = Phase::Upgrade; return true;
    }
    bool choose(unsigned choice) {
        if (phase != Phase::Upgrade || choice >= offers.size()) return false;
        const auto u = offers[choice];
        if (!eligible(u) || offer_rarities[choice] < upgrade_min_rarity(u)) return false;
        const unsigned gain = upgrade_gain(u, offer_rarities[choice]);
        if (u == Upgrade::MasterSword) master_sword += gain;
        else if (u == Upgrade::DualWield) dual_wield += gain;
        else if (u == Upgrade::ChainSpin) chain_spin += gain;
        else if (u != Upgrade::Rupees) ranks[static_cast<unsigned>(u)] += gain;
        if (u == Upgrade::Heart) maximum_hearts += gain;
        phase = Phase::Running;
        return true;
    }
    unsigned chest_price(bool free, bool fixed = false) const { return free ? 0 : fixed ? 40 : chest_cost(); }
    unsigned chest_cost() const { return 20 + chests_opened * (chests_opened + 4); }
    unsigned burn_damage() const { return rank(Upgrade::FireArrows); }
    unsigned burn_duration() const { return (3 + (std::max(1u, rank(Upgrade::FireArrows)) - 1) / 3) * ticks_per_second; }
    unsigned bonus_damage() const { return talents.rank(Talent::BonusDamage); }
    unsigned melee_power() const { return 3 + rank(Upgrade::Power) + sword_bonus() + bonus_damage(); }
    unsigned melee_speed_percent() const { return 100 + gear_bonus(GearFamily::Sword,GearStat::Speed) + 5*std::min(upgrade_cap(Upgrade::DualWield),dual_wield)/2 + 5*talents.rank(Talent::MeleeHaste); }
    float melee_speed() const { return melee_speed_percent()/100.f; }
    unsigned melee_cooldown() const {
        const unsigned normalSpeed=melee_speed_percent()-gear_bonus(GearFamily::Sword,GearStat::Speed);
        const unsigned normal=std::max(6u,30*cooldown_percent()/normalSpeed);
        return std::max(1u,normal*normalSpeed/melee_speed_percent());
    }
    unsigned life_regen_interval() const { return (20-std::min(5u,talents.rank(Talent::LifeRegen))-std::min(5u,rank(Upgrade::LifeRegen)))*ticks_per_second; }
    bool restricted_healing() const { return endless && ticks>=40*60*ticks_per_second; }
    bool life_steal_active() const { return !restricted_healing() && rank(Upgrade::LifeSteal); }
    bool life_regen_due() const { return phase==Phase::Running && ticks && ticks%life_regen_interval()==0; }
    unsigned auto_rupees() const { return ticks && ticks%(5*ticks_per_second)==0 ? talents.rank(Talent::AutoRupees) : 0; }
    unsigned jump_splash_percent() const {
        const auto level = std::min(30u, rank(Upgrade::JumpStrike));
        return level ? 30 + 5 * (level - 1) : 0;
    }
    unsigned jump_splash_power() const {
        return jump_splash_percent() ? std::max(1u, (melee_power() * jump_splash_percent() + 50) / 100) : 0;
    }
    unsigned jump_cooldown() const { return cooldown(6 * ticks_per_second); }
    bool roll_jump_strike() { return master_sword && rank(Upgrade::JumpStrike) && next_random() % 100 < 20; }
    unsigned melee_targets() const { return dual_wield ? std::min(5u, 2 + dual_wield / 10) : 1; }
    unsigned extra_projectiles(Upgrade weapon=Upgrade::Count) const { return std::min(3u,base_rank(Upgrade::Multishot)+gear_levels(Upgrade::Multishot,gear_family(weapon))); }
    unsigned ranged_shots(Upgrade weapon=Upgrade::Count) const { return 1 + extra_projectiles(weapon); }
    unsigned arrow_targets() const { return 1 + std::min(10u,rank(Upgrade::Piercing))+gear_bonus(Upgrade::Bow,GearStat::Piercing); }
    float knockback_distance(bool heavy, bool elite, DamageSource source=DamageSource::Other) const {
        return ((heavy ? 240.f : 0.f) + 40.f*std::min(10u,rank(Upgrade::Knockback))) * (elite ? 0.f : 1.f) * (1.f+gear_knockback_percent(source)/100.f);
    }
    unsigned bow_base_shots() const { const auto level=rank(Upgrade::Bow); return !level ? 0 : level>=20 ? 5 : level>=10 ? 3 : 1; }
    unsigned bow_shots() const { return bow_base_shots() ? bow_base_shots()+extra_projectiles(Upgrade::Bow) : 0; }
    float bow_fan_degrees() const { return bow_base_shots() ? (bow_base_shots()-1)*11.25f : 0.f; }
    static float projectile_spread(unsigned index, unsigned count, float degrees) {
        if (count < 2 || index >= count) return 0;
        const int halfSteps=count%2 ? (index%2 ? int(index+1) : -int(index)) :
            (index%2 ? -int(index) : int(index+1));
        return float(halfSteps)*degrees*3.14159265358979323846f/(180.f*2.f*(count-1));
    }
    float arrow_spread(unsigned index) const { return projectile_spread(index,bow_shots(),bow_fan_degrees()); }
    float wave_spread(unsigned index) const { return projectile_spread(index,ranged_shots(),12.f*extra_projectiles()); }
    bool great_spin(Upgrade weapon) const { return (weapon==Upgrade::Wave || weapon==Upgrade::Spin) && rank(weapon)>=15; }
    const char* upgrade_name(Upgrade u) const { return u==Upgrade::Spin && great_spin(u) ? "Great Spin Attack" : names[unsigned(u)]; }
    float wave_radius() const { return radius(great_spin(Upgrade::Wave) ? 100.f : 65.f); }
    // Center-out emission preserves a balanced, evenly spaced fan, including
    // even counts. Three arrows cover 22.5 degrees; five cover -22.5..+22.5.
    static float bow_spread(unsigned index, unsigned count) {
        count=std::clamp(count,1u,5u);if(index>=count)return 0;
        const int halfSteps=count%2 ? (index%2 ? int(index+1) : -int(index)) :
            (index%2 ? -int(index) : int(index+1));
        return float(halfSteps)*(5.625f*3.14159265358979323846f/180.f);
    }
    unsigned boomerang_hops() const { return rank(Upgrade::Boomerang) ? std::min(10u,1+(rank(Upgrade::Boomerang)-1)/3)+gear_bonus(Upgrade::Boomerang,GearStat::Hops) : 0; }
    bool owns(ChestReward reward) const {
        if (reward<=ChestReward::Ball) return rank(chest_weapon(reward))!=0;
        if (reward==ChestReward::MasterSword) return master_sword!=0;
        if (reward==ChestReward::DarkLink) return dark_link!=0;
        if (reward==ChestReward::RedPotion) return red_potion;
        if (reward==ChestReward::OrdonSword) return dual_wield != 0;
        if (reward==ChestReward::Shield) return shield != 0;
        return false;
    }
    unsigned equipment_count() const {
        unsigned count=0;for(unsigned i=0;i<unsigned(ChestReward::Count);++i) count+=owns(ChestReward(i));
        return count;
    }
    unsigned reroll_cost() const { return infinite_rerolls() || free_rerolls ? 0 : 50*(paid_rerolls+1); }
    bool pay_reroll(unsigned& rupees, unsigned reserved=0) {
        if ((phase!=Phase::Upgrade && phase!=Phase::TreasureChoice) || rupees<reserved || rupees-reserved<reroll_cost()) return false;
        rupees-=reroll_cost(); if(!infinite_rerolls()){if(free_rerolls)--free_rerolls;else ++paid_rerolls;}return true;
    }
    bool banish(Upgrade u) {
        if (!can_banish() || phase!=Phase::Upgrade || u>=Upgrade::Count || !eligible(u)) return false;
        if(!infinite_banishes()) --banishes;
        banned_upgrades[unsigned(u)]=true;
        if(u==Upgrade::MasterSword)banned_items[unsigned(ChestReward::MasterSword)]=true;
        if(u==Upgrade::DualWield)banned_items[unsigned(ChestReward::OrdonSword)]=true;
        if(u>=Upgrade::Bow && u<=Upgrade::Ball)banned_items[unsigned(u)-unsigned(Upgrade::Bow)]=true;
        return true;
    }
    bool banish(ChestReward reward, bool bottles) {
        if (!can_banish() || phase!=Phase::TreasureChoice || !chest_eligible(reward,bottles)) return false;
        if(!infinite_banishes()) --banishes;
        banned_items[unsigned(reward)]=true;
        if(reward==ChestReward::MasterSword)banned_upgrades[unsigned(Upgrade::MasterSword)]=true;
        if(reward==ChestReward::OrdonSword)banned_upgrades[unsigned(Upgrade::DualWield)]=true;
        const auto weapon=chest_weapon(reward);if(weapon!=Upgrade::Count)banned_upgrades[unsigned(weapon)]=true;
        return true;
    }
    static constexpr unsigned master_sword_max_rank = 30;
    static constexpr unsigned dark_max_rank = 30;
    unsigned dark_interval() const { return 20 * ticks_per_second; }
    // Odd levels increase damage; even levels increase duration. Level 30 is permanent.
    unsigned dark_duration() const { return (5 + std::min(dark_max_rank - 1, dark_link) / 2) * ticks_per_second; }
    unsigned dark_damage_percent() const { return dark_link ? 130 + 5 * ((std::min(dark_max_rank, dark_link) - 1) / 2) : 100; }
    bool dark_permanent() const { return dark_link >= dark_max_rank; }
    bool dark_active() const { return dark_link && (dark_permanent() || dark_remaining); }
    unsigned dark_power(unsigned base) const {
        return dark_active() ? unsigned((uint64_t(base) * dark_damage_percent() + 50) / 100) : base;
    }
    bool chest_eligible(ChestReward reward, bool bottle_available = false) const {
        if (reward>=ChestReward::Count || banned_items[unsigned(reward)]) return false;
        if (!owns(reward) && equipment_count()>=4) return false;
        if (reward <= ChestReward::Ball) return rank(chest_weapon(reward)) < upgrade_cap(chest_weapon(reward));
        if (reward == ChestReward::MasterSword) return master_sword < master_sword_max_rank;
        if (reward == ChestReward::OrdonSword) return dual_wield < upgrade_cap(Upgrade::DualWield);
        if (reward == ChestReward::DarkLink) return !dark_permanent();
        if (reward == ChestReward::Shield) return shield < 10;
        return reward == ChestReward::RedPotion && bottle_available;
    }
    bool chest_rewards_left(bool bottle_available = false) const {
        for (unsigned i = 0; i < unsigned(ChestReward::Count); ++i)
            if (chest_eligible(ChestReward(i), bottle_available)) return true;
        return false;
    }
    std::array<ChestReward, 3> roll_chest_offers(bool bottle_available = false) {
        std::array<ChestReward, unsigned(ChestReward::Count)> pool{};
        unsigned count = 0;
        for (unsigned i = 0; i < pool.size(); ++i)
            if (chest_eligible(ChestReward(i), bottle_available)) pool[count++] = ChestReward(i);
        std::array<ChestReward, 3> result;
        result.fill(ChestReward::Count);
        for (auto& reward : result) {
            if (!count) break;
            const unsigned choice = next_random() % count;
            reward = pool[choice]; pool[choice] = pool[--count];
        }
        return result;
    }
    unsigned chest_gain(ChestReward reward, Rarity rarity, unsigned bottle_capacity = 0) const {
        unsigned remaining = 0;
        if (reward <= ChestReward::Ball) remaining = upgrade_cap(chest_weapon(reward)) - std::min(upgrade_cap(chest_weapon(reward)), rank(chest_weapon(reward)));
        else if (reward == ChestReward::MasterSword) remaining = master_sword_max_rank - std::min(master_sword_max_rank, master_sword);
        else if (reward == ChestReward::Shield) remaining = 10 - std::min(10u, shield);
        else if (reward == ChestReward::OrdonSword) remaining = upgrade_cap(Upgrade::DualWield) - std::min(upgrade_cap(Upgrade::DualWield), dual_wield);
        else if (reward == ChestReward::DarkLink) remaining = dark_max_rank - std::min(dark_max_rank, dark_link);
        else if (reward == ChestReward::RedPotion) remaining = bottle_capacity;
        return std::min(reward_units(rarity), remaining);
    }
    unsigned chest_rupees(ChestReward reward, Rarity rarity, unsigned bottle_capacity = 0) const {
        return 20 * (reward_units(rarity) - chest_gain(reward, rarity, bottle_capacity));
    }
    // Apply exactly the selected reward. No reroll, payment or rank change on
    // an invalid/stale selection; presentation commits this preview only once.
    // A key waives payment, not the ordinary chest's price progression. The
    // presentation layer reserves/commits the native key with this preview.
    bool buy_chest(unsigned& rupees, ChestReward reward, unsigned bottle_capacity = 0, Rarity rarity = Rarity::Common, bool free = false, bool fixed = false, bool key = false) {
        const unsigned price = key ? 0 : chest_price(free, fixed);
        if (phase != Phase::Treasure || rupees < price || !chest_eligible(reward, bottle_capacity != 0)) return false;
        const unsigned gain = chest_gain(reward, rarity, bottle_capacity);
        rupees = rupees - price + chest_rupees(reward, rarity, bottle_capacity);
        if (!free && !fixed) ++chests_opened;
        if (reward == ChestReward::RedPotion) red_potion = true;
        if (reward == ChestReward::MasterSword) master_sword += gain;
        else if (reward == ChestReward::OrdonSword) dual_wield += gain;
        else if (reward == ChestReward::DarkLink) dark_link += gain;
        else if (reward == ChestReward::Shield) shield += gain;
        else if (reward <= ChestReward::Ball) ranks[unsigned(chest_weapon(reward))] += gain;
        return true;
    }
    unsigned item_cooldown(Upgrade weapon) const {
        const unsigned base = weapon==Upgrade::Bow?75:(weapon==Upgrade::Boomerang || weapon==Upgrade::Lightning)?85:weapon==Upgrade::Bombs?110:100;
        const unsigned floor=weapon==Upgrade::Bow?20:(weapon==Upgrade::Boomerang || weapon==Upgrade::Lightning)?30:weapon==Upgrade::Bombs?45:40;
        const unsigned normal=std::max(floor,cooldown(base*100/(100+5*(std::max(1u,rank(weapon))-1)/2),weapon));
        return std::max(1u,normal*100/(100+gear_bonus(weapon,GearStat::Speed)));
    }
    bool tick(bool playable) {
        if (phase != Phase::Running || !playable) return false;
        ++ticks;
        if (gibdo_scream_cooldown) --gibdo_scream_cooldown;
        if (horde_remaining) --horde_remaining;
        // Independent one-second trials: E(events / 5 minutes) = 300 / 150 = 2.
        // Overlaps add time, never multiply the active spawn boost.
        if ((endless || ticks < run_duration()) && ticks % ticks_per_second == 0 && next_random() % 150 == 0)
            horde_remaining += horde_duration;
        if (!endless && ticks >= run_duration()) horde_remaining = 0;
        if (dark_remaining) --dark_remaining;
        if (dark_link && !dark_permanent() && ++dark_clock >= dark_interval()) {
            dark_clock = 0; dark_remaining = dark_duration();
        }
        return true;
    }
    // Fixed map/time curves: never scale from the player's actual gear or luck.
    // Normal runs stop at 20m. Endless extrapolates the last map segment and
    // compounds its own minute bonus, within native signed-16-bit HP limits.
    unsigned enemy_health(unsigned base, bool elite, unsigned map=0, bool finalBoss=false) const {
        constexpr uint64_t scale=1000000;
        constexpr unsigned times[]{0,60,300,600,900,1200};
        constexpr uint64_t curves[5][6]{
            {1000000,1000000,4092209,10758235,19215847,28296574},
            {1000000,1082457,8761003,25392559,49660833,67439452},
            {1000000,1042207,8387484,23263089,44298512,59427164},
            {1000000,1000000,5696760,14616823,24382634,34535097},
            {1000000,1097053,4979767,12787987,22408492,32725714}};
        constexpr unsigned finalHp[]{2300,5400,4800,2800,2600};
        map=map<5?map:0;
        const unsigned time=endless?ticks:std::min(ticks,duration);
        unsigned i=1;while(i<5 && time>times[i]*ticks_per_second)++i;
        const uint64_t factor=curves[map][i-1]+(curves[map][i]-curves[map][i-1])*
            (time-times[i-1]*ticks_per_second)/((times[i]-times[i-1])*ticks_per_second);
        const uint64_t hp=finalBoss ? uint64_t(finalHp[map])*scale*(endless?double(factor)/curves[map][5]:1.0) : elite ?
            std::max(120*scale,32*factor) : std::clamp(base,1u,32767u)*factor;
        return unsigned(std::min(32767.0,std::floor(double(hp)*RunOptions::difficulty(elite || finalBoss ? options.boss_hp : options.enemy_hp,5)*endless_factor()/scale+.5)));
    }
    // Only the four scheduled encounter slots get the fixed doubled HP budget.
    // Boss difficulty applies to all elites; ordinary actors use enemy difficulty.
    unsigned spawn_enemy_health(unsigned base, bool elite, unsigned map, int eliteSlot) const {
        const bool timedBoss=elite && eliteSlot>=0 && unsigned(eliteSlot)<timed_elite_count;
        const unsigned hp=enemy_health(base,elite,map,timedBoss && unsigned(eliteSlot)==timed_elite_count-1);
        return timedBoss ? std::min(32767u,hp*2) : hp;
    }
    // Fixed population ceiling; time and Horde difficulty only affect spawn cadence/batches.
    unsigned target_population() const { return 345u; }
    unsigned spawn_batch() {
        spawn_progress+=unsigned(RunOptions::difficulty(options.horde,5)*2)*(horde_remaining?3:1);
        const unsigned batch=spawn_progress/2;spawn_progress%=2;return batch;
    }
    unsigned spawn_interval() const { return std::max(15u, 75u - std::min(60u, ticks / (10 * ticks_per_second))); }
    unsigned cooldown_percent(Upgrade weapon=Upgrade::Count) const {
        const auto rank = std::min(30u,base_rank(Upgrade::Haste)+gear_levels(Upgrade::Haste,gear_family(weapon)));
        return 10000/(100+5*rank/2);
    }
    unsigned cooldown(unsigned base,Upgrade weapon=Upgrade::Count) const { return std::max(6u, base * cooldown_percent(weapon) / 100); }
    unsigned shield_cooldown() const {return std::max(30u,cooldown(9000/(100+3*rank(Upgrade::ShieldAttack))));}
    bool normal_kill_drops() {
        drop_progress+=2;const unsigned threshold=unsigned(RunOptions::difficulty(options.horde,5)*2);
        if(drop_progress<threshold)return false;
        drop_progress-=threshold;return true;
    }
    // Independent bonus, once per ordinary reward opportunity: 2%, up to 3% with Luck.
    bool roll_exp_magnet() { return next_random()%10000<lucky_chance(200); }
    // At base Luck: 2.5% heart, 45% rupee, 52.5% nothing. A second lucky
    // roll chooses rupee value, so Luck improves both frequency and quality.
    enum class Drop { None, Heart, GreenRupee, BlueRupee, YellowRupee, RedRupee, PurpleRupee, OrangeRupee, SilverRupee, Count };
    Drop roll_drop() {
        const unsigned roll = next_random() % 10000;
        if (roll < lucky_chance(250)) return restricted_healing()?Drop::None:Drop::Heart;
        if (roll < lucky_chance(500) || roll >= lucky_chance(5000)) return Drop::None;
        const unsigned color = next_random() % 10000;
        unsigned threshold = 0;
        constexpr std::array<unsigned, 6> chances{5, 15, 80, 200, 400, 1000};
        for (unsigned i = 0; i < chances.size(); ++i) {
            threshold += lucky_chance(chances[i]);
            if (color < threshold) return Drop(unsigned(Drop::SilverRupee) - i);
        }
        return Drop::GreenRupee;
    }
    enum class PotDrop { HeartPiece, GreenRupee, BlueRupee, YellowRupee, RedRupee,
                         PurpleRupee, OrangeRupee, SilverRupee, SmallKey, Count };
    PotDrop roll_pot_drop() {
        const unsigned rare = next_random()%10000, chance = lucky_chance(200);
        if (rare < chance) return restricted_healing()?PotDrop::GreenRupee:PotDrop::HeartPiece;
        if (rare < 2*chance) return PotDrop::SmallKey;
        const unsigned color=next_random()%10000;
        constexpr std::array<unsigned,6> chances{10,40,150,600,1000,2200};
        unsigned threshold=0;
        for (unsigned i=0;i<chances.size();++i) {
            threshold+=lucky_chance(chances[i]);
            if (color<threshold) return PotDrop(unsigned(PotDrop::SilverRupee)-i);
        }
        return PotDrop::GreenRupee;
    }
    unsigned block_chance() const { return std::min(10u, shield) * 5; }
    unsigned shield_tier() const { return shield >= 7 ? 2 : shield >= 4 ? 1 : 0; }
    unsigned sword_bonus() const { return gear_bonus(GearFamily::Sword,GearStat::Damage) + (master_sword ? 2 + master_sword : 0); }
    unsigned attack_power(unsigned weapon) const { return 2 + rank(Upgrade(weapon)) + rank(Upgrade::Power) + (weapon < 2 ? sword_bonus() : gear_bonus(Upgrade(weapon),GearStat::Damage)) + bonus_damage(); }
    float area_radius(float base,Upgrade weapon=Upgrade::Count) const { return base*std::sqrt(radius(1.f,weapon)); }
    float hurricane_scale() const { return rank(Upgrade::Hurricane) ? 2.f : 1.f; }
    float bomb_arrow_radius() const { return area_radius(170+12*5); }
    float shockwave_radius() const { return radius(55+8*5); }
    float dark_aura_radius() const { return radius(300.f); }
    float bomb_radius() const { return area_radius(170+12*rank(Upgrade::Bombs),Upgrade::Bombs)*(1.f+gear_bonus(Upgrade::Bombs,GearStat::BlastRadius)/100.f); }
    float shield_radius() const { return area_radius(60+3*rank(Upgrade::ShieldAttack)); }
    float item_range(Upgrade weapon) const { return ((weapon==Upgrade::Bow || weapon==Upgrade::Boomerang) ? radius(item_target_range,weapon) : item_target_range)*(1.f+gear_bonus(weapon,GearStat::Reach)/100.f); }
    float boomerang_hop_range() const { return radius(800.f,Upgrade::Boomerang)*(1.f+gear_bonus(Upgrade::Boomerang,GearStat::HopReach)/100.f); }
    float radius(float base,Upgrade weapon=Upgrade::Count) const { return base * (1.0f + 0.075f * std::min(30u,base_rank(Upgrade::Reach)+gear_levels(Upgrade::Reach,gear_family(weapon)))); }
    float melee_reach(float base) const { return base * (1.f + .15f * rank(Upgrade::Reach))*(1.f+gear_bonus(GearFamily::Sword,GearStat::Reach)/100.f); }
    float melee_area_radius(float base) const { return base * (1.f + 2.f*(std::sqrt(radius(1.f))-1.f))*(1.f+gear_bonus(GearFamily::Sword,GearStat::Reach)/100.f); }
    float melee_radius(float base) const { return 1.3f * melee_reach(base); }
    unsigned multi_strikes() const { return dual_wield ? std::min(3u,rank(Upgrade::MultiStrike)) : 0; }
    unsigned lightning_hops() const { return rank(Upgrade::Lightning) ? 2*std::min(10u,1+(rank(Upgrade::Lightning)-1)/3) : 0; }
    unsigned lightning_power() const { return attack_power(unsigned(Upgrade::Lightning))+master_sword/2+gear_bonus(GearFamily::Sword,GearStat::Damage); }
    float lightning_hop_range() const { return melee_reach(800.f); }
    static constexpr float lightning_speed=42.f*5;
    static constexpr unsigned lightning_chance=2000;
    unsigned lightning_proc_chance() const { return lightning_chance*luck_percent()/100; }
    void end(bool won) { if (phase == Phase::Running || phase == Phase::Upgrade || phase == Phase::TreasureChoice || phase == Phase::Treasure) phase = won ? Phase::Victory : Phase::Defeat; }
};

// A presentation snapshot, never used to resume gameplay. Store the awarded
// score explicitly so future balance changes cannot rewrite historical scores.
struct HighScoreRun {
    Run run;
    uint32_t score=0, meta_level=1, unlock_points=0;
    bool present() const { return run.phase==Phase::Victory || run.phase==Phase::Defeat; }
    bool cheated() const { return run.options.allow_cheats || run.modded; }
    static constexpr unsigned legacy_word_count=20+unsigned(Upgrade::Rupees)+2*damage_source_count;
    static constexpr unsigned word_count=legacy_word_count+3;
    using Words=std::array<uint32_t,word_count>;
    Words encode() const {
        Words words{};
        if(!present() || run.debug_run)return words;
        unsigned i=0;const auto put=[&](uint32_t n){words[i++]=n;};
        put(run.phase==Phase::Victory?2:1);put(score);put(meta_level);put(unlock_points);
        // v15 stores half steps and Boss HP in spare flag bits, preserving offsets.
        unsigned halves=0,bit=0;
        for(float value:{run.options.enemy_hp,run.options.enemy_damage,run.options.enemy_speed,run.options.horde})
            halves|=(unsigned(value*2)&1u)<<bit++;
        put(unsigned(run.options.allow_cheats)|(unsigned(run.modded)<<1)|(unsigned(run.red_potion)<<2)|(unsigned(run.endless)<<3)|
            (unsigned(run.options.legacy_difficulty)<<4)|(unsigned(run.options.weighted_difficulty)<<5)|(halves<<8)|(unsigned(run.options.boss_hp*2)<<16));
        put(run.ticks);put(run.kills);put(run.level);put(run.timed_elites);put(run.guard_elites);put(run.chests_opened);
        put(run.master_sword);put(run.shield);put(run.dual_wield);put(run.dark_link);
        put(run.options.enemy_hp);put(run.options.enemy_damage);put(run.options.enemy_speed);put(run.options.horde);
        put(run.debug_options().run_minutes);
        for(auto rank:run.ranks)put(rank);
        for(auto damage:run.damage.dealt){put(uint32_t(damage));put(uint32_t(damage>>32));}
        put(unsigned(run.twilit_rank()*2));put(run.endless_start);put(run.normal_score);
        return words;
    }
    bool decode(std::span<const uint32_t,word_count> words, unsigned version=16) {
        if(!words[0]) {
            if(std::any_of(words.begin(),words.end(),[](auto word){return word!=0;}))return false;
            *this={};return true;
        }
        if(words[0]>2 || !words[2] || words[3]>=words[2] || (words[4]&~(version>=16?0x001f0f3fu:version==15?0x001f0f1fu:15u)))return false;
        HighScoreRun next;auto& r=next.run;
        unsigned i=0;const auto get=[&](){return words[i++];};
        r.phase=get()==2?Phase::Victory:Phase::Defeat;next.score=get();next.meta_level=get();next.unlock_points=get();
        const auto flags=get();r.options.allow_cheats=flags&1;r.modded=flags&2;r.red_potion=flags&4;r.endless=flags&8;
        r.ticks=get();r.kills=get();r.level=get();r.timed_elites=get();r.guard_elites=get();r.chests_opened=get();
        r.master_sword=get();r.shield=get();r.dual_wield=get();r.dark_link=get();
        r.options.enemy_hp=get();r.options.enemy_damage=get();r.options.enemy_speed=get();r.options.horde=get();
        r.options.debug.run_minutes=get();
        r.options.legacy_difficulty=version<15||(flags&16u);
        r.options.weighted_difficulty=version==15?!r.options.legacy_difficulty:version>=16&&(flags&32u);
        if(r.options.legacy_difficulty&&r.options.weighted_difficulty)return false;
        const bool historical=r.options.legacy_difficulty||r.options.weighted_difficulty;
        if(version>=15) {
            unsigned bit=8;
            for(float* value:{&r.options.enemy_hp,&r.options.enemy_damage,&r.options.enemy_speed,&r.options.horde})
                *value+=((flags>>bit++)&1u)*.5f;
            r.options.boss_hp=float((flags>>16)&31u)*.5f;
            if(r.options.boss_hp<1||r.options.boss_hp>(historical?10:5))return false;
        }
        if(r.options.enemy_hp<1 || r.options.enemy_hp>(historical?10:5) || r.options.enemy_damage<1 || r.options.enemy_damage>(historical?10:5) ||
           r.options.enemy_speed<1 || r.options.enemy_speed>(historical?3:5) || r.options.horde<1 || r.options.horde>5 ||
           (r.options.debug.run_minutes!=5 && r.options.debug.run_minutes!=10 && r.options.debug.run_minutes!=20))return false;
        for(auto& rank:r.ranks)rank=get();
        uint64_t total=0;
        for(auto& damage:r.damage.dealt) {
            const uint64_t lo=get(),hi=get();damage=lo|(hi<<32);
            if(damage>UINT64_MAX-total)return false;
            total+=damage;
        }
        r.rank_snapshot=float(get())/(version>=15?2.f:1.f);r.endless_start=get();r.normal_score=get();
        // Older snapshots either omitted rank or stored the unadjusted sum.
        if(version<10)r.rank_snapshot=r.options.twilit_rank();
        else if(version==10) {
            if(r.rank_snapshot!=r.options.twilit_rank()+4)return false;
            r.rank_snapshot-=4;
        }
        if(r.rank_snapshot!=r.options.twilit_rank() || (r.endless && r.endless_start>r.ticks) ||
           (!r.endless && (r.endless_start || r.normal_score)))return false;
        *this=next;return true;
    }
};

// Explicit little-endian encoding, independent of ABI padding and game version.
struct Records {
    uint32_t runs = 0, wins = 0, best_ticks = 0, best_kills = 0, best_level = 0;
    // Retain the unclassified legacy aggregate for migration; never promote it
    // into Ranked without an integrity-tagged result snapshot.
    std::array<uint32_t,5> high_scores{};
    std::array<HighScoreRun,5> ranked{}, cheated{}, endless_ranked{}, endless_cheated{};
    const HighScoreRun& high_score(unsigned map,bool assisted,bool endless=false) const { return (endless?(assisted?endless_cheated:endless_ranked):(assisted?cheated:ranked))[map]; }
    void clear_high_scores() { high_scores={};ranked={};cheated={};endless_ranked={};endless_cheated={}; }
    void delete_high_score(unsigned map,bool assisted,bool endless=false) {
        if(map>=high_scores.size())return;
        (endless?(assisted?endless_cheated:endless_ranked):(assisted?cheated:ranked))[map]={};
        if(!endless)high_scores[map]=std::max(ranked[map].score,cheated[map].score);
    }
    // v12 appends the two equipment talents after the unchanged v10/v11
    // header and score snapshots; legacy offsets must never consume new ranks.
    // v13 appends one Chain Spin rank per snapshot after the v12 talent words.
    // v14 appends independent boss HP settings; all prior offsets stay fixed.
    static constexpr unsigned legacy_talent_count=19;
    static constexpr unsigned v11_word_count=34+20*HighScoreRun::word_count;
    static constexpr unsigned v11_byte_count=4*v11_word_count;
    static constexpr unsigned v12_word_count=v11_word_count+2, v12_byte_count=4*v12_word_count;
    static constexpr unsigned v13_word_count=v12_word_count+20, v13_byte_count=4*v13_word_count;
    static constexpr unsigned word_count=v13_word_count+20;
    static constexpr unsigned v9_byte_count=4*(34+10*HighScoreRun::legacy_word_count);
    static constexpr unsigned byte_count=4*word_count;
    static constexpr unsigned v8_score_words=20+30+2*18;
    static constexpr unsigned v8_byte_count=4*(34+10*v8_score_words);
    uint32_t cleared_maps=0;
    bool map_unlocked(unsigned map) const {
        return map<high_scores.size() && (map==0 || (cleared_maps & (1u<<(map-1))));
    }
    uint64_t total_xp=0;
    Talents talents;
    unsigned level() const { return 1+meta_levels(total_xp); }
    unsigned available_points() const { return meta_levels(total_xp)-std::min(meta_levels(total_xp),talents.spent()); }
    uint64_t level_xp() const { return total_xp-meta_threshold(level()-1); }
    unsigned next_level_xp() const { return 2000+500*(level()-1); }
    bool set_meta_level(unsigned requested) {
        // Preserve purchased talents and the save's XP bounds. This changes
        // ordinary progression, not a run override or a locked debug level.
        const auto target=std::clamp(requested,1+talents.spent(),1+meta_levels(max_meta_xp));
        if(target==level())return false;
        total_xp=meta_threshold(target-1);
        return true;
    }
    bool buy(Talent t) { if(!available_points() || !talents.can_buy(t))return false;++talents.ranks[unsigned(t)];return true; }
    void finish(const Run& run, unsigned map=0) {
        if (run.debug_run || (run.phase != Phase::Victory && run.phase != Phase::Defeat)) return;
        if(map>=high_scores.size())return;
        if(!run.endless)high_scores[map]=std::max(high_scores[map],run.score());
        total_xp=std::min(max_meta_xp,total_xp+run.earned_meta_xp());
        if(!run.endless){++runs; wins += run.phase == Phase::Victory;}
        if(run.phase==Phase::Victory)cleared_maps|=1u<<map;
        best_ticks = std::max(best_ticks, run.ticks);
        best_kills = std::max(best_kills, run.kills);
        best_level = std::max(best_level, run.level);
        const bool assisted=run.options.allow_cheats || run.modded;
        auto& best=(run.endless?(assisted?endless_cheated:endless_ranked):(assisted?cheated:ranked))[map];
        if(!best.present() || run.score()>best.score)best={run,run.score(),level(),available_points()};
    }
    std::array<uint8_t,byte_count> encode() const {
        std::array<uint32_t,word_count> words{0x56525348,16,runs,wins,best_ticks,best_kills,best_level};
        for(unsigned i=0;i<5;++i)words[7+i]=high_scores[i];
        words[12]=uint32_t(total_xp);words[13]=uint32_t(total_xp>>32);
        for(unsigned i=0;i<legacy_talent_count;++i)words[14+i]=talents.ranks[i];
        words[v11_word_count]=talents.rank(Talent::EquipmentDropRate);
        words[v11_word_count+1]=talents.rank(Talent::EquipmentRarity);
        words[33]=cleared_maps;
        unsigned offset=34, extension=v12_word_count, bossExtension=v13_word_count;
        for(const auto* category:{&ranked,&cheated,&endless_ranked,&endless_cheated})for(const auto& best:*category) {
            words[extension++]=best.present()&&!best.run.debug_run?best.run.chain_spin:0;
            words[bossExtension++]=best.present()&&!best.run.debug_run?best.run.options.boss_hp:0;
            const auto record=best.encode();
            std::copy(record.begin(),record.end(),words.begin()+offset);offset+=record.size();
        }
        std::array<uint8_t,byte_count> bytes{};
        for(unsigned i=0;i<words.size();++i)for(unsigned b=0;b<4;++b)bytes[i*4+b]=uint8_t(words[i]>>(8*b));
        return bytes;
    }
    bool decode(std::span<const uint8_t> bytes) {
        if(bytes.size()!=28 && bytes.size()!=100 && bytes.size()!=104 && bytes.size()!=108 && bytes.size()!=124 && bytes.size()!=132 && bytes.size()!=136 && bytes.size()!=v8_byte_count && bytes.size()!=v9_byte_count && bytes.size()!=v11_byte_count && bytes.size()!=v12_byte_count && bytes.size()!=v13_byte_count && bytes.size()!=byte_count)return false;
        std::array<uint32_t,word_count> words{};
        for(unsigned i=0;i<bytes.size()/4;++i)for(unsigned b=0;b<4;++b)words[i]|=uint32_t(bytes[i*4+b])<<(8*b);
        if(words[0]!=0x56525348 || words[3]>words[2])return false;
        if(!((words[1]==1 && bytes.size()==28)||(words[1]==2 && bytes.size()==100)||(words[1]==3 && bytes.size()==104)||(words[1]==4 && bytes.size()==108)||(words[1]==5 && bytes.size()==124)||(words[1]==6 && bytes.size()==132)||(words[1]==7 && bytes.size()==136)||(words[1]==8 && bytes.size()==v8_byte_count)||(words[1]==9 && bytes.size()==v9_byte_count)||((words[1]==10 || words[1]==11) && bytes.size()==v11_byte_count)||(words[1]==12 && bytes.size()==v12_byte_count)||(words[1]==13 && bytes.size()==v13_byte_count)||((words[1]==14 || words[1]==15 || words[1]==16) && bytes.size()==byte_count)))return false;
        Records next;
        // Legacy saves have aggregate wins only; scores also include defeats.
        if(words[1]>=7){
            if(words[33] & ~((1u<<high_scores.size())-1))return false;
            next.cleared_maps=words[33];
        }
        next.runs=words[2];next.wins=words[3];next.best_ticks=words[4];next.best_kills=words[5];next.best_level=words[6];
        if(words[1]>=2){
            for(unsigned i=0;i<5;++i)next.high_scores[i]=words[7+i];
            next.total_xp=uint64_t(words[12])|(uint64_t(words[13])<<32);
            for(unsigned i=0;i<legacy_talent_count;++i)next.talents.ranks[i]=words[14+i];
            if(words[1]>=12) {
                next.talents.ranks[unsigned(Talent::EquipmentDropRate)]=words[v11_word_count];
                next.talents.ranks[unsigned(Talent::EquipmentRarity)]=words[v11_word_count+1];
            }
            if(next.total_xp>max_meta_xp || !next.talents.valid() || next.talents.spent()>meta_levels(next.total_xp))return false;
        }
        if(words[1]>=8) {
            unsigned offset=34, extension=v12_word_count, bossExtension=v13_word_count;
            for(unsigned category=0;category<(words[1]>=10?4u:2u);++category)for(auto& best:category==0?next.ranked:category==1?next.cheated:category==2?next.endless_ranked:next.endless_cheated) {
                HighScoreRun::Words snapshot{};
                if(words[1]==8) {
                    std::copy_n(words.begin()+offset,20+30,snapshot.begin());
                    std::copy_n(words.begin()+offset+20+30,2*18,snapshot.begin()+20+unsigned(Upgrade::Rupees));
                } else std::copy_n(words.begin()+offset,words[1]==9?HighScoreRun::legacy_word_count:snapshot.size(),snapshot.begin());
                if(!best.decode(snapshot,words[1]) || (best.present() && (best.cheated()!=bool(category%2) || best.run.endless!=(category>=2))))return false;
                if(words[1]>=13) {const auto spin=words[extension++];if(spin>1||(!best.present()&&spin))return false;best.run.chain_spin=spin;}
                if(best.present()&&words[1]<15)best.run.options.boss_hp=best.run.options.enemy_hp; // Legacy HP applied to both.
                if(words[1]>=14) {
                    const auto bossHp=words[bossExtension++];
                    if(best.present()?(bossHp<1 || bossHp>10):bossHp!=0)return false;
                    if(best.present()) {
                        if(words[1]<15)best.run.options.boss_hp=bossHp;
                        else if(unsigned(best.run.options.boss_hp)!=bossHp)return false;
                    }
                }
                offset+=words[1]==8?v8_score_words:words[1]==9?HighScoreRun::legacy_word_count:HighScoreRun::word_count;
            }
        }
        *this=next;return true;
    }
};
} // namespace dawnlight::survivors
