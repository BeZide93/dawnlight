"""Exercise the production Cave/arena transition hook with engine state stubs."""
from pathlib import Path
import re
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / "src/new_save_modes.cpp").read_text()


def function(name, source=source):
    match = re.search(r"^(?:void|bool|HookAction) " + name + r"\([^\n]*\) \{", source, re.M)
    assert match, name
    depth = 0
    start = source.index("{", match.start())
    for end in range(start, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[match.start():end + 1]
    raise AssertionError(name)


stubs = r'''
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <any>
using s8 = signed char; using s16 = short; using s32 = int; using BOOL = int;
constexpr int TRUE=1, FALSE=0;
#include <unordered_set>
#include <vector>
using ActorId = unsigned;
using process_method_func = int (*)(void*);
struct process_method_class { process_method_func create_method, delete_method, execute_method; };
struct process_class { ActorId id; bool initialized=false; };
struct fopAc_ac_c : process_class {
    const process_method_class* sub_method=nullptr;
    int profile=0;
    struct { short y=0; } shape_angle;
    struct { decltype(shape_angle) angle; } current;
};
struct e_tk_class : fopAc_ac_c { int mExecuteState=0; };
struct e_tk2_class : fopAc_ac_c { int mActionTimer[4]{}; };
struct daE_ZS_c : fopAc_ac_c { int field_0x671=0; };
ActorId fpcM_GetID(const void* p) { return static_cast<const process_class*>(p)->id; }
ActorId fopAcM_GetID(fopAc_ac_c* p) { return p->id; }
bool fopAcM_IsActor(void* p) { return static_cast<process_class*>(p)->initialized; }
int fopAcM_GetName(fopAc_ac_c* p) { return p->profile; }
std::unordered_set<ActorId> s_testActors;
std::vector<std::pair<void*, bool>> s_testProcessStack;
struct ModContext {}; struct dSv_save_c {};
struct dMenu_save_c { int mEndStatus=2; int getEndStatus() { return mEndStatus; } };
struct dGameover_c { bool mIsDemoSave=false; dMenu_save_c* dMs_c=nullptr; };
int gameOverStatus=0;
int dComIfGp_getGameoverStatus() { return gameOverStatus; }
int gameOverType=0;
int dMeter2Info_getGameOverType() { return gameOverType; }
struct dSv_memBit_c { enum { STAGE_BOSS_ENEMY=3, STAGE_BOSS_DEMO=5, STAGE_BOSS_ENEMY_2=7 }; };
enum { fpcNm_B_TN_e, fpcNm_TBOX_e, fpcNm_TBOX2_e, fpcNm_Obj_Carry_e,
       fpcNm_TAG_EVT_e, fpcNm_TAG_MSG_e, fpcNm_TAG_EVTAREA_e, fpcNm_TAG_EVTMSG_e,
       testDoorProfile, fpcNm_E_TK_e, fpcNm_E_TK2_e, fpcNm_E_ZS_e };
struct dStage_objectNameInf { int procname; };
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods {
template<class T> T arg(void* args, int n) { return std::any_cast<T>(static_cast<std::any*>(args)[n]); }
template<class T> T& arg_ref(void* args, int n) { return *std::any_cast<T>(&static_cast<std::any*>(args)[n]); }
}
constexpr int kBossRushStateHub=0, kBossRushStateCaveOfOrdeals=3;
const char* kBossRushReturnStage="D_MN09C";
const char* kBossRushDarknutAreaStage="D_DLBR0";
const char* kCaveOfOrdealsStage="D_SB01";
const char* kIntroSkipStage="F_SP108";
constexpr s16 kBossRushReturnPoint=0, kCaveOfOrdealsPoint=0;
@ARENA_POINT@
constexpr s8 kBossRushReturnRoom=0, kBossRushReturnLayer=0;
constexpr s8 kBossRushDarknutAreaRoom=51, kBossRushDarknutAreaLayer=0;
constexpr s8 kCaveOfOrdealsRoom=0, kCaveOfOrdealsLayer=-1, kIntroSkipRoom=0;
bool active=true, resetting=false;
bool s_testEnemyCreateInProgress=false;
bool sHubArrivalWarpPending=false, sBossRushArrivalReady=true;
int state=3, bossIndex=0, stay=0;
unsigned restartParam=0;
void dComIfGs_setRestartRoomParam(unsigned value) { restartParam=value; }
std::string current="D_SB01";
bool is_bossrush_game_mode_active() { return active; }
bool ensure_bossrush_runtime_for_save() { return active; }
bool is_boss_rush() { return active; }
int boss_rush_state() { return state; }
void set_boss_rush_state(int value) { state=value; }
void set_boss_rush_index(int value) { bossIndex=value; }
bool is_current_stage_name(const char* value) { return current==value; }
bool is_bossrush_darknut_area_stage() { return current==kBossRushDarknutAreaStage && stay==51; }
int dComIfGp_roomControl_getStayNo() { return stay; }
dSv_save_c* dComIfGs_getSaveData() { return nullptr; }
bool is_opening_stage(const char* stage,s16,s8,s8) { return stage && std::strcmp(stage,"TITLE")==0; }
bool is_reset_to_opening_transition() { return resetting; }
bool is_vanilla_new_file_stage(const char*,s16,s8,s8) { return false; }
bool is_intro_skipped() { return false; }
void prepare_bossrush_start() {}
void prepare_intro_skip_start() {}
void set_bossrush_return_place() {}
void clear_pending_midna_flow_action() {}
void clear_hub_confirm_state() {}
void reset_hub_actor_ids() {}
void reset_direct_final_boss_state() {}
void reset_bossrush_hazards() {}
'''

# Read the real destination point so the test catches accidental restart spawns.
stubs = stubs.replace("@ARENA_POINT@", re.search(
    r"constexpr s16 kBossRushDarknutAreaPoint = [^;]+;", source).group())

spawner = (Path(__file__).resolve().parents[1] / "src/enemy_spawner.cpp").read_text()
# Compile the real process callbacks too: the room check runs before actor_type
# has been initialized, and nested native processes must retain cleared flags.
functions = "\n".join(function(name, spawner) for name in (
    "is_test_actor", "before_process", "after_process", "enemy_spawner_process_active",
)) + "\n"
assert "add_post<SpawnerProcessHook>(svc_hook, after_process)" in spawner
functions += "\n".join(function(name) for name in (
    "is_bossrush_darknut_area_active", "on_bossrush_area_dungeon_bit_pre",
    "on_bossrush_area_switch_pre", "on_bossrush_area_actor_name_post",
    "prepare_darknut_area_entry", "prepare_cave_entrance_return",
    "prepare_hub_return_from_cave", "set_next_stage_args", "on_cave_gameover_close_pre", "on_set_next_stage_pre",
))

cases = r'''
void reset(const char* stage="D_SB01", int room=0, int mode=3) {
    current=stage; stay=room; state=mode; active=true; resetting=false;
    sHubArrivalWarpPending=false; sBossRushArrivalReady=true;
    gameOverStatus=0; gameOverType=0;
}
void route(const char* requested, const char* expected, int expectedState, int expectedRoom,
           int expectedPoint, int expectedLayer) {
    std::any args[]={requested,s16(42),s8(12),s8(2),float(7.5),unsigned(9)};
    assert(on_set_next_stage_pre(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(std::strcmp(mods::arg<const char*>(args,0),expected)==0);
    assert(state==expectedState);
    assert(mods::arg<s8>(args,2)==expectedRoom);
    assert(mods::arg<s16>(args,1)==expectedPoint);
    assert(mods::arg<s8>(args,3)==expectedLayer);
    assert(mods::arg<float>(args,4)==7.5 && mods::arg<unsigned>(args,5)==9);
}
int main() {
    // Entrance exit goes to the private, empty Darknut room in hub state.
    reset(); route("F_SP124","D_DLBR0",0,51,0,0);
    assert(!sHubArrivalWarpPending);
    // dStage_playerInit uses restart coordinates and unmodified room bits for
    // point -1. Other points use authored coordinates but can still inherit a
    // warp/demo parameter override. Exercise the destination after that boundary.
    for (unsigned stale : {0u, 0xCA000u, 0xCA009u}) {
        reset(); restartParam=stale;
        route("F_SP124","D_DLBR0",0,51,0,0);
        assert(restartParam==0);
        const unsigned authoredParam=0x10000u;
        unsigned playerParam=kBossRushDarknutAreaPoint==-1 ? restartParam :
            ((restartParam!=0 ? restartParam : authoredParam) & ~0x3fu) | kBossRushDarknutAreaRoom;
        assert(playerParam==(authoredParam | 51u));
        current=kBossRushDarknutAreaStage; stay=playerParam & 0x3f;
        assert(is_bossrush_darknut_area_active());
        dStage_objectNameInf info{fpcNm_B_TN_e}; auto* actor=&info;
        on_bossrush_area_actor_name_post(nullptr,nullptr,&actor,nullptr);
        assert(actor==nullptr);
    }
    // Fairy exits on every tenth floor, including alternate fountain choices.
    for (int floor : {9,19,29,39,49}) {
        for (const char* spring : {"F_SP108","F_SP115","F_SP116","F_SP113"}) {
            reset("D_SB01",floor);
            route(spring,"D_MN09C",0,0,0,0);
            assert(sHubArrivalWarpPending && !sBossRushArrivalReady);
        }
        // Descending/continuing inside the Cave must retain the exact spawn.
        reset("D_SB01",floor); route("D_SB01","D_SB01",3,12,42,2);
    }
    // Both confirmed answers resume through native death recovery, then
    // always target the Cave entrance, regardless of the original exit/floor.
    for (int floor : {0,9,19,29,39,49}) {
        for (const char* nativeTarget : {"F_SP124","D_SB01","D_MN09C"}) {
            for (int answer : {0,1}) {
                reset("D_SB01",floor);
                dMenu_save_c menu{answer}; dGameover_c gameover{true,&menu};
                std::any args[]={&gameover};
                on_cave_gameover_close_pre(nullptr,args,nullptr,nullptr);
                assert(menu.mEndStatus==1);
                on_cave_gameover_close_pre(nullptr,args,nullptr,nullptr);
                assert(menu.mEndStatus==1);
                gameOverStatus=2; // native saveClose_proc resumes PROC_DEAD
                restartParam=0xC9009;
                route(nativeTarget,"D_SB01",3,0,0,-1);
                assert(restartParam==0 && !sHubArrivalWarpPending);
                // After scene initialization, the normal exit still goes to the arena.
                gameOverStatus=0; stay=0;
                route("F_SP124","D_DLBR0",0,51,0,0);
            }
        }
    }
    // Other saves, other stages and demo/save prompts keep native behavior.
    for (int scenario=0;scenario<5;++scenario) {
        reset();
        if (scenario==0) active=false;
        if (scenario==1) current="D_DLBR0";
        if (scenario==2) gameOverType=1;
        if (scenario==3) resetting=true;
        if (scenario==4) gameOverType=2;
        dMenu_save_c menu{0}; dGameover_c gameover{true,&menu};
        std::any args[]={&gameover};
        on_cave_gameover_close_pre(nullptr,args,nullptr,nullptr);
        assert(menu.mEndStatus==0);
    }
    reset();
    dMenu_save_c pendingMenu{2}; dGameover_c pendingGameover{false,&pendingMenu};
    std::any pendingArgs[]={&pendingGameover};
    on_cave_gameover_close_pre(nullptr,pendingArgs,nullptr,nullptr);
    assert(pendingMenu.mEndStatus==2);
    // Both arena doors lead to the Cave entrance, including Temple destination.
    reset("D_DLBR0",51,0); route("D_MN06","D_SB01",3,0,0,-1);
    reset("D_DLBR0",51,0); route("D_MN06B","D_SB01",3,0,0,-1);
    // Explicit hub returns must not be captured by the entrance route.
    reset(); route("D_MN09C","D_MN09C",0,0,0,0);
    reset("D_DLBR0",51,0); route("D_MN09C","D_MN09C",0,12,42,2);
    // Vanilla saves, actual Darknut fights and title/reset paths pass through.
    reset(); active=false; route("F_SP124","F_SP124",3,12,42,2);
    reset("D_DLBR0",51,2); route("D_MN06","D_MN06",2,12,42,2);
    reset(); route("TITLE","TITLE",3,12,42,2);
    reset(); resetting=true; route("F_SP124","F_SP124",3,12,42,2);
    // Completion/gate overrides must be in effect before actor creation and
    // must leave the real arena and non-Boss-Rush saves alone.
    reset("D_DLBR0",51,0);
    for (int bit : {3,5,7}) {
        std::any args[]={nullptr,bit}; int result=0;
        assert(on_bossrush_area_dungeon_bit_pre(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(result==1);
    }
    for (int room : {51,-1}) {
        std::any args[]={nullptr,2,room}; int result=0;
        assert(on_bossrush_area_switch_pre(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(result==1);
    }
    // Simulate the exact first-create boundary: the process is registered but
    // does not have actor_type yet. Native enemy admission must see switch off.
    process_class testEnemy{42}, nativeGate{43};
    std::any processArgs[]={process_method_func(nullptr),static_cast<void*>(&testEnemy)};
    std::any switchArgs[]={nullptr,2,51}; int switchResult=-1;
    std::any bossArgs[]={nullptr,3}; int bossResult=-1;
    auto checkRoom = [&](int cleared) {
        assert(on_bossrush_area_switch_pre(nullptr,switchArgs,&switchResult,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(on_bossrush_area_dungeon_bit_pre(nullptr,bossArgs,&bossResult,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(switchResult==cleared && bossResult==cleared);
    };
    // Production create_actor enters native creation before it returns actorId,
    // so the actor cannot be in s_testActors yet. The explicit create scope
    // must expose an uncleared room during that window.
    assert(!s_testActors.contains(42));
    s_testEnemyCreateInProgress=true;
    checkRoom(0);
    s_testEnemyCreateInProgress=false;
    checkRoom(1);
    s_testActors.insert(42);
    assert(!enemy_spawner_process_active());
    before_process(nullptr,processArgs,nullptr,nullptr);
    assert(enemy_spawner_process_active());
    checkRoom(0); // fopAc_Create must not discard the enemy group.
    std::any skippedArgs[]={process_method_func(nullptr),static_cast<void*>(&nativeGate)};
    after_process(nullptr,skippedArgs,nullptr,nullptr); // another mod skipped its pre
    checkRoom(0);
    before_process(nullptr,processArgs,nullptr,nullptr); // nested profile creation
    checkRoom(0);
    processArgs[1]=static_cast<void*>(&nativeGate);
    before_process(nullptr,processArgs,nullptr,nullptr);
    checkRoom(1); // a nested gate still sees the open/cleared room
    after_process(nullptr,processArgs,nullptr,nullptr);
    checkRoom(0);
    processArgs[1]=static_cast<void*>(nullptr);
    before_process(nullptr,processArgs,nullptr,nullptr);
    checkRoom(1);
    after_process(nullptr,processArgs,nullptr,nullptr);
    after_process(nullptr,processArgs,nullptr,nullptr);
    checkRoom(0);
    after_process(nullptr,processArgs,nullptr,nullptr);
    checkRoom(1);
    assert(!enemy_spawner_process_active() && s_testProcessStack.empty());
    // Scope still works for an initialized actor during execution/deletion.
    process_method_class methods{nullptr, +[](void*){return 1;}, +[](void*){return 1;}};
    fopAc_ac_c initializedEnemy; initializedEnemy.id=42;
    initializedEnemy.initialized=true; initializedEnemy.sub_method=&methods;
    processArgs[0]=methods.execute_method; processArgs[1]=static_cast<void*>(&initializedEnemy);
    before_process(nullptr,processArgs,nullptr,nullptr); checkRoom(0);
    after_process(nullptr,processArgs,nullptr,nullptr); checkRoom(1);
    processArgs[0]=methods.delete_method;
    before_process(nullptr,processArgs,nullptr,nullptr);
    assert(!s_testActors.contains(42));
    after_process(nullptr,processArgs,nullptr,nullptr);
    assert(!enemy_spawner_process_active());
    for (int profile=0;profile<=testDoorProfile;++profile) {
        dStage_objectNameInf info{profile}; auto* result=&info;
        on_bossrush_area_actor_name_post(nullptr,nullptr,&result,nullptr);
        assert((result!=nullptr)==(profile==testDoorProfile));
    }
    for (const char* stage : {"D_DLBR0","D_SB01","D_MN09C"}) {
        reset(stage,51,0);
        std::any args[]={nullptr,3,51}; int result=0;
        assert(on_bossrush_area_dungeon_bit_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
        assert(on_bossrush_area_switch_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
        dStage_objectNameInf info{fpcNm_B_TN_e}; auto* actor=&info;
        on_bossrush_area_actor_name_post(nullptr,nullptr,&actor,nullptr);
        assert(actor==&info);
    }
    reset("D_DLBR0",51,0); active=false;
    std::any args[]={nullptr,3,51}; int result=0;
    assert(on_bossrush_area_dungeon_bit_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
    assert(on_bossrush_area_switch_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
    std::cout << "Boss Rush Cave/arena route and isolation checks passed\n";
}
'''

with tempfile.TemporaryDirectory() as temporary:
    folder = Path(temporary)
    cpp = folder / "routes.cpp"
    binary = folder / "routes"
    cpp.write_text(stubs + functions + cases)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
