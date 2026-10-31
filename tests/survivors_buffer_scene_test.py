"""Production effect snapshots and active-package recognition without changing gameplay."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
fixture=r'''
#define DAWNLIGHT_FREEZE_DIAGNOSTICS 1
#include "survivors_buffer_diagnostics.hpp"
#include "survivors_performance_policy.hpp"
#include "survivors_rules.hpp"
#include <cassert>
#include <ranges>
#include <string>
#include <vector>
using namespace dawnlight::survivors;
namespace survivor_perf=dawnlight::survivor_perf;
namespace dawnlight::survivor_perf{bool mode=false;bool enabled(){return mode;}}
unsigned s_map=2;Run s_run;bool s_hub=true,gift=true;
ChestReward s_chosenTreasure=ChestReward::Count;
bool transitioning(){return false;}bool dComIfGp_isPauseFlag(){return true;}
bool dComIfGp_event_runCheck(){return true;}bool ui_visible(){return false;}
int dMeter2Info_getWindowStatus(){return 1;}int dMeter2Info_getPauseStatus(){return 0;}
bool hub_zelda_gift_busy(){return gift;}
survivor_perf::Budget s_visualBudget;
struct Enemy{unsigned id=0;bool initialized=true,retiring=false,credited=false;};
struct Effect{unsigned id=0,burn=90;struct Burn{unsigned remaining=0;};std::array<Burn,3> legendaryBurns{};std::array<unsigned,2> emitters{};};
struct Actor{int health=50;};std::array<Actor,4> actors;
std::array<Enemy,4>s_enemies;std::array<Effect,4>s_effects;
Actor* actor_by_id(unsigned id){return id<actors.size()?&actors[id]:nullptr;}
struct Emitter{bool isEnableDeleteEmitter(){return false;}} emitter;
Emitter* dComIfGp_particle_getEmitter(unsigned key){return key?&emitter:nullptr;}
struct Attack{bool active=true,exploding=false,reducedVisual=false;unsigned weapon=0;std::array<unsigned,9> emitters{};};
std::array<Attack,5>s_attacks;
struct Bolt{bool active=true,reducedVisual=false;std::array<unsigned,2>emitters{};};
std::array<Bolt,1>s_lightning;
std::array<unsigned,3>s_fireEdgeEmitters{1,2,3};std::array<unsigned,2>s_jumpEmitters{4,5};
struct Package{struct{std::string id,name,version;}metadata;bool active=true;};
struct Loader{std::vector<Package> packages;auto active_mods(){return packages|std::views::filter([](const auto& p){return p.active;});}}loader;
using LoaderFn=Loader&(*)();LoaderFn s_loader=nullptr;
'''
main=r'''
int main(){
 using survivor_perf::Visual;
 for(unsigned i=0;i<4;++i){s_enemies[i].id=s_effects[i].id=i;}
 s_enemies[2].retiring=true;s_effects[3].id=99;
 s_effects[0].emitters={1,2};
 s_attacks[0].reducedVisual=true;
 s_attacks[1].exploding=true;s_attacks[1].emitters.fill(1);
 s_attacks[2].weapon=unsigned(Upgrade::Bow);s_run.ranks[unsigned(Upgrade::FireArrows)]=1;
 s_attacks[3].weapon=unsigned(Upgrade::Boomerang);s_attacks[3].emitters[0]=1;s_attacks[3].emitters[1]=2;
 s_attacks[4].weapon=unsigned(Upgrade::Ball);
 auto scene=survivors_buffer_scene();
 assert(scene.map==2&&scene.hub&&scene.enemies==3&&scene.projectiles==6&&scene.itemEvent==1);
 assert(scene.reward==unsigned(ChestReward::OrdonSword)&&scene.paused&&scene.event&&scene.menu);
 assert(scene.effects[unsigned(Visual::Burn)].active==2&&scene.effects[0].full==1&&scene.effects[0].reduced==0);
 assert(scene.effects[unsigned(Visual::Explosion)].emitters==9);
 assert(scene.effects[unsigned(Visual::Wave)].reduced==1);
 assert(scene.effects[unsigned(Visual::Boomerang)].full==1);
 assert(scene.effects[unsigned(Visual::Chain)].full==1);
 survivor_perf::mode=true;gift=false;s_run.phase=Phase::Treasure;s_chosenTreasure=ChestReward::OrdonSword;
 scene=survivors_buffer_scene();assert(scene.itemEvent==2);
 assert(scene.effects[0].reduced==1&&scene.effects[unsigned(Visual::FireArrow)].reduced==1);
 assert(scene.effects[unsigned(Visual::Chain)].reduced==1);
 assert(scene.effects[unsigned(Visual::FireEdge)].emitters==3&&scene.effects[unsigned(Visual::Jump)].emitters==2);
 for(const auto& e:s_effects)assert(e.burn==90); // Snapshots never mutate damage clocks.
 assert(survivors_fog_inventory().state==-1);
 s_loader=+[]()->Loader&{return loader;};assert(survivors_fog_inventory().state==0);
 loader.packages.push_back({{"example.deferred-fog","Deferred Fog","1.0"},false});
 assert(survivors_fog_inventory().state==0); // Installed but disabled must not count.
 loader.packages.push_back({{"example.DEFERRED_FOG\n","Example","2.0\n"},true});
 auto fog=survivors_fog_inventory();assert(fog.state==1&&fog.matches==1);
 assert(std::string(fog.id.data())=="example.DEFERRED_FOG_"&&std::string(fog.version.data())=="2.0_");
 loader.packages.push_back({{"other.id","Deferred Fog Renderer","3"},true});
 assert(survivors_fog_inventory().matches==2);
 assert(!survivor_perf::fog_name_matches("deferred shadows"));
 assert(!survivor_perf::fog_name_matches("fog renderer"));
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'scene.cpp';exe=Path(tmp)/'scene'
    cpp.write_text(fixture+(ROOT/'src/survivors_buffer_scene.inc').read_text()+(ROOT/'src/survivors_fog_inventory.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Buffer context: live effects, reduced visuals, item events and active fog inventory passed')
