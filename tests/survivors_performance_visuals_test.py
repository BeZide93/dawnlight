"""Exercise production live-toggle cleanup and nearest visible burn selection."""
from pathlib import Path
import os, subprocess, tempfile
from survivors_performance_fixture import helpers

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT/'src/survivors_performance_visuals.inc').read_text()
source = source[:source.index('// One reusable packet')]
fixture = r'''
#include <array>
#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>
#include <cstdint>
using u32=uint32_t;
struct cXyz{float x=0,y=0,z=0;float abs2(const cXyz& b)const{return (x-b.x)*(x-b.x)+(y-b.y)*(y-b.y)+(z-b.z)*(z-b.z);}};
struct Actor{int health=50;struct{cXyz pos;}current;cXyz eyePos;};
using daAlink_c=Actor;
constexpr unsigned enemy_capacity=64;
struct Enemy{bool initialized=true,retiring=false,credited=false;unsigned id=0;};
struct Effect{unsigned id=0,burn=90;struct Burn{unsigned remaining=0;};std::array<Burn,3> legendaryBurns{};std::array<u32,2> emitters{};};
struct Attack{bool active=true,exploding=false,reducedVisual=false;unsigned damage=100,life=20;std::array<u32,9> emitters{};};
struct Bolt{bool active=true,reducedVisual=false;std::array<u32,2> emitters{};};
std::array<Enemy,enemy_capacity> s_enemies;
std::array<Effect,enemy_capacity> s_effects;
std::array<Actor,enemy_capacity> actors;
std::array<bool,enemy_capacity> s_burnVisuals{};
std::array<Attack,2> s_attacks;std::array<Bolt,2> s_lightning;
std::array<u32,3> s_fireEdgeEmitters{};std::array<u32,2> s_jumpEmitters{};
unsigned stopped=0;
struct Emitter{void stopCreateParticle(){}void quitImmortalEmitter(){}void becomeInvalidEmitter(){++stopped;}} emitter;
Emitter* dComIfGp_particle_getEmitter(u32 key){return key?&emitter:nullptr;}
Actor* actor_by_id(unsigned id){return id<actors.size()?&actors[id]:nullptr;}
float projection[4][4]{{},{},{},{0,0,0,2}};
bool dComIfGd_getView(){return true;}
auto* dComIfGd_getProjViewMtx(){return &projection;}
void mDoLib_project(const cXyz* a,cXyz* b){*b=*a;}
struct mDoGph_gInf_c{static float getMinXF(){return 0;}static float getMaxXF(){return 640;}static float getHeightF(){return 480;}};
'''
boundaries = r'''
namespace dawnlight::survivor_perf{
unsigned testTier=0;unsigned tier(){return testTier;}
void counts(unsigned,unsigned,unsigned,unsigned,unsigned){}
}
void stop_burn_visual(Effect& effect){stop_visual_emitters(effect.emitters);}
'''
main = r'''
int main(){
 daAlink_c link;
 for(unsigned i=0;i<enemy_capacity;++i){s_enemies[i].id=s_effects[i].id=i;actors[i].current.pos.x=float(i);actors[i].eyePos={100,100,0};s_effects[i].emitters={1,2};}
 // The nearest enemy is off-screen; visible enemies receive the first 32 slots.
 actors[0].eyePos.x=-1;
 s_attacks[0].emitters[0]=3;s_attacks[0].exploding=true;
 survivor_perf::testEnabled=true;prepare_performance_visuals(&link);
 assert(stopped==129&&!s_burnVisuals[0]);
 assert(std::count(s_burnVisuals.begin(),s_burnVisuals.end(),true)==32);
 for(unsigned i=1;i<=32;++i){assert(s_burnVisuals[i]);assert(full_visual(s_effects[i].emitters,survivor_perf::Effect::Burn,2));s_effects[i].emitters={1,2};}
 assert(s_visualBudget.used()==64);
 survivor_perf::testTier=2;prepare_performance_visuals(&link);
 assert(stopped==193&&s_visualBudget.used()==0&&s_visualBudget.total_limit()==64);
 assert(std::count(s_burnVisuals.begin(),s_burnVisuals.end(),true)==16);
 for(const auto& e:s_effects)assert(e.burn==90); // Cleanup cannot change status clocks.
 for(const auto& a:s_attacks)assert(a.damage==100&&a.life==20);
 survivor_perf::testEnabled=false;prepare_performance_visuals(&link);
 assert(s_attacks[0].reducedVisual&&!s_attacks[1].reducedVisual); // A lingering blast stays visible.
 assert(!s_lightning[0].reducedVisual&&s_visualBudget.used()==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; exe=Path(tmp)/'test'
    cpp.write_text(fixture+helpers()+boundaries+source+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Performance visuals: live toggle, tier cleanup, burn selection and combat-state preservation passed')
