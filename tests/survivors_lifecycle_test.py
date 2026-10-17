"""Exercise production death accounting, deferred cleanup and stage routing with engine stubs."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()
def extract(name):
    match = re.search(r'^(?:void|bool|HookAction|ModResult) ' + name + r'\(', source, re.M)
    assert match, name
    start = source.index('{', match.start())
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[match.start():end + 1]
    raise AssertionError(name)
stubs = r'''
#include <cassert>
#include <any>
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_map>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned; using ProfileName=int; using s8=signed char; using s16=short;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_E_OC_e=1, fpcNm_E_BA_e=2;
struct ModContext {};
struct ModError {};
using ModResult=int;
constexpr ModResult MOD_OK=0;
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods {
template<class T> T arg(void* args, int n) { return std::any_cast<T>(static_cast<std::any*>(args)[n]); }
template<class T> T& arg_ref(void* args, int n) { return *std::any_cast<T>(&static_cast<std::any*>(args)[n]); }
}
struct fopAc_ac_c { int health=0; struct { int pos=0; } current; };
struct e_ba_class : fopAc_ac_c { bool mIsDying=false; };
struct daE_OC_c : fopAc_ac_c { int action=0; int getActionMode() { return action; } };
struct Enemy {
    ActorId id=kNone; ProfileName profile=0;
    bool initialized=false, credited=false, retiring=false, elite=false;
    unsigned age=0;
};
std::array<Enemy,enemy_capacity> s_enemies;
Run s_run;
enum class SwordAction { None, Normal, Spin };
SwordAction s_swordAction=SwordAction::None;
unsigned s_comboStep=0,s_comboTimeout=0;
unsigned s_meleeCooldown=0; bool s_wavePending=false;
bool arena=true, transition=false, s_active=true, s_restart=false;
bool s_saveReady=false, s_gameStarted=true;
bool s_loaded=false, s_resultsStored=false, s_storeFailed=false, s_saveRequested=false;
ActorId s_player=kNone, s_savePrompt=kNone;
unsigned s_spawnCooldown=0, s_spawnFailures=0, s_warmup=0;
std::array<unsigned,3> s_cooldowns{};
std::array<int,64> s_orbs{};
Records s_records;
void close_window() {} void clear_attacks() {} void clear_loot() {} void detach_attack_references() {}
void d_GameOver_Delete(ActorId) {} void dComIfGp_setGameoverStatus(int) {}
constexpr int MAX_SELECT_ITEM=4, MAX_ITEM_SLOTS=24, dItemNo_NONE_e=255;
constexpr int TF_STATUS_HUMAN=0, dItemNo_WEAR_KOKIRI_e=47, KOKIRI_CLOTHES_FLAG=1;
constexpr int dItemNo_SWORD_e=40, COLLECT_ORDON_SWORD=1;
struct dSv_event_flag_c { enum { F_0250, F_0302, M_010 }; };
struct Status {
    std::array<int,MAX_SELECT_ITEM> slots{}, mixes{};
    void setSelectItemIndex(int i,int v){slots.at(i)=v;}
    void setMixItemIndex(int i,int v){mixes.at(i)=v;}
};
struct ReturnPlace { void set(const char*,int,int){} };
struct Player {
    Status status; ReturnPlace place;
    Status& getPlayerStatusA(){return status;}
    ReturnPlace& getPlayerReturnPlace(){return place;}
};
struct Save { Player player; Player& getPlayer(){return player;} } save;
struct Play {
    std::array<int,MAX_SELECT_ITEM> selected{};
    void setSelectItem(int i,int v){selected.at(i)=v;}
};
struct Game { Play play; } g_dComIfG_gameInfo;
std::array<int,MAX_ITEM_SLOTS> inventory{};
Save* dComIfGs_getSaveData(){return &save;}
void save_state_set_boss_rush_active(bool){} void save_state_set_intro_skipped(bool){}
void dComIfGs_onEventBit(int){} void dComIfGs_setTransformStatus(int){}
void dComIfGs_onItemFirstBit(int){} void dComIfGs_setCollectClothes(int){}
void dComIfGs_setSelectEquipClothes(int){} void dComIfGs_setCollectSword(int){}
void dComIfGs_setSelectEquipSword(int){} void dComIfGs_setSelectEquipShield(int){}
void dComIfGs_setBButtonItemKey(int){} void dComIfGs_setMaxLife(int){}
void dComIfGs_setWalletSize(int){}
void dComIfGs_setLife(int){} void dComIfGs_setRupee(int){}
// A live third-slot hook can dereference the absent Link actor during file
// select. Any live slot setter is forbidden in this pre-game fixture.
constexpr int COLLECT_MASTER_SWORD=10,dItemNo_MASTER_SWORD_e=11;
void dComIfGs_offCollectSword(int){}
void dComIfGs_offItemFirstBit(int){}
void dComIfGs_setSelectItemIndex(int,int){assert(false && "live item hook before Link exists");}
void dComIfGs_setMixItemIndex(int,int){assert(false && "live mix hook during save preparation");}
void dComIfGs_setItem(int i,int v){
    // Native setItem refreshes the play selection if this slot is assigned.
    for (int slot : save.player.status.slots) assert(slot==dItemNo_NONE_e);
    inventory.at(i)=v;
}
constexpr char kBlob[]="records";
struct mDoRst {
    static inline bool resetting=false, menu=false;
    static bool isReset(){return resetting;} static bool isReturnToMenu(){return menu;}
};
bool transitioning(){return transition;}
const char kStage[]="D_DLBR0";
const char* dComIfGp_getStartStageName(){return arena?kStage:"F_SP102";}
int dComIfGp_getStartStageRoomNo(){return arena?51:0;}
unsigned restartParam=123;
void dComIfGs_setRestartRoomParam(unsigned value){restartParam=value;}
struct Collider { unsigned power=0; unsigned GetAtAtp(){return power;} };
bool native_sword_collider(const Collider*){return false;}
struct Attack { Collider collider; };
std::array<Attack,16> s_attacks;
struct dCcU_AtInfo { Collider* mpCollider; unsigned mAttackPower; };
unsigned xp=0;
void add_orb(int,unsigned value){xp+=value;}
unsigned rewards=0;
void drop_reward(fopAc_ac_c*){++rewards;}
void finish(bool won){s_run.end(won);}
std::set<ActorId> pending;
std::unordered_map<ActorId,fopAc_ac_c*> actors;
bool fpcM_IsCreating(ActorId id){return pending.contains(id);}
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id); return it==actors.end()?nullptr:it->second;}
struct Service {
    unsigned attempts=0;
    void delete_actor(ModContext*,ActorId){++attempts;}
    ModResult set_blob(ModContext*,const char*,const void*,size_t){return MOD_OK;}
} service;
Service* svc_save=&service;
Service* svc_actor=&service; ModContext* mod_ctx=nullptr;
'''
main = r'''
int main() {
    save.player.status.slots.fill(dItemNo_NONE_e);
    save.player.status.mixes.fill(dItemNo_NONE_e);
    prepare_save(); // Even a blank new save must not call live item hooks.
    // Cover reused/loaded slots, not only a blank new-save buffer. The HUD's
    // play cache and all saved selections must agree before inventory clears.
    for (int pass=0;pass<2;++pass) {
        save.player.status.slots={0,4,10,23};save.player.status.mixes={4,10,23,0};
        g_dComIfG_gameInfo.play.selected.fill(40);inventory.fill(40);
        prepare_save();
        for (int i=0;i<MAX_SELECT_ITEM;++i) {
            assert(save.player.status.slots[i]==dItemNo_NONE_e);
            assert(save.player.status.mixes[i]==dItemNo_NONE_e);
            assert(g_dComIfG_gameInfo.play.selected[i]==dItemNo_NONE_e);
        }
        for (int item : inventory) assert(item==dItemNo_NONE_e);
    }
    s_run.start(1);
    daE_OC_c actor; actor.health=0;
    Enemy enemy{7,fpcNm_E_OC_e,true,false,false,false,0};
    transition=true; credit_death(enemy,&actor); assert(xp==0);
    transition=false; enemy.retiring=true; credit_death(enemy,&actor); assert(xp==0);
    enemy.retiring=false; enemy.initialized=false; credit_death(enemy,&actor); assert(xp==0);
    enemy.initialized=true; actor.health=10; credit_death(enemy,&actor); assert(xp==0);
    assert(rewards==0);
    actor.health=0; credit_death(enemy,&actor); assert(xp==3 && s_run.kills==1 && rewards==1);
    credit_death(enemy,&actor); assert(xp==3 && s_run.kills==1 && rewards==1);
    enemy={8,fpcNm_E_OC_e,true,false,false,true,0}; actor.health=1; actor.action=9;
    credit_death(enemy,&actor); assert(s_run.phase==Phase::Victory && xp==33 && rewards==1);
    s_run.start(2);
    s_enemies[0]={9,fpcNm_E_OC_e,false,false,true,false,0}; pending.insert(9);
    cleanup_enemies(); assert(s_enemies[0].id==9 && service.attempts==0);
    pending.clear(); actors[9]=&actor;
    cleanup_enemies(); assert(service.attempts==1 && s_enemies[0].id==9);
    cleanup_enemies(); assert(service.attempts==2); // Retry refused/asynchronous deletes.
    actors.clear(); cleanup_enemies(); assert(s_enemies[0].id==kNone && s_run.kills==0);
    s_run.elite_spawned=true; s_enemies[0]={10,fpcNm_E_OC_e,true,false,false,true,0};
    cleanup_enemies(); assert(!s_run.elite_spawned && s_run.phase==Phase::Running);
    std::any args[]={static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
    s_active=false;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(std::strcmp(std::any_cast<const char*>(args[0]),"F_SP102")==0);
    s_active=true; arena=false;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(std::strcmp(std::any_cast<const char*>(args[0]),kStage)==0);
    assert(std::any_cast<s8>(args[2])==51 && std::any_cast<s16>(args[1])==0 && restartParam==0);
    arena=true; s_run.phase=Phase::Running;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    s_restart=true; assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    s_restart=false;mDoRst::resetting=true;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    mDoRst::resetting=false;mDoRst::menu=true;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    mDoRst::menu=false;

    // Actual lifecycle callbacks: selecting/playing a mode is not selecting a save.
    activated(nullptr,nullptr);
    assert(s_active && !s_saveReady && !s_gameStarted && !in_arena());
    auto opening = [&] {
        args[0]=static_cast<const char*>("F_SP102");args[1]=s16(100);args[2]=s8(0);args[3]=s8(10);
        restartParam=123;
        assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
        assert(std::strcmp(std::any_cast<const char*>(args[0]),"F_SP102")==0);
        assert(std::any_cast<s16>(args[1])==100 && std::any_cast<s8>(args[3])==10 && restartParam==123);
    };
    opening(); // Logo -> opening scene must not start an arena beneath the title UI.
    before_game_start(nullptr,nullptr,nullptr,nullptr);
    assert(!s_gameStarted); // No selected slot, including failed direct-save loads.
    for (bool brightnessScreen : {false,true}) {
        new_save(nullptr,nullptr); // Native callback is after Link AND Epona names.
        assert(s_saveReady && !s_gameStarted && !in_arena());
        opening(); // Preparation alone cannot redirect stages or unlock gameplay.
        if (brightnessScreen) opening(); // Optional TV screen still owns the scene.
        loaded(nullptr,nullptr); // Native new-save flow also calls on_save_loaded.
        assert(s_saveReady && !s_gameStarted);
        before_game_start(nullptr,nullptr,nullptr,nullptr);
        assert(s_gameStarted && in_arena());
        args[0]=static_cast<const char*>(kStage);
        assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
        args[0]=static_cast<const char*>("F_SP108");args[1]=s16(21);args[2]=s8(1);args[3]=s8(13);
        assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
        assert(std::strcmp(std::any_cast<const char*>(args[0]),kStage)==0);
        assert(std::any_cast<s16>(args[1])==0 && std::any_cast<s8>(args[2])==51 && std::any_cast<s8>(args[3])==0);
        s_run.start(1);
        before_opening(nullptr,nullptr,nullptr,nullptr);
        assert(!s_saveReady && !s_gameStarted && !in_arena());
        opening(); // Return to title disarms even without a reset callback.
    }
    loaded(nullptr,nullptr); // Existing slot / successful command-line load.
    before_game_start(nullptr,nullptr,nullptr,nullptr);
    assert(s_gameStarted);
    reset(nullptr,nullptr);
    assert(!s_saveReady && !s_gameStarted && !in_arena());
    opening();
    loaded(nullptr,nullptr);mDoRst::resetting=true;
    before_game_start(nullptr,nullptr,nullptr,nullptr);assert(!s_gameStarted);
    mDoRst::resetting=false;mDoRst::menu=true;
    before_game_start(nullptr,nullptr,nullptr,nullptr);assert(!s_gameStarted);
    mDoRst::menu=false;
    deactivated(nullptr,nullptr);
    assert(!s_active && !s_saveReady && !s_gameStarted);
    s_active=true;
    for (unsigned power=1;power<=12;++power) {
        s_attacks[0].collider.power=power;
        dCcU_AtInfo info{&s_attacks[0].collider,200};std::any hit[]={&info};
        after_power(nullptr,hit,nullptr,nullptr);assert(info.mAttackPower==power);
    }
    Collider foreign{4};dCcU_AtInfo info{&foreign,200};std::any hit[]={&info};
    after_power(nullptr,hit,nullptr,nullptr);assert(info.mAttackPower==200);
    s_active=false;info.mpCollider=&s_attacks[0].collider;
    after_power(nullptr,hit,nullptr,nullptr);assert(info.mAttackPower==200);
    std::cout<<"Survivors lifecycle: true deaths, pending spawns, cleanup and routing passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; binary=Path(tmp)/'test'
    cpp.write_text(stubs+'\n'+ '\n'.join(extract(n) for n in [
        'in_arena','retire_enemies','reset_runtime','prepare_save','activated','deactivated','new_save','loaded','reset',
        'cleanup_enemies','credit_death','before_game_start','before_opening','before_stage','after_power'])+'\n'+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
