#include "../src/survivors_rules.hpp"
#include <cassert>
#include <iostream>
using namespace dawnlight::survivors;
int main() {
    Run run;
    assert(std::all_of(run.ranks.begin(),run.ranks.end(),[](auto r){return r==0;}));
    std::array<unsigned,4> drops{};
    run.start(123);
    for(unsigned i=0;i<10000;++i) ++drops[static_cast<unsigned>(run.roll_drop())];
    assert(drops[1]>350 && drops[1]<650); // Rare hearts, not an every-kill heal.
    assert(drops[2]>3500 && drops[2]<4500 && drops[3]>350 && drops[3]<650);
    run = {};
    assert(!run.tick(true));
    run.start(0);
    for (unsigned i = 0; i < duration; ++i) {
        const auto before = run.ticks;
        assert(!run.tick(false) && run.ticks == before);
        assert(run.tick(true));
        assert(run.target_population() < enemy_capacity);
        assert(run.spawn_interval() >= 15);
    }
    assert(run.ticks == duration && run.phase == Phase::Running);
    run.xp = 100;
    assert(run.level_up());
    const auto offers = run.offers;
    assert(offers[0] != offers[1] && offers[0] != offers[2] && offers[1] != offers[2]);
    assert(!run.tick(true) && run.ticks == duration);
    assert(!run.level_up() && run.offers == offers); // Closing/reopening cannot reroll.
    assert(!run.choose(3));
    const auto previous = run.rank(offers[0]);
    assert(run.choose(0));
    if (offers[0] != Upgrade::Rupees) assert(run.rank(offers[0]) == previous + 1);
    assert(!run.choose(0)); // Double activation is ignored.
    assert(run.level_up()); // Excess XP remains available.
    for (uint32_t seed = 1; seed < 1000; ++seed) {
        run.start(seed);
        run.ranks.fill(5); run.ranks[2] = 4; run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::Aura || offer == Upgrade::Rupees);
        assert(std::count(run.offers.begin(), run.offers.end(), Upgrade::Aura) == 1);
        run.start(seed); run.ranks.fill(5); run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::Rupees);
        assert(run.choose(2));
        assert(run.cooldown(40) == 20);
    }
    run.start(1);
    run.ranks[3] = run.ranks[4] = run.ranks[5] = run.ranks[6] = 1;
    assert(!run.eligible(Upgrade::Heart)); // Four passive slots, existing ranks can improve.
    assert(run.eligible(Upgrade::Power) && run.eligible(Upgrade::Spin) && run.eligible(Upgrade::Rupees));
    // Chest weapons cannot be acquired from level-up offers before a purchase.
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i)
        assert(!run.eligible(static_cast<Upgrade>(i)));
    unsigned wallet = 19;
    ChestReward reward = ChestReward::Count;
    run.phase = Phase::Treasure;
    const auto rng = run.random;
    assert(!run.buy_chest(wallet, reward) && wallet == 19 && run.random == rng && run.chest_cost() == 20);
    wallet = 20;
    assert(run.buy_chest(wallet, reward) && wallet == 0 && run.chest_cost() == 40);
    assert((reward == ChestReward::MasterSword && run.master_sword) ||
           (run.rank(chest_weapon(reward)) == 1 && run.eligible(chest_weapon(reward)))); // Even with four occupied passives.
    assert(!run.tick(true)); // Get Item cannot advance encounter time.
    run.start(20); run.phase = Phase::Treasure;
    wallet = 10000;
    for (unsigned n = 0; n < 21; ++n) {
        assert(run.chest_cost() == 20 * (n + 1));
        assert(run.buy_chest(wallet, reward) && run.rank(chest_weapon(reward)) <= 5);
    }
    assert(wallet == 5380 && !run.chest_rewards_left());
    assert(run.master_sword && run.sword_bonus() == 3);
    assert(run.chest_rewards_left(true)); // A refill remains available after all weapons.
    const auto maxedRng = run.random;
    assert(!run.buy_chest(wallet, reward) && wallet == 5380 && run.random == maxedRng);
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i) {
        const auto weapon = static_cast<Upgrade>(i);
        assert(run.rank(weapon) == 5 && !run.eligible(weapon));
        const auto fast = run.item_cooldown(weapon);
        run.ranks[i] = 1;
        assert(run.item_cooldown(weapon) > fast);
        run.ranks[i] = 5;
    }
    wallet = 10000;
    assert(run.buy_chest(wallet, reward, true) && reward == ChestReward::RedPotion);
    run.start(2);
    assert(!run.master_sword && run.sword_bonus() == 0);
    assert(run.chests_opened == 0 && run.chest_cost() == 20 && run.chest_rewards_left());
    wallet = 100;
    assert(!run.buy_chest(wallet, reward) && wallet == 100); // No purchase outside the reward phase.
    Records records;
    records.finish(run); assert(records.runs == 0); // Live runs cannot become records.
    run.end(false); assert(run.phase == Phase::Defeat && !run.tick(true));
    records.finish(run); assert(records.runs == 1 && records.wins == 0);
    run.end(true); assert(run.phase == Phase::Defeat); // A late kill cannot change a loss.
    run.start(42); run.ticks = duration + 100; run.kills = 123; run.level = 24; run.end(true);
    records.finish(run);
    assert(records.wins == 1 && records.runs == 2 && records.best_kills == 123);
    assert(records.best_ticks == duration + 100 && records.best_level == 24);
    Records loaded;
    auto bytes = records.encode();
    assert(loaded.decode(bytes) && loaded.encode() == bytes);
    auto corrupt = bytes; corrupt[4] = 2;
    assert(!loaded.decode(corrupt) && loaded.encode() == bytes);
    assert(!loaded.decode(std::span(bytes.data(), bytes.size() - 1)));
    corrupt = bytes; corrupt[12] = 250; assert(!loaded.decode(corrupt));
    run.start(9); assert(run.xp == 0 && run.kills == 0 && run.ticks == 0 && run.level == 1);
    assert(run.ranks[0] == 0 && run.ranks[1] == 0 && run.ranks[7] == 0);
    std::cout << "Survivors rules: timing, choices, caps, restart and records passed\n";
}
