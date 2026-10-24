"""Exercise production death accounting, deferred cleanup and stage routing with engine stubs."""
from pathlib import Path
import re
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()+"\n"+(root/'src/survivors_maps_ui.inc').read_text()
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
namespace enemy_hud {void reset(){}template<class T>void forget(T){}}

void stop_survivors_music(){}
unsigned archiveBegins=0,archiveReleases=0;
void begin_enemy_archives(){++archiveBegins;}
void release_enemy_archives(){++archiveReleases;}
bool s_victory=false;void reset_ending(){s_victory=false;}void begin_victory(int){s_victory=true;}

#include <cassert>
#include <chrono>
#include <any>
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_map>
#include "survivors_rules.hpp"
#include "actor_slot_index.hpp"
using dawnlight::ActorSlotIndex;
#include "../include/survivors_maps.hpp"
using namespace dawnlight::survivors;
using u16=unsigned short;using ActorId=unsigned; using ProfileName=int; using s8=signed char; using s16=short;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_E_OC_e=1, fpcNm_E_BA_e=2,fpcNm_E_RD_e=4;
struct ModContext {};
struct ModError {};
using ModResult=int;
constexpr ModResult MOD_OK=0,MOD_UNAVAILABLE=1;
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
    int eliteSlot=-1;bool swordKill=false;
};
unsigned eliteChests=0;void drop_elite_chest(Enemy&,fopAc_ac_c*){++eliteChests;}
std::array<Enemy,enemy_capacity> s_enemies;
ActorSlotIndex<ActorId> s_enemyIndex;
std::vector<int> s_enemyHitSources,s_enemyContactFrames;
std::unordered_map<ActorId,int>s_bossTactics;
void clear_player_contacts(){}
Run s_run;RunOptions s_options;
GearInventory s_gear;bool s_gearWritable=true;unsigned s_gearNoticeTicks=0,gearRolls=0;
void restore_gear_collection(){} void close_gear_window(){}
bool store_gear(const GearInventory& gear){s_gear=gear;return true;}
void drop_boss_gear(){++gearRolls;}
void clear_gear_notices(){s_gearNoticeTicks=0;}
enum class HubMenu{None,Talents,Run};unsigned s_hubClosingWindow=0;HubMenu s_hubMenu=HubMenu::None;
bool s_hub=true,s_hubDeparture=false,s_hubDoorStarted=false,s_arrivingRun=false,s_allowHubDoor=false,s_hubSaveRequested=false;
ActorId s_hubDoor=kNone;unsigned s_hubDoorTicks=0;
unsigned s_window=0,s_refreshChoices=0;
void retire_hub_zelda(){}
void load_option_config(){}void update_survivors_pedestal(bool){}
void update_survivors_equipment_chest(bool,int,int){}

