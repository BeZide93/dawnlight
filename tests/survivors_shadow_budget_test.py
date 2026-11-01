"""Exercise production shadow hooks, including nested ownership and failed install."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include "survivors_performance_policy.hpp"
#include <cassert>
#include <cstddef>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <algorithm>
namespace survivor_perf=dawnlight::survivor_perf;
namespace dawnlight::survivor_perf {bool mode=true;unsigned level=0;bool enabled(){return mode;}unsigned tier(){return level;}}
struct ModContext{};ModContext* mod_ctx=nullptr;
using HookAction=int;constexpr int HOOK_CONTINUE=0,HOOK_SKIP_ORIGINAL=1,MOD_OK=0;
struct Vec{float x=0;Vec operator-(Vec b)const{return{x-b.x};}float abs()const{return std::abs(x);}};
struct fopAc_ac_c{bool actor=true;unsigned id=0;struct{Vec pos;}current;};
fopAc_ac_c player;
auto* dComIfGp_getPlayer(int){return &player;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
bool is_actor_process(void* p){return static_cast<fopAc_ac_c*>(p)->actor;}
struct Enemy{bool initialized=true,elite=false,retiring=false;};std::map<unsigned,Enemy> enemies;
Enemy* tracked(unsigned id){auto i=enemies.find(id);return i==enemies.end()?nullptr:&i->second;}
bool arena=true,s_hub=false,transition=false;
bool in_arena(){return arena;}bool transitioning(){return transition;}
enum class Phase{Running,Ready};struct{Phase phase=Phase::Running;}s_run;
struct Args{void* method=nullptr;void* actor=nullptr;};
struct SurvivorsShadowActorDraw{};struct SurvivorsNormalShadow{};
int svc_hook=0,installCalls=0,failCall=0,uninstalls=0;
namespace mods {
 template<class T>T arg(void* p,int index){assert(index==1);return static_cast<T>(static_cast<Args*>(p)->actor);}
 template<class H,class F>int hook_add_pre(int,F){return ++installCalls==failCall?1:0;}
 template<class H,class F>int hook_add_post(int,F){return ++installCalls==failCall?1:0;}
 template<class H>void hook_uninstall(int){++uninstalls;}
}
struct Log{unsigned warnings=0;void warn(ModContext*,const char*){++warnings;}}logService;
auto* svc_log=&logService;
'''
main=r'''
int main(){
 using namespace survivor_perf;
 for(int fail=1;fail<=3;++fail){installCalls=uninstalls=0;failCall=fail;initialize_shadow_budget();assert(!presentation.shadowAvailable&&uninstalls==2);}
 failCall=0;installCalls=0;initialize_shadow_budget();assert(presentation.shadowAvailable);
 fopAc_ac_c enemy;enemy.id=1;enemy.current.pos.x=1300;enemies[1]={};Args outer{nullptr,&enemy};
 int result=99;
 before_shadow_actor_draw(nullptr,&outer,nullptr,nullptr);
 assert(before_normal_enemy_shadow(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==0);
 assert(presentation.shadowSuppressed==1);
 // A nested Link/untracked/elite draw cannot inherit the normal enemy suppression.
 fopAc_ac_c other;other.id=2;Args nested{nullptr,&other};
 for(int kind=0;kind<3;++kind){
  if(kind==1)enemies[2]={true,true,false};
  if(kind==2)other.actor=false;
  before_shadow_actor_draw(nullptr,&nested,nullptr,nullptr);result=99;
  assert(before_normal_enemy_shadow(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE&&result==99);
  after_shadow_actor_draw(nullptr,&nested,nullptr,nullptr);
 }
 assert(before_normal_enemy_shadow(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 after_shadow_actor_draw(nullptr,&outer,nullptr,nullptr);assert(!s_shadowDrawDepth);
 // Native key result is untouched when within range; tier 2 rejects even nearby normals.
 for(unsigned tier=0;tier<3;++tier)for(float distance:{100.f,700.f,1300.f}){
  level=tier;enemy.current.pos.x=distance;result=77;
  before_shadow_actor_draw(nullptr,&outer,nullptr,nullptr);
  const bool skip=tier==2||(tier==1?distance>600:distance>1200);
  assert(before_normal_enemy_shadow(nullptr,nullptr,&result,nullptr)==(skip?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE));
  assert(result==(skip?0:77));after_shadow_actor_draw(nullptr,&outer,nullptr,nullptr);
 }
 for(unsigned off=0;off<6;++off){
  mode=true;arena=true;s_hub=false;transition=false;s_run.phase=Phase::Running;enemies[1]={};
  if(off==0)mode=false;if(off==1)arena=false;if(off==2)s_hub=true;if(off==3)transition=true;
  if(off==4)s_run.phase=Phase::Ready;if(off==5)enemies[1].retiring=true;
  result=77;before_shadow_actor_draw(nullptr,&outer,nullptr,nullptr);
  assert(before_normal_enemy_shadow(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE&&result==77);
  after_shadow_actor_draw(nullptr,&outer,nullptr,nullptr);assert(!s_shadowDrawDepth);
 }
 mode=true;arena=true;s_hub=false;transition=false;s_run.phase=Phase::Running;enemies[1]={};level=2;
 // More nesting than the fixed stack fails open, then restores the previous scope.
 std::array<Args,70> deep{};
 for(auto& a:deep){a.actor=&enemy;before_shadow_actor_draw(nullptr,&a,nullptr,nullptr);}
 assert(before_normal_enemy_shadow(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
 for(size_t i=deep.size();i>64;--i)after_shadow_actor_draw(nullptr,&deep[i-1],nullptr,nullptr);
 assert(before_normal_enemy_shadow(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 for(size_t i=64;i;--i)after_shadow_actor_draw(nullptr,&deep[i-1],nullptr,nullptr);
 assert(!s_shadowDrawDepth);shutdown_shadow_budget();assert(!presentation.shadowAvailable);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'shadow.cpp';exe=Path(tmp)/'shadow'
    cpp.write_text(fixture+(root/'src/survivors_shadow_budget.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors shadows: native fallback, scope nesting, tiers, actor isolation and shutdown passed')
