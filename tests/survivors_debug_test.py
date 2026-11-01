"""Debug defaults, forced-off behavior, rarity floor, charges and run clocks."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=r'''
#include <cassert>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
int main(){
 assert(!debug_session_enabled);debug_session_enabled=true;
 RunOptions options;assert(!options.debug.unlock_all_maps);
 assert(!options.debug.max_equipment_drop_rate&&!options.debug.always_legendary_equipment);
 options.debug.always_legendary_equipment=5;options.debug.clamp();assert(options.debug.always_legendary_equipment==unsigned(debug_enabled));
 options.debug.max_equipment_drop_rate=5;options.debug.clamp();assert(options.debug.max_equipment_drop_rate==unsigned(debug_enabled));
 options.debug.unlock_all_maps=true;options.debug.clamp();assert(options.debug.unlock_all_maps==debug_enabled);
 options.debug={4,10,1,1,5};Run run;run.start(71,options);
 assert(run.run_duration()==(debug_enabled?5u:20u)*60*30);
 assert(run.boss_interval()==run.run_duration()/4);
 assert(run.player_damage(123)==(debug_enabled?1230u:123u));
 assert(run.options.debug.minimum_rarity==(debug_enabled?4u:0u));
 assert(run.options.debug.infinite_rerolls==unsigned(debug_enabled));
 assert(run.options.debug.infinite_banishes==unsigned(debug_enabled));
 assert(run.target_population()==300);
 for(unsigned count:{0u,1u,42u,299u,300u,301u,345u,999u}){
  options.max_normal_enemies=count;run.start(71,options);
  assert(run.target_population()==std::min(count,300u));
 }
 options.max_normal_enemies=42;run.start(71,options);
 options.max_normal_enemies=12;assert(run.target_population()==42u);
 debug_session_enabled=false;run.start(71,options);assert(run.target_population()==12);
 debug_session_enabled=true;run.start(71,options);
 // Even stale/injected state cannot bypass a disabled build.
 run.options.debug={4,10,1,1,5};
 for(unsigned floor=0;floor<5;++floor){run.options.debug.minimum_rarity=floor;
  for(unsigned n=0;n<5000;++n){auto rarity=run.roll_rarity();if(debug_enabled)assert(unsigned(rarity)>=floor);}
 }
 run.options.debug.minimum_rarity=4;run.roll_upgrades();
 if(debug_enabled)for(auto rarity:run.offer_rarities)assert(rarity==Rarity::Legendary);
 run.free_rerolls=run.banishes=0;run.phase=Phase::Upgrade;unsigned rupees=0;
 for(unsigned i=0;i<100;++i){assert(run.pay_reroll(rupees)==debug_enabled);assert(rupees==0&&run.paid_rerolls==0);}
 assert(run.can_banish()==debug_enabled);
 for(auto u:{Upgrade::Power,Upgrade::Haste,Upgrade::Reach})assert(run.banish(u)==debug_enabled);
 assert(run.banishes==0);run.phase=Phase::TreasureChoice;
 assert(run.banish(ChestReward::Bow,true)==debug_enabled);
 assert(!run.pay_reroll(rupees,20)); // Free reroll never pays for the chest.
 rupees=20;if(debug_enabled){assert(run.pay_reroll(rupees,20)&&rupees==20);}
 for(unsigned minutes:{20u,10u,5u}){run.options.debug.run_minutes=minutes;
  const unsigned end=(debug_enabled?minutes:20u)*60*30;assert(run.run_duration()==end);
  run.phase=Phase::Running;run.ticks=end-1;run.horde_remaining=150;assert(!run.tick(false)&&run.ticks==end-1);
  assert(run.tick(true)&&run.ticks==end&&!run.horde_remaining);
 }
 run.options.debug.player_damage=999;assert(run.player_damage(100000)==32767);
 run.start(1);assert(run.run_duration()==duration&&run.player_damage(10)==10&&!run.infinite_rerolls()&&!run.infinite_banishes());
 // Debug classification is captured even with every individual setting at default.
 Run clean;debug_session_enabled=false;clean.start(1,options);
 assert(!clean.debug_run&&clean.run_duration()==duration&&clean.player_damage(10)==10);
 Records records;clean.end(true);records.finish(clean);const auto before=records.encode();
 debug_session_enabled=true;Run test;test.start(2);test.ticks=duration;test.kills=100;test.end(true);
 assert(test.debug_run==debug_enabled);debug_session_enabled=false;
 if(debug_enabled){
  for(bool cheats:{false,true})for(bool modded:{false,true})for(bool victory:{false,true}) {
   Run excluded=test;excluded.options.allow_cheats=cheats;excluded.modded=modded;excluded.end(victory);
   for(unsigned map=0;map<5;++map){records.finish(excluded,map);assert(records.encode()==before);}
  }
  assert(test.earned_meta_xp()==0);records.finish(test,1);assert(records.encode()==before&&!records.map_unlocked(2));
  HighScoreRun snapshot{test,test.score(),1,0};assert(snapshot.encode()==HighScoreRun::Words{});
  assert(test.continue_endless());test.ticks+=9000;test.end(false);records.finish(test,2);
  assert(test.debug_run&&test.earned_meta_xp()==0&&records.encode()==before);
 }
 test.start(3);test.end(true);records.finish(test,1);assert(!test.debug_run&&records.map_unlocked(2)&&records.total_xp>0);
 // An explicit Meta Level edit is ordinary save data in both build configurations.
 Records edited;edited.cleared_maps=3;edited.high_scores[0]=12345;edited.runs=7;
 assert(edited.set_meta_level(30)&&edited.level()==30&&edited.level_xp()==0&&edited.available_points()==29);
 assert(edited.buy(Talent::MaxHealth));edited.total_xp+=123;
 assert(!edited.set_meta_level(30)&&edited.level_xp()==123); // UI no-op preserves partial XP.
 assert(edited.set_meta_level(0)&&edited.level()==2&&edited.talents.spent()==1);
 assert(edited.set_meta_level(~0u)&&edited.total_xp<=max_meta_xp);
 Records loaded;assert(loaded.decode(edited.encode()));
 assert(edited.set_meta_level(30));const auto saved=edited.encode();
 debug_session_enabled=false;options.debug.clamp();assert(loaded.decode(saved)&&loaded.level()==30);
 assert(loaded.cleared_maps==3&&loaded.high_scores[0]==12345&&loaded.runs==7&&loaded.talents.spent()==1);
 Run normal;normal.start(4);normal.ticks=duration;normal.kills=5000;normal.end(true);
 assert(!normal.debug_run&&normal.earned_meta_xp()>=loaded.next_level_xp());
 const auto xp=loaded.total_xp;loaded.finish(normal);
 assert(loaded.level()>30&&loaded.total_xp==xp+normal.earned_meta_xp());
 assert(edited.decode(loaded.encode())&&edited.level()==loaded.level());
 DebugOptions invalid{99,0,5,5,7};invalid.clamp();assert(invalid.run_minutes==20&&invalid.player_damage==1);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(source)
 for enabled in [0,1]:
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Survivors debug: enabled/disabled defaults, rarity floor, infinite resources, damage clamp and timers passed')