void begin_starting_item_choice(){if(s_run.starting_item_pending)s_run.phase=Phase::TreasureChoice;}
enum class SwordAction { None, Normal, Spin, Jump };
SwordAction s_swordAction=SwordAction::None;
unsigned s_comboStep=0,s_comboTimeout=0;
unsigned s_meleeCooldown=0; bool s_wavePending=false;
unsigned s_map=0,s_requestedMap=0;
const Arena& arena(){return arenas[s_map];}
void clear_arena_geometry(){}
void retire_boss_ring(){}void cleanup_boss_ring(){}
void reset_midna_dialog(){}
void reset_elite_encounters(){}
void reset_gale_charges(){}
bool midna_dialog_busy(){return false;}
bool s_arenaMeshReady=true;
unsigned s_chestDelay=0;
void dMeter2Info_setSword(int,bool){}
bool arenaPresent=true, transition=false, s_active=true, s_restart=false;
bool s_saveReady=false, s_gameStarted=true;
bool s_progressSaving=false,s_progressSaved=false;
bool s_loaded=false, s_resultsStored=false, s_storeFailed=false, s_saveRequested=false;
ActorId s_player=kNone, s_savePrompt=kNone, s_bossHudTarget=kNone;
unsigned s_spawnCooldown=0, s_spawnFailures=0, s_warmup=0, s_saveStartTicks=0;
std::array<unsigned,3> s_cooldowns{};
std::array<int,64> s_orbs{};
Records s_records;
void retire_enemy_arrows(){} void cleanup_enemy_arrows(){} void clear_king_waves(ActorId=kNone){} void clear_zelda_light_orbs(ActorId=kNone){}
void retire_horde_portals(){} void cleanup_horde_portals(){}
void cleanup_palace_ferries(){}
void retire_pots(){} void retire_pickups(){}
void queue_exp_magnet(const Enemy&,fopAc_ac_c*){}
void clear_enemy_effects(){}
void close_powerup_window() {} void close_window() {} void clear_attacks() {} void clear_loot() {} void clear_combat_feedback() {} void detach_attack_references() {}
void d_GameOver_Delete(ActorId) {} void dComIfGp_setGameoverStatus(int) {}
constexpr int MAX_SELECT_ITEM=4, MAX_ITEM_SLOTS=24, dItemNo_NONE_e=255;
constexpr int TF_STATUS_HUMAN=0, dItemNo_WEAR_KOKIRI_e=47, KOKIRI_CLOTHES_FLAG=1;
constexpr int dItemNo_SWORD_e=40, COLLECT_ORDON_SWORD=1;
struct dSv_event_flag_c { enum { F_0250, F_0302, M_010, M_067 }; };
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
void dComIfGs_onDarkClearLV(int){}
void dComIfGs_onEventBit(int){} void dComIfGs_setTransformStatus(int){}
void dComIfGs_onItemFirstBit(int){} void dComIfGs_setCollectClothes(int){}
void dComIfGs_setSelectEquipClothes(int){} void dComIfGs_setCollectSword(int){}
void dComIfGs_setSelectEquipSword(int){} void dComIfGs_setSelectEquipShield(int){}
void dComIfGs_setBButtonItemKey(int){} void dComIfGs_setMaxLife(int){}
void dComIfGs_setWalletSize(int){}
int life=12;int dComIfGs_getLife(){return life;}
int dComIfGs_getMaxLifeGauge(){return 12;}
void dComIfGs_setLife(int n){life=n;} void dComIfGs_setRupee(int){}
using u8=unsigned char;struct Chest{bool free=false,fixed=false;};
int nativeKeys=0,pendingKeys=0;
int dComIfGs_getKeyNum(){return nativeKeys;}int dComIfGp_getItemKeyNumCount(){return pendingKeys;}
void dComIfGp_clearItemKeyNumCount(){pendingKeys=0;}void dComIfGs_setKeyNum(u8 n){nativeKeys=n;}
#include "survivors_keys.inc"

