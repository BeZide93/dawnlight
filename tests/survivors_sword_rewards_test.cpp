#include "../src/survivors_rules.hpp"
#include "survivors_legacy_records.hpp"
#include "../src/survivors_multistrike.hpp"
#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
using namespace dawnlight::survivors;
int main() {
    Run run;run.start(9);
    assert(run.eligible(Upgrade::Lightning)&&!run.eligible(Upgrade::MasterSword));
    assert(!run.eligible(Upgrade::MultiStrike));run.dual_wield=1;
    assert(run.eligible(Upgrade::MultiStrike));
    for(unsigned rarity=0;rarity<5;++rarity) {
        run.ranks[unsigned(Upgrade::MultiStrike)]=0;run.phase=Phase::Upgrade;
        run.offers[0]=Upgrade::MultiStrike;run.offer_rarities[0]=Rarity(rarity);
        assert(run.choose(0)==(rarity>=3));
        assert(run.rank(Upgrade::MultiStrike)==(rarity<3?0:rarity-2));
    }
    run.ranks[unsigned(Upgrade::MultiStrike)]=2;run.phase=Phase::Upgrade;
    assert(run.choose(0)&&run.multi_strikes()==3&&!run.eligible(Upgrade::MultiStrike));
    for(unsigned level=1;level<=30;++level)for(unsigned reach:{0u,10u,30u})for(unsigned haste:{0u,30u}) {
        run.ranks[unsigned(Upgrade::Lightning)]=run.ranks[unsigned(Upgrade::Boomerang)]=level;
        run.ranks[unsigned(Upgrade::Reach)]=reach;run.ranks[unsigned(Upgrade::Haste)]=haste;
        run.ranks[unsigned(Upgrade::Power)]=30;run.master_sword=0;
        assert(run.lightning_power()==run.attack_power(unsigned(Upgrade::Boomerang)));
        assert(run.lightning_hops()==2*run.boomerang_hops());
        assert(run.item_cooldown(Upgrade::Lightning)==run.item_cooldown(Upgrade::Boomerang));
        assert(std::abs((run.lightning_hop_range()/800-1)-2*(run.boomerang_hop_range()/800-1))<.001f);
        assert(std::abs((run.melee_area_radius(1)-1)-2*(run.area_radius(1)-1))<.001f);
        run.master_sword=30;assert(run.lightning_power()==run.attack_power(unsigned(Upgrade::Boomerang))+15);
    }
    assert(Run::lightning_speed==210&&Run::lightning_chance==2000);
    for(unsigned luck=0;luck<=30;++luck) {
        run.ranks[unsigned(Upgrade::Luck)]=luck;
        assert(run.lightning_proc_chance()==2000+200*std::min(luck,20u));
    }
    // Dual Wield adds an absolute percentage-point bonus, only to melee sources.
    run.start(8);run.dual_wield=30;
    assert(run.crit_chance_basis_points()==0&&run.melee_crit_bonus()==3000);
    unsigned meleeCrits=0,rangedCrits=0;
    for(unsigned i=0;i<10000;++i) {
        bool crit=false;run.critical_power(10,&crit,true);meleeCrits+=crit;
        run.critical_power(10,&crit,false);rangedCrits+=crit;
    }
    assert(meleeCrits>2800&&meleeCrits<3200&&!rangedCrits);
    for(auto s:{DamageSource::Sword,DamageSource::Spin,DamageSource::Jump,DamageSource::Shield,DamageSource::DualWield,DamageSource::Lightning})assert(melee_critical_source(s));
    assert(!melee_critical_source(DamageSource::Wave)&&!melee_critical_source(DamageSource::Bow));
    for(auto s:{DamageSource::Sword,DamageSource::Spin,DamageSource::Jump,DamageSource::Wave,DamageSource::DualWield,DamageSource::Lightning})assert(sword_kill_source(s));
    for(auto s:{DamageSource::Shield,DamageSource::Bow,DamageSource::Bombs,DamageSource::DarkAura,DamageSource::FireEdge})assert(!sword_kill_source(s));
    DamageQueue q;q.swordPending=true;q.add(DamageSource::Sword,4);q.consume();assert(q.swordApplied&&!q.swordPending);
    q.add(DamageSource::Sword,1,true);q.consume();assert(!q.swordApplied); // Sword-attributed Fire Edge burn is not a direct sword kill.
    // Level-up cannot unlock Master Sword, including stale/injected offers.
    // Chest acquisition opens the upgrade pool without taking another slot.
    for(unsigned seed=1;seed<=100;++seed) {
        run.start(seed);run.roll_upgrades();
        for(auto offer:run.offers)assert(offer!=Upgrade::MasterSword);
    }
    run.start(1);run.phase=Phase::Upgrade;run.offers[0]=Upgrade::MasterSword;run.offer_rarities[0]=Rarity::Legendary;
    assert(!run.choose(0)&&!run.master_sword&&run.equipment_count()==0);
    unsigned money=40;run.phase=Phase::Treasure;
    assert(run.buy_chest(money,ChestReward::MasterSword,0,Rarity::Common,true));
    assert(run.master_sword==1&&run.eligible(Upgrade::MasterSword));
    run.phase=Phase::Upgrade;
    assert(run.choose(0)&&run.master_sword==6&&run.rank(Upgrade::MasterSword)==6&&run.equipment_count()==1);
    run.ranks[8]=run.ranks[9]=run.ranks[10]=1;
    assert(run.equipment_count()==4&&run.eligible(Upgrade::MasterSword));
    run.banned_items[unsigned(ChestReward::MasterSword)]=true;assert(!run.eligible(Upgrade::MasterSword));
    run.banned_items[unsigned(ChestReward::MasterSword)]=false;
    run.master_sword=29;run.phase=Phase::Upgrade;assert(run.choose(0)&&run.master_sword==30&&!run.eligible(Upgrade::MasterSword));
    run.start(1);run.ranks[8]=run.ranks[9]=run.ranks[10]=run.ranks[11]=1;assert(!run.eligible(Upgrade::MasterSword));
    run.start(1);run.master_sword=1;run.phase=Phase::Upgrade;run.banishes=1;assert(run.banish(Upgrade::MasterSword)&&!run.chest_eligible(ChestReward::MasterSword));
    // Recreate the exact v8 layout, with both integrity categories and all maps.
    Records records;run.start(1);run.phase=Phase::Victory;run.master_sword=7;run.dual_wield=12;
    for(unsigned i=0;i<30;++i)run.ranks[i]=i;
    for(unsigned i=0;i<18;++i)run.damage.dealt[i]=uint64_t(i+1)<<33;
    for(bool cheats:{false,true})for(unsigned map=0;map<5;++map){run.options.allow_cheats=cheats;records.finish(run,map);}
    const auto bytes=legacy_records(records);std::vector<uint8_t> old(bytes.begin(),bytes.begin()+136);old[4]=8;
    for(unsigned n=0;n<10;++n) {
        const auto start=136+n*HighScoreRun::word_count*4;
        old.insert(old.end(),bytes.begin()+start,bytes.begin()+start+50*4);
        const auto damage=start+(20+unsigned(Upgrade::Rupees))*4;
        old.insert(old.end(),bytes.begin()+damage,bytes.begin()+damage+36*4);
    }
    assert(old.size()==Records::v8_byte_count);
    Records restored;assert(restored.decode(old));
    assert(restored.cleared_maps==31&&restored.total_xp==records.total_xp);
    for(bool cheats:{false,true})for(unsigned map=0;map<5;++map) {
        const auto& r=restored.high_score(map,cheats).run;
        assert(r.master_sword==7&&r.dual_wield==12&&r.damage.dealt==run.damage.dealt);
        assert(r.rank(Upgrade::Lightning)==0&&r.rank(Upgrade::MultiStrike)==0);
        for(unsigned i=0;i<30;++i)assert(r.ranks[i]==i);
    }
    Records roundtrip;assert(roundtrip.decode(restored.encode())&&roundtrip.encode()==restored.encode());
    MultiStrikeState series;series.start(StrikeSeries::Spin,3);
    for(unsigned n=3;n>0;--n){assert(series.count(StrikeSeries::Spin)==n);series.consume(StrikeSeries::Spin);}
    assert(!series.count(StrikeSeries::Spin));
    std::cout<<"Survivors sword rewards: scaling, rarity, crit scope, leech provenance and v8 migration passed\n";
}
