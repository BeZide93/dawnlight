"""Compile the actual fairy interception/return code against a small engine fixture."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/cave_boss_visits.cpp').read_text()
source = source[:source.index('ModResult initialize_cave_boss_visits')]
source = re.sub(r'^#include .*\n', '', source, flags=re.M)
source = re.sub(r'^DEFINE_HOOK.*\n', '', source, flags=re.M)
source = source.replace('SaveObserverHandle s_observer = 0;', '') + '\n}\n'
fixture = r'''
#include "cave_boss_pool.hpp"
#include <any>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
using s8 = signed char; using s16 = short; using BOOL = int;
constexpr int TRUE=1;
struct ModContext {};
enum HookAction {HOOK_CONTINUE, HOOK_SKIP_ORIGINAL};
namespace mods {
template<class T> T arg(void* args, int n) {return std::any_cast<T>(static_cast<std::any*>(args)[n]);}
}
struct cXyz {float x,y,z; bool operator==(const cXyz&) const = default;};
struct daNpc_Fairy_c {
    int mStatus=1, mSwitchBit=7, room=9;
    struct {int flags=3;} attention_info;
};
int fopAcM_GetRoomNo(daNpc_Fairy_c* fairy) {return fairy->room;}
struct Player {struct {cXyz pos{1,2,3};} current; struct {s16 y=123;} shape_angle;} player;
bool hasPlayer=true, enabled=true, active=true, available=true, rejectWarp=false;
std::string stage="D_SB01";
int table=12, eventResets=0, messageKills=0, timerStarts=0, timerEnds=0;
int chosen=-1, warpRoom=-1, warpYaw=0, zoneResets=0, visits=0;
cXyz warpPosition{};
bool switches[240]{};
struct dSv_memory_c {int value=0;};
struct dSv_danBit_c {int value=0;};
struct SaveInfo {
    dSv_memory_c memory{88}; dSv_danBit_c dungeon{77};
    dSv_memory_c& getMemory(){return memory;}
    dSv_danBit_c& getDan(){return dungeon;}
} saveInfo;
struct SaveData {std::array<dSv_memory_c,32> tables{}; dSv_memory_c& getSave(int t){return tables.at(t);}} saveData;
struct Event {void reset(daNpc_Fairy_c*){++eventResets;}} event;
Event* dComIfGp_getEvent(){return &event;}
void dMsgObject_onKillMessageFlag(){++messageKills;}
Player* dComIfGp_getPlayer(int){return hasPlayer ? &player : nullptr;}
const char* dComIfGp_getStartStageName(){return stage.c_str();}
void* dComIfGp_getStageStagInfo(){return nullptr;}
int dStage_stagInfo_GetSaveTbl(void*){return table;}
SaveInfo* dComIfGs_getSaveInfo(){return &saveInfo;}
SaveData* dComIfGs_getSaveData(){return &saveData;}
bool dComIfGs_isSwitch(int sw,int){return switches[sw];}
void dComIfGs_onSwitch(int sw,int){switches[sw]=true;}
void dComIfGs_offSwitch(int sw,int){switches[sw]=false;}
void dComIfGp_roomControl_initZone(){++zoneResets;}
float cM_rndF(float remaining){assert(remaining>0);return 0;}
namespace dawnlight {
bool cave_boss_visits_enabled(){return enabled;}
bool save_state_boss_rush_active(){return active;}
bool cave_boss_warp_available(){return available;}
bool warp_to_cave_boss(unsigned boss){if(rejectWarp)return false;chosen=boss;++visits;return true;}
void warp_back_to_cave(const cXyz& pos,short yaw,signed char room){warpPosition=pos;warpYaw=yaw;warpRoom=room;}
void begin_cave_boss_timer(int){++timerStarts;}
void end_cave_boss_timer(){++timerEnds;}
void reset_cave_boss_visits();
}
'''
fixture += source
fixture += r'''
int main() {
    using namespace dawnlight;
    daNpc_Fairy_c fairy;
    std::any args[]={&fairy}; BOOL result=0;
    // Ordinary saves/stages, disabled option and visible fairies keep native dialogue.
    for(int scenario=0; scenario<5; ++scenario) {
        if(scenario==0)active=false;
        if(scenario==1)enabled=false;
        if(scenario==2)stage="elsewhere";
        if(scenario==3)fairy.mStatus=0;
        if(scenario==4)fairy.room=8;
        assert(fairy_talk(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
        assert(visits==0 && eventResets==0);
        active=true;enabled=true;stage="D_SB01";fairy.mStatus=1;fairy.room=9;
    }
    // A failed/busy warp never consumes a boss or changes the native fairy state.
    available=false;
    assert(fairy_talk(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(visits==0 && result==TRUE && eventResets==1 && messageKills==1);
    available=true;rejectWarp=true;
    fairy_talk(nullptr,args,&result,nullptr);
    assert(s_pool.used==0 && !s_visit.active);
    rejectWarp=false;
    std::set<int> bosses;
    for(int room : {9,19,29,39,49}) {
        fairy.room=room;
        player.current.pos={float(room),10,-20};
        player.shape_angle.y=room*100;
        saveInfo.memory.value=room+1000;saveInfo.dungeon.value=room+2000;
        for(int sw=192;sw<240;++sw)switches[sw]=(sw%2==0);
        assert(fairy_talk(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(fairy.mStatus==1 && bosses.insert(chosen).second);
        assert(s_visit.active && s_visit.room==room);
        // Simulate unrelated boss stage memory, including the same switch numbers.
        stage="boss";
        saveInfo.memory.value=-1;saveInfo.dungeon.value=-2;
        for(int sw=192;sw<240;++sw)switches[sw]=(sw%2!=0);
        enabled=false; // Return is owed even if the setting is switched off in battle.
        assert(return_from_cave_boss());
        assert(warpRoom==room && warpYaw==room*100 && warpPosition==player.current.pos);
        assert(saveData.tables[table].value==room+1000);
        assert(!return_from_cave_boss()); // Victory hooks cannot return twice.
        std::any danArgs[]={&saveInfo.dungeon,s8(table)};
        dungeon_init(nullptr,danArgs,nullptr,nullptr);
        assert(saveInfo.dungeon.value==-2); // Only restore inside the Cave.
        stage="D_SB01";
        danArgs[1]=s8(table+1);
        dungeon_init(nullptr,danArgs,nullptr,nullptr);
        assert(saveInfo.dungeon.value==-2);
        danArgs[1]=s8(table);
        dungeon_init(nullptr,danArgs,nullptr,nullptr);
        assert(saveInfo.dungeon.value==room+2000);
        switches[fairy.mSwitchBit]=false;
        fairy_params(nullptr,args,nullptr,nullptr);
        assert(switches[fairy.mSwitchBit]); // Gate opens with the option now disabled.
        for(int sw=192;sw<240;++sw)assert(switches[sw]==(sw%2==0));
        enabled=true;fairy_params(nullptr,args,nullptr,nullptr);
        assert(fairy.attention_info.flags==0);
        const int oldVisits=visits;
        fairy_talk(nullptr,args,&result,nullptr);
        assert(visits==oldVisits); // Re-entering a cleared fairy room never rolls again.
    }
    assert(cave_boss_final_cleared() && bosses.size()==5 && timerStarts==5 && timerEnds==5);
    assert(zoneResets==5);
    save_reset(nullptr,0,nullptr);
    assert(!cave_boss_final_cleared() && s_pool.used==0);
    fairy_talk(nullptr,args,&result,nullptr);
    assert(chosen==0 && s_visit.active);
    reset_cave_boss_visits(); // Death/manual exit/mode reset abandons the pending return.
    assert(!return_from_cave_boss());
    std::cout << "Cave fairy interception, unique bosses, same-room returns and state restoration: OK\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'visits.cpp'
    cpp.write_text(fixture)
    binary = Path(tmp) / 'visits'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
