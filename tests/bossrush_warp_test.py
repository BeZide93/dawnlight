"""Compile the production warp lifecycle functions against small engine stubs."""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / "src/new_save_modes.cpp").read_text()


def function(name):
    import re
    match = re.search(r"^(?:void|bool|HookAction) " + name + r"\([^\n]*\) \{", source, re.M)
    assert match, name
    depth = 0
    start = source.index("{", match.start())
    for end in range(start, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[match.start():end + 1]
    raise AssertionError(name)


# Check integration ordering as well as the executable helpers below.
update = function("update_bossrush")
assert update.index("if (sBossRushWarpInFlight)") < update.index("restore_bossrush_hub_load_state()")
assert update.index("sBossRushArrivalAnimating") < update.index("ensure_direct_final_boss_started()")
assert "prepare_bossrush_entry(entry)" not in function("set_bossrush_next_stage")
assert "kBossRushCavePortalIndex" not in function("process_pending_midna_flow_action")
assert "sBossRushWarpInFlight" in function("is_direct_final_ganondorf_active")
assert "sBossRushDepartureStarted || dComIfGp_isEnableNextStage()" in function("on_ganondorf_execute_pre")
assert "return_to_hub_after_replay_victory()" in update
assert "return_to_hub_after_replay_victory()" in function("redirect_replay_to_hub")
assert "start_bossrush_warp(" in function("warp_to_bossrush_hub_from_midna")
for hook in ("WarpPlayerCreateHook", "WarpPlayerExecuteHook", "WarpPlayerDrawHook"):
    assert f"uninstall_bossrush_hook<{hook}>" in source

stubs = r'''
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
using s8 = signed char; using s16 = short; using u16 = unsigned short;
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods { template<class T> T arg(void* args, int) { return *static_cast<T*>(args); } }
constexpr int cPhs_COMPLEATE_e = 4;
constexpr int kBossRushStateHub = 0, kBossRushStateReplay = 2, kBossRushStateRun = 1;
const char* kBossRushReturnStage = "D_MN09C";
const char* kCaveOfOrdealsStage = "D_SB01";
constexpr s16 kBossRushReturnPoint=0;
constexpr s8 kBossRushReturnRoom=0, kBossRushReturnLayer=-1;
bool active = true, bossRush = true, next = false, peek = false, event = false;
bool wolf = false, havePlayer = true;
int state = 2, prepared = 0, resets = 0, armed = 0, changes = 0;
std::string currentStage = "D_MN09C", destinationStage;
s16 destinationPoint; s8 destinationRoom, destinationLayer;
bool sBossRushWarpPending = false, sBossRushWarpInFlight = false;
bool sBossRushDepartureStarted = false, sBossRushArrivalReady = false;
bool sBossRushArrivalAnimating = false, sBossRushResetRunOnDeparture = false;
bool sHubArrivalWarpPending = false, sCaveArrivalWarpPending = false;
u16 sBossRushWarpFrames = 0;
struct BossRushWarpDestination { char stage[16] = {}; s16 point=0; s8 room=0, layer=0; };
BossRushWarpDestination sBossRushWarpDestination;
struct Vec { void set(float,float,float) {} };
struct daMidna_c { void changeDemoMode(int) {} } midna;
struct daAlink_c {
    enum { PROC_WARP = 8 };
    int mProcID=0, wait=0, initCount=0, initDirection=-1, initMode=-1;
    bool failInit=false;
    float field_0x347c=0, mNormalSpeed=0; s16 mDamageTimer=0;
    struct { void* mAnimeHeap = reinterpret_cast<void*>(1); } mAnmHeap3;
    struct { int field_0x3008=0; } mProcVar0;
    struct { int field_0x300c=0; } mProcVar2;
    struct { int field_0x3012=0; } mProcVar5;
    Vec speed;
    int getClothesChangeWaitTimer() { return wait; }
    bool procCoWarpInit(int direction, int mode) {
        if (failInit || mProcID==PROC_WARP) return false;
        ++initCount; mProcID=PROC_WARP; initDirection=direction; initMode=mode;
        mProcVar2.field_0x300c=direction; return true;
    }
    void offPlayerNoDraw() {}
} player;
struct daPy_py_c {
    static bool checkNowWolf() { return wolf; }
    static daMidna_c* getMidnaActor() { return &midna; }
};
daAlink_c* daAlink_getAlinkActorClass() { return havePlayer ? &player : nullptr; }
bool is_bossrush_game_mode_active() { return active; }
bool is_boss_rush() { return bossRush; }
bool is_current_stage_name(const char* stage) { return currentStage==stage; }
bool dComIfGp_isEnableNextStage() { return next; }
bool fopOvlpM_IsPeek() { return peek; }
bool dComIfGp_event_runCheck() { return event; }
int boss_rush_state() { return state; }
int boss_rush_index() { return 0; }
int kBossRushEntries[] = {0};
void prepare_bossrush_entry(int) { ++prepared; }
void clear_all_boss_flags() { ++resets; }
void arm_bossrush_hub_return_warp() { ++armed; }
void set_bossrush_return_place() {}
void dComIfGp_setNextStage(const char* stage, s16 point, s8 room, s8 layer) {
    ++changes; next=true; destinationStage=stage; destinationPoint=point;
    destinationRoom=room; destinationLayer=layer;
}
'''

functions = "\n".join(function(name) for name in (
    "clear_bossrush_warp_request", "set_bossrush_warp_destination",
    "start_bossrush_warp", "finish_bossrush_departure", "update_bossrush_warp",
    "update_bossrush_arrival_warp", "on_skip_portal_obj_warp_pre",
    "return_to_hub_after_replay_victory",
    "on_warp_player_create_post", "on_warp_player_execute_pre", "on_warp_player_draw_pre",
))

cases = r'''
void reset() {
    clear_bossrush_warp_request(); sBossRushWarpInFlight=false;
    sBossRushArrivalReady=false; sBossRushArrivalAnimating=false;
    sBossRushResetRunOnDeparture=false;
    sHubArrivalWarpPending=false; sCaveArrivalWarpPending=false;
    next=false; peek=false; event=false; wolf=false; havePlayer=true;
    active=true; bossRush=true; state=2;
    prepared=resets=armed=changes=0; currentStage="D_MN09C"; player={};
}
void created() {
    int phase=1;
    on_warp_player_create_post(nullptr,nullptr,&phase,nullptr);
    assert(!sBossRushArrivalReady);
    phase=cPhs_COMPLEATE_e;
    on_warp_player_create_post(nullptr,nullptr,&phase,nullptr);
}
int main() {
    // All departure destinations wait out events and unavailable player resources.
    for (const char* target : {"D_MN01A", "D_MN09C", "D_SB01"}) {
        reset(); event=true;
        assert(start_bossrush_warp(target,23,4,-1));
        assert(!start_bossrush_warp("BAD",0,0,0));
        for (int n=0;n<220;++n) update_bossrush_warp();
        assert(!player.initCount && !changes && !prepared);
        event=false; havePlayer=false; update_bossrush_warp(); assert(!changes);
        havePlayer=true; player.failInit=true; update_bossrush_warp(); assert(!changes);
        player.failInit=false; update_bossrush_warp();
        assert(player.initDirection==0 && player.initMode==1);
        for (int n=0;n<220;++n) update_bossrush_warp();
        assert(!changes); // No timeout teleport with Link still visible.
        assert(on_skip_portal_obj_warp_pre(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(changes==1 && prepared==1 && destinationStage==target);
        assert(destinationPoint==23 && destinationRoom==4 && destinationLayer==-1);
        assert(sBossRushWarpInFlight && !sBossRushWarpPending);
        assert(on_skip_portal_obj_warp_pre(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(changes==1); // Repeated native callbacks during fade are consumed.
        update_bossrush_warp(); assert(sBossRushWarpInFlight && changes==1);
        next=false; currentStage=target; player={}; created();
        assert(!sBossRushWarpInFlight && sBossRushArrivalReady);
        assert(on_warp_player_draw_pre(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
        assert(player.initCount==0); // Boss arrival remains native.
    }
    // Same-stage Ganondorf -> hub: arrival cannot run on the outgoing actor.
    for (const char* target : {"D_MN09C", "D_SB01"}) {
        reset(); state=0;
        bool& pending = std::strcmp(target,"D_MN09C")==0 ? sHubArrivalWarpPending : sCaveArrivalWarpPending;
        pending=true; start_bossrush_warp(target,0,0,-1);
        update_bossrush_arrival_warp(pending,target);
        assert(player.initCount==0 && pending);
        update_bossrush_warp();
        update_bossrush_arrival_warp(pending,target);
        assert(player.initDirection==0 && pending);
        on_skip_portal_obj_warp_pre(nullptr,nullptr,nullptr,nullptr);
        assert(prepared==0);
        next=false; currentStage=target; player={}; created(); peek=true;
        update_bossrush_arrival_warp(pending,target);
        int result=0;
        assert(on_warp_player_draw_pre(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(result==1 && player.initCount==0);
        peek=false; player.wait=1;
        update_bossrush_arrival_warp(pending,target); assert(pending);
        player.wait=0; wolf=true;
        auto* link=&player;
        on_warp_player_execute_pre(nullptr,&link,nullptr,nullptr);
        assert(player.initDirection==1 && player.initMode==0 && !pending);
        assert(player.mProcVar5.field_0x3012==1 && player.mDamageTimer>=2);
        assert(sBossRushArrivalAnimating);
        assert(on_warp_player_draw_pre(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
        on_warp_player_execute_pre(nullptr,&link,nullptr,nullptr);
        assert(player.initCount==1); // No repeated restart of arrival.
        player.mProcID=0; on_warp_player_execute_pre(nullptr,&link,nullptr,nullptr);
        assert(!sBossRushArrivalAnimating);
        assert(armed==(std::strcmp(target,"D_SB01")==0 ? 1 : 0));
    }
    reset(); sBossRushResetRunOnDeparture=true; state=kBossRushStateRun;
    start_bossrush_warp("D_MN01A",0,0,-1); update_bossrush_warp();
    assert(resets==0); player.field_0x347c=-0.5f;
    for (int n=0;n<180;++n) update_bossrush_warp();
    assert(resets==1 && changes==1 && prepared==1);
    // Direct-fight victory transitions immediately, but hub arrival still waits
    // for the new player (especially when returning from the same Ganondorf stage).
    reset(); state=kBossRushStateHub;
    return_to_hub_after_replay_victory();
    assert(changes==1 && destinationStage==kBossRushReturnStage);
    assert(player.initCount==0 && !sBossRushWarpPending && sBossRushWarpInFlight);
    update_bossrush_arrival_warp(sHubArrivalWarpPending,kBossRushReturnStage);
    assert(player.initCount==0);
    next=false; player={}; created();
    update_bossrush_arrival_warp(sHubArrivalWarpPending,kBossRushReturnStage);
    assert(player.initDirection==1 && !sHubArrivalWarpPending);
    reset(); active=false;
    assert(on_skip_portal_obj_warp_pre(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
    assert(on_warp_player_draw_pre(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
    std::cout << "Boss Rush warp lifecycle checks passed\n";
}
'''

with tempfile.TemporaryDirectory(prefix="dawnlight-warp-test-") as directory:
    cpp = Path(directory) / "test.cpp"
    exe = Path(directory) / "test"
    cpp.write_text(stubs + functions + cases)
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
