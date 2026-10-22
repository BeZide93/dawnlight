"""Equipment distribution, persistence corruption, capacity and actual Run stats."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=r'''
#include "survivors_rules.hpp"
#include <cassert>
#include <algorithm>
#include <tuple>
using namespace dawnlight::survivors;
int main() {
 // Every talent rank shifts cumulative stat odds upward, within native caps.
 for(unsigned rank=0;rank<=10;++rank) {
  Talents talents;talents.ranks[unsigned(Talent::EquipmentDropRate)]=rank;
  assert(talents.equipment_drop_basis_points()==2000+500*rank);
  unsigned weights[5]{};
  for(unsigned roll=0;roll<1000;++roll) {
   const auto stat=GearInventory::quality_stat(roll,rank);++weights[stat-1];
   if(rank)assert(stat>=GearInventory::quality_stat(roll,rank-1));
   else assert(stat==GearInventory::stat(roll/10));
  }
  assert(weights[0]==500-30*rank&&weights[1]==250&&weights[2]==150+10*rank&&
         weights[3]==80+12*rank&&weights[4]==20+8*rank);
  for(unsigned variant=0;variant<10;++variant)for(unsigned roll=0;roll<100;++roll) {
   unsigned calls=0;auto item=GearInventory::roll([&](unsigned n){
    unsigned value=calls++==0?0:calls==2?variant:calls>5?0:roll;assert(value<n);return value;
   },rank);
   assert(item.valid());
   if(item.kind==GearKind::SharpshooterBow)assert(item.third==(roll<90-2*rank?1:2));
   if(item.kind==GearKind::HurricaneBoomerang)assert(item.second==(roll<90-2*rank?1:2));
  }
 }
 // v12 preserves score offsets and map clears, appending the new talent ranks.
 Records perks;perks.total_xp=meta_threshold(119);perks.cleared_maps=31;
 for(unsigned i=0;i<unsigned(Talent::Count);++i)while(perks.talents.can_buy(Talent(i)))assert(perks.buy(Talent(i)));
 assert(perks.talents.spent()==119&&!perks.available_points());
 Run scored;scored.start(42);scored.end(true);perks.finish(scored,2);
 auto encoded=perks.encode();assert(encoded[4]==14);
 Records read;assert(read.decode(encoded)&&read.encode()==encoded);
 for(auto t:{Talent::EquipmentDropRate,Talent::EquipmentRarity}) {
  auto copy=read;assert(copy.talents.refund(t)&&copy.available_points()==read.available_points()+1);
 }
 // v11 has no equipment ranks; clear-map bits and first score words are not ranks.
 auto v12=encoded;v12[4]=12;assert(read.decode(std::span(v12.data(),Records::v12_byte_count)));
 assert(!read.ranked[2].run.chain_spin&&read.talents.rank(Talent::EquipmentRarity)==10);
 auto legacyPerks=encoded;legacyPerks[4]=11;
 assert(read.decode(std::span(legacyPerks.data(),Records::v11_byte_count)));
 assert(!read.talents.rank(Talent::EquipmentDropRate)&&!read.talents.rank(Talent::EquipmentRarity));
 assert(read.cleared_maps==31&&read.ranked[2].encode()==perks.ranked[2].encode());
 assert(read.total_xp==perks.total_xp&&read.available_points()==perks.available_points()+20);
 assert(!read.decode(legacyPerks)); // Version/length mismatch.
 const auto intact=read.encode();
 for(unsigned offset:{Records::v11_byte_count,Records::v11_byte_count+4}) {
  auto bad=encoded;bad[offset]=11;assert(!read.decode(bad)&&read.encode()==intact);
 }
 auto unfunded=encoded;for(unsigned i=48;i<56;++i)unfunded[i]=0;
 assert(!read.decode(unfunded)&&read.encode()==intact);
 Talents snapshot;snapshot.ranks[unsigned(Talent::EquipmentDropRate)]=10;
 Run active;active.start(42,{},snapshot);snapshot={};
 assert(active.talents.equipment_drop_basis_points()==7000);
 GearInventory gear;
 assert(gear.equipped(GearSlot::Weapon)==GearItem{});
 unsigned counts[5]{};for(unsigned i=0;i<100;++i)++counts[GearInventory::stat(i)-1];
 assert(counts[0]==50&&counts[1]==25&&counts[2]==15&&counts[3]==8&&counts[4]==2);
 unsigned rolls[]{0,0,99,75,50};unsigned pos=0;
 auto sword=GearInventory::roll([&](unsigned n){assert(rolls[pos]<n);return rolls[pos++];});
 assert(sword.kind==GearKind::Sword&&sword.first==5&&sword.second==3&&sword.third==2&&pos==5);
 unsigned tunicRolls[]{4,0,99};pos=0;
 auto tunic=GearInventory::roll([&](unsigned n){assert(tunicRolls[pos]<n);return tunicRolls[pos++];});
 assert(tunic.kind==GearKind::Tunic&&tunic.first==1&&tunic.second==5&&tunic.third==0&&pos==3);
 for(auto kind:{GearKind::Sword,GearKind::Tunic}) {
  unsigned rarities[5]{};
  for(unsigned a=1;a<=5;++a)for(unsigned b=1;b<=5;++b)for(unsigned c=kind==GearKind::Sword?1:0;c<=(kind==GearKind::Sword?5u:0u);++c){
   GearItem item{kind,uint8_t(a),uint8_t(b),uint8_t(c)};assert(item.valid());++rarities[item.rarity()];
   auto better=item;if(better.first<5){++better.first;assert(better.rarity()>=item.rarity());}
  }
  for(auto n:rarities)assert(n);
 }
 for(unsigned i=0;i<GearInventory::capacity;++i)assert(gear.add(i%2?tunic:sword)==i);
 auto full=gear;assert(gear.add(sword)==GearInventory::none&&gear==full);
 assert(gear.equip(0)&&gear.equip(1)&&gear.weapon==0&&gear.tunic==1);
 assert(gear.equip(2)&&gear.weapon==2&&gear.tunic==1);
 assert(gear.erase(2)&&gear.weapon==GearInventory::none&&gear.tunic==1);
 assert(gear.add(sword)==2); // First free slot, row-major; never displace equipped items.
 assert(!gear.equip(GearInventory::none)&&!gear.erase(GearInventory::none)&&gear.add(GearItem{})==GearInventory::none);
 gear.equip(2);
 auto bytes=gear.encode();GearInventory loaded;assert(loaded.decode(bytes)&&loaded==gear);
 for(unsigned i=0;i<bytes.size();++i){auto broken=bytes;broken[i]^=0x80;assert(!loaded.decode(broken)&&loaded==gear);}
 assert(!loaded.decode(std::span(bytes.data(),bytes.size()-1))&&loaded==gear);
 Run base;base.start(123);Run equipped=base;equipped.equip_gear(gear);
 assert(equipped.maximum_hearts==base.maximum_hearts+tunic.first);
 assert(equipped.melee_power()==base.melee_power()+sword.first);
 assert(equipped.attack_power(unsigned(Upgrade::Spin))==base.attack_power(unsigned(Upgrade::Spin))+sword.first);
 assert(equipped.attack_power(unsigned(Upgrade::Wave))==base.attack_power(unsigned(Upgrade::Wave))+sword.first);
 assert(equipped.lightning_power()==base.lightning_power()+sword.first);
 assert(equipped.attack_power(unsigned(Upgrade::Bow))==base.attack_power(unsigned(Upgrade::Bow)));
 assert(equipped.melee_crit_bonus()==base.melee_crit_bonus()+sword.second*100);
 assert(equipped.dodge_basis_points()==base.dodge_basis_points()+tunic.second*100);
 assert(equipped.melee_speed_percent()==base.melee_speed_percent()+sword.third);
 gear.erase(1);assert(equipped.gear_tunic==tunic); // Active build never changes with inventory.
 equipped.phase=Phase::Victory;assert(equipped.continue_endless()&&equipped.gear_weapon==sword);
 equipped.start(444);assert(equipped.gear_weapon==GearItem{});
 // Equipment storage cannot be touched by high-score reset or migration.
 Records records;records.clear_high_scores();GearInventory check;assert(check.decode(bytes)&&check==loaded);
 // Validate every variant, all roll combinations and normalized rarities.
 for(unsigned k=1;k<unsigned(GearKind::Count);++k) {
  auto kind=GearKind(k);unsigned rarities[5]{};
  GearItem item{kind,1,1,uint8_t(kind==GearKind::Tunic?0:1)};
  for(unsigned a=1;a<=5;++a)for(unsigned b=1;b<=5;++b)for(unsigned c=kind==GearKind::Tunic?0:1;c<=(kind==GearKind::Tunic?0u:5u);++c) {
   item.first=a;item.second=b;item.third=c;if(!item.valid())continue;
   ++rarities[item.rarity()];
   GearInventory inv;assert(inv.add(item)==0&&inv.equip(0));
   if(item.is_weapon())assert(inv.weapon==0&&inv.tunic==GearInventory::none);else assert(inv.tunic==0&&inv.weapon==GearInventory::none);
   GearInventory roundtrip;assert(roundtrip.decode(inv.encode())&&roundtrip==inv);
  }
  for(auto n:rarities)assert(n);
 }
 // Both legacy formats preserve their 20 entries/indices, initialize page
 // two empty and translate old sentinel 20 to new sentinel 40.
 const auto checksum=[](auto& b){uint32_t hash=2166136261u;for(unsigned i=0;i<b.size()-4;++i)hash=(hash^b[i])*16777619u;for(unsigned i=0;i<4;++i)b[b.size()-4+i]=uint8_t(hash>>(8*i));};
 GearInventory migrated,legacyExpected;
 std::copy_n(loaded.items.begin(),20,legacyExpected.items.begin());legacyExpected.weapon=loaded.weapon;legacyExpected.tunic=loaded.tunic;
 std::array<uint8_t,GearInventory::legacy_byte_count> legacy{};
 std::copy_n(bytes.begin(),8,legacy.begin());
 for(unsigned i=0;i<20;++i)for(unsigned j=0;j<4;++j)legacy[8+4*i+j]=bytes[8+5*i+j];
 for(uint8_t version:{1,2}) {
  legacy[4]=version;checksum(legacy);assert(migrated.decode(legacy)&&migrated==legacyExpected);
  for(unsigned i=20;i<40;++i)assert(migrated.items[i]==GearItem{});
  assert(migrated.encode()[4]==4);
  auto unequipped=legacy;unequipped[5]=unequipped[6]=20;checksum(unequipped);
  assert(migrated.decode(unequipped)&&migrated.weapon==40&&migrated.tunic==40);
 }
 // v2 can contain specialized weapons, v1 cannot. Page-two slot 20 is a
 // real equipped item in v3, never the legacy empty sentinel.
 legacy[4]=2;legacy[8]=uint8_t(GearKind::HerosBow);checksum(legacy);
 assert(migrated.decode(legacy)&&migrated.items[0].kind==GearKind::HerosBow);
 legacy[4]=1;checksum(legacy);assert(!migrated.decode(legacy));
 auto secondPage=loaded;assert(secondPage.equip(20)&&secondPage.weapon==20);
 auto secondBytes=secondPage.encode();assert(migrated.decode(secondBytes)&&migrated==secondPage);
 assert(secondPage.unequip(20)&&secondPage.weapon==40&&secondPage.items[20]==loaded.items[20]);
 assert(!secondPage.unequip(20)&&!secondPage.unequip(40));
 assert(migrated.decode(bytes)&&migrated==loaded);
 auto invalid=bytes;invalid[8]=uint8_t(GearKind::Count);checksum(invalid);assert(!migrated.decode(invalid)&&migrated==loaded);
 invalid=bytes;invalid[5]=1;checksum(invalid);assert(!migrated.decode(invalid));
 invalid=bytes;invalid[4]=5;checksum(invalid);assert(!migrated.decode(invalid));
 invalid=bytes;invalid[4]=2;checksum(invalid);assert(!migrated.decode(invalid)); // Version/size mismatch.
 invalid=bytes;invalid[8]=uint8_t(GearKind::SharpshooterBow);invalid[11]=3;checksum(invalid);assert(!migrated.decode(invalid));
 for(unsigned n=0;n<GearInventory::legacy_byte_count;++n)assert(!migrated.decode(std::span(bytes.data(),n)));
 // Inventory rows fill page one before page two, including holes after deletion.
 GearInventory pages;for(unsigned i=0;i<40;++i)assert(pages.add(sword)==i);
 assert(pages.add(tunic)==40&&pages.equip(39)&&pages.weapon==39);
 assert(pages.erase(20)&&pages.add(tunic)==20&&pages.equip(20)&&pages.tunic==20);
 assert(pages.unequip(39)&&pages.weapon==40&&pages.tunic==20&&pages.items[39]==sword);
 assert(pages.unequip(20)&&pages.tunic==40&&pages.items[20]==tunic);
 // Roll every variant deterministically; count affixes have exactly 90/10 odds.
 for(unsigned variant=0;variant<10;++variant)for(unsigned roll=0;roll<100;++roll) {
  unsigned calls=0;auto item=GearInventory::roll([&](unsigned n){unsigned value=calls++==0?0:calls==2?variant:calls>5?0:roll;assert(value<n);return value;});
  assert(item.valid()&&item.is_weapon());
  if(item.kind==GearKind::SharpshooterBow)assert(item.third==(roll<90?1:2));
  if(item.kind==GearKind::HurricaneBoomerang)assert(item.second==(roll<90?1:2));
 }
 // v3 keeps both pages and equipment but never invents a random perk on load.
 std::array<uint8_t,GearInventory::v3_byte_count> v3{};std::copy_n(bytes.begin(),8,v3.begin());v3[4]=3;
 for(unsigned i=0;i<40;++i)for(unsigned j=0;j<4;++j)v3[8+4*i+j]=bytes[8+5*i+j];
 checksum(v3);assert(migrated.decode(v3)&&migrated==loaded);
 Run unarmed;unarmed.start(123);
 const auto make=[&](GearKind kind,unsigned value=5){auto r=unarmed;r.gear_weapon={kind,uint8_t(value),uint8_t(value),uint8_t(value)};if(kind==GearKind::SharpshooterBow)r.gear_weapon.third=value==5?2:1;if(kind==GearKind::HurricaneBoomerang)r.gear_weapon.second=value==5?2:1;assert(r.gear_weapon.valid());return r;};
 for(auto kind:{GearKind::HerosBow,GearKind::SharpshooterBow,GearKind::StormBoomerang,GearKind::HurricaneBoomerang,GearKind::BombBag,GearKind::SiegeBombs,GearKind::IronBall,GearKind::BreakerBall}) {
  auto r=make(kind);assert(r.melee_power()==unarmed.melee_power()&&r.melee_speed_percent()==100&&r.melee_crit_bonus()==0);
  for(auto weapon:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball}) {
   assert(r.attack_power(unsigned(weapon))==unarmed.attack_power(unsigned(weapon))); // Still locked.
   r.ranks[unsigned(weapon)]=1;
  }
  Run plain=r;plain.gear_weapon={};
  for(auto weapon:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball})
   assert(r.attack_power(unsigned(weapon))==plain.attack_power(unsigned(weapon))+(r.gear_family(weapon)==r.gear_weapon.family()?5:0));
 }
 for(unsigned value:{1u,5u}) {
  auto great=make(GearKind::Greatsword,value);
  assert(great.melee_power()==unarmed.melee_power()+value&&great.melee_crit_bonus()==0&&great.melee_speed_percent()==100);
  assert(std::abs(great.melee_reach(100)-float(100+2*value))<.001f);
  assert(std::abs(great.melee_area_radius(100)-float(100+2*value))<.001f);
  great.ranks[unsigned(Upgrade::Knockback)]=1;
  assert(std::abs(great.knockback_distance(false,false,DamageSource::Sword)-40.f*(1+.05f*value))<.001f);
  assert(great.knockback_distance(false,false,DamageSource::Bow)==40);
  assert(great.knockback_distance(false,true,DamageSource::Sword)==0);
  auto sniper=make(GearKind::SharpshooterBow,value);sniper.ranks[unsigned(Upgrade::Bow)]=1;
  assert(sniper.arrow_targets()==(value==5?3u:2u));
  assert(std::abs(sniper.item_range(Upgrade::Bow)-item_target_range*(1+.05f*value))<.001f);
  sniper.ranks[unsigned(Upgrade::Piercing)]=10;assert(sniper.arrow_targets()==(value==5?13u:12u));
  auto storm=make(GearKind::StormBoomerang,value);storm.ranks[unsigned(Upgrade::Boomerang)]=1;
  assert(std::abs(storm.boomerang_hop_range()-800*(1+.05f*value))<.001f);
  assert(storm.item_cooldown(Upgrade::Boomerang)<unarmed.item_cooldown(Upgrade::Boomerang));
  auto hurricane=make(GearKind::HurricaneBoomerang,value);hurricane.ranks[unsigned(Upgrade::Boomerang)]=1;
  assert(hurricane.boomerang_hops()==(value==5?3u:2u));
  assert(std::abs(hurricane.item_flight_speed(Upgrade::Boomerang)-42*(1+.05f*value))<.001f);
  assert(std::abs(hurricane.item_flight_speed(Upgrade::Boomerang,true)-55*(1+.05f*value))<.001f);
  assert(hurricane.hurricane_scale()==1); // Equipment name does not grant legendary Hurricane.
  hurricane.ranks[unsigned(Upgrade::Boomerang)]=30;assert(hurricane.boomerang_hops()==(value==5?12u:11u));
  for(auto kind:{GearKind::BombBag,GearKind::SiegeBombs}) {
   auto bombs=make(kind,value);bombs.ranks[unsigned(Upgrade::Bombs)]=1;
   assert(std::abs(bombs.bomb_radius()-182*(1+.02f*value))<.001f);
   assert(bombs.bomb_arrow_radius()==unarmed.bomb_arrow_radius());
   if(kind==GearKind::BombBag)assert(bombs.item_cooldown(Upgrade::Bombs)<unarmed.item_cooldown(Upgrade::Bombs));
   else {bombs.ranks[unsigned(Upgrade::Knockback)]=1;assert(std::abs(bombs.knockback_distance(false,false,DamageSource::Bombs)-40*(1+.05f*value))<.001f);assert(bombs.knockback_distance(false,false,DamageSource::Sword)==40);}
  }
  auto iron=make(GearKind::IronBall,value);iron.ranks[unsigned(Upgrade::Ball)]=1;
  assert(std::abs(iron.ball_range()-900*(1+.05f*value))<.001f);
  assert(iron.item_cooldown(Upgrade::Ball)<unarmed.item_cooldown(Upgrade::Ball));
  auto breaker=make(GearKind::BreakerBall,value);breaker.ranks[unsigned(Upgrade::Ball)]=1;
  assert(std::abs(breaker.item_flight_speed(Upgrade::Ball)-40*(1+.05f*value))<.001f);
  assert(std::abs(breaker.item_flight_speed(Upgrade::Ball,true)-55*(1+.05f*value))<.001f);
  assert(std::abs(breaker.knockback_distance(true,false,DamageSource::Ball)-240*(1+.05f*value))<.001f);
 }
 auto bow=make(GearKind::HerosBow);bow.ranks[unsigned(Upgrade::Bow)]=1;
 unsigned bowCrits=0;for(unsigned i=0;i<30000;++i){bool hit=false;bow.critical_power(10,&hit,false,true);bowCrits+=hit;}
 assert(bowCrits>1300&&bowCrits<1700);
 for(unsigned i=0;i<1000;++i){bool hit=true;bow.critical_power(10,&hit,true);assert(!hit);bow.critical_power(10,&hit);assert(!hit);}
 assert(bow.item_cooldown(Upgrade::Bow)<unarmed.item_cooldown(Upgrade::Bow));
 // Equipment hearts keep the 20-heart maximum; other equipment bonuses
 // remain effective after normal upgrade caps and cooldown floors.
 GearInventory cappedGear;cappedGear.add({GearKind::Tunic,5,5,0});cappedGear.equip(0);
 Run health;health.start(1);health.equip_gear(cappedGear);
 health.maximum_hearts=19;assert(health.eligible(Upgrade::Heart));
 health.phase=Phase::Upgrade;health.offers[0]=Upgrade::Heart;health.offer_rarities[0]=Rarity::Legendary;
 assert(health.choose(0)&&health.maximum_hearts==20&&!health.eligible(Upgrade::Heart));
 assert(health.dodge_basis_points()==health.talents.dodge_basis_points()+500);
 for(auto kind:{GearKind::HerosBow,GearKind::StormBoomerang,GearKind::BombBag,GearKind::IronBall}) {
  auto r=make(kind);auto weapon=kind==GearKind::HerosBow?Upgrade::Bow:kind==GearKind::StormBoomerang?Upgrade::Boomerang:kind==GearKind::BombBag?Upgrade::Bombs:Upgrade::Ball;
  r.ranks[unsigned(weapon)]=30;r.ranks[unsigned(Upgrade::Haste)]=30;
  auto plain=r;plain.gear_weapon={};assert(r.item_cooldown(weapon)<plain.item_cooldown(weapon));
 }
 auto fastSword=make(GearKind::Sword);fastSword.dual_wield=30;fastSword.ranks[unsigned(Upgrade::Haste)]=30;
 auto plainSword=fastSword;plainSword.gear_weapon={};assert(fastSword.melee_cooldown()<plainSword.melee_cooldown());
 // Switching families replaces the one Weapon slot without losing Tunic.
 GearInventory shared;shared.add(tunic);shared.equip(0);
 for(auto kind:{GearKind::Sword,GearKind::HerosBow,GearKind::StormBoomerang,GearKind::BombBag,GearKind::IronBall}) {
  auto index=shared.add({kind,1,1,1});assert(shared.equip(index)&&shared.weapon==index&&shared.tunic==0);
 }

 // Debug forcing supports every family/count affix and retains every Legendary perk option.
 for(unsigned variant=0;variant<11;++variant)for(unsigned pick=0;pick<(variant<2?5u:variant<10?4u:1u);++pick) {
  unsigned calls=0;auto item=GearInventory::roll([&](unsigned bound){
   const auto n=calls++;const unsigned value=n==0?(variant==10?4:0):n==1?variant:pick;
   assert(value<bound);return value;
  },0,true);
  assert(item.valid()&&item.rarity()==4);
  const auto stats=item.stats();
  for(unsigned i=0;i<stats.size();++i){const auto affix=item.definition().affixes[i];assert(stats[i]==(affix.stat==GearStat::None?0:affix.count?2:5));}
  if(item.is_weapon())assert(item.perk==gear_perk_pool(item.family())[pick]&&calls==3);
  else assert(item.perk==GearPerk::None&&calls==1);
 }
 // Exactly one family-compatible perk on new Epic/Legendary weapons; none on tunics/lower tiers.
 for(unsigned variant=0;variant<10;++variant)for(unsigned quality:{0u,94u,99u})for(unsigned pick=0;pick<(quality==99?4u:3u)+(variant<2?1u:0u);++pick) {
  unsigned calls=0;auto item=GearInventory::roll([&](unsigned bound){
   const auto n=calls++;const unsigned value=n==0?0:n==1?variant:n<5?quality:pick;assert(value<bound);if(n==5)assert(bound==(quality==99?4u:3u)+(variant<2?1u:0u));return value;
  });
  assert(item.valid());
  if(item.rarity()<3)assert(item.perk==GearPerk::None&&calls==5);
  else {assert(item.perk==gear_perk_pool(item.family())[pick]);assert(item.perk_levels()==(pick==(variant<2?4u:3u)?1:item.rarity()==4?2:1));}
  GearInventory inv;inv.add(item);inv.equip(0);GearInventory copy;assert(copy.decode(inv.encode())&&copy==inv);
 }
 for(auto bad:std::array{GearItem{GearKind::Sword,1,1,1,GearPerk::Wave},GearItem{GearKind::Sword,4,4,4,GearPerk::FireEdge},
     GearItem{GearKind::Tunic,5,5,0,GearPerk::Haste},GearItem{GearKind::IronBall,5,5,5,GearPerk::BombArrow}})assert(!bad.valid());
 // Perks are read separately from earned ranks, never mutate a build, never unlock chest weapons.
 for(auto pair:std::array{std::pair{GearKind::HerosBow,Upgrade::Bow},std::pair{GearKind::StormBoomerang,Upgrade::Boomerang},
     std::pair{GearKind::BombBag,Upgrade::Bombs},std::pair{GearKind::IronBall,Upgrade::Ball}}) {
  Run r;r.start(42);r.gear_weapon={pair.first,5,5,5,GearPerk::Multishot};
  assert(!r.base_rank(pair.second)&&r.extra_projectiles(pair.second)==0);
  r.ranks[unsigned(pair.second)]=1;assert(r.extra_projectiles(pair.second)==2&&r.extra_projectiles()==0);
  for(auto other:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball})if(other!=pair.second)assert(!r.extra_projectiles(other));
  r.ranks[unsigned(Upgrade::Multishot)]=2;assert(r.extra_projectiles(pair.second)==3); // Hard cap, no array overflow.
  r.gear_weapon={};assert(r.extra_projectiles(pair.second)==2);
 }
 for(auto pair:std::array{std::pair{GearKind::StormBoomerang,Upgrade::Boomerang},std::pair{GearKind::BombBag,Upgrade::Bombs},std::pair{GearKind::IronBall,Upgrade::Ball}}) {
  Run r;r.start(42);r.ranks[unsigned(pair.second)]=1;r.gear_weapon={pair.first,5,5,5};auto base=r;
  r.gear_weapon.perk=GearPerk::Reach;
  assert(r.radius(100,pair.second)>base.radius(100,pair.second)&&r.radius(100)==base.radius(100));
  if(pair.second==Upgrade::Bombs)assert(r.bomb_radius()>base.bomb_radius());
  if(pair.second==Upgrade::Ball)assert(r.ball_range()>base.ball_range());
  if(pair.second==Upgrade::Boomerang)assert(r.boomerang_hop_range()>base.boomerang_hop_range());
  r.gear_weapon.perk=GearPerk::Haste;assert(r.item_cooldown(pair.second)<base.item_cooldown(pair.second)&&r.cooldown(120)==base.cooldown(120));
 }
 Run swordPerk;swordPerk.start(42);swordPerk.gear_weapon={GearKind::Sword,4,4,4,GearPerk::Wave};
 assert(swordPerk.rank(Upgrade::Wave)==1&&swordPerk.base_rank(Upgrade::Wave)==0);
 swordPerk.phase=Phase::Upgrade;swordPerk.offers[0]=Upgrade::Wave;swordPerk.offer_rarities[0]=Rarity::Common;
 assert(swordPerk.choose(0)&&swordPerk.base_rank(Upgrade::Wave)==1&&swordPerk.rank(Upgrade::Wave)==2);
 swordPerk.equip_drop({GearKind::Sword,5,5,5,GearPerk::Spin});assert(swordPerk.rank(Upgrade::Wave)==1&&swordPerk.rank(Upgrade::Spin)==2);
 swordPerk.gear_weapon.perk=GearPerk::MultiStrike;assert(!swordPerk.multi_strikes());swordPerk.dual_wield=1;assert(swordPerk.multi_strikes()==2);
 swordPerk.ranks[unsigned(Upgrade::MultiStrike)]=2;assert(swordPerk.multi_strikes()==3);
 // Lightning is an ordinary sword perk; its appended ID preserves every existing saved perk.
 static_assert(unsigned(GearPerk::FireEdge)==9&&unsigned(GearPerk::ChainSpin)==13&&unsigned(GearPerk::Lightning)==14);
 for(auto kind:{GearKind::Sword,GearKind::Greatsword})for(uint8_t tier:{4,5}) {
  Run r;r.start(42);const unsigned bonus=tier==5?2:1;
  r.equip_drop({kind,tier,tier,tier,GearPerk::Lightning});
  assert(r.gear_weapon.valid()&&r.gear_weapon.perk_levels()==bonus);
  assert(r.gear_weapon.perk_description()=="Bonus: +"+std::to_string(bonus)+" Lightning Strike");
  assert(r.rank(Upgrade::Lightning)==bonus&&!r.base_rank(Upgrade::Lightning)&&r.lightning_hops()>0&&r.lightning_power()>0);
  r.equip_drop({GearKind::HerosBow,4,4,4});assert(!r.rank(Upgrade::Lightning)&&!r.lightning_hops());
  r.equip_drop({kind,tier,tier,tier,GearPerk::Lightning});
  r.phase=Phase::Upgrade;r.offers[0]=Upgrade::Lightning;r.offer_rarities[0]=Rarity::Common;
  assert(r.choose(0)&&r.base_rank(Upgrade::Lightning)==1&&r.rank(Upgrade::Lightning)==1+bonus);
  r.gear_weapon={};assert(r.rank(Upgrade::Lightning)==1&&r.lightning_hops()>0);
  r.equip_drop({kind,tier,tier,tier,GearPerk::Lightning});r.ranks[unsigned(Upgrade::Lightning)]=30;
  assert(r.rank(Upgrade::Lightning)==30&&r.base_rank(Upgrade::Lightning)==30);
 }
 for(auto bad:std::array{GearItem{GearKind::Sword,3,3,3,GearPerk::Lightning},
     GearItem{GearKind::HerosBow,5,5,5,GearPerk::Lightning},GearItem{GearKind::Tunic,5,5,0,GearPerk::Lightning}})assert(!bad.valid());
 // Legendary effects are Boolean; acquiring the run version and later changing gear retains it.
 for(auto tuple:std::array{std::tuple{GearKind::Sword,GearPerk::FireEdge,Upgrade::FireEdge,Upgrade::Count},
     std::tuple{GearKind::HerosBow,GearPerk::BombArrow,Upgrade::BombArrow,Upgrade::Bow},
     std::tuple{GearKind::StormBoomerang,GearPerk::Hurricane,Upgrade::Hurricane,Upgrade::Boomerang},
     std::tuple{GearKind::BombBag,GearPerk::FireBomb,Upgrade::FireBomb,Upgrade::Bombs},
     std::tuple{GearKind::IronBall,GearPerk::ChainSpin,Upgrade::ChainSpin,Upgrade::Ball}}) {
  const auto [kind,perk,upgrade,weapon]=tuple;Run r;r.start(42);if(weapon!=Upgrade::Count)r.ranks[unsigned(weapon)]=1;
  r.phase=Phase::Upgrade;r.offers[0]=upgrade;r.offer_rarities[0]=Rarity::Legendary;assert(r.choose(0));
  r.gear_weapon={kind,5,5,5,perk};assert(r.rank(upgrade)==1);r.gear_weapon={};assert(r.rank(upgrade)==1);
 }
 Run chain;chain.start(42);assert(!chain.eligible(Upgrade::ChainSpin));chain.ranks[unsigned(Upgrade::Ball)]=1;
 assert(chain.eligible(Upgrade::ChainSpin)&&upgrade_min_rarity(Upgrade::ChainSpin)==Rarity::Legendary);
 chain.phase=Phase::Upgrade;chain.offers[0]=Upgrade::ChainSpin;chain.offer_rarities[0]=Rarity::Epic;assert(!chain.choose(0));
 chain.offer_rarities[0]=Rarity::Legendary;assert(chain.choose(0)&&chain.chain_spin==1&&!chain.eligible(Upgrade::ChainSpin));
 chain.end(true);Records spinRecord;spinRecord.finish(chain,0);auto spinBytes=spinRecord.encode();Records spinRead;
 assert(spinRead.decode(spinBytes)&&spinRead.ranked[0].run.chain_spin==1);
 spinBytes[4*Records::v12_word_count]=2;assert(!spinRead.decode(spinBytes));

}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(source)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors equipment: distributions, all rarities, capacity, selection, corruption and combat bonuses passed')
