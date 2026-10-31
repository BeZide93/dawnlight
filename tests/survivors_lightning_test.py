from survivors_performance_fixture import helpers
"""Production lightning propagation, targeting, damage provenance and cleanup."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <cassert>
#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;using u32=unsigned;using u16=unsigned short;
constexpr ActorId kNone=~0u;
struct cXyz {
 float x=0,y=0,z=0;cXyz()=default;cXyz(float x,float y,float z):x(x),y(y),z(z){}
 cXyz operator+(cXyz p)const{return{x+p.x,y+p.y,z+p.z};}
 cXyz operator-(cXyz p)const{return{x-p.x,y-p.y,z-p.z};}
 cXyz operator*(float n)const{return{x*n,y*n,z*n};}
 float abs2(cXyz p)const{auto d=*this-p;return d.x*d.x+d.y*d.y+d.z*d.z;}
 float abs()const{return std::sqrt(abs2({}));}float abs(cXyz p)const{return(*this-p).abs();}
};
struct fopAc_ac_c{ActorId id=0;int health=100;cXyz eyePos;};
struct daAlink_c:fopAc_ac_c{int tevStr=0;}player;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
struct Enemy{ActorId id=kNone;bool initialized=false,retiring=false,credited=false,dormant=true;};
std::array<Enemy,30>s_enemies;std::array<fopAc_ac_c,30>actors;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id==id)return &e;return nullptr;}
fopAc_ac_c* actor_by_id(ActorId id){for(auto& a:actors)if(a.id==id&&id!=kNone)return &a;return nullptr;}
ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}
struct Effect{unsigned pending=0;DamageQueue attribution;bool pendingCritical=false;};std::array<Effect,30>effects;
Effect& effect_for(Enemy& enemy){return effects[&enemy-s_enemies.data()];}
void queue_effect_damage(Effect& e,DamageSource source,unsigned power){e.pending+=power;e.attribution.add(source,power);}
unsigned pushes=0;void queue_knockback(Enemy&,fopAc_ac_c*,bool,DamageSource){++pushes;}
ActorId blocked=kNone;bool boss_blocks_from(const Enemy& e,fopAc_ac_c*,const cXyz&){return e.id==blocked;}
struct dBgS_LinChk{cXyz from,to;void Set(const cXyz* a,const cXyz* b,daAlink_c*){from=*a;to=*b;}};
float wallAt=1.e10;struct Bg{bool LineCross(dBgS_LinChk* w){return w->from.x<wallAt&&w->to.x>=wallAt;}} bg;
Bg& dComIfG_Bgsp(){return bg;}
struct Emitter{bool deleted=false;void stopCreateParticle(){}void quitImmortalEmitter(){}void becomeInvalidEmitter(){deleted=true;}};
std::map<u32,Emitter>emitters;u32 nextEmitter=1;
Emitter* dComIfGp_particle_getEmitter(u32 key){auto i=emitters.find(key);return i==emitters.end()?nullptr:&i->second;}
u32 dComIfGp_particle_set(u32 key,u16,cXyz*,int*,void*,void*,int,void*,int,void*,void*,void*) {
 if(!key){key=nextEmitter++;}emitters.try_emplace(key);return key;
}
struct GXColor{unsigned char r,g,b,a;};unsigned lines=0;
void dDbVw_drawLineXlu(cXyz&,cXyz&,GXColor,int,int){++lines;}
u16 lightning_strike_effect(unsigned part){return u16(part+1);}
Run s_run;unsigned s_map=0;std::array<unsigned,unsigned(Upgrade::Rupees)>s_cooldowns{};
'''
main=r'''
void fresh(unsigned count=5){
 clear_lightning();s_run.start(1);s_run.ranks[unsigned(Upgrade::Lightning)]=1;s_cooldowns={};
 s_enemies={};actors={};effects={};pushes=lines=0;blocked=kNone;wallAt=1.e10;
 for(unsigned i=0;i<count;++i){s_enemies[i]={i+1,true};actors[i]={i+1,100,cXyz(float(i)*420,0,0)};}
}
void proc(){for(unsigned i=0;i<100&&!s_cooldowns[unsigned(Upgrade::Lightning)];++i)trigger_lightning(&actors[0]);assert(s_cooldowns[unsigned(Upgrade::Lightning)]);}
unsigned active(){return unsigned(std::count_if(s_lightning.begin(),s_lightning.end(),[](auto& b){return b.active;}));}
int main(){
 // Identical RNG rolls between 20% and 60% only proc with Luck; cooldown still gates them.
 bool luckTriggered=false;
 for(unsigned seed=1;seed<1000&&!luckTriggered;++seed){
  fresh();s_run.start(seed);s_run.ranks[unsigned(Upgrade::Lightning)]=1;
  auto base=s_run,boosted=s_run;
  if(base.roll_chance(2000)||!boosted.roll_chance(6000))continue;
  trigger_lightning(&actors[0]);assert(!active()&&!s_cooldowns[unsigned(Upgrade::Lightning)]);
  s_run.start(seed);s_run.ranks[unsigned(Upgrade::Lightning)]=1;s_run.ranks[unsigned(Upgrade::Luck)]=20;
  trigger_lightning(&actors[0]);assert(active()==1);
  const auto timer=s_cooldowns[unsigned(Upgrade::Lightning)];assert(timer==s_run.item_cooldown(Upgrade::Lightning));
  trigger_lightning(&actors[0]);assert(active()==1&&s_cooldowns[unsigned(Upgrade::Lightning)]==timer);
  luckTriggered=true;
 }
 assert(luckTriggered);
 fresh();proc();assert(active()==1);const auto cooldown=s_cooldowns[unsigned(Upgrade::Lightning)];
 trigger_lightning(&actors[0]);assert(active()==1&&s_cooldowns[unsigned(Upgrade::Lightning)]==cooldown);
 update_lightning(&player);assert(effects[0].pending==3&&effects[0].attribution.swordPending);
 update_lightning(&player);assert(!effects[1].pending); // halfway to the next target
 auto it=std::find_if(s_lightning.begin(),s_lightning.end(),[](auto& b){return b.active;});assert(it->position.x==210);
 draw_lightning();assert(lines==16);
 update_lightning(&player);assert(effects[1].pending==3);
 for(unsigned i=0;i<30;++i)update_lightning(&player);
 assert(effects[2].pending==3&&!effects[3].pending&&!active()&&pushes==3); // first + two hops
 for(auto& [key,e]:emitters){(void)key;assert(e.deleted);}
 fresh();blocked=2;proc();for(unsigned i=0;i<40;++i)update_lightning(&player);
 assert(!effects[1].pending&&!effects[2].pending); // target 3 is beyond level-1 hop range
 fresh();wallAt=300;proc();for(unsigned i=0;i<30;++i)update_lightning(&player);assert(!effects[1].pending&&!active());
 fresh();proc();actors[0].health=0;for(unsigned i=0;i<30;++i)update_lightning(&player);
 assert(!effects[0].pending&&effects[1].pending&&effects[2].pending); // original victim died before discharge
 fresh(25);s_run.ranks[unsigned(Upgrade::Lightning)]=30;proc();
 for(unsigned i=0;i<150;++i)update_lightning(&player);
 for(unsigned i=0;i<21;++i){assert(effects[i].pending==32);}assert(!effects[21].pending&&!active());
 // Every Multishot rank increases chain count; starts are unique when reachable.
 for(unsigned shots=0;shots<=3;++shots){
  fresh(10);for(unsigned i=0;i<10;++i)actors[i].eyePos={float(i)*70,0,0};
  s_run.ranks[unsigned(Upgrade::Multishot)]=shots;proc();assert(active()==1+shots);
  std::array<bool,30>seen{};
  for(const auto& bolt:s_lightning)if(bolt.active){assert(!seen[bolt.target-1]);seen[bolt.target-1]=true;}
  update_lightning(&player);
  // Assigned first legs are reserved even before arrival: each hop prefers
  // unassigned enemies over another chain's closer starting target.
  for(const auto& bolt:s_lightning)if(bolt.active&&bolt.visitedCount>1){
   assert(!seen[bolt.target-1]);seen[bolt.target-1]=true;
  }
 }
 // Maximum hops keep the full four-chain budget and release shared cast state.
 fresh(25);for(unsigned i=0;i<25;++i)actors[i].eyePos={float(i)*20,0,0};
 s_run.ranks[unsigned(Upgrade::Lightning)]=30;s_run.ranks[unsigned(Upgrade::Multishot)]=3;proc();
 std::weak_ptr<LightningTargets> cast;
 for(const auto& b:s_lightning)if(b.active){cast=b.targets;break;}
 for(unsigned i=0;i<150;++i)update_lightning(&player);
 unsigned total=0;for(const auto& e:effects)total+=e.pending;
 assert(total==4*21*32&&!active()&&cast.expired());
 for(const auto& b:s_lightning)if(b.visitedCount==21)
  for(unsigned i=0;i<b.visitedCount;++i)for(unsigned j=i+1;j<b.visitedCount;++j)assert(b.visited[i]!=b.visited[j]);
 // A lone target still receives all four independent chains, once per chain.
 fresh(1);s_run.ranks[unsigned(Upgrade::Multishot)]=3;proc();
 for(unsigned i=0;i<30;++i)update_lightning(&player);
 assert(effects[0].pending==12&&!active());
 // Fallback balances reuse after exhausting the reachable candidates.
 fresh(2);s_run.ranks[unsigned(Upgrade::Multishot)]=3;proc();
 unsigned initial[2]{};for(const auto& b:s_lightning)if(b.active)++initial[b.target-1];
 assert(initial[0]==2&&initial[1]==2);
 for(unsigned i=0;i<40;++i)update_lightning(&player);
 assert(effects[0].pending==12&&effects[1].pending==12&&!active());
 // Extra chains obey the same range, guard, death and wall exclusions.
 fresh(6);for(unsigned i=0;i<6;++i)actors[i].eyePos={float(i)*100,0,0};
 actors[1].health=0;blocked=3;wallAt=450;
 s_run.ranks[unsigned(Upgrade::Multishot)]=3;proc();
 for(const auto& b:s_lightning)if(b.active)assert(b.target==1||b.target==4||b.target==5);
 fresh();s_run.dual_wield=1;s_run.ranks[unsigned(Upgrade::MultiStrike)]=3;s_run.ranks[unsigned(Upgrade::Multishot)]=3;
 proc();assert(active()==16);update_lightning(&player);
 assert(effects[0].pending>=6&&effects[0].pending<=12); // Two starts here; two at the other reachable target. Repeats deferred.
 for(unsigned i=0;i<60;++i)update_lightning(&player);
 assert(effects[0].pending>=24&&effects[0].pending<=96&&!active());
 for(const auto& bolt:s_lightning)assert(!bolt.targets);
 fresh();proc();update_lightning(&player);clear_lightning();assert(!active());
 auto old=effects[0].pending;for(unsigned i=0;i<100;++i)update_lightning(&player);assert(effects[0].pending==old);
 // Visual pressure cannot change chain selection, damage, repeats or lifetime.
 const auto simulate=[&](bool performance){
  fresh(30);survivor_perf::testEnabled=performance;s_visualBudget.clear();
  s_run.dual_wield=1;s_run.ranks[unsigned(Upgrade::MultiStrike)]=3;s_run.ranks[unsigned(Upgrade::Multishot)]=3;
  s_run.ranks[unsigned(Upgrade::Lightning)]=30;proc();unsigned maximum=0;
  for(unsigned tick=0;tick<2000&&active();++tick){
   update_lightning(&player);maximum=std::max(maximum,s_visualBudget.count(survivor_perf::Effect::Lightning));
   if(performance)assert(s_visualBudget.used()<=16);
  }
  assert(!active());if(performance)assert(maximum<=8);
  std::array<unsigned,30> damage{};for(unsigned i=0;i<30;++i)damage[i]=effects[i].pending;return damage;
 };
 assert(simulate(false)==simulate(true));assert(s_visualBudget.used()==0);

}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+helpers()+"void performance_line(cXyz a,cXyz b,GXColor c){dDbVw_drawLineXlu(a,b,c,1,10);}"+(root/'src/survivors_lightning.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors lightning: speed, hops, walls, guards, dead targets, multishot/repeats, effects and cleanup passed')
