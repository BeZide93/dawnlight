#include "../src/survivors_rules.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
using namespace dawnlight::survivors;
int main() {
    // Every half step affects rewards; Boss HP follows the Enemy HP weight.
    float RunOptions::* const fields[]{&RunOptions::enemy_hp,&RunOptions::boss_hp,&RunOptions::enemy_damage,&RunOptions::enemy_speed,&RunOptions::horde};
    constexpr float maxima[]{5,5,5,5,5},weights[]{20,20,20,20,20};
    for(unsigned i=0;i<5;++i)for(float multiplier=1;multiplier<=maxima[i];multiplier+=.5f) {
        RunOptions setting;setting.*fields[i]=multiplier;
        assert(setting.twilit_rank()==multiplier+4);
        assert(setting.score_percent()==100+(multiplier-1)*weights[i]);
    }
    RunOptions maxDifficulty{5,5,5,5,200};maxDifficulty.boss_hp=5;
    assert(maxDifficulty.twilit_rank()==25&&maxDifficulty.score_percent()==500);
    Run half;half.start(1);half.options.enemy_hp=half.options.boss_hp=half.options.enemy_damage=1.5f;
    assert(half.enemy_health(8,false)==12&&half.enemy_health(8,true)==180&&half.incoming_damage(4)==6);
    // Half Horde multipliers preserve fractional spawn and reward remainders.
    for(float multiplier=1;multiplier<=5;multiplier+=.5f)for(bool event:{false,true}) {
        half.start(1);half.options.horde=multiplier;half.horde_remaining=event;
        unsigned spawned=0,drops=0;
        for(unsigned i=0;i<90;++i)spawned+=half.spawn_batch();
        assert(spawned==unsigned(90*multiplier*(event?3:1)));
        for(unsigned i=0;i<unsigned(multiplier*20);++i)drops+=half.normal_kill_drops();
        assert(drops==20);
    }

    // Field-based map progression, with difficulty and final map rounding. Boss budgets
    // are absolute, separate from normal/elite HP, and do not grow after 20m.
    Run hp;hp.start(1);
    constexpr unsigned normalEnd[]{226,540,594,653,719};
    constexpr unsigned normalStart[]{8,8,9,10,11},eliteStart[]{120,120,132,145,160};
    constexpr unsigned permille[]{1000,1000,1100,1210,1331};
    const auto scaled=[&](unsigned hp,unsigned map){return std::min(32767u,(hp*permille[map]+500)/1000);};
    constexpr unsigned eliteEnd[]{905,2158,2374,2611,2872};
    constexpr unsigned bosses[]{2300,5400,5400,5400,5400};
    for(unsigned map=0;map<5;++map) {
        hp.ticks=0;assert(hp.enemy_health(8,false,map)==normalStart[map]&&hp.enemy_health(8,true,map)==eliteStart[map]);
        unsigned previous=0;
        for(unsigned tick=0;tick<=duration;tick+=30){hp.ticks=tick;
            unsigned now=hp.enemy_health(8,false,map);assert(now>=previous);previous=now;}
        assert(hp.enemy_health(8,false,map)==normalEnd[map]);
        assert(hp.enemy_health(8,true,map)==eliteEnd[map]);
        for(unsigned multiplier:{1u,3u,5u}) {
            hp.options.boss_hp=multiplier;
            assert(hp.enemy_health(1200,true,map,true)==scaled(bosses[map]*multiplier,map));
        }
        hp.options.boss_hp=1;hp.ticks=~0u;
        assert(hp.enemy_health(8,false,map)==normalEnd[map]);
    }
    // All timed slots, map curves, difficulty and Endless; never multiply guards.
    for(unsigned map=0;map<5;++map)for(bool endless:{false,true})for(unsigned difficulty:{1u,10u}) {
        hp.start(1);hp.endless=endless;hp.options.boss_hp=difficulty;
        for(int slot=0;slot<4;++slot) {
            hp.ticks=(slot+1)*elite_interval+(endless?duration:0);
            const auto baseline=hp.enemy_health(8,true,map?1:0,slot==3);
            assert(hp.spawn_enemy_health(8,true,map,slot)==scaled(baseline*2,map));
            assert(hp.spawn_enemy_health(8,false,map,slot)==hp.enemy_health(8,false,map));
        }
        for(int slot:{-1,4,5,6,7})assert(hp.spawn_enemy_health(8,true,map,slot)==hp.enemy_health(8,true,map));
    }
    // Exact approved 20-minute boss and guard preview, including odd HP totals.
    constexpr unsigned scheduled[5][4]{{262,688,1230,4600},{560,1626,3178,10800},
        {616,1789,3496,11880},{678,1967,3845,13068},{745,2164,4230,14375}};
    constexpr unsigned guards[3][4]{{120,280,813,1589},{132,308,894,1748},{145,339,984,1923}};
    hp.start(1);
    for(unsigned map=0;map<5;++map)for(int slot=0;slot<4;++slot){
        hp.ticks=(slot+1)*elite_interval;
        assert(hp.spawn_enemy_health(1200,true,map,slot)==scheduled[map][slot]);
    }
    for(unsigned map=1;map<=3;++map)for(unsigned step=0;step<4;++step){
        hp.ticks=step*elite_interval;
        assert(hp.spawn_enemy_health(8,true,map,4)==guards[map-1][step]);
    }
    // Intermediate times and Endless reuse Field's complete curve, including
    // normal species HP, the guard minimum, difficulty and native HP saturation.
    for(bool endless:{false,true})for(unsigned tick:{0u,1800u,5000u,15000u,36000u,40000u,~0u})
    for(float difficulty:{1.f,1.5f,5.f})for(unsigned map=2;map<5;++map){
        hp.start(1);hp.endless=endless;hp.ticks=tick;
        hp.options.enemy_hp=hp.options.boss_hp=difficulty;
        for(bool elite:{false,true})for(int slot:{-1,0,1,2,3,4})
            assert(hp.spawn_enemy_health(8,elite,map,slot)==scaled(hp.spawn_enemy_health(8,elite,1,slot),map));
    }
    // Independent HP sliders across every map, timed slot, guard and Endless.
    for(unsigned map=0;map<5;++map)for(bool endless:{false,true})
    for(unsigned normal=1;normal<=10;++normal)for(unsigned boss=1;boss<=10;++boss) {
        Run baseline;baseline.start(1);baseline.endless=endless;baseline.ticks=300*ticks_per_second;
        hp=baseline;hp.options.enemy_hp=normal;hp.options.boss_hp=boss;
        Run expected=baseline;expected.options.enemy_hp=normal;
        assert(hp.spawn_enemy_health(8,false,map,-1)==expected.spawn_enemy_health(8,false,map,-1));
        expected=baseline;expected.options.boss_hp=boss;
        for(int slot:{-1,0,1,2,3,4,5,6,7})
            assert(hp.spawn_enemy_health(8,true,map,slot)==expected.spawn_enemy_health(8,true,map,slot));
    }
    hp.start(1);hp.options.enemy_hp=10;assert(hp.enemy_health(8,false)==40&&hp.enemy_health(8,true)==120);
    hp.options.boss_hp=10;hp.options.enemy_hp=1;assert(hp.enemy_health(8,false)==8&&hp.enemy_health(8,true)==600);
    hp.options.boss_hp=0;assert(hp.enemy_health(8,true)==120);hp.options.boss_hp=100;assert(hp.enemy_health(8,true)==600);
    hp.start(1);
    hp.ticks=1800;hp.options.enemy_hp=3;assert(hp.enemy_health(8,false,1)==26);
    hp.ticks=duration;hp.options.enemy_hp=10;assert(hp.enemy_health(~0u,false)==32767);
    hp.start(1);hp.phase=Phase::Upgrade;for(unsigned i=0;i<1800;++i)hp.tick(true);
    assert(hp.enemy_health(8,false)==8);hp.phase=Phase::Running;
    for(unsigned i=0;i<1800;++i)hp.tick(false);
    assert(hp.enemy_health(8,false)==8);
    // Fixed 11.25-degree spacing: three arrows span 22.5 degrees, five span 45.
    for(unsigned count=1;count<=5;++count){std::array<float,10> fan{};
        for(unsigned i=0;i<count;++i)fan[i]=Run::bow_spread(i,count);
        std::sort(fan.begin(),fan.begin()+count);
        constexpr float step=11.25f*3.14159265358979323846f/180.f;
        assert(std::abs(fan[0]+fan[count-1])<1.e-6f);
        assert(std::abs(fan[count-1]-fan[0]-(count-1)*step)<1.e-6f);
        for(unsigned i=1;i<count;++i)assert(std::abs(fan[i]-fan[i-1]-step)<1.e-6f);
    }
    for(unsigned level:{1u,9u,10u,14u,15u,19u,20u,30u}) {
        Run r;r.start(1);r.ranks[unsigned(Upgrade::Wave)]=r.ranks[unsigned(Upgrade::Spin)]=level;
        assert(r.great_spin(Upgrade::Wave)==(level>=15)&&r.great_spin(Upgrade::Spin)==(level>=15));
        assert(r.wave_radius()==(level>=15?100.f:65.f));
        r.ranks[unsigned(Upgrade::Reach)]=30;
        assert(r.wave_radius()==(level>=15?325.f:211.25f));
    }
    // Dual Wield uses one rank for chest and level-up upgrades, including rarity
    // caps/excess refunds, shield coexistence and cross-source banishment.
    Run dual;dual.start(1);assert(!dual.eligible(Upgrade::DualWield));
    unsigned dualMoney=1000;dual.phase=Phase::Treasure;assert(dual.buy_chest(dualMoney,ChestReward::OrdonSword,true));
    assert(dual.dual_wield==1&&dual.rank(Upgrade::DualWield)==1&&dual.melee_speed_percent()==102);
    assert(dual.eligible(Upgrade::DualWield));
    dual.phase=Phase::Upgrade;dual.offers[0]=Upgrade::DualWield;dual.offer_rarities[0]=Rarity::Legendary;
    assert(dual.choose(0)&&dual.dual_wield==6&&dual.melee_targets()==2&&dual.melee_speed_percent()==115);
    dual.phase=Phase::Treasure;assert(dual.buy_chest(dualMoney,ChestReward::OrdonSword,true)&&dual.rank(Upgrade::DualWield)==7);
    dual.dual_wield=27;dual.phase=Phase::Upgrade;assert(dual.upgrade_gain(Upgrade::DualWield,Rarity::Legendary)==3);
    assert(dual.upgrade_rupees(Upgrade::DualWield,Rarity::Legendary)==40);
    assert(dual.choose(0)&&dual.dual_wield==30&&!dual.eligible(Upgrade::DualWield));
    assert(dual.melee_targets()==5&&dual.melee_speed_percent()==175);
    dual.phase=Phase::Treasure;assert(dual.buy_chest(dualMoney,ChestReward::Shield,true)&&dual.dual_wield==30&&dual.shield==1&&!dual.eligible(Upgrade::DualWield));
    assert(dual.melee_speed_percent()==175);
    for(bool chest:{false,true}) {
        dual.start(1);dual.dual_wield=1;dual.phase=chest?Phase::TreasureChoice:Phase::Upgrade;
        assert(chest?dual.banish(ChestReward::OrdonSword,false):dual.banish(Upgrade::DualWield));
        assert(!dual.eligible(Upgrade::DualWield)&&!dual.chest_eligible(ChestReward::OrdonSword));
    }

    dual.start(1);dual.dual_wield=1;dual.shield=1;
    assert(dual.eligible(Upgrade::DualWield)&&dual.eligible(Upgrade::ShieldAttack));
    dual.talents.ranks[unsigned(Talent::MeleeHaste)]=5;dual.ranks[unsigned(Upgrade::Haste)]=5;
    assert(dual.melee_speed_percent()==127&&dual.melee_cooldown()==21&&dual.cooldown(30)==26);

    Run jump; jump.start(12);
    assert(!jump.eligible(Upgrade::JumpStrike));
    jump.master_sword=1;assert(jump.eligible(Upgrade::JumpStrike));
    const auto jumpRng=jump.random;assert(!jump.roll_jump_strike()&&jump.random==jumpRng);
    for(unsigned level=1;level<=30;++level){
        jump.ranks[unsigned(Upgrade::JumpStrike)]=level;
        assert(jump.jump_splash_percent()==30+5*(level-1));
        assert(jump.jump_splash_power()==(6*(30+5*(level-1))+50)/100);
        assert(jump.eligible(Upgrade::JumpStrike)==(level<30));
    }
    jump.ranks[unsigned(Upgrade::JumpStrike)]=1;
    unsigned triggers=0;for(unsigned i=0;i<100000;++i)triggers+=jump.roll_jump_strike();
    assert(triggers>19500&&triggers<20500);
    jump.ranks[unsigned(Upgrade::Haste)]=30;assert(jump.jump_cooldown()<180&&jump.jump_cooldown()>=6);
    jump.phase=Phase::Upgrade;assert(jump.banish(Upgrade::JumpStrike));
    assert(!jump.eligible(Upgrade::JumpStrike)&&jump.rank(Upgrade::JumpStrike)==1);
    jump.start(12);assert(!jump.rank(Upgrade::JumpStrike)&&!jump.banned_upgrades[unsigned(Upgrade::JumpStrike)]);

    // Critical chance scales with full Luck, independently of the drop bonus cap.
    Run crit;crit.start(98765);
    const auto initialRandom=crit.random;
    assert(crit.critical_power(7)==7&&crit.random==initialRandom);
    assert(crit.crit_damage_percent()==200);
    for(unsigned chanceRank:{1u,30u})for(unsigned luckRank:{0u,10u,20u}){
        crit.start(98765);crit.ranks[unsigned(Upgrade::CritChance)]=chanceRank;
        crit.ranks[unsigned(Upgrade::Luck)]=luckRank;
        crit.ranks[unsigned(Upgrade::CritDamage)]=30;
        const unsigned expected=chanceRank*75*(100+5*luckRank/2)/100;
        assert(crit.crit_chance_basis_points()==expected&&crit.crit_damage_percent()==350);
        const auto before=crit.random;assert(crit.critical_power(0)==0&&crit.random==before);
        unsigned criticalHits=0;
        constexpr unsigned trials=200000;
        for(unsigned i=0;i<trials;++i){const auto hit=crit.critical_power(100);
            assert(hit==100||hit==350);criticalHits+=hit==350;}
        assert(std::abs(int(criticalHits*10000/trials)-int(expected))<40);
    }
    for(auto power:{Upgrade::CritChance,Upgrade::CritDamage}){
        assert(upgrade_cap(power)==30);
        crit.start(1);crit.ranks[unsigned(power)]=29;crit.phase=Phase::Upgrade;
        crit.offers[0]=power;crit.offer_rarities[0]=Rarity::Legendary;
        assert(crit.eligible(power)&&crit.upgrade_gain(power,Rarity::Legendary)==1);
        assert(crit.upgrade_rupees(power,Rarity::Legendary)==80&&crit.choose(0));
        assert(crit.rank(power)==30&&!crit.eligible(power));
        crit.start(1);crit.ranks[unsigned(power)]=5;crit.phase=Phase::Upgrade;
        assert(crit.banish(power)&&crit.rank(power)==5&&!crit.eligible(power));
        crit.start(1);assert(crit.rank(power)==0&&crit.eligible(power));
    }
    for(unsigned rank=0;rank<=30;++rank){
        crit.ranks[unsigned(Upgrade::CritDamage)]=rank;
        assert(crit.crit_damage_percent()==200+5*rank);
    }
    crit.ranks[unsigned(Upgrade::CritChance)]=30;crit.random=1;
    assert(crit.critical_power(3)==11); // x3.5 rounds once to native integer health.
    // Published chart checkpoints (basis points), sampled from actual rarity rolls.
    constexpr std::array<std::array<unsigned,5>,7> chart{{{6000,2500,1000,400,100},{5818,2546,1069,449,118},{5464,2618,1204,554,159},{5123,2668,1334,667,208},{4330,2706,1624,974,365},{3632,2648,1854,1298,568},{3030,2525,2020,1616,808}}};
    unsigned row=0;
    for(unsigned luck:{0u,1u,3u,5u,10u,15u,20u}) {
        Run sample;sample.start(98765);sample.ranks[unsigned(Upgrade::Luck)]=luck;
        assert(sample.luck_percent()==100+10*luck);
        assert(sample.lucky_chance(2000)==2000+200*std::min(5u,luck));
        constexpr unsigned trials=500000;
        std::array<unsigned,5> rarities{};
        std::array<unsigned,unsigned(Run::Drop::Count)> loot{};
        for(unsigned n=0;n<trials;++n){++rarities[unsigned(sample.roll_rarity())];++loot[unsigned(sample.roll_drop())];}
        for(unsigned i=0;i<5;++i)assert(std::abs(int(rarities[i]*10000/trials)-int(chart[row][i]))<40);
        ++row;
        assert(std::abs(int(loot[1]*10000/trials)-int(sample.lucky_chance(250)))<30);
        assert(std::abs(int(loot[0]*10000/trials)-int(10000-sample.lucky_chance(5000)+sample.lucky_chance(250)))<40);
        const unsigned rupees=trials-loot[0]-loot[1];
        const unsigned valuable=rupees-loot[unsigned(Run::Drop::GreenRupee)];
        assert(std::abs(int(valuable*10000/rupees)-int(sample.lucky_chance(1700)))<80);
        for(unsigned i=1;i<loot.size();++i)assert(loot[i]); // Every native color is reachable.
    }
    // All Luck ranks remain eligible up to 20; rarity grants/overflow use that cap.
    for(unsigned initial=0;initial<=20;++initial)for(unsigned tier=0;tier<5;++tier){
        Run r;r.start(1);r.ranks[unsigned(Upgrade::Luck)]=initial;
        assert(r.eligible(Upgrade::Luck)==(initial<20));
        const unsigned gain=std::min(20-initial,tier+1);
        assert(r.upgrade_gain(Upgrade::Luck,Rarity(tier))==gain);
        assert(r.upgrade_rupees(Upgrade::Luck,Rarity(tier))==20*(tier+1-gain));
        if(initial<20){r.phase=Phase::Upgrade;r.offers[0]=Upgrade::Luck;r.offer_rarities[0]=Rarity(tier);
            assert(r.choose(0)&&r.luck_percent()==100+10*(initial+gain));}
    }
    // Higher rarity Luck must not inflate enemy or pot drops past their old cap.
    Run capped,maximum;capped.start(58);maximum.start(58);
    capped.ranks[unsigned(Upgrade::Luck)]=5;maximum.ranks[unsigned(Upgrade::Luck)]=20;
    for(unsigned n=0;n<10000;++n){assert(capped.roll_drop()==maximum.roll_drop());
        assert(capped.roll_pot_drop()==maximum.roll_pot_drop());}
    maximum.start(58);assert(maximum.luck_percent()==100&&maximum.eligible(Upgrade::Luck));
    for(unsigned rarity=0;rarity<5;++rarity) {
        for(unsigned u=0;u<unsigned(Upgrade::Rupees);++u)for(unsigned initial:{0u,4u}) {
            Run r;r.start(13);r.shield=1;r.red_potion=true;r.master_sword=1;r.ranks[8]=r.ranks[9]=r.ranks[10]=r.ranks[2]=1;r.dark_link=1;r.ranks[u]=initial;
            if(Upgrade(u)==Upgrade::DualWield){r.shield=0;r.dual_wield=initial;}
            if(Upgrade(u)==Upgrade::MasterSword){r.master_sword=initial;r.ranks[8]=r.ranks[9]=r.ranks[10]=0;}
            if(Upgrade(u)==Upgrade::MultiStrike)r.dual_wield=1;
            // Weapons must be chest-unlocked first.
            if(!initial && ((u>=unsigned(Upgrade::Bow)&&u<=unsigned(Upgrade::Ball)) || Upgrade(u)==Upgrade::DualWield || Upgrade(u)==Upgrade::MasterSword))continue;
            r.phase=Phase::Upgrade;r.offers[0]=Upgrade(u);r.offer_rarities[0]=Rarity(rarity);
            if (initial>=upgrade_cap(Upgrade(u)) || Rarity(rarity)<upgrade_min_rarity(Upgrade(u))) {assert(!r.choose(0));continue;}
            const unsigned units=upgrade_units(Upgrade(u),Rarity(rarity));
            const auto random=r.random;const unsigned gain=std::min(upgrade_cap(Upgrade(u))-initial,units);
            assert(r.upgrade_rupees(Upgrade(u),Rarity(rarity))==20*(units-gain));
            assert(r.choose(0)&&r.rank(Upgrade(u))==initial+gain&&r.random==random);
            assert(!r.choose(0));
        }
        for(unsigned item=0;item<unsigned(ChestReward::Count);++item) {
            Run r;r.start(14);r.phase=Phase::Treasure;r.master_sword=4;r.shield=9;r.ranks[8]=4;
            const auto reward=ChestReward(item);const auto gain=r.chest_gain(reward,Rarity(rarity),3);
            const unsigned extra=r.chest_rupees(reward,Rarity(rarity),3);unsigned money=20;
            assert(gain+extra/20==rarity+1);
            assert(r.buy_chest(money,reward,3,Rarity(rarity))&&money==extra&&r.chests_opened==1);
            assert(r.master_sword<=30&&r.shield<=10&&r.rank(Upgrade::Bow)<=30);
        }
    }
    // Every rarity can advance the sword beyond Light Sword, but never past 30.
    for(unsigned initial=0;initial<=30;++initial)for(unsigned tier=0;tier<5;++tier){
        Run sword;sword.start(17);sword.phase=Phase::Treasure;sword.master_sword=initial;
        const auto rarity=Rarity(tier);const unsigned gain=std::min(30-initial,tier+1);
        assert(sword.chest_eligible(ChestReward::MasterSword)==(initial<30));
        assert(sword.chest_gain(ChestReward::MasterSword,rarity)==gain);
        unsigned money=100;
        assert(sword.buy_chest(money,ChestReward::MasterSword,0,rarity,true)==(initial<30));
        assert(sword.master_sword==initial+gain);
        assert(sword.sword_bonus()==2+sword.master_sword);
        assert(money==100+(initial<30?20*(tier+1-gain):0));
    }
    // Bonus magnets use the ordinary drop Luck multiplier, independently of item drops.
    Run magnets, luckyMagnets;magnets.start(99);luckyMagnets.start(99);
    luckyMagnets.ranks[unsigned(Upgrade::Luck)]=20;
    unsigned plain=0,lucky=0;
    for(unsigned i=0;i<200000;++i) {
        const bool a=magnets.roll_exp_magnet(),b=luckyMagnets.roll_exp_magnet();
        assert(!a||b);plain+=a;lucky+=b;
    }
    assert(plain>3600&&plain<4400&&lucky>5500&&lucky<6500);
    assert(luckyMagnets.lucky_chance(200)==300);
    Run run;
    assert(std::all_of(run.ranks.begin(),run.ranks.end(),[](auto r){return r==0;}));
    std::array<unsigned,unsigned(Run::Drop::Count)> drops{};
    run.start(123);
    for(unsigned i=0;i<10000;++i) ++drops[static_cast<unsigned>(run.roll_drop())];
    assert(drops[1]>170 && drops[1]<330); // Rare hearts, not an every-kill heal.
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
    const auto gain = run.upgrade_gain(offers[0], run.offer_rarities[0]);
    assert(run.choose(0));
    if (offers[0] != Upgrade::Rupees) assert(run.rank(offers[0]) == previous + gain);
    assert(!run.choose(0)); // Double activation is ignored.
    assert(run.level_up()); // Excess XP remains available.
    for (uint32_t seed = 1; seed < 1000; ++seed) {
        run.start(seed);
        run.ranks.fill(30); run.chain_spin=1; run.ranks[unsigned(Upgrade::Luck)]=20; run.shield=1; run.ranks[2] = 29; run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::ShieldAttack || offer == Upgrade::Rupees);
        assert(std::count(run.offers.begin(), run.offers.end(), Upgrade::ShieldAttack) == 1);
        run.start(seed); run.ranks.fill(30); run.chain_spin=1; run.ranks[unsigned(Upgrade::Luck)]=20; run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::Rupees);
        assert(run.choose(2));
        assert(run.cooldown(40) == 22);
    }
    run.start(1);
    run.ranks[3] = run.ranks[4] = run.ranks[5] = run.ranks[6] = 1;
    assert(run.eligible(Upgrade::Heart)); // Hearts are independent of other passives.
    assert(run.eligible(Upgrade::Power) && run.eligible(Upgrade::Spin) && run.eligible(Upgrade::Rupees));
    // Chest weapons cannot be acquired from level-up offers before a purchase.
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i)
        assert(!run.eligible(static_cast<Upgrade>(i)));
    assert(!run.eligible(Upgrade::FireArrows) && !run.eligible(Upgrade::ShieldAttack));
    run.ranks[8]=1;run.shield=1;
    assert(run.eligible(Upgrade::FireArrows) && run.eligible(Upgrade::ShieldAttack));
    run.ranks[8]=0;run.shield=0;
    for(unsigned rank=1;rank<=10;++rank){run.shield=rank;assert(run.block_chance()==rank*5);
        assert(run.shield_tier()==(rank>=7?2u:rank>=4?1u:0u));}
    run.shield=0;
    unsigned wallet = 19;
    ChestReward reward = ChestReward::Bow;
    run.phase = Phase::Treasure;
    const auto rng = run.random;
    assert(!run.buy_chest(wallet, reward) && wallet == 19 && run.random == rng && run.chest_cost() == 20);
    wallet = 20;
    assert(run.buy_chest(wallet, reward) && wallet == 0 && run.chest_cost() == 25);
    assert((reward == ChestReward::OrdonSword && run.dual_wield) || (reward == ChestReward::Shield && run.shield==1) ||
           (reward == ChestReward::DarkLink && run.dark_link) ||
           (reward == ChestReward::MasterSword && run.master_sword) ||
           (run.rank(chest_weapon(reward)) == 1 && run.eligible(chest_weapon(reward)))); // Even with four occupied passives.
    assert(!run.tick(true)); // Get Item cannot advance encounter time.
    run.start(20); run.phase = Phase::Treasure;
    wallet = 20000000;
    for (unsigned n = 0; n < 120; ++n) {
        assert(run.chest_cost() == 20+n*(n+4));
        reward = ChestReward(n%4);
        assert(run.buy_chest(wallet, reward) && run.rank(chest_weapon(reward)) <= 30);
    }
    assert(wallet == 19400220 && !run.chest_rewards_left());
    assert(run.equipment_count()==4&&!run.chest_eligible(ChestReward::MasterSword));
    assert(!run.chest_rewards_left(true)); // Four weapons leave no slot for a first potion.
    const auto maxedRng = run.random;
    assert(!run.buy_chest(wallet, reward) && wallet == 19400220 && run.random == maxedRng);
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i) {
        const auto weapon = static_cast<Upgrade>(i);
        assert(run.rank(weapon) == 30 && !run.eligible(weapon));
        const auto fast = run.item_cooldown(weapon);
        run.ranks[i] = 1;
        assert(run.item_cooldown(weapon) > fast);
        run.ranks[i] = 30;
    }
    run.start(20);run.phase=Phase::Treasure;run.banned_items.fill(true);run.banned_items[unsigned(ChestReward::RedPotion)]=false;
    wallet = 100000;
    reward = run.roll_chest_offers(true)[0];
    assert(run.buy_chest(wallet, reward, true) && reward == ChestReward::RedPotion);
    // Offers are distinct, capped and do not purchase anything. Buy exactly the chosen reward.
    for (unsigned seed=1; seed<100; ++seed) {
        Run choices; choices.start(seed);
        const auto offers=choices.roll_chest_offers(true);
        assert(offers[0]!=offers[1]&&offers[0]!=offers[2]&&offers[1]!=offers[2]);
        for(auto offered:offers)assert(choices.chest_eligible(offered,true));
        assert(choices.chests_opened==0);
        choices.phase=Phase::Treasure;unsigned money=20;const auto rng=choices.random;
        assert(choices.buy_chest(money,ChestReward::Shield)&&choices.shield==1);
        assert(!choices.master_sword&&!choices.dual_wield&&choices.rank(Upgrade::Bow)==0);
        assert(choices.random==rng&&money==0);
        assert(!choices.buy_chest(money,ChestReward::Count));
        choices.ranks.fill(30);choices.ranks[unsigned(Upgrade::Ball)]=0;choices.shield=9;choices.banned_items[unsigned(ChestReward::OrdonSword)]=true;
        auto few=choices.roll_chest_offers(true);
        assert(few[0]==ChestReward::Shield&&few[1]==ChestReward::Count&&few[2]==ChestReward::Count);
        assert((few[0]==ChestReward::Shield||few[1]==ChestReward::Shield));
        few=choices.roll_chest_offers(false);
        assert(few[0]==ChestReward::Shield&&few[1]==ChestReward::Count&&few[2]==ChestReward::Count);
        choices.shield=10;few=choices.roll_chest_offers();
        for(auto offered:few)assert(offered==ChestReward::Count);
    }
    run.start(2);
    assert(!run.master_sword && run.sword_bonus() == 0);
    assert(run.chests_opened == 0 && run.chest_cost() == 20 && run.chest_rewards_left());
    wallet = 100;
    assert(!run.buy_chest(wallet, reward) && wallet == 100); // No purchase outside the reward phase.
    // Exact pricing, burn ranks, timed Dark Link and capped multihit grants.
    Run effects; effects.start(7);
    for(unsigned n=0;n<5;++n){effects.chests_opened=n;assert((effects.chest_cost()==std::array<unsigned,5>{20,25,32,41,52}[n]));}
    for(unsigned rank=1;rank<=5;++rank){effects.ranks[unsigned(Upgrade::FireArrows)]=rank;
        assert(effects.burn_damage()==rank&&effects.burn_duration()==(rank<4?90u:120u));}
    effects.dark_link=1;
    for(unsigned i=0;i<599;++i)effects.tick(true);
    assert(!effects.dark_active()&&effects.dark_power(100)==100);
    effects.tick(true);assert(effects.dark_active()&&effects.dark_remaining==150&&effects.dark_power(100)==130);
    assert(effects.dark_power(0)==0&&effects.dark_power(3)==4&&effects.dark_power(5)==7);
    for(auto phase:{Phase::Upgrade,Phase::TreasureChoice,Phase::Treasure}){
        effects.phase=phase;for(int i=0;i<100;++i)effects.tick(true);
        assert(effects.dark_remaining==150&&effects.dark_clock==0);
    }
    effects.phase=Phase::Running;effects.tick(false);assert(effects.dark_remaining==150);
    for(unsigned i=0;i<150;++i)effects.tick(true);
    assert(!effects.dark_active());
    for(unsigned rank=1;rank<=30;++rank){
        const auto previousDamage=effects.dark_damage_percent(),previousDuration=effects.dark_duration();
        effects.dark_link=rank;
        assert(effects.dark_interval()==600);
        assert(effects.dark_duration()==(5+std::min(29u,rank)/2)*30);
        assert(effects.dark_damage_percent()==130+5*((rank-1)/2));
        assert(effects.dark_permanent()==(rank==30));
        assert(effects.chest_eligible(ChestReward::DarkLink)==(rank<30));
        if(rank>1&&rank<30){
            assert(effects.dark_damage_percent()==previousDamage+(rank%2?5:0));
            assert(effects.dark_duration()==previousDuration+(rank%2?0:30));
        }
        effects.dark_remaining=0;
        assert(effects.dark_power(100)==(rank==30?200u:100u));
        effects.dark_remaining=1;
        assert(effects.dark_power(100)==effects.dark_damage_percent());
        effects.dark_clock=0;effects.tick(true);
        assert(effects.dark_active()==(rank==30));
    }
    assert(effects.dark_permanent()&&effects.dark_active()&&effects.dark_power(100)==200);
    effects.dark_link=28;effects.phase=Phase::Treasure;effects.chests_opened=0;unsigned money=100;
    assert(effects.buy_chest(money,ChestReward::DarkLink,false,Rarity::Legendary));
    assert(effects.dark_link==30&&effects.dark_permanent()&&money==140);
    effects.dual_wield=0;effects.chests_opened=0;
    assert(effects.buy_chest(money,ChestReward::OrdonSword,false,Rarity::Common)&&effects.melee_targets()==2);
    assert(effects.buy_chest(money,ChestReward::OrdonSword,false,Rarity::Legendary)&&effects.melee_targets()==2);
    assert(effects.chest_eligible(ChestReward::OrdonSword));
    effects.dual_wield=30;assert(!effects.chest_eligible(ChestReward::OrdonSword));
    effects.start(7);assert(!effects.dark_active()&&!effects.dark_clock&&!effects.dual_wield&&effects.dark_power(100)==100);
    // Free elite chests grant exactly one item with no currency or price change.
    Run freeRun; freeRun.start(9); freeRun.phase=Phase::Treasure;
    freeRun.chests_opened=7; unsigned zero=0;
    assert(freeRun.buy_chest(zero,ChestReward::Bow,0,Rarity::Common,true));
    assert(zero==0 && freeRun.chests_opened==7 && freeRun.rank(Upgrade::Bow)==1);
    assert(duration==20*60*30 && elite_interval==5*60*30 && timed_elite_count==4);
    // Every ordinary upgrade and weapon can cross the old cap safely.
    for (unsigned rank=0;rank<=30;++rank) {
        Run high;high.start(1);high.shield=1;high.red_potion=true;high.master_sword=1;high.ranks.fill(rank);
        for (unsigned u=0;u<unsigned(Upgrade::Rupees);++u) {
            if (Upgrade(u)==Upgrade::Luck || Upgrade(u)==Upgrade::DualWield || Upgrade(u)>=Upgrade::Knockback) continue;
            assert(upgrade_cap(Upgrade(u))==30);
            if (rank) assert(high.eligible(Upgrade(u))==(rank<30));
        }
        const auto interval=high.item_cooldown(Upgrade::Bow);
        assert(interval>=20 && interval<=75);
        assert(high.shield_cooldown()>=30 && high.shield_cooldown()<=90);
        if(rank){Run previous=high;previous.ranks.fill(rank-1);
            assert(high.cooldown(100)<=previous.cooldown(100));
            assert(interval<=previous.item_cooldown(Upgrade::Bow));}
    }
    // Restricted offers respect the rolled rarity instead of upgrading rarity
    // for free. Multishot has its own reward units, caps and overflow payment.
    Run powers;powers.start(91);powers.ranks[unsigned(Upgrade::Bow)]=1;
    unsigned specialSeen[3]{};
    for(unsigned n=0;n<30000;++n){
        powers.roll_upgrades();
        for(unsigned i=0;i<3;++i){auto u=powers.offers[i];
            assert(powers.offer_rarities[i]>=upgrade_min_rarity(u));
            if(u==Upgrade::Multishot)++specialSeen[0];
            if(u==Upgrade::LifeRegen)++specialSeen[1];
            if(u==Upgrade::LifeSteal)++specialSeen[2];
        }
    }
    for(auto seen:specialSeen)assert(seen);
    for(unsigned rank=0;rank<3;++rank)for(auto rarity:{Rarity::Epic,Rarity::Legendary}){
        powers.ranks[unsigned(Upgrade::Multishot)]=rank;
        const unsigned units=rarity==Rarity::Epic?1:2;
        assert(powers.upgrade_gain(Upgrade::Multishot,rarity)==std::min(3-rank,units));
        assert(powers.upgrade_rupees(Upgrade::Multishot,rarity)==20*(units-std::min(3-rank,units)));
    }
    for(unsigned rank=0;rank<=3;++rank)for(unsigned bow:{1u,10u,20u,30u}){
        powers.ranks[unsigned(Upgrade::Multishot)]=rank;powers.ranks[unsigned(Upgrade::Bow)]=bow;
        const unsigned base=bow>=20?5:bow>=10?3:1;
        assert(powers.bow_shots()==base+rank&&powers.ranged_shots()==1+rank);
        assert(powers.bow_fan_degrees()==float(base-1)*11.25f);
        float sum=0,extent=0;
        for(unsigned i=0;i<powers.bow_shots();++i){const float angle=powers.arrow_spread(i);sum+=angle;extent=std::max(extent,std::abs(angle));}
        assert(std::abs(sum)<.0001f&&std::abs(extent-powers.bow_fan_degrees()*3.14159265f/360)<.0001f);
    }
    for(unsigned perk=0;perk<=5;++perk)for(unsigned rank=0;rank<=5;++rank){
        powers.talents.ranks[unsigned(Talent::LifeRegen)]=perk;powers.ranks[unsigned(Upgrade::LifeRegen)]=rank;
        assert(powers.life_regen_interval()==(20-perk-rank)*30);
    }
    for(unsigned rank=0;rank<=10;++rank){
        powers.ranks[unsigned(Upgrade::Piercing)]=rank;assert(powers.arrow_targets()==1+rank);
        powers.ranks[unsigned(Upgrade::Knockback)]=rank;
        assert(powers.knockback_distance(false,false)==40*rank);
        assert(powers.knockback_distance(true,false)==240+40*rank);
        assert(powers.knockback_distance(true,true)==0);
    }
    assert(upgrade_cap(Upgrade::Knockback)==10&&upgrade_cap(Upgrade::Piercing)==10);
    assert(upgrade_cap(Upgrade::Multishot)==3&&upgrade_cap(Upgrade::LifeRegen)==5&&upgrade_cap(Upgrade::LifeSteal)==1);
    Run heart;heart.start(2);heart.ranks[3]=heart.ranks[4]=heart.ranks[5]=heart.ranks[6]=30;
    assert(heart.eligible(Upgrade::Heart));heart.maximum_hearts=19;
    assert(heart.upgrade_gain(Upgrade::Heart,Rarity::Legendary)==1);
    assert(heart.upgrade_rupees(Upgrade::Heart,Rarity::Legendary)==80);
    heart.phase=Phase::Upgrade;heart.offers[0]=Upgrade::Heart;heart.offer_rarities[0]=Rarity::Legendary;
    assert(heart.choose(0)&&heart.maximum_hearts==20&&!heart.eligible(Upgrade::Heart));
    for(unsigned horde=1;horde<=5;++horde){
        Run swarm;RunOptions settings;settings.horde=horde;swarm.start(3,settings);
        for(unsigned tick=0;tick<=duration;++tick) {
            swarm.ticks=tick;
            assert(swarm.target_population()==300);
            assert(swarm.spawn_interval()==(tick<600*ticks_per_second ? 75-tick/(10*ticks_per_second) : 15));
            swarm.horde_remaining=0;assert(swarm.spawn_batch()==horde);
            swarm.horde_remaining=1;assert(swarm.spawn_batch()==3*horde);
        }
        for(unsigned kill=1;kill<=50;++kill)assert(swarm.normal_kill_drops()==(kill%horde==0));
        swarm.ticks=duration;assert(swarm.target_population()==300&&swarm.target_population()+elite_capacity<enemy_capacity);
    }
    // Fixed-price exploration chests neither follow nor raise the regular price.
    Run fixed;fixed.start(8);fixed.chests_opened=10;fixed.phase=Phase::Treasure;
    unsigned fixedWallet=39;
    assert(!fixed.buy_chest(fixedWallet,ChestReward::Bow,0,Rarity::Common,false,true));
    assert(fixedWallet==39&&fixed.chests_opened==10);
    fixedWallet=40;assert(fixed.buy_chest(fixedWallet,ChestReward::Bow,0,Rarity::Common,false,true));
    assert(fixedWallet==0&&fixed.chests_opened==10&&fixed.chest_cost()==160);
    // Random hordes have no per-window quota, pause with simulation and extend only time.
    unsigned events=0,overlaps=0,maxEvents=0;
    for(unsigned seed=1;seed<=1000;++seed) {
        Run h;h.start(seed);unsigned count=0;
        for(unsigned second=1;second<300;++second) {
            h.ticks=second*ticks_per_second-1;
            const auto before=h.horde_remaining;
            h.tick(true);
            const unsigned baseline=before ? before-1 : 0;
            if(h.horde_remaining>baseline){++events;++count;overlaps+=before!=0;
                assert(h.horde_remaining==baseline+horde_duration);}
            assert(h.spawn_batch()==(h.horde_remaining ? 3u : 1u));
            const auto saved=h.horde_remaining,rng=h.random;
            h.tick(false);assert(h.horde_remaining==saved&&h.random==rng);
            h.phase=Phase::Upgrade;h.tick(true);assert(h.horde_remaining==saved&&h.random==rng);
            h.phase=Phase::Running;
            for(unsigned n=1;n<ticks_per_second;++n)h.tick(true);
        }
        maxEvents=std::max(maxEvents,count);
    }
    assert(events>1800&&events<2200&&overlaps>0&&maxEvents>2);
    Run ending;ending.start(3);ending.ticks=duration-1;ending.horde_remaining=300;ending.tick(true);
    assert(!ending.horde_remaining&&ending.spawn_batch()==1);
    ending.start(5);assert(!ending.horde_remaining);
    RunOptions invalid{0,100,100,0,1000};invalid.boss_hp=0;invalid.clamp();assert(invalid.boss_hp==1);invalid.boss_hp=100;invalid.clamp();assert(invalid.boss_hp==5);
    assert(invalid.enemy_hp==1&&invalid.enemy_damage==5&&invalid.enemy_speed==5&&invalid.horde==1&&invalid.camera_distance==500);
    assert(RunOptions{}.camera_distance==200);
    // Shield and Ordon Sword use independent equipment slots and retain ranks.
    Run slots;slots.start(51);slots.ranks.fill(1);slots.ranks[unsigned(Upgrade::Ball)]=0;
    slots.dual_wield=8;assert(slots.equipment_count()==4);
    assert(slots.eligible(Upgrade::Heart)&&slots.eligible(Upgrade::Magnet));
    assert(!slots.chest_eligible(ChestReward::Shield));
    assert(!slots.chest_eligible(ChestReward::Ball)&&!slots.chest_eligible(ChestReward::MasterSword));
    assert(slots.chest_eligible(ChestReward::Bow)&&!slots.chest_eligible(ChestReward::RedPotion,true));
    for(bool shieldFirst:{false,true}) {
        slots.start(51);slots.master_sword=1;slots.dark_link=1;wallet=10000;slots.phase=Phase::Treasure;
        const auto first=shieldFirst?ChestReward::Shield:ChestReward::OrdonSword;
        const auto second=shieldFirst?ChestReward::OrdonSword:ChestReward::Shield;
        assert(slots.buy_chest(wallet,first,0,Rarity::Legendary));
        assert(slots.buy_chest(wallet,second,0,Rarity::Rare));
        assert(slots.shield==(shieldFirst?5u:3u)&&slots.dual_wield==(shieldFirst?3u:5u));
        assert(slots.equipment_count()==4&&slots.eligible(Upgrade::ShieldAttack)&&slots.eligible(Upgrade::DualWield));
        assert(!slots.chest_eligible(ChestReward::Bow));
        assert(slots.buy_chest(wallet,second)&&slots.equipment_count()==4);
    }
    for(unsigned rank=1;rank<=30;++rank){
        slots.ranks[unsigned(Upgrade::Bow)]=slots.ranks[unsigned(Upgrade::Boomerang)]=rank;
        assert(slots.bow_shots()==(rank>=20?5u:rank>=10?3u:1u)&&slots.boomerang_hops()==1+(rank-1)/3);
    }
    for(unsigned rank=1;rank<=30;++rank){slots.dual_wield=rank;
        assert(slots.melee_targets()==(rank<10?2u:rank<20?3u:rank<30?4u:5u));
        assert(slots.melee_speed_percent()==100+5*rank/2);}
    // Both equipment paths clamp every rarity and compensate only excess levels.
    for(auto reward:{ChestReward::OrdonSword,ChestReward::DarkLink})
    for(unsigned rank=0;rank<=30;++rank)for(unsigned tier=0;tier<5;++tier){
        Run item;item.start(1);item.phase=Phase::Treasure;
        if(reward==ChestReward::OrdonSword)item.dual_wield=rank;else item.dark_link=rank;
        const unsigned gain=std::min(30-rank,tier+1);unsigned wallet=0;
        assert(item.chest_gain(reward,Rarity(tier))==gain);
        assert(item.buy_chest(wallet,reward,0,Rarity(tier),true)==(rank<30));
        assert((reward==ChestReward::OrdonSword?item.dual_wield:item.dark_link)==rank+gain);
        assert(wallet==(rank<30?20*(tier+1-gain):0));
    }
    slots.shield=0;assert(!slots.chest_eligible(ChestReward::OrdonSword)&&!slots.chest_eligible(ChestReward::Shield));
    // Shared reroll budget, increasing paid costs and reservation of the purchase price.
    Run actions;actions.start(7);actions.phase=Phase::Upgrade;wallet=250;
    assert(actions.pay_reroll(wallet)&&wallet==250&&actions.reroll_cost()==50);
    actions.phase=Phase::TreasureChoice;
    assert(actions.pay_reroll(wallet,20)&&wallet==200&&actions.reroll_cost()==100);
    actions.phase=Phase::Upgrade;
    assert(actions.pay_reroll(wallet)&&wallet==100&&actions.reroll_cost()==150);
    assert(!actions.pay_reroll(wallet)&&wallet==100&&actions.paid_rerolls==2);
    // Banish preserves ownership/ranks and excludes a weapon from both sources.
    actions.ranks[unsigned(Upgrade::Bow)]=6;
    assert(actions.banish(Upgrade::Bow)&&actions.rank(Upgrade::Bow)==6&&actions.owns(ChestReward::Bow));
    assert(!actions.eligible(Upgrade::Bow)&&!actions.chest_eligible(ChestReward::Bow));
    assert(!actions.banish(Upgrade::Power));
    for(unsigned i=0;i<100;++i){actions.roll_upgrades();for(auto u:actions.offers)assert(u!=Upgrade::Bow);}
    actions.start(7);assert(actions.free_rerolls==1&&actions.banishes==1&&actions.reroll_cost()==0);
    actions.phase=Phase::TreasureChoice;actions.shield=6;
    assert(actions.banish(ChestReward::Shield,true)&&actions.shield==6&&actions.owns(ChestReward::Shield));
    assert(!actions.chest_eligible(ChestReward::Shield));
    actions.banned_upgrades.fill(true);actions.roll_upgrades();
    for(auto u:actions.offers)assert(u==Upgrade::Count);
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
    auto corrupt = bytes; corrupt[4] = 99;
    assert(!loaded.decode(corrupt) && loaded.encode() == bytes);
    assert(!loaded.decode(std::span(bytes.data(), bytes.size() - 1)));
    corrupt = bytes; corrupt[12] = 250; assert(!loaded.decode(corrupt));
    run.start(9); assert(run.xp == 0 && run.kills == 0 && run.ticks == 0 && run.level == 1);
    assert(run.ranks[0] == 0 && run.ranks[1] == 0 && run.ranks[7] == 0);
    std::cout << "Survivors rules: timing, choices, caps, restart and records passed\n";
}
