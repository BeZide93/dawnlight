#include "../src/survivors_rules.hpp"
#include <cassert>
#include "survivors_legacy_records.hpp"
#include <iostream>
using namespace dawnlight;
using namespace dawnlight::survivors;
void high_score_tests(){
    static_assert(Records::byte_count<65536);
    for(bool cheats:{false,true})for(bool modded:{false,true}) {
        Records records;Run run;run.start(42);run.phase=Phase::Victory;
        run.options.allow_cheats=cheats;run.modded=modded;run.ticks=12345;run.kills=321;run.level=42;
        run.timed_elites=4;run.guard_elites=12;run.chests_opened=7;
        run.master_sword=3;run.shield=2;run.dual_wield=4;run.dark_link=5;run.red_potion=true;
        run.options.boss_hp=4;run.options.enemy_hp=5;run.options.enemy_damage=4;run.options.enemy_speed=3;run.options.horde=5;
        for(unsigned i=0;i<run.ranks.size();++i)run.ranks[i]=i;
        for(unsigned i=0;i<run.damage.dealt.size();++i)run.damage.dealt[i]=(uint64_t(1)<<35)+i;
        for(unsigned map=0;map<5;++map)records.finish(run,map);
        const bool assisted=cheats||modded;
        for(unsigned map=0;map<5;++map) {
            const auto& best=records.high_score(map,assisted);
            assert(best.present()&&best.score==run.score()&&best.run.damage.dealt==run.damage.dealt);
            assert(!records.high_score(map,!assisted).present());
        }
        const auto before=records.high_score(0,assisted).encode();
        auto worse=run;worse.kills=1;records.finish(worse,0);assert(records.high_score(0,assisted).encode()==before);
        run.damage.dealt[0]=0;records.finish(run,0);assert(records.high_score(0,assisted).encode()==before); // Ties retain original run.
        ++run.kills;records.finish(run,0);assert(records.high_score(0,assisted).run.damage.dealt[0]==0);
        Records restored;const auto bytes=records.encode();assert(restored.decode(bytes)&&restored.encode()==bytes);
        assert(restored.high_score(0,assisted).run.ranks==run.ranks);
        assert(restored.high_score(0,assisted).run.options.boss_hp==4);
        auto v13=legacy_records(records);v13[4]=13;Records migrated;
        assert(migrated.decode(std::span(v13.data(),Records::v13_byte_count)));
        assert(migrated.high_score(0,assisted).run.options.boss_hp==5);
        assert(migrated.high_score(0,assisted).score==restored.high_score(0,assisted).score);
        assert(migrated.decode(migrated.encode())&&migrated.high_score(0,assisted).run.options.boss_hp==5);
        for(unsigned invalid:{0u,11u,255u}) {
            auto badBoss=bytes;badBoss[4*(Records::v13_word_count+(assisted?5:0))]=invalid;
            assert(!restored.decode(badBoss)&&restored.encode()==bytes);
        }
        auto absentBoss=bytes;absentBoss[4*(Records::v13_word_count+(assisted?0:5))]=1;
        assert(!restored.decode(absentBoss)&&restored.encode()==bytes);
        const auto preserved=restored;
        restored.clear_high_scores();
        assert(restored.cleared_maps==31&&restored.total_xp==preserved.total_xp&&restored.runs==preserved.runs&&restored.wins==preserved.wins);
        for(unsigned map=0;map<5;++map)assert(!restored.ranked[map].present()&&!restored.cheated[map].present()&&restored.map_unlocked(map));
        assert(restored.decode(restored.encode()));
        auto legacy=bytes;legacy[4]=7;
        assert(restored.decode(std::span(legacy.data(),136))&&restored.cleared_maps==31&&restored.total_xp==preserved.total_xp);
        for(unsigned map=0;map<5;++map)assert(!restored.ranked[map].present()&&!restored.cheated[map].present());
        assert(restored.high_scores==preserved.high_scores); // Unknown legacy integrity never becomes Ranked.
        auto corrupt=bytes;
        const unsigned start=136+(assisted?5*HighScoreRun::word_count*4:0);
        corrupt[start+16]^=3; // Move an intact snapshot into the wrong category.
        if(assisted)corrupt[start+16]=4; // Strip both integrity bits.
        assert(!restored.decode(corrupt));
        corrupt=bytes;corrupt[start]=3;assert(!restored.decode(corrupt));
        corrupt=bytes;corrupt[start+16]=0x80;assert(!restored.decode(corrupt));
        corrupt=bytes;corrupt[start+60]=0;assert(!restored.decode(corrupt)); // Invalid difficulty.
        const auto intact=restored.encode();
        assert(!restored.decode(std::span(bytes.data(),bytes.size()-4))&&restored.encode()==intact);
    }
    Records zero;Run run;run.start(1);run.end(false);zero.finish(run,0);
    assert(zero.ranked[0].present()&&zero.ranked[0].score==0); // A valid zero-score run is still viewable.
    const auto words=zero.ranked[0].encode();HighScoreRun score;
    assert(score.decode(words)&&score.present());
    auto bad=words;const unsigned damageStart=20+unsigned(Upgrade::Rupees);
    bad[damageStart]=bad[damageStart+1]=UINT32_MAX;bad[damageStart+2]=1;
    assert(!score.decode(bad)); // Refuse overflowing totals.
}
void half_step_records_tests() {
    Records records;
    for(bool assisted:{false,true})for(bool endless:{false,true})for(unsigned map=0;map<5;++map) {
        Run run;RunOptions options{2.5f,3.5f,1.5f,4.5f,200};options.boss_hp=4.5f;options.allow_cheats=assisted;
        run.start(1,options);run.end(true);
        if(endless){assert(run.continue_endless());run.end(false);}
        records.finish(run,map);
    }
    Records restored;const auto bytes=records.encode();assert(restored.decode(bytes)&&restored.encode()==bytes);
    for(bool assisted:{false,true})for(bool endless:{false,true})for(unsigned map=0;map<5;++map) {
        const auto& best=restored.high_score(map,assisted,endless);assert(best.present());
        const auto& o=best.run.options;
        assert(o.enemy_hp==2.5f&&o.enemy_damage==3.5f&&o.enemy_speed==1.5f&&o.horde==4.5f&&o.boss_hp==4.5f);
        assert(best.run.twilit_rank()==16.5f&&o.score_percent()==330);
        HighScoreRun alone;assert(alone.decode(best.encode())&&alone.run.options.boss_hp==4.5f);
    }
    // A historical rank zero stays zero despite its independent Boss HP setting.
    Records historical;Run old;old.start(1);old.options.boss_hp=10;old.end(true);historical.finish(old,0);
    assert(restored.decode(legacy_records(historical)));
    assert(restored.ranked[0].run.twilit_rank()==0&&restored.ranked[0].run.options.score_percent()==100);
    const auto oldScore=restored.ranked[0].score;
    assert(restored.decode(restored.encode())&&restored.ranked[0].score==oldScore);
    assert(restored.ranked[0].run.twilit_rank()==0&&restored.ranked[0].run.options.boss_hp==10);
    // v15's temporary weighted formula and x10 caps remain readable.
    Records weighted;Run prior;prior.start(2);prior.options={10,9.5f,2.5f,4.5f,200};
    prior.options.boss_hp=6.5f;prior.options.weighted_difficulty=true;prior.end(true);weighted.finish(prior,1);
    auto v15=weighted.encode();v15[4]=15;
    for(unsigned n=0;n<20;++n)v15[4*(34+n*HighScoreRun::word_count+4)]&=~32u;
    assert(restored.decode(v15)&&restored.ranked[1].run.twilit_rank()==28);
    assert(restored.ranked[1].run.options.score_percent()==445);
    assert(restored.decode(restored.encode())&&restored.ranked[1].run.options.boss_hp==6.5f);
    Run fresh;fresh.start(1,old.options);assert(fresh.twilit_rank()==9);
}
int main(){
    half_step_records_tests();
    high_score_tests();
    Records gates;Run finished;finished.start(1);
    assert(gates.map_unlocked(0)&&!gates.map_unlocked(1)&&!gates.map_unlocked(5)&&!gates.map_unlocked(~0u));
    for(unsigned map=0;map<5;++map){
        assert(gates.map_unlocked(map));
        finished.phase=Phase::Defeat;gates.finish(finished,map);
        assert(!(gates.cleared_maps&(1u<<map))&&!gates.map_unlocked(map+1));
        finished.phase=Phase::Victory;gates.finish(finished,map);
        assert(gates.cleared_maps&(1u<<map));
        if(map<4)assert(gates.map_unlocked(map+1));
        Records roundtrip;assert(roundtrip.decode(gates.encode())&&roundtrip.cleared_maps==gates.cleared_maps);
    }
    auto badMask=gates.encode();badMask[132]=32;
    assert(!gates.decode(badMask)&&gates.cleared_maps==31);
    Run r;r.start(1);assert(r.score()==0);
    for(unsigned level=0;level<=5;++level) {
        Talents perks;
        for(auto t:{Talent::BonusDamage,Talent::MeleeHaste,Talent::CritChance,Talent::CritDamage,Talent::AutoRupees})perks.ranks[unsigned(t)]=level;
        r.start(1,{},perks);
        assert(r.melee_power()==3+level&&r.attack_power(unsigned(Upgrade::Bow))==2+level);
        assert(r.crit_chance_basis_points()==100*level&&r.crit_damage_percent()==200+10*level);
        assert(r.melee_speed_percent()==100+5*level&&r.melee_cooldown()==3000/(100+5*level));
        assert(!r.auto_rupees());r.ticks=149;assert(!r.auto_rupees());r.tick(true);assert(r.auto_rupees()==level);
        r.ranks[unsigned(Upgrade::Power)]=3;r.master_sword=2;
        assert(r.melee_power()==10+level);
        r.ranks[unsigned(Upgrade::CritDamage)]=30;assert(r.crit_damage_percent()==350+10*level);
    }
    for(unsigned rank=0;rank<=10;++rank) {
        Talents perks;perks.ranks[unsigned(Talent::BonusDamage)]=perks.ranks[unsigned(Talent::CritChance)]=rank;
        r.start(9,{},perks);assert(r.bonus_damage()==rank&&r.melee_power()==3+rank&&r.crit_chance_basis_points()==100*rank);
    }
    for(unsigned rank=0;rank<=5;++rank) {
        Talents perks;perks.ranks[unsigned(Talent::LegendaryChance)]=rank;
        r.start(9,{},perks);auto weights=r.rarity_weights();uint64_t total=0;for(auto w:weights)total+=w;
        assert(total==1000000000&&weights[4]==10000000+6000000*rank);
        const auto base=weights;
        for(unsigned luck=0;luck<=20;++luck) {
            r.ranks[unsigned(Upgrade::Luck)]=luck;weights=r.rarity_weights();total=0;for(auto w:weights)total+=w;
            assert(total<=UINT32_MAX&&uint64_t(weights[4])*1000000000>=uint64_t(base[4])*total);
            for(unsigned n=0;n<100;++n)assert(r.roll_rarity()<=Rarity::Legendary);
        }
    }
    r.start(9);r.talents.ranks[unsigned(Talent::LegendaryChance)]=5;
    unsigned legendary=0;for(unsigned i=0;i<100000;++i)legendary+=r.roll_rarity()==Rarity::Legendary;
    assert(legendary>3700&&legendary<4300);
    r.start(1);assert(!r.bonus_damage()&&r.melee_speed_percent()==100&&r.crit_damage_percent()==200);
    r.ticks=60*30;r.kills=100;r.level=5;assert(r.score()==1320);
    r.timed_elites=1;r.guard_elites=2;r.kills=103;assert(r.score()==2320);
    r.options.horde=5;r.kills=503;assert(r.score()==4176); // Same normalized kills, +80% difficulty.
    r.options={5,5,5,5,500};r.options.boss_hp=5;assert(r.options.score_percent()==500&&r.score()==11600);
    r.end(true);assert(r.score()==26600);
    Records records;records.finish(r,2);assert(records.high_scores[2]==26600&&records.total_xp==26600);
    assert(records.high_scores[0]==0&&records.runs==1&&records.wins==1);
    assert(records.level()==8&&records.available_points()==7&&records.level_xp()==2100&&records.next_level_xp()==5500);
    Records untouched=records;r.start(1);records.finish(r,0);assert(records.encode()==untouched.encode());
    r.end(false);records.finish(r,5);assert(records.encode()==untouched.encode());
    records.finish(r,2);assert(records.high_scores[2]==26600); // Never replace a higher score.
    records.total_xp=meta_threshold(99);assert(records.available_points()==99);
    assert(!records.buy(Talent::SprintSpeed)&&!records.buy(Talent::Glider)&&!records.buy(Talent::Gale)&&!records.buy(Talent::GaleCharges));
    for(unsigned i=0;i<unsigned(Talent::EquipmentDropRate);++i){
        for(unsigned n=0;n<talent_caps[i];++n)assert(records.buy(Talent(i)));
        assert(!records.buy(Talent(i)));
    }
    assert(records.available_points()==0&&records.talents.spent()==99&&records.talents.valid());
    auto refunded=records;
    assert(!refunded.talents.refund(Talent::Count));
    assert(refunded.talents.refund(Talent::ManualJump));
    assert(refunded.available_points()==5&&refunded.talents.valid());
    assert(!refunded.talents.refund(Talent::ManualJump));
    assert(refunded.talents.refund(Talent::SprintSpeed)&&refunded.available_points()==6);
    assert(refunded.talents.rank(Talent::SprintSpeed)==1);
    assert(refunded.talents.refund(Talent::Sprint)&&refunded.available_points()==8);
    assert(refunded.talents.rank(Talent::SprintSpeed)==0&&refunded.talents.valid());
    assert(refunded.total_xp==records.total_xp&&refunded.high_scores==records.high_scores);
    Records refundLoaded;assert(refundLoaded.decode(refunded.encode()));
    r.start(3,{},records.talents);
    assert(r.maximum_hearts==13&&r.free_rerolls==6&&r.banishes==6);
    assert(r.rank(Upgrade::Heart)==0&&r.rank(Upgrade::Magnet)==0&&r.rank(Upgrade::CritChance)==0);
    assert(r.magnet_range()==450&&r.crit_chance_basis_points()==1000);
    r.ranks[unsigned(Upgrade::CritChance)]=30;r.ranks[unsigned(Upgrade::Luck)]=20;
    assert(r.crit_chance_basis_points()==4875);
    unsigned crits=0;for(int i=0;i<10000;++i){auto damage=r.critical_power(100);assert(damage==100||damage==250);crits+=damage==250;}
    assert(crits>4650&&crits<5100);
    r.ranks[unsigned(Upgrade::Magnet)]=30;assert(r.magnet_range()==6300);
    unsigned money=0;
    for(auto u:{Upgrade::Power,Upgrade::Haste,Upgrade::Reach,Upgrade::Heart,Upgrade::CritDamage,Upgrade::Knockback}){
        r.phase=Phase::Upgrade;assert(r.pay_reroll(money));assert(r.banish(u));
    }
    assert(r.free_rerolls==0&&r.banishes==0);
    assert(!r.pay_reroll(money)&&r.reroll_cost()==50&&!r.banish(Upgrade::Magnet));
    r.start(3,{},records.talents);assert(r.free_rerolls==6&&r.banishes==6&&r.paid_rerolls==0);
    r.start(3);assert(r.maximum_hearts==3&&r.crit_chance_basis_points()==0&&r.magnet_range()==150);
    int64_t value=99;
    for(auto setting:{ModeSetting::Sprint,ModeSetting::Jump,ModeSetting::Glide,ModeSetting::Gale}){
        assert(talent_mode_override({},setting,value)&&value==0);
        assert(talent_mode_override(records.talents,setting,value)&&value==1);
    }
    assert(talent_mode_override(records.talents,ModeSetting::GaleCharges,value)&&value==3);
    assert(talent_mode_override({},ModeSetting::GaleCharges,value)&&value==1);
    assert(records.talents.sprint_multiplier()==1.5f);
    for(unsigned rank=0;rank<=5;++rank){Talents t;t.ranks[unsigned(Talent::Dodge)]=rank;
        assert(t.dodge_basis_points()==(rank ? 500+125*(rank-1) : 0));}
    Records loaded;const auto bytes=records.encode();assert(loaded.decode(bytes)&&loaded.encode()==bytes);
    auto corrupt=bytes;corrupt[4]=99;assert(!loaded.decode(corrupt)&&loaded.encode()==bytes);
    corrupt=bytes;corrupt[56]=11;assert(!loaded.decode(corrupt)); // Max Health exceeds the new cap.
    corrupt=bytes;corrupt[76]=0;assert(!loaded.decode(corrupt)); // Glider without Manual Jump.
    corrupt=bytes;for(unsigned i=48;i<56;++i)corrupt[i]=0;assert(!loaded.decode(corrupt)); // Unfunded ranks.
    assert(!loaded.decode(std::span(bytes.data(),bytes.size()-1)));
    // Existing fully spent saves keep their old ranks and can grow into the
    // expanded caps once more points are earned; no migration/reset is needed.
    auto oldCaps=records;oldCaps.total_xp=meta_threshold(73);
    oldCaps.talents.ranks[unsigned(Talent::MaxHealth)]=5;
    oldCaps.talents.ranks[unsigned(Talent::Magnet)]=5;
    oldCaps.talents.ranks[unsigned(Talent::Rerolls)]=2;
    oldCaps.talents.ranks[unsigned(Talent::Banishes)]=2;
    Records restored;assert(restored.decode(oldCaps.encode()));
    assert(restored.talents.ranks==oldCaps.talents.ranks&&restored.available_points()==0);
    restored.total_xp=meta_threshold(99);
    for(auto t:{Talent::MaxHealth,Talent::Magnet,Talent::Rerolls,Talent::Banishes})
        while(restored.talents.can_buy(t))assert(restored.buy(t));
    assert(restored.talents.ranks==records.talents.ranks&&restored.available_points()==0);
    auto legacyRecords=records;
    legacyRecords.talents.ranks[unsigned(Talent::CritChance)]=5;
    legacyRecords.talents.ranks[unsigned(Talent::BonusDamage)]=5;
    legacyRecords.talents.ranks[unsigned(Talent::LegendaryChance)]=0;
    legacyRecords.talents.ranks[unsigned(Talent::LifeRegen)]=0;
    const auto legacyBytes=legacyRecords.encode();
    auto v2=legacyBytes;v2[4]=2;
    assert(loaded.decode(std::span(v2.data(),100)) && loaded.talents.rank(Talent::Dodge)==0);
    assert(loaded.total_xp==records.total_xp && loaded.high_scores==records.high_scores);
    for(unsigned i=0;i<11;++i)assert(loaded.talents.ranks[i]==legacyRecords.talents.ranks[i]);
    assert(loaded.available_points()==46); // New perks never spends legacy points.
    assert(loaded.talents.rank(Talent::StartingItem)==0);
    auto v3=legacyBytes;v3[4]=3;assert(loaded.decode(std::span(v3.data(),104)));
    assert(loaded.talents.rank(Talent::Dodge)==5&&loaded.talents.rank(Talent::StartingItem)==0&&loaded.available_points()==41);
    auto v4=legacyBytes;v4[4]=4;assert(loaded.decode(std::span(v4.data(),108)));
    assert(loaded.talents.rank(Talent::StartingItem)==1&&loaded.available_points()==40);
    for(auto talent:{Talent::AutoRupees,Talent::BonusDamage,Talent::MeleeHaste,Talent::CritDamage}) {
        assert(!loaded.talents.rank(talent));
        auto invalid=bytes;invalid[56+4*unsigned(talent)]=talent_caps[unsigned(talent)]+1;assert(!loaded.decode(invalid));
    }
    assert(loaded.total_xp==records.total_xp&&loaded.high_scores==records.high_scores);
    auto v6=legacyBytes;v6[4]=6;assert(loaded.decode(std::span(v6.data(),132)));
    assert(loaded.cleared_maps==0&&loaded.high_scores==records.high_scores&&loaded.talents.ranks==legacyRecords.talents.ranks);
    auto v5=legacyBytes;v5[4]=5;assert(loaded.decode(std::span(v5.data(),124)));
    assert(loaded.talents.ranks==legacyRecords.talents.ranks&&loaded.available_points()==20);
    assert(!loaded.talents.rank(Talent::LegendaryChance)&&!loaded.talents.rank(Talent::LifeRegen));
    for(auto talent:{Talent::CritChance,Talent::LegendaryChance,Talent::LifeRegen}) {
        auto invalid=bytes;invalid[56+4*unsigned(talent)]=talent_caps[unsigned(talent)]+1;assert(!loaded.decode(invalid));
    }
    auto badStarting=bytes;badStarting[104]=2;assert(!loaded.decode(badStarting));
    auto refundedStarting=records;assert(refundedStarting.talents.refund(Talent::StartingItem));
    assert(refundedStarting.available_points()==1&&refundedStarting.talents.rank(Talent::StartingItem)==0);
    r.start(12,{},records.talents);assert(r.starting_item_pending);
    r.start(12);assert(!r.starting_item_pending);
    auto badDodge=bytes;badDodge[100]=6;assert(!loaded.decode(badDodge));
    std::array<uint8_t,28> legacy{};const std::array<uint32_t,7> words{0x56525348,1,5,2,36000,400,18};
    for(unsigned i=0;i<7;++i)for(unsigned b=0;b<4;++b)legacy[i*4+b]=uint8_t(words[i]>>(8*b));
    assert(loaded.decode(legacy)&&loaded.runs==5&&loaded.wins==2&&loaded.best_level==18);
    assert(loaded.total_xp==0&&loaded.level()==1&&loaded.available_points()==0&&loaded.talents.spent()==0);
    for(unsigned n=0;n<100;++n){assert(meta_levels(meta_threshold(n))==n);if(n)assert(meta_levels(meta_threshold(n)-1)==n-1);}
    std::cout<<"Survivors progression: score, map records, XP, talents, run reset and save migration passed\n";
}
