"""Execute the production timer context and native Cave completion predicate with host fixtures."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/new_save_modes.cpp').read_text()

def function(name):
    m = re.search(r'^(?:void\*|ClearTimerContext|void) ' + name + r'\([^\n]*\) \{', source, re.M)
    assert m, name
    start = source.index('{', m.start())
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if depth == 0:
            return source[m.start():end + 1]
    raise AssertionError(name)

fixture = r'''
#include <cassert>
#include <string>
#include "clear_timer_state.hpp"
using namespace dawnlight;
struct ClearTimerContext {bool active=false,counting=false,dead=false; int encounter=-1;};
bool active=true, save=true, gameplay=true, resetting=false, shadeActive=false;
bool transition=false, overlap=false, pause=false, ui=false, event=false;
bool sAdvancePending=false,sBossRushWarpPending=false,sBossRushWarpInFlight=false;
bool sHubArrivalWarpPending=false,sCaveArrivalWarpPending=false,sBossRushArrivalAnimating=false;
constexpr int kBossRushStateHub=0,kBossRushStateRun=1,kBossRushStateReplay=2,kBossRushStateCaveOfOrdeals=3;
constexpr int kFinalBeastRoom=51;
constexpr char kCaveOfOrdealsStage[]="D_SB01";
int state=0,index=0,room=0,gameOver=0;
std::string stage="hub";
struct Player {bool dead=false; bool checkDeadHP(){return dead;}} player;
Player* daAlink_getAlinkActorClass(){return &player;}
bool is_bossrush_game_mode_active(){return active;}
bool is_boss_rush(){return save;}
bool can_update_bossrush_gameplay(){return gameplay;}
bool is_reset_to_opening_transition(){return resetting;}
int dMeter2Info_getGameOverType(){return gameOver;}
int boss_rush_state(){return state;}
int boss_rush_index(){return index;}
int dComIfGp_roomControl_getStayNo(){return room;}
bool is_current_stage_name(const char* name){return stage==name;}
bool heroes_shade_timer_active(){return shadeActive;}
bool dComIfGp_isEnableNextStage(){return transition;}
bool fopOvlpM_IsPeek(){return overlap;}
bool dComIfGp_isPauseFlag(){return pause;}
bool ui_document_visible(){return ui;}
bool dComIfGp_event_runCheck(){return event;}
int kBossRushEntries[18]={};
bool is_current_stage(int){return stage=="boss";}
struct fopAc_ac_c {int name=20,frontRoom=48,backRoom=49,front=3,back=0,sw=1,sw2=2;bool actor=true;};
constexpr int fpcNm_DOOR20_e=20;
bool fopAcM_IsActor(void* p){return static_cast<fopAc_ac_c*>(p)->actor;}
int fopAcM_GetName(fopAc_ac_c* p){return p->name;}
struct door_param2_c {
static int getFRoomNo(fopAc_ac_c* p){return p->frontRoom;}
static int getBRoomNo(fopAc_ac_c* p){return p->backRoom;}
static int getFrontOption(fopAc_ac_c* p){return p->front;}
static int getBackOption(fopAc_ac_c* p){return p->back;}
static int getSwbit(fopAc_ac_c* p){return p->sw;}
static int getSwbit2(fopAc_ac_c* p){return p->sw2;}
};
bool switches[256]{};
bool dComIfGs_isSwitch(int sw,int room){assert(room==48);return switches[sw];}
'''
fixture += r'''
struct ModError {};
#define MOD_ERROR_INIT {}
constexpr int MOD_OK=0;
constexpr const char* kBossRushGameModeId="bossrush";
void* mod_ctx=nullptr;
bool setting=true,sBossRushModeRegistered=true;
int registrations=0,unregistrations=0,cancellations=0,timerUpdates=0;
bool boss_rush_enabled(){return setting;}
int register_bossrush_mode(ModError*){++registrations;sBossRushModeRegistered=true;return MOD_OK;}
struct Modes {int unregister_game_mode(void*,const char*){++unregistrations;active=false;return MOD_OK;}} modes;
auto* svc_game_mode=&modes;
struct Log {void warn(void*,const char*){assert(false);}} logService;
auto* svc_log=&logService;
void cancel_clear_timer(){++cancellations;}
void update_clear_timer(){++timerUpdates;}
void update_heroes_shade_audio(){}
void retry_pending_actor_deletes(){}
'''
fixture += function('update_new_save_modes') + '\n'
fixture += function('clear_timer_context') + '\n' + function('cleared_final_cave_door')
fixture += r'''
int main(){
    assert(clear_timer_context().active && !clear_timer_context().counting);
    state=kBossRushStateReplay;stage="boss";index=3;
    assert(clear_timer_context().counting && clear_timer_context().encounter==3);
    for(bool* guard:{&transition,&overlap,&pause,&ui,&event,&sAdvancePending,&sBossRushWarpPending,
         &sBossRushWarpInFlight,&sHubArrivalWarpPending,&sCaveArrivalWarpPending,&sBossRushArrivalAnimating}){
        *guard=true;assert(!clear_timer_context().counting);*guard=false;
    }
    player.dead=true;assert(clear_timer_context().dead && !clear_timer_context().counting);player.dead=false;
    gameOver=1;assert(clear_timer_context().dead);gameOver=0;
    active=false;assert(!clear_timer_context().active);active=true;
    save=false;assert(!clear_timer_context().active);save=true;
    resetting=true;assert(!clear_timer_context().active);resetting=false;
    state=kBossRushStateRun;index=15;stage="D_MN09A";room=50;
    assert(clear_timer_context().encounter==15);
    room=51;assert(clear_timer_context().encounter==16);
    stage="D_MN09B";assert(clear_timer_context().encounter==timing::run);
    stage="D_MN09C";assert(clear_timer_context().encounter==17);
    state=kBossRushStateCaveOfOrdeals;stage="D_SB01";
    assert(clear_timer_context().counting && clear_timer_context().encounter==timing::cave);
    stage="hub";assert(!clear_timer_context().counting);
    state=kBossRushStateHub;shadeActive=true;
    assert(clear_timer_context().counting && clear_timer_context().encounter==timing::shade);
    shadeActive=false;assert(!clear_timer_context().counting);

    update_new_save_modes();assert(registrations==0 && unregistrations==0);
    setting=false;update_new_save_modes();
    assert(!sBossRushModeRegistered && unregistrations==1 && cancellations==1 && !active);
    update_new_save_modes();assert(unregistrations==1);
    setting=true;update_new_save_modes();assert(sBossRushModeRegistered && registrations==1);
    update_new_save_modes();assert(registrations==1 && timerUpdates==5);

    fopAc_ac_c door;
    assert(!cleared_final_cave_door(&door,nullptr)); // includes delayed/pending spawns
    switches[1]=true;assert(cleared_final_cave_door(&door,nullptr));
    door.frontRoom=47;assert(!cleared_final_cave_door(&door,nullptr));
    door.backRoom=48;door.back=3;
    assert(!cleared_final_cave_door(&door,nullptr)); // don't use the other side's switch
    switches[2]=true;assert(cleared_final_cave_door(&door,nullptr));
    door.sw2=255;assert(!cleared_final_cave_door(&door,nullptr));
    door.sw2=2;door.name=12;assert(!cleared_final_cave_door(&door,nullptr));
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'context.cpp'
    cpp.write_text(fixture)
    binary = Path(tmp) / 'context'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Clear timer gameplay guards, final phases and native Cave door completion: OK')
