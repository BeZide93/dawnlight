"""Exercise indexed lookup and the production process hook across actor lifetimes."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()

def function(text, signature):
    start = text.index(signature)
    body = text.index('{', start)
    depth = 0
    for end in range(body, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError(signature)

fixture = r'''
namespace enemy_hud {template<class... T>void observe(T...){}template<class T>void forget(T){}}

#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <vector>
#include <unordered_map>
#include "actor_slot_index.hpp"
using dawnlight::ActorSlotIndex;
using ActorId=unsigned;
constexpr ActorId kNone=~0u;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}}
using process_method_func=int(*)(void*);
int execute(void*){return 1;} int destroy(void*){return 1;} int create(void*){return 1;}
struct process_method_class {process_method_func execute_method=execute,delete_method=destroy;} methods;
struct fopAc_ac_c {ActorId id;void* sub_method=&methods;bool actor=true;int name=0,room=0;};
struct PalaceFerry{bool retiring=false;}ferry;
bool ferryOwned=false;
PalaceFerry* tracked_palace_ferry(ActorId id){return ferryOwned&&id==7?&ferry:nullptr;}
struct{ActorId id=kNone;}s_bossRing;
HookAction process_boss_ring(fopAc_ac_c*,void*,void*){return HOOK_CONTINUE;}
struct daE_OC_c:fopAc_ac_c {int mMoveRange=0,field_0x6e3=0;int getActionMode(){return 0;}void setActionMode(int,int){}};
constexpr int fpcNm_E_OC_e=42;
bool is_actor_process(void* p){return static_cast<fopAc_ac_c*>(p)->actor;}
ActorId fopAcM_GetID(fopAc_ac_c* p){return p->id;}
struct Enemy {ActorId id=kNone;bool initialized=false,retiring=false,dormant=false,credited=false;int profile=0;unsigned specialCooldown=0,summonCooldown=0;};
struct ArenaPot {ActorId id=kNone;bool initialized=false,retiring=false;};
struct RunPickup {ActorId id=kNone;bool retiring=false;};
struct EnemyArrow {ActorId id=kNone;bool retiring=false,shell=false;ActorId parent=kNone;};
std::array<Enemy,372>s_enemies;
std::array<ArenaPot,72>s_pots;
std::array<EnemyArrow,1488>s_enemyArrows;
std::vector<RunPickup>s_pickups;
ActorSlotIndex<ActorId>s_enemyIndex,s_potIndex,s_enemyArrowIndex,s_pickupIndex;
EnemyArrow* tracked_enemy_arrow(ActorId id){return id==kNone?nullptr:s_enemyArrowIndex.find(s_enemyArrows,id);}
bool arenaActive=false,paused=false,s_victory=false;
bool in_arena(){return arenaActive;}
ActorId s_player=99;
enum class Phase {Running};struct {Phase phase=Phase::Running;} s_run;
struct Frame {void* args;ActorId id;bool executed=false;};std::vector<Frame>s_processFrames;
unsigned retained=0,meshes=0,moves=0,engages=0;
void retain_treasure_actor(fopAc_ac_c*,void*){++retained;}
void collect_arena_mesh(fopAc_ac_c*){++meshes;}
bool pause_run_actor(bool){return paused;}
bool pickup_waiting(RunPickup&,fopAc_ac_c*){return false;}
void move_pickup(RunPickup&,fopAc_ac_c*){++moves;}
void initialize_enemy(Enemy* e,fopAc_ac_c*){e->initialized=true;}
void engage_enemy(fopAc_ac_c*,Enemy&){++engages;}
void track_enemy_facing(fopAc_ac_c*){}
bool extended_enemy(int){return false;}bool update_extended_enemy(fopAc_ac_c*,Enemy&){return true;}
void advance_elite_cooldowns(fopAc_ac_c*,Enemy&){}
struct BossState{struct{void tick(){}}stagger;};std::unordered_map<ActorId,BossState>s_bossTactics;
bool boss_tactics_owned(const Enemy&){return false;}
void before_boss_tactics(Enemy&,fopAc_ac_c*){}
void clear_zelda_light_orbs(ActorId=kNone){}
void update_boss_projectile(fopAc_ac_c*,bool){}
'''

cases = r'''
int main(){
    fopAc_ac_c actor{7};int result=0;
    std::any args[]={process_method_func(execute),static_cast<void*>(&actor)};
    // Only the active Palace ferry's Execute freezes with Link. Rendering,
    // creation and destruction still run, including unload while paused.
    for(bool active:{false,true})for(bool pause:{false,true})
    for(bool owned:{false,true})for(bool retiring:{false,true})
    for(auto method:{execute,create,destroy}) {
        arenaActive=active;paused=pause;ferryOwned=owned;ferry.retiring=retiring;
        args[0]=process_method_func(method);result=0;
        const bool frozen=active&&owned&&(pause||retiring)&&method==execute;
        assert(before_process(nullptr,args,&result,nullptr)==(frozen?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE));
        assert(result==(frozen?1:0));
    }
    arenaActive=paused=ferryOwned=false;args[0]=process_method_func(execute);
    assert(before_process(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
    assert(!tracked(7)&&!tracked_pot(7)&&!tracked_pickup(7));
    assert(!tracked(kNone)&&!tracked_pot(kNone)&&!tracked_pickup(kNone));
    // An ID missed earlier becomes owned immediately, including while creation
    // is still pending. Slots and IDs can both be reused after native deletion.
    for(unsigned iteration=0;iteration<100;++iteration){
        auto slot=iteration%s_enemies.size();
        s_enemies[slot].id=7;s_enemyIndex.add(7,slot);
        assert(tracked(7)==&s_enemies[slot]);
        args[0]=process_method_func(create);
        before_process(nullptr,args,&result,nullptr);
        assert(tracked(7)&&!tracked(7)->initialized);
        args[0]=process_method_func(execute);
        before_process(nullptr,args,&result,nullptr);
        assert(tracked(7)->initialized&&s_processFrames.back().executed);
        // Retirement must still freeze Execute after leaving the arena.
        tracked(7)->retiring=true;
        assert(before_process(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
        assert(!s_processFrames.back().executed);
        args[0]=process_method_func(destroy);
        before_process(nullptr,args,&result,nullptr);
        assert(!tracked(7)&&s_enemies[slot].id==kNone);
        s_processFrames.clear();
    }
    // Preserve independent category handling even if an ID appears in two lists.
    s_pots[0].id=7;s_pots[0].initialized=true;s_potIndex.add(7,0);
    s_enemyArrows[0].id=7;s_enemyArrowIndex.add(7,0);
    s_pickups.push_back({7});s_pickupIndex.add(7,0);
    before_process(nullptr,args,&result,nullptr);
    assert(!tracked_pot(7)&&!tracked_pickup(7)&&!tracked_enemy_arrow(7));
    assert(!s_pots[0].initialized);
    // Vector growth cannot invalidate an index, and compaction repairs positions.
    for(unsigned id=10;id<2010;++id){s_pickups.push_back({id});s_pickupIndex.add(id,s_pickups.size()-1);}
    for(unsigned id=10;id<2010;++id)assert(tracked_pickup(id)->id==id);
    std::erase_if(s_pickups,[](const auto& p){return p.id==kNone||p.id%2==0;});
    s_pickupIndex.rebuild(s_pickups,kNone);
    for(unsigned id=10;id<2010;++id)assert(bool(tracked_pickup(id))==bool(id%2));
    // Unrelated Execute still collects arena geometry and retains treasure;
    // non-actor requests never reach those actor-specific helpers.
    arenaActive=true;actor.id=9000;args[0]=process_method_func(execute);
    unsigned oldMeshes=meshes;
    before_process(nullptr,args,&result,nullptr);assert(meshes==oldMeshes+1);
    unsigned oldRetained=retained;actor.actor=false;
    before_process(nullptr,args,&result,nullptr);assert(retained==oldRetained);
    // A miss must not touch any records, independent of fixed-array capacity.
    struct Counted {
        using value_type=Enemy;
        std::array<Enemy,1932> data{};unsigned accesses=0;
        std::size_t size()const{return data.size();}
        Enemy& operator[](std::size_t i){++accesses;return data[i];}
    } records;
    ActorSlotIndex<ActorId> index;
    records.data[1900].id=42;index.add(42,1900);
    for(unsigned i=0;i<540;++i)assert(!index.find(records,9000));
    assert(records.accesses==0&&index.find(records,42)==&records.data[1900]);
    records.data[1900].id=43;assert(!index.find(records,42));
    index.erase(42);index.add(43,1900);assert(index.find(records,43));
}
'''

production = function(source, 'Enemy* tracked(')
for name, signature in [('survivors_pots.inc', 'ArenaPot* tracked_pot('),
                        ('survivors_pickups.inc', 'RunPickup* tracked_pickup(')]:
    production += function((root / 'src' / name).read_text(), signature)
production += function(source, 'HookAction before_process(')
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; binary=Path(tmp)/'test'
    cpp.write_text(fixture+production+cases)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors process index: ID/slot reuse, deletion outside arena, vector growth/compaction and misses without record scans passed')
