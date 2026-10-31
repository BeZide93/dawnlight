"""Potion ownership, slot budget, healing ranks and native drinking-hook isolation."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <any>
#include <cassert>
#include "survivors_items.hpp"
using namespace dawnlight::survivors;
using u8=unsigned char;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}
 template<class T>T& arg_ref(void* p,int i){return std::any_cast<T&>(static_cast<std::any*>(p)[i]);}}
constexpr int dItemNo_RED_BOTTLE_e=1,otherBottle=2;
struct daAlink_c{int id=1,mEquipItem=dItemNo_RED_BOTTLE_e;}player;
int fopAcM_GetID(daAlink_c* link){return link->id;}
Run s_run;int s_player=1;bool active=true;
bool in_arena(){return active;}
'''
main=r'''
int main(){
 s_run.start(1);assert(!s_run.owns(ChestReward::RedPotion)&&!s_run.eligible(Upgrade::PotionHealing));
 s_run.phase=Phase::Treasure;unsigned money=10000;
 for(auto item:{ChestReward::Bow,ChestReward::Boomerang,ChestReward::Bombs})assert(s_run.buy_chest(money,item));
 assert(s_run.buy_chest(money,ChestReward::RedPotion,4,Rarity::Legendary));
 assert(s_run.equipment_count()==4&&s_run.owns(ChestReward::RedPotion));
 assert(s_run.rank(Upgrade::PotionHealing)==0&&s_run.potion_heal_quarters()==4);
 assert(!s_run.chest_eligible(ChestReward::Ball)&&!s_run.chest_eligible(ChestReward::Shield));
 assert(!s_run.chest_eligible(ChestReward::RedPotion,false)&&s_run.chest_eligible(ChestReward::RedPotion,true));
 // Drinking changes the bottle contents, not permanent ownership or slot count.
 assert(s_run.eligible(Upgrade::PotionHealing)&&s_run.equipment_count()==4);
 for(unsigned tier=0;tier<5;++tier){
  Run upgraded=s_run;upgraded.phase=Phase::Upgrade;upgraded.offers[0]=Upgrade::PotionHealing;
  upgraded.offer_rarities[0]=Rarity(tier);assert(upgraded.choose(0));
  assert(upgraded.potion_heal_quarters()==5+tier);
 }
 std::any drink[]={&player};
 for(unsigned rank=0;rank<=30;++rank){
  s_run.ranks[unsigned(Upgrade::PotionHealing)]=rank;s_run.phase=Phase::Running;
  assert(s_run.eligible(Upgrade::PotionHealing)==(rank<30));
  std::any heal[]={32.f,u8(0)};
  before_potion_drink(nullptr,drink,nullptr,nullptr);
  assert(before_potion_heal(nullptr,heal,nullptr,nullptr)==HOOK_CONTINUE);
  assert(std::any_cast<float>(heal[0])==4.f+rank);
  after_potion_drink(nullptr,nullptr,nullptr,nullptr);
  heal[0]=32.f;before_potion_heal(nullptr,heal,nullptr,nullptr);
  assert(std::any_cast<float>(heal[0])==32.f); // A separate healing pickup is unchanged.
 }
 assert(item_description(ChestReward::RedPotion,s_run).find("8.50 hearts")!=std::string::npos);
 assert(upgrade_value(Upgrade::PotionHealing,s_run)=="8.50 hearts per red potion");
 assert(s_run.upgrade_gain(Upgrade::PotionHealing,Rarity::Legendary)==0);
 s_run.phase=Phase::Treasure;assert(s_run.buy_chest(money,ChestReward::RedPotion,1));
 assert(s_run.equipment_count()==4&&s_run.rank(Upgrade::PotionHealing)==30);
 for(unsigned reason=0;reason<7;++reason){
  active=true;player={};s_run.phase=Phase::Running;s_run.red_potion=true;
  std::any heal[]={32.f,u8(0)};
  if(reason==0)active=false;if(reason==1)player.id=2;if(reason==2)player.mEquipItem=otherBottle;
  if(reason==3)s_run.phase=Phase::Ready;if(reason==4)s_run.red_potion=false;
  if(reason==5)heal[0]=-4.f;if(reason==6)heal[1]=u8(1);
  const float initial=std::any_cast<float>(heal[0]);
  before_potion_drink(nullptr,drink,nullptr,nullptr);before_potion_heal(nullptr,heal,nullptr,nullptr);
  assert(std::any_cast<float>(heal[0])==initial);after_potion_drink(nullptr,nullptr,nullptr,nullptr);
 }
 active=true;player={};s_run.phase=Phase::Running;s_run.red_potion=true;s_run.endless=true;
 for(unsigned tick:{40*60*ticks_per_second-1,40*60*ticks_per_second,40*60*ticks_per_second+1}) {
  s_run.ticks=tick;std::any heal[]={4.f,u8(0)};
  assert(before_potion_heal(nullptr,heal,nullptr,nullptr)==(s_run.restricted_healing()?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE));
  before_potion_drink(nullptr,drink,nullptr,nullptr);
  assert(before_potion_heal(nullptr,heal,nullptr,nullptr)==HOOK_CONTINUE&&std::any_cast<float>(heal[0])==34.f);
  after_potion_drink(nullptr,nullptr,nullptr,nullptr);
  heal[0]=-4.f;assert(before_potion_heal(nullptr,heal,nullptr,nullptr)==HOOK_CONTINUE);
 }
 // Greed blocks every positive native heal, including owned red potions.
 s_run.start(1);s_run.red_potion=true;player={};active=true;s_run.apply_fate(FateEffect::Greed);
 for(auto phase:{Phase::Running,Phase::Upgrade,Phase::Treasure}){
  s_run.phase=phase;before_potion_drink(nullptr,drink,nullptr,nullptr);
  for(float amount:{-4.f,0.f,4.f,32.f}){
   std::any heal[]={amount,u8(0)};
   assert(before_potion_heal(nullptr,heal,nullptr,nullptr)==(amount>0?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE));
  }
  after_potion_drink(nullptr,nullptr,nullptr,nullptr);
 }
 active=false;std::any normalHeal[]={4.f,u8(0)};
 assert(before_potion_heal(nullptr,normalHeal,nullptr,nullptr)==HOOK_CONTINUE);active=true;
 s_run.clear_fate();assert(before_potion_heal(nullptr,normalHeal,nullptr,nullptr)==HOOK_CONTINUE);
 s_run.start(1);assert(!s_run.red_potion&&s_run.equipment_count()==0&&s_run.rank(Upgrade::PotionHealing)==0);
 s_run.phase=Phase::Treasure;for(unsigned i=0;i<4;++i)assert(s_run.buy_chest(money,ChestReward(i)));
 assert(!s_run.buy_chest(money,ChestReward::RedPotion,4));
}
'''
core=(root/'src/survivors.cpp').read_text()
assert 'DEFINE_HOOK(&daAlink_c::commonBottleDrink, SurvivorsPotionDrink);' in core
assert 'DEFINE_HOOK(&dComIfGp_setItemLifeCount, SurvivorsPotionHeal);' in core
assert 'PRE(SurvivorsPotionDrink, before_potion_drink); POST(SurvivorsPotionDrink, after_potion_drink);' in core
assert 'PRE(SurvivorsPotionHeal, before_potion_heal);' in core
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_potions.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors potions: shared slot, ownership, refill, rarity ranks, healing and native-call isolation passed')
