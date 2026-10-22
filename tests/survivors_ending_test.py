"""Exercise production end-run handoffs, reward isolation and native Game Over timing."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()

def function(text,signature):
    start=text.index(signature);brace=text.index('{',start);depth=0
    for end in range(brace,len(text)):
        depth+=(text[end]=='{')-(text[end]=='}')
        if not depth:return text[start:end+1]+'\n'
    raise AssertionError(signature)

fixture=r'''
namespace enemy_hud {void tick(){}}

#include <array>
#include <any>
#include <cassert>
#include <cmath>
#include <iostream>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u8=unsigned char;using ActorId=unsigned;using fpc_ProcID=unsigned;constexpr unsigned kNone=~0u;
struct ModContext{};ModContext* mod_ctx=nullptr;enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,unsigned n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}}
struct cXyz{float x=0,y=0,z=0;cXyz operator-(cXyz p)const{return{x-p.x,y-p.y,z-p.z};}
 float absXZ()const{return std::sqrt(x*x+z*z);}};
struct fopAc_ac_c{int health=10;unsigned id=0;struct{cXyz pos;}current;};
struct e_ba_class:fopAc_ac_c{bool mIsDying=false;};struct daE_OC_c:fopAc_ac_c{int getActionMode(){return 0;}};
struct daAlink_c:fopAc_ac_c{} player;
constexpr int fpcNm_E_BA_e=1,fpcNm_E_OC_e=2,fpcNm_E_RD_e=3;
struct Enemy{unsigned id=kNone;int profile=0;bool credited=false,retiring=false,initialized=true,elite=false;int eliteSlot=-1;bool swordKill=false;};
std::array<Enemy,5>s_enemies;std::array<fopAc_ac_c,5>actors;
auto* actor_by_id(ActorId id){return id<actors.size()?&actors[id]:nullptr;}
unsigned fopAcM_GetID(fopAc_ac_c* actor){return actor->id;}
auto* daAlink_getAlinkActorClass(){return &player;}
Run s_run;Records s_records;unsigned s_map=0;
bool s_active=true,s_victory=false,s_deathPending=false,s_deathDismiss=false,s_resultsStored=false;
bool s_progressSaved=false,s_storeFailed=false,s_hub=false;
unsigned s_victoryTicks=0,s_player=42;fpc_ProcID s_deathGameOver=kNone;
bool arena=true,paused=false,event=false,ui=false,transition=false;unsigned life=12;
bool saving=false;bool save_modal_active(){return saving;}
bool in_arena(){return arena;}bool transitioning(){return transition;}bool ui_visible(){return ui;}
bool playable(){return arena&&!paused&&!event&&!ui&&!transition;}
bool midna_dialog_busy(){return false;}bool dComIfGp_event_runCheck(){return event;}
bool dComIfGp_isPauseFlag(){return paused;}
unsigned dComIfGs_getLife(){return life;}unsigned dComIfGs_getMaxLifeGauge(){return 12;}
unsigned dComIfGs_getMaxLife(){return 15;}void dComIfGs_setLife(unsigned n){life=n;}
unsigned dComIfGs_getRupee(){return 0;}unsigned dComIfGs_getRupeeMax(){return 999;}
void dComIfGs_setRupee(unsigned){assert(false);}
unsigned cleared=0,retired=0,pickupsRetired=0,enemyArrowsRetired=0,spawns=0,attacks=0,orbs=0,chests=0,magnets=0;
void clear_attacks(){++cleared;}void clear_enemy_effects(){}void retire_enemy_arrows(){++enemyArrowsRetired;}
void retire_enemies(){++retired;++pickupsRetired;for(auto& e:s_enemies)e.retiring=true;}
void interrupt_auto_actions(daAlink_c*){}void close_window(){ui=false;}void close_powerup_window(){}
void load_records(){}constexpr const char* kBlob="test";constexpr int MOD_OK=0;
struct Save{unsigned writes=0;int set_blob(ModContext*,const char*,const void*,size_t){++writes;return 0;}}save;
auto* svc_save=&save;
void update_combat_feedback(){}void update_enemy_effects(){}
void update_spawns(daAlink_c*){++spawns;}void update_pots(daAlink_c*){}
void update_weapons(daAlink_c*){++attacks;}void update_orbs(daAlink_c*){++orbs;}
struct Pickup{bool retiring=false;};unsigned pickupMoves=0;
 bool pickup_waiting(Pickup&,fopAc_ac_c*){return true;}
 void move_pickup(Pickup&,fopAc_ac_c*){++pickupMoves;}
 bool chest_spawning_allowed(){return true;}
 void update_chests(daAlink_c*){assert(false);}void update_exp_magnets(daAlink_c*){++magnets;}
void drop_reward(fopAc_ac_c*){assert(false);}void add_orb(cXyz,unsigned){++orbs;}
void drop_elite_chest(Enemy&,fopAc_ac_c*){++chests;}void queue_exp_magnet(Enemy&,fopAc_ac_c*){++magnets;}
struct Wave{cXyz origin;float radius=0;unsigned samples=0,releases=0;void sample(){++samples;}void release(){++releases;}}s_victoryWave;
void end_survivors_integrity(){}
void update_survivors_integrity(bool&){}
void begin_survivors_integrity(bool,bool&){}
void finish(bool);void begin_victory(const cXyz&);void begin_defeat();void update_victory();
// Game Over boundary: real native transition/animation methods are compiled below.
enum{PROC_PLAYER_ANM_WAIT,PROC_DISP_FADE_OUT,PROC_DISP_WAIT,PROC_DEMO_FADE_IN,PROC_DEMO_FADE_OUT,
 PROC_SAVE_OPEN,PROC_SAVE_MOVE,PROC_SAVE_CLOSE,PROC_DELETE_WAIT};
struct dGameover_c{unsigned id=71;unsigned char mProc=PROC_PLAYER_ANM_WAIT;bool mIsDemoSave=false;int mTimer=0;void* mpHeap=nullptr;
 int _execute();void playerAnmWait_init();void playerAnmWait_proc();void dispFadeOut_init();void dispFadeOut_proc();
 void dispWait_init();void dispWait_proc();};
namespace mDoGph_gInf_c{void startFadeOut(int){}void startFadeIn(int){}}
using JKRHeap=void;JKRHeap* mDoExt_setCurrentHeap(void*){return nullptr;}
using Step=void(dGameover_c::*)();
Step init_process[]={&dGameover_c::playerAnmWait_init,&dGameover_c::dispFadeOut_init,&dGameover_c::dispWait_init};
Step move_process[]={&dGameover_c::playerAnmWait_proc,&dGameover_c::dispFadeOut_proc,&dGameover_c::dispWait_proc};
unsigned fpcM_GetID(dGameover_c* p){return p->id;}
bool nativeExists=true;unsigned gameOverType=0,deletes=0;
void* fopMsgM_SearchByID(unsigned){return nativeExists?&player:nullptr;}bool fpcM_IsCreating(unsigned){return false;}
unsigned dMeter2Info_getGameOverType(){return gameOverType;}
void d_GameOver_Delete(unsigned& id){++deletes;id=kNone;}void dComIfGp_setGameoverStatus(int){}
void dComIfGp_offPauseFlag(){paused=false;}void dComIfGp_event_reset(){event=false;}
'''
main=r'''
void fresh(){s_run.start(19);s_run.ticks=s_run.run_duration()+50;s_records={};save={};
 s_resultsStored=false;s_victory=s_deathPending=s_deathDismiss=false;s_victoryTicks=0;s_deathGameOver=kNone;
 retired=pickupsRetired=spawns=attacks=chests=magnets=orbs=deletes=0;
 life=12;ui=event=paused=transition=false;arena=true;gameOverType=0;nativeExists=true;
 player.id=s_player;s_enemies={};for(unsigned i=0;i<actors.size();++i){actors[i]={};actors[i].id=i;}
}
int main(){
 fresh();
 // Final boss, live horde, earlier elite and pending actor: final boss wins
 // regardless of earlier elites, and its rewards are created before cleanup.
 s_enemies[0]={0,0,false,false,true,true,3};actors[0].health=0;actors[0].current.pos={200,80,300};
 s_enemies[1]={1,0,false,false,true,false,-1};actors[1].current.pos={205,80,300};
 s_enemies[2]={2,0,false,false,true,true,0};actors[2].current.pos={5200,400,300};
 s_enemies[3]={99,0,false,false,false,false,-1};
 s_run.damage.dealt[0]=100;
 credit_death(s_enemies[0],&actors[0]);
 assert(s_victory&&s_run.kills==1&&s_run.timed_elites==1&&chests==1&&magnets==1&&orbs==1);
 assert(s_victoryWave.origin.x==200&&s_victoryWave.origin.y==80&&s_victoryWave.origin.z==300);
 assert(!pickupsRetired&&!save.writes&&s_run.phase==Phase::Running);
 assert(pause_run_actor(true)&&!pause_run_actor(false));
 saving=true;assert(pause_run_actor(true)&&pause_run_actor(false));saving=false;
 Pickup loot;int pickupResult=0;
 assert(run_pickup(&loot,&actors[0],&pickupResult)==HOOK_CONTINUE&&pickupMoves==1);
 s_run.phase=Phase::Upgrade;assert(run_pickup(&loot,&actors[0],&pickupResult)==HOOK_SKIP_ORIGINAL&&pickupMoves==1);
 s_run.phase=Phase::Running;
 const auto score=s_run.score(),ticks=s_run.ticks;const auto damage=s_run.damage.total();
 std::any playerArgs[]={&player};
 after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_enemies[1].retiring&&!s_enemies[2].retiring);
 actors[1].health=0;credit_death(s_enemies[1],&actors[1]);
 assert(s_run.kills==1&&chests==1&&s_run.damage.total()==damage);
 for(unsigned i=1;i<60;++i)after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_enemies[2].retiring&&s_enemies[3].retiring&&!pickupsRetired);
 assert(!attacks&&!spawns&&s_run.score()==score&&s_run.ticks==ticks&&s_run.damage.total()==damage);
 ui=true;for(int i=0;i<60;++i)after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_victoryTicks==60);ui=false;event=true;
 after_player(nullptr,playerArgs,nullptr,nullptr);assert(s_victoryTicks==60);event=false;
 s_run.phase=Phase::Treasure;after_player(nullptr,playerArgs,nullptr,nullptr);assert(s_victoryTicks==60);
 s_run.phase=Phase::Running;
 for(unsigned i=60;i<299;++i)after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(!save.writes&&s_victory&&s_victoryTicks==299);
 // Expiry must not swallow the upgrade from the last collected XP orb.
 s_run.xp=s_run.required_xp();after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_run.phase==Phase::Upgrade&&s_victoryTicks==300&&!save.writes);
 s_run.phase=Phase::Running;after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_run.phase==Phase::Victory&&s_resultsStored&&save.writes==1&&s_records.wins==1);
 assert(s_run.kills==1&&s_run.damage.total()==damage&&s_run.ticks==ticks&&pickupsRetired==1);
 finish(true);finish(false);assert(save.writes==1);
 // The repeated final-boss slot does not trigger a second victory sweep in Endless.
 fresh();s_run.phase=Phase::Victory;assert(s_run.continue_endless());
 s_enemies[0]={0,0,false,false,true,true,3};actors[0].health=0;
 credit_death(s_enemies[0],&actors[0]);
 assert(!s_victory&&s_run.phase==Phase::Running&&s_run.timed_elites==1&&chests==1&&!save.writes);
 // Native defeat must neither heal nor pause Link before his death animation.
 fresh();life=0;int result=0;
 assert(before_death(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
 assert(s_deathPending&&s_run.phase==Phase::Defeat&&!life&&!save.writes);
 event=true;assert(!pause_run_actor(false)&&pause_run_actor(true));
 dGameover_c screen;std::any screenArgs[]={&screen};
 for(int i=0;i<120;++i){assert(before_game_over(nullptr,screenArgs,&result,nullptr)==HOOK_CONTINUE);
  screen._execute();assert(update_ending(&player));}
 assert(screen.mProc==PROC_PLAYER_ANM_WAIT&&!save.writes&&!deletes&&!life);
 screen.mIsDemoSave=true; // Native Link signals only after the death animation.
 unsigned visibleFrames=0;
 for(int i=0;i<200&&!s_deathDismiss;++i){
  const auto action=before_game_over(nullptr,screenArgs,&result,nullptr);
  if(action==HOOK_CONTINUE){if(screen.mProc==PROC_DISP_WAIT)++visibleFrames;screen._execute();}
 }
 assert(s_deathDismiss&&visibleFrames==60&&screen.mProc==PROC_DELETE_WAIT&&!save.writes&&!life);
 assert(update_ending(&player)&&deletes==1&&!save.writes); // Native heap not freed yet.
 nativeExists=false;assert(!update_ending(&player));
 assert(save.writes==1&&s_resultsStored&&!s_deathPending&&s_run.phase==Phase::Defeat&&life==12&&!event);
 finish(false);assert(save.writes==1);
 // Manual End Run keeps its immediate path; other modes/save prompts untouched.
 fresh();finish(false);assert(save.writes==1&&!s_deathPending);
 fresh();arena=false;assert(before_death(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE&&!s_deathPending);
 fresh();s_deathPending=true;gameOverType=1;screen.mProc=PROC_DISP_WAIT;screen.mTimer=0;
 assert(before_game_over(nullptr,screenArgs,&result,nullptr)==HOOK_CONTINUE&&!s_deathDismiss);
 std::cout<<"Survivors ending: last-boss rewards, unscored sweep, loot grace, paused countdown, native death/title and one result commit passed\n";
}
'''
ending=(root/'src/survivors_ending.inc').read_text().replace('#include "survivors_victory_wave.inc"','')
native=(root/'dusklight/src/d/d_gameover.cpp').read_text()
code=fixture+'\nunsigned s_gearNoticeTicks=0;void update_gear_notice(){if(s_gearNoticeTicks)--s_gearNoticeTicks;}void clear_gear_notices(){s_gearNoticeTicks=0;}void drop_boss_gear(){}\n'+''.join(function(native,s) for s in ['int dGameover_c::_execute()',
 'void dGameover_c::playerAnmWait_init()','void dGameover_c::playerAnmWait_proc()',
 'void dGameover_c::dispFadeOut_init()','void dGameover_c::dispFadeOut_proc()',
 'void dGameover_c::dispWait_init()','void dGameover_c::dispWait_proc()'])+ending+''.join(
 function(core,s) for s in ['void finish(bool won) {','void credit_death(', 'void after_player(',
 'HookAction before_death(', 'bool pause_run_actor('])
code+='HookAction run_pickup(Pickup* pickup,fopAc_ac_c* actor,void* result){\n'+function(core,'    if (pickup) {\n        // Waiting pickups')+'return HOOK_CONTINUE;\n}\n'+main
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test';cpp.write_text(code)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