// A live third-slot hook can dereference the absent Link actor during file
// select. Any live slot setter is forbidden in this pre-game fixture.
constexpr int COLLECT_LIGHT_SWORD=12,dItemNo_LIGHT_SWORD_e=13,COLLECT_WOODEN_SHIELD=0,COLLECT_HYLIAN_SHIELD=2;
void dComIfGs_offCollectShield(int){}
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
const char* dComIfGp_getStartStageName(){return arenaPresent?arenas[s_map].stage:"F_SP102";}
int dComIfGp_getStartStageRoomNo(){return arenaPresent?arenas[s_map].room:0;}
unsigned restartParam=123;
void dComIfGs_setRestartRoomParam(unsigned value){restartParam=value;}
struct Collider { unsigned power=0; unsigned GetAtAtp(){return power;} };
struct daAlink_c{Collider mGuardAtCps,mAtSph;};
daAlink_c* daAlink_getAlinkActorClass(){return nullptr;}
bool native_sword_collider(const Collider*){return false;}
struct Attack { Collider collider; };
std::array<Attack,16> s_attacks;
struct Effect{Collider collider;unsigned damage=0,fixedDamage=0;bool critical=false;};std::array<Effect,24>s_effects;
constexpr int HIT_TYPE_LINK_HEAVY_ATTACK=9;
struct dCcU_AtInfo { Collider* mpCollider; unsigned mAttackPower; int mHitType=0; };
unsigned xp=0;
void add_orb(int,unsigned value){xp+=value;}
unsigned rewards=0;
void drop_reward(fopAc_ac_c*){++rewards;}
void end_survivors_integrity(){}
void update_survivors_integrity(bool&){}
void begin_survivors_integrity(bool,bool&){}
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
stubs += r'''
struct BulblinHit{bool critical=false;unsigned basePower=0,scaledPower=0;};BulblinHit* bulblin_power_hit(dCcU_AtInfo*){return nullptr;}
bool reuse_bulblin_power(dCcU_AtInfo*){return false;}
void mark_damage_critical(dCcU_AtInfo*,bool){}
void mark_damage_split(dCcU_AtInfo*,unsigned,unsigned){}
struct BulblinPowerCache{dCcU_AtInfo* info;~BulblinPowerCache(){}};
'''
stubs += "\nDamageAmounts hit_damage_sources(const Collider*){return {};}\n"
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
        nativeKeys=7;pendingKeys=1;prepare_save();assert(chest_keys()==0&&pendingKeys==0);
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
    actor.health=0; credit_death(enemy,&actor); assert(xp==3 && s_run.kills==1 && rewards==1 && gearRolls==0);
    credit_death(enemy,&actor); assert(xp==3 && s_run.kills==1 && rewards==1 && gearRolls==0);
    enemy={8,fpcNm_E_OC_e,true,false,false,true,0};enemy.eliteSlot=int(timed_elite_count); actor.health=1; actor.action=9;
    credit_death(enemy,&actor); assert(s_run.phase==Phase::Running && xp==33 && rewards==1 && eliteChests==1);
    assert(s_run.guard_elites==1&&s_run.timed_elites==0&&gearRolls==1);
    credit_death(enemy,&actor);assert(gearRolls==1); // Stationary boss gets one roll per genuine death.
    enemy.credited=false;enemy.eliteSlot=0;credit_death(enemy,&actor);
    assert(s_run.timed_elites==1&&gearRolls==2);credit_death(enemy,&actor);assert(s_run.timed_elites==1&&gearRolls==2);
    // Life Steal heals once per credited death, independent of reward drops.
    s_run.ranks[unsigned(Upgrade::LifeSteal)]=1;life=4;
    enemy.credited=false;credit_death(enemy,&actor);assert(life==4); // Non-sword death.
    enemy.credited=false;enemy.swordKill=true;credit_death(enemy,&actor);assert(life==5);
    credit_death(enemy,&actor);assert(life==5);
    enemy.credited=false;enemy.retiring=true;credit_death(enemy,&actor);assert(life==5);
    enemy.retiring=false;life=12;credit_death(enemy,&actor);assert(life==12);
    enemy.credited=false;life=0;credit_death(enemy,&actor);assert(life==0);
    s_run.endless=true;s_run.ticks=40*60*ticks_per_second-1;life=4;enemy.credited=false;
    credit_death(enemy,&actor);assert(life==5);
    ++s_run.ticks;enemy.credited=false;credit_death(enemy,&actor);assert(life==5);
    s_run.endless=false;s_run.ticks=0;
    s_run.ranks[unsigned(Upgrade::LifeSteal)]=0;life=12;
    // Horde throttles only normal kill rewards; elites never consume the counter.
    s_run.options.horde=5;s_run.drop_progress=0;xp=rewards=0;
    for(unsigned n=1;n<=10;++n){enemy.credited=false;enemy.elite=false;credit_death(enemy,&actor);
        assert(xp==(n/5)*3&&rewards==n/5);}
    enemy.credited=false;enemy.elite=true;credit_death(enemy,&actor);
    assert(xp==36&&rewards==2&&s_run.drop_progress==0);
    s_run.start(2);
    s_enemies[0]={9,fpcNm_E_OC_e,false,false,true,false,0}; pending.insert(9);s_enemyIndex.add(9,0);s_bossTactics[9]=123;
    cleanup_enemies(); assert(s_enemies[0].id==9 && service.attempts==0);
    assert(s_enemyIndex.find(s_enemies,9)==&s_enemies[0]);
    pending.clear(); actors[9]=&actor;
    cleanup_enemies(); assert(service.attempts==1 && s_enemies[0].id==9);
    cleanup_enemies(); assert(service.attempts==2); // Retry refused/asynchronous deletes.
    actors.clear(); cleanup_enemies(); assert(s_enemies[0].id==kNone && s_run.kills==0);
    assert(!s_enemyIndex.find(s_enemies,9)&&!s_bossTactics.contains(9));
    s_enemies[0]={10,fpcNm_E_OC_e,true,false,false,true,0};
    cleanup_enemies(); assert(s_enemies[0].id==kNone && s_run.phase==Phase::Running);
    std::any args[]={static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
    s_active=false;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(std::strcmp(std::any_cast<const char*>(args[0]),"F_SP102")==0);
    s_active=true; arenaPresent=false;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(std::strcmp(std::any_cast<const char*>(args[0]),kStage)==0);
    assert(std::any_cast<s8>(args[2])==51 && std::any_cast<s16>(args[1])==0 && restartParam==0);
    arenaPresent=true; s_run.phase=Phase::Running;
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
    opening(); // Logo -> opening scene must not start an arenaPresent beneath the title UI.
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
    // A preview selection requests a reload, but neither changes the live map
    // before that reload nor starts combat. Duplicate/invalid/running clicks fail.
    s_map=0;s_requestedMap=0;s_restart=false;s_run.phase=Phase::Ready;
    unsigned choice=3;select_arena(nullptr,&choice);
    assert(s_requestedMap==3 && s_map==0 && !s_restart && s_run.phase==Phase::Ready);
    choice=2;select_arena(nullptr,&choice);assert(s_requestedMap==2);
    s_restart=false;s_map=3;choice=3;select_arena(nullptr,&choice);assert(!s_restart);
    choice=99;select_arena(nullptr,&choice);assert(!s_restart);
    s_run.phase=Phase::Running;choice=1;select_arena(nullptr,&choice);assert(!s_restart);
    s_run.phase=Phase::Ready;s_arenaMeshReady=false;begin_run();
    assert(s_run.phase==Phase::Ready);
    s_arenaMeshReady=true;s_restart=true;begin_run();assert(s_run.phase==Phase::Ready);
    s_restart=false;s_progressSaving=true;begin_run();assert(s_run.phase==Phase::Ready);
    choice=2;select_arena(nullptr,&choice);assert(!s_restart);
    s_progressSaving=false;s_bossHudTarget=7;begin_run();assert(s_run.phase==Phase::Running && s_run.ticks==0 && s_bossHudTarget==kNone);
    s_records.talents.ranks[unsigned(Talent::StartingItem)]=1;s_run.phase=Phase::Ready;
    begin_run();assert(s_run.phase==Phase::TreasureChoice&&s_run.starting_item_pending&&s_run.ticks==0);
    const auto seed=s_run.random;begin_run();assert(s_run.random==seed);
    s_records.talents={};s_run.phase=Phase::Ready;begin_run();
    assert(s_run.phase==Phase::Running&&!s_run.starting_item_pending);
    // Every selected map routes through the same post-file-select gate.
    s_active=true;s_gameStarted=true;s_run.phase=Phase::Ready;arenaPresent=true;
    for(unsigned map=0;map<arenas.size();++map){
        s_map=map;s_restart=false;
        std::any next[]={static_cast<const char*>("F_SP108"),s16(99),s8(9),s8(9)};
        assert(before_stage(nullptr,next,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        s_restart=true;
        assert(before_stage(nullptr,next,nullptr,nullptr)==HOOK_CONTINUE);
        assert(std::strcmp(std::any_cast<const char*>(next[0]),arenas[map].stage)==0);
        assert(std::any_cast<s8>(next[2])==arenas[map].room);
        s_gameStarted=false;
        next[0]=static_cast<const char*>("F_SP102");
        assert(before_stage(nullptr,next,nullptr,nullptr)==HOOK_CONTINUE);
        assert(std::strcmp(std::any_cast<const char*>(next[0]),"F_SP102")==0);
        s_gameStarted=true;
    }
    std::cout<<"Survivors lifecycle: true deaths, pending spawns, cleanup and routing passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; binary=Path(tmp)/'test'
    cpp.write_text(stubs+'\n'+ '\n'.join(extract(n) for n in [
        'in_arena','retire_enemies','reset_runtime','prepare_save','activated','deactivated','new_save','loaded','reset',
        'begin_run','select_arena','cleanup_enemies','credit_death','before_game_start','before_opening','before_stage','after_power'])+'\n'+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
