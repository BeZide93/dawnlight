"""Fate outcomes, paused timers, reward fractions, native sprint and pickup hooks."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <any>
#include <cassert>
#include <string>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using s32=int;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {
 template<class T>T arg(void* a,int i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}
 template<class T>T& arg_ref(void* a,int i){return std::any_cast<T&>(static_cast<std::any*>(a)[i]);}
}
struct cXyz {float x,y,z;cXyz(float a,float b,float c):x(a),y(b),z(c){}cXyz operator+(cXyz p)const{return{x+p.x,y+p.y,z+p.z};}};
struct daAlink_c {
 enum daAlink_ANM{ANM_RUN,ANM_RUN_B,ANM_WAIT};enum{PROC_MOVE,PROC_DAMAGE};
 unsigned id=1;int mProcID=PROC_MOVE;
};
unsigned fopAcM_GetID(const daAlink_c* p){return p->id;}
struct JPABaseEmitter{};
constexpr unsigned kNone=~0u,dItemNo_RECOVERY_FAILY_e=40;
Run s_run;unsigned s_player=1,s_spawnCooldown=80,life=12,fairies=0,popups=0;
bool active=true,paused=false,failFairy=false;float pendingLife=0;FateEffect lastEffect=FateEffect::Count;
bool in_arena(){return active;}bool playable(){return active&&!paused;}
unsigned dComIfGs_getLife(){return life;}void dComIfGs_setLife(unsigned value){life=value;}
float dComIfGp_getItemLifeCount(){return pendingLife;}void dComIfGp_clearItemLifeCount(){pendingLife=0;}
unsigned create_run_pickup(cXyz pos,unsigned item){
 assert(pos.x==10&&pos.y==70&&pos.z==30&&item==dItemNo_RECOVERY_FAILY_e);
 return failFairy?kNone:++fairies;
}
void fate_feedback(const cXyz& pos,FateEffect effect){assert(pos.x==10&&pos.y==20&&pos.z==30);++popups;lastEffect=effect;}
'''
main=r'''
int main(){
 Run r;r.start(73);
 // Equal-probability rejection sampler reaches every outcome without empty rolls.
 std::array<unsigned,unsigned(FateEffect::Count)> counts{};
 for(unsigned i=0;i<80000;++i)++counts[unsigned(r.roll_fate())];
 for(auto count:counts)assert(count>9500&&count<10500);
 for(unsigned i=0;i<unsigned(FateEffect::Count);++i){
  const auto effect=FateEffect(i);r.start(4);r.apply_fate(effect);
  assert(r.fate_ticks(effect)==(timed_fate(effect)?450u:0u));
  if(!timed_fate(effect)||effect==FateEffect::Horde)continue;
  for(auto phase:{Phase::Upgrade,Phase::TreasureChoice,Phase::Treasure,Phase::Ready}){
   r.phase=phase;for(unsigned n=0;n<500;++n)r.tick(true);assert(r.fate_ticks(effect)==450);
  }
  r.phase=Phase::Running;for(unsigned n=0;n<500;++n)r.tick(false);assert(r.fate_ticks(effect)==450);
  for(unsigned n=0;n<449;++n)r.tick(true);assert(r.fate_ticks(effect)==1);
  r.tick(true);assert(!r.fate_ticks(effect)&&r.fate_speed()==1&&r.luck_percent()==100);
  r.apply_fate(effect);r.tick(true);r.apply_fate(effect);assert(r.fate_ticks(effect)==450);
  r.end(false);assert(!r.fate_ticks(effect));
 }
 r.start(4);r.horde_remaining=123;r.apply_fate(FateEffect::Horde);assert(r.horde_remaining==573);
 r.apply_fate(FateEffect::Sprint);assert(r.fate_speed()==1.5f);
 r.apply_fate(FateEffect::Slow);assert(r.fate_speed()==.5f&&!r.fate_ticks(FateEffect::Sprint));
 r.apply_fate(FateEffect::Sprint);assert(r.fate_speed()==1.5f&&!r.fate_ticks(FateEffect::Slow));
 r.apply_fate(FateEffect::Greed);assert(r.greed()&&r.fate_ticks(FateEffect::Sprint));
 unsigned xp=0,rupees=0;for(unsigned i=0;i<100;++i){xp+=r.gain_xp(1);rupees+=r.gain_rupees(1);}
 assert(xp==150&&rupees==150);assert(r.gain_xp(1)==1&&r.gain_rupees(1)==1);
 assert(r.preview_rupees(1)==2&&r.preview_rupees(1)==2&&r.gain_rupees(1)==2);
 assert(r.gain_xp(5)==8); // XP has its own half-unit remainder.
 r.ranks[unsigned(Upgrade::LifeSteal)]=1;r.ticks=r.life_regen_interval();
 assert(!r.life_steal_active()&&!r.life_regen_due());
 r.phase=Phase::Treasure;r.ranks[unsigned(Upgrade::Bow)]=29;
 unsigned money=1000;const auto price=r.chest_price(false);
 assert(r.buy_chest(money,ChestReward::Bow,0,Rarity::Legendary));
 assert(money==1000-price+120); // Four excess ranks: 80 + 40, cost unchanged.
 r.clear_fate();assert(r.gain_xp(1)==1&&r.gain_rupees(1)==1);
 assert(r.life_steal_active());r.phase=Phase::Running;assert(r.life_regen_due());
 for(unsigned rank=0;rank<=20;++rank)for(unsigned perk=0;perk<=5;++perk){
  r.start(1);r.ranks[unsigned(Upgrade::Luck)]=rank;r.ranks[unsigned(Upgrade::CritChance)]=20;
  r.talents.ranks[unsigned(Talent::LegendaryChance)]=perk;
  const auto luck=r.luck_percent(),drop=r.lucky_chance(200),crit=r.crit_chance_basis_points();
  r.apply_fate(FateEffect::Luck);assert(r.luck_percent()==luck*3/2&&r.lucky_chance(200)==drop*3/2);
  assert(r.crit_chance_basis_points()>crit);auto high=r.rarity_weights();
  uint64_t total=0;for(auto w:high){assert(w);total+=w;}assert(total<=UINT32_MAX);
  for(unsigned n=0;n<1000;++n)assert(unsigned(r.roll_rarity())<5);
  r.apply_fate(FateEffect::Misfortune);assert(!r.fate_ticks(FateEffect::Luck)&&r.luck_percent()==luck/2);
  assert(r.crit_chance_basis_points()<crit);auto low=r.rarity_weights();
  assert(uint64_t(high[4])*low[0]>uint64_t(low[4])*high[0]);
 }
 for(unsigned hp=0;hp<=80;++hp)assert(Run::cursed_life(hp)==(hp+1)/2);
 // Exercise actual runtime dispatch, including immediate effects and queued healing.
 for(unsigned i=0;i<unsigned(FateEffect::Count);++i){
  const auto effect=FateEffect(i);unsigned seed=1;
  for(;;++seed){r.start(seed);if(r.roll_fate()==effect)break;}
  s_run.start(seed);life=13;fairies=popups=0;pendingLife=8;s_spawnCooldown=80;
  assert(activate_fate(nullptr,{10,20,30}));assert(popups==1&&lastEffect==effect);
  assert(life==(effect==FateEffect::Curse?7u:13u));assert(fairies==(effect==FateEffect::Fairy?1u:0u));
  assert(pendingLife==(effect==FateEffect::Greed?0:8));
  assert(s_spawnCooldown==(effect==FateEffect::Horde?0u:80u));
  assert(s_run.fate_ticks(effect)==(timed_fate(effect)?450u:0u));
  if(effect==FateEffect::Fairy){s_run.start(seed);failFairy=true;popups=0;
   assert(!activate_fate(nullptr,{10,20,30})&&!popups);failFairy=false;}
  if(effect==FateEffect::Greed){s_run.start(seed);pendingLife=-4;activate_fate(nullptr,{10,20,30});assert(pendingLife==-4);}
 }
 // Native positive currency only; spending/damage/story mode are untouched.
 s_run.start(1);s_run.apply_fate(FateEffect::Greed);
 std::any reward[]={s32(1)};before_fate_rupees(nullptr,reward,nullptr,nullptr);assert(std::any_cast<s32>(reward[0])==1);
 reward[0]=s32(1);before_fate_rupees(nullptr,reward,nullptr,nullptr);assert(std::any_cast<s32>(reward[0])==2);
 for(s32 amount:{-40,0,100}){active=amount<=0;reward[0]=amount;before_fate_rupees(nullptr,reward,nullptr,nullptr);assert(std::any_cast<s32>(reward[0])==amount);}
 active=true;daAlink_c link;s_run.apply_fate(FateEffect::Sprint);
 for(unsigned gate=0;gate<6;++gate){
  active=gate!=1;paused=gate==2;link.id=gate==3?2:1;link.mProcID=gate==4?daAlink_c::PROC_DAMAGE:daAlink_c::PROC_MOVE;
  s_run.phase=gate==5?Phase::Upgrade:Phase::Running;
  std::any animation[]={static_cast<const daAlink_c*>(&link),daAlink_c::ANM_RUN};
  before_fate_sprint(nullptr,animation,nullptr,nullptr);
  assert(std::any_cast<daAlink_c::daAlink_ANM>(animation[1])==(gate?daAlink_c::ANM_RUN:daAlink_c::ANM_RUN_B));
 }
 active=true;paused=false;link={};s_run.phase=Phase::Running;s_run.apply_fate(FateEffect::Slow);
 std::any animation[]={static_cast<const daAlink_c*>(&link),daAlink_c::ANM_RUN};
 before_fate_sprint(nullptr,animation,nullptr,nullptr);assert(std::any_cast<daAlink_c::daAlink_ANM>(animation[1])==daAlink_c::ANM_RUN);
 std::any particle[]={static_cast<JPABaseEmitter*>(nullptr)};
 assert(before_fate_fairy_particles(nullptr,particle,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 active=false;assert(before_fate_fairy_particles(nullptr,particle,nullptr,nullptr)==HOOK_CONTINUE);
 active=true;JPABaseEmitter emitter;particle[0]=&emitter;assert(before_fate_fairy_particles(nullptr,particle,nullptr,nullptr)==HOOK_CONTINUE);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_fate.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Fate: all outcomes, timers, opposites, Greed rewards/healing, Luck overflow, fairy and sprint hooks passed')
