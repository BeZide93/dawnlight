"""Exercise real lifecycle callbacks with asynchronous archives and native timer phases.

The fake native loader retires the old model at timer 2 and creates an unposed
replacement at completion. Each simulated draw requires a live, posed model;
movement is compared with an uninterrupted jumping/falling player. This is a
lifecycle regression harness, not a full game/renderer simulation.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/fierce_deity.cpp").read_text()


def function(name):
    start = source.rfind("\n", 0, source.index(f" {name}(")) + 1
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


fixture = r'''
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
using u8 = uint8_t;
using u32 = uint32_t;
using fpc_ProcID = uint32_t;
constexpr fpc_ProcID fpcM_ERROR_PROCESS_ID_e = 0xffffffff;
constexpr u8 dItemNo_NONE_e = 0xff, dItemNo_ARMOR_e = 0x31;
constexpr u8 dItemNo_WEAR_CASUAL_e = 0x2e, dItemNo_WEAR_ZORA_e = 0x30;
constexpr u8 tunic = 0x2f;
using BOOL = int;
constexpr BOOL FALSE = 0;
using Clock = std::chrono::steady_clock;
constexpr float kMeterGainPerAttack = 5.0f;
enum class FierceDeityVisual : int { MagicArmor, Dark, DarkMagic };
FierceDeityVisual selectedVisual = FierceDeityVisual::MagicArmor;
FierceDeityVisual fierce_deity_visual() { return selectedVisual; }
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
using process_method_func = int(*)(void*);
struct process_method_class { process_method_func execute_method; };
int vanillaExecute(void*) { return 1; }
int outerExecute(void*) { return 1; }
process_method_class playerMethods{vanillaExecute}, outerMethods{outerExecute};
constexpr int fpcNm_ALINK_e = 1, fopAc_ENEMY_e = 2;
struct leafdraw_class { int name = fpcNm_ALINK_e; };
struct fopAc_ac_c : leafdraw_class { int group = fopAc_ENEMY_e; };
struct daPy_py_c { enum { FLG2_UNK_280000 = 0x280000 }; };
int frame = 0, loadDelay = 4, loads = 0, equipmentWrites = 0;
bool failStart = false, failSync = false, enabled = true, paused = false;
bool interfereWithNativeLoad = false;
u8 savedClothes = tunic, equippedClothes = tunic;
struct Resource { int refs = 0, readyAt = 0; bool failed = false; };
std::map<std::string, Resource> resources;
const char* outfit_archive(u8);
int dComIfG_setObjectRes(const char* name, u8, void* heap) {
    assert(heap == nullptr); // must not preload into the live player's heap
    ++loads;
    if (failStart) return 0;
    auto& r = resources[name];
    if (r.refs++ == 0) { r.readyAt = frame + loadDelay; r.failed = failSync; }
    return 1;
}
int dComIfG_syncObjectRes(const char* name) {
    auto& r = resources.at(name);
    assert(r.refs > 0);
    return frame < r.readyAt ? 1 : (r.failed ? -1 : 0);
}
int dComIfG_deleteObjectResMain(const char* name) {
    auto& r = resources.at(name);
    assert(r.refs > 0 && frame >= r.readyAt); // no destruction of in-flight I/O
    --r.refs;
    return 1;
}
u8 dComIfGs_getSelectEquipClothes() { return savedClothes; }
void dComIfGs_setSelectEquipClothes(u8 value) { savedClothes = value; ++equipmentWrites; }
void dComIfGp_setSelectEquipClothes(u8 value) { equippedClothes = value; ++equipmentWrites; }
struct daAlink_c : fopAc_ac_c {
    fpc_ProcID id = 1;
    const process_method_class* sub_method = &playerMethods;
    const char* mArcName = "Kmdl";
    int mClothesChangeWaitTimer = 0, reloadCalls = 0, replacements = 0, updates = 0;
    bool wolf = false, dead = false, sceneChange = false, event = false, special = false;
    bool riding = false, modelAlive = true, posed = true;
    int x = 1000, y = 800, vy = 15, modelX = 1000, modelY = 800;
    bool checkWolf() const { return wolf; }
    bool checkDeadHP() const { return dead; }
    bool checkSceneChangeAreaStart() const { return sceneChange; }
    bool checkEventRun() const { return event; }
    bool checkHorseRide() const { return riding; }
    bool checkCanoeRide() const { return false; }
    bool checkBoardRide() const { return false; }
    bool checkSpinnerRide() const { return false; }
    bool checkNoResetFlg2(int) const { return special; }
    int getSumouMode() const { return 0; }
    void setClothesChange(int) { mClothesChangeWaitTimer = 4; }
    void loadModelDVD() {
        if (mClothesChangeWaitTimer == 0) return;
        ++reloadCalls;
        --mClothesChangeWaitTimer;
        if (mClothesChangeWaitTimer == 2) {
            dComIfG_deleteObjectResMain(mArcName);
            modelAlive = posed = false;
            mArcName = outfit_archive(savedClothes);
        } else if (mClothesChangeWaitTimer == 1) {
            if (interfereWithNativeLoad) { mClothesChangeWaitTimer = 2; return; }
            auto& r = resources.at(mArcName);
            assert(r.refs > 0 && dComIfG_syncObjectRes(mArcName) == 0);
            ++r.refs; // native resLoad acquires Link's reference
            mClothesChangeWaitTimer = 0;
            modelAlive = true;
            posed = false;
            modelX = modelY = 0; // freshly allocated model has no world pose
            ++replacements;
        }
    }
    void execute() {
        loadModelDVD();
        assert(modelAlive && mClothesChangeWaitTimer == 0);
        ++updates;
        x += 3; y += vy; --vy;
        modelX = x; modelY = y; posed = true;
    }
    void draw() {
        assert(mClothesChangeWaitTimer == 0); // no invisible frame
        assert(modelAlive && posed && modelX == x && modelY == y);
    }
};
daAlink_c* currentLink = nullptr;
daAlink_c* daAlink_getAlinkActorClass() { return currentLink; }
fpc_ProcID fopAcM_GetID(daAlink_c* link) { return link->id; }
int fpcM_GetName(leafdraw_class* p) { return p->name; }
int fopAcM_GetGroup(fopAc_ac_c* p) { return p->group; }
bool fierce_deity_enabled() { return enabled; }
bool menu_or_pause_active() { return paused; }
namespace mods {
template <class T> T arg(void* args, int index) {
    return reinterpret_cast<T>(static_cast<void**>(args)[index]);
}
}
bool fierce_deity_transition_busy() { return false; }
void fierce_deity_transition_prepare(daAlink_c*, bool, bool, bool) {}
void fierce_deity_transition_commit(daAlink_c*) {}
void fierce_deity_transition_tick(daAlink_c*) {}
void fierce_deity_transition_cancel(daAlink_c*) {}
int spinUpdates = 0, drainUpdates = 0;
void update_spin_activation(daAlink_c*) { ++spinUpdates; }
void update_drain(daAlink_c*) { ++drainUpdates; }
void refresh_foot_baseline(daAlink_c*) {}
constexpr unsigned AT_TYPE_NORMAL_SWORD = 2, AT_TYPE_MASTER_SWORD = 0x04000000;
constexpr unsigned HIT_TYPE_LINK_NORMAL_ATTACK = 1;
struct Collider {
    unsigned type = AT_TYPE_NORMAL_SWORD;
    unsigned ChkAtType(unsigned mask) const { return type & mask; }
};
struct dCcU_AtInfo {
    daAlink_c* mpActor;
    Collider* mpCollider;
    unsigned mHitType = HIT_TYPE_LINK_NORMAL_ATTACK;
};
'''

state = source[source.index("enum class ModelSwapState"):
               source.index("SaveObserverHandle s_saveObserver")]
preload_state = source[source.index("struct OutfitPreload"):
                       source.index("bool same_archive")]
callbacks = "".join(function(name) for name in (
    "same_archive", "release_preload", "poll_preload", "cancel_preload",
    "outfit_archive", "prepare_outfit", "is_sword_attack", "restore_equipment_selection",
    "deactivate", "can_transform", "activate", "update_visual_selection",
    "reset_for_link", "same_link", "on_save_started", "before_player_delete",
    "after_damage_check", "service_model_swap", "before_magic_armor_ability",
    "dispatched_player", "before_player_execute", "after_player_execute",
    "fierce_deity_active", "fierce_deity_dark_visual_active", "fierce_deity_model_reload_active",
))

checks = r'''
void* dispatch_target(const process_method_class& methods) {
#ifdef __APPLE__
    return reinterpret_cast<void*>(methods.execute_method);
#else
    return const_cast<process_method_class*>(&methods);
#endif
}
HookAction dispatch(void* actor, const process_method_class& methods = playerMethods) {
    void* args[] = {dispatch_target(methods), actor};
    int result = 0;
    HookAction action = before_player_execute(nullptr, args, &result, nullptr);
    if (action == HOOK_SKIP_ORIGINAL) assert(result == 1);
    return action;
}
void tick(daAlink_c& link) {
    ++frame;
    int expectedX = link.x + 3, expectedY = link.y + link.vy;
    int updates = link.updates;
    assert(dispatch(&link) == HOOK_CONTINUE);
    assert(!fierce_deity_model_reload_active());
    link.execute();
    void* args[] = {dispatch_target(playerMethods), &link};
    after_player_execute(nullptr, args, nullptr, nullptr);
    link.draw();
    assert(link.x == expectedX && link.y == expectedY && link.updates == updates + 1);
    assert(savedClothes == equippedClothes && !s_state.equipmentOverridden);
}
void ticks(daAlink_c& link, int count = 10) { while (count--) tick(link); }
void start(daAlink_c& link, u8 clothes = tunic) {
    // Previous scenario must not leave in-flight I/O behind.
    assert(!s_preload.archive);
    resources.clear();
    s_state = {}; s_preload = {};
    link = {}; currentLink = &link;
    savedClothes = equippedClothes = clothes;
    link.mArcName = outfit_archive(clothes);
    resources[link.mArcName] = {1, frame, false};
    enabled = true; paused = failStart = failSync = false;
    interfereWithNativeLoad = false;
    selectedVisual = FierceDeityVisual::MagicArmor;
    loadDelay = 4; loads = equipmentWrites = spinUpdates = drainUpdates = 0;
    tick(link);
}
void finish(daAlink_c& link) {
    deactivate(&link, true);
    ticks(link);
    assert(!s_preload.archive && !s_state.modelSwapped && !s_state.active);
    assert(same_archive(link.mArcName, outfit_archive(savedClothes)));
    int refs = 0;
    for (auto& [name, r] : resources) refs += r.refs;
    assert(refs == 1); // only the live player's archive remains
}
int main() {
    daAlink_c link;
    start(link);
    fopAc_ac_c enemy;
    assert(dispatch(&enemy) == HOOK_CONTINUE);
    assert(dispatch(&link, outerMethods) == HOOK_CONTINUE);
    assert(spinUpdates == 1 && drainUpdates == 1);
    Collider collider;
    dCcU_AtInfo attack{&link, &collider};
    void* hit[] = {&enemy, &attack};
    for (int i = 0; i < 6; ++i) after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 30);
    attack.mHitType = 0; after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 30);

    // Every outfit / visual transition, in a trajectory that passes from ascent
    // to falling. Neither archive I/O nor the commit may consume a player tick.
    for (u8 outfit : {tunic, dItemNo_WEAR_CASUAL_e, dItemNo_WEAR_ZORA_e, dItemNo_ARMOR_e}) {
        for (auto from : {FierceDeityVisual::MagicArmor, FierceDeityVisual::Dark, FierceDeityVisual::DarkMagic}) {
            for (auto to : {FierceDeityVisual::MagicArmor, FierceDeityVisual::Dark, FierceDeityVisual::DarkMagic}) {
                start(link, outfit);
                selectedVisual = from; activate(&link);
                assert(savedClothes == outfit && link.mClothesChangeWaitTimer == 0);
                ticks(link);
                const char* expected = from == FierceDeityVisual::Dark ? outfit_archive(outfit) : "Mmdl";
                assert(same_archive(link.mArcName, expected));
                int before = link.replacements;
                selectedVisual = to; ticks(link);
                assert(savedClothes == outfit && s_state.meter == 100);
                expected = to == FierceDeityVisual::Dark ? outfit_archive(outfit) : "Mmdl";
                assert(same_archive(link.mArcName, expected));
                if (from != FierceDeityVisual::Dark && to != FierceDeityVisual::Dark)
                    assert(link.replacements == before);
                assert(fierce_deity_dark_visual_active() == (to != FierceDeityVisual::MagicArmor));
                BOOL result = 1;
                auto action = before_magic_armor_ability(nullptr, nullptr, &result, nullptr);
                assert((action == HOOK_CONTINUE) == (to == FierceDeityVisual::Dark));
                finish(link);
            }
        }
    }

    // Loading leaves the live outfit/save state intact; abort before completion.
    start(link); activate(&link); tick(link);
    assert(s_preload.archive && link.replacements == 0 && equipmentWrites == 0);
    selectedVisual = FierceDeityVisual::Dark; tick(link);
    assert(s_preload.archive && s_preload.cancelled);
    ticks(link);
    assert(!s_preload.archive && s_state.displayedDark);
    finish(link);

    // Disable in both loading and installed states, including a delayed restore.
    for (int delay : {1, 10}) {
        start(link); activate(&link); ticks(link, delay);
        enabled = false; ticks(link, 12);
        assert(same_archive(link.mArcName, "Kmdl") && !s_state.modelSwapped);
        finish(link);
    }

    // Retarget an in-progress restore back to armor, without destroying its I/O.
    start(link); activate(&link); ticks(link);
    selectedVisual = FierceDeityVisual::Dark; tick(link);
    assert(s_preload.archive);
    selectedVisual = FierceDeityVisual::DarkMagic; ticks(link);
    assert(same_archive(link.mArcName, "Mmdl") && !s_preload.archive);
    finish(link);

    // Restore whichever outfit is currently selected, including changes while
    // a previous restore archive was still loading.
    start(link); activate(&link); ticks(link);
    selectedVisual = FierceDeityVisual::Dark; tick(link);
    savedClothes = equippedClothes = dItemNo_WEAR_ZORA_e;
    ticks(link, 16);
    assert(same_archive(link.mArcName, "Zmdl"));
    finish(link);

    // Load failure must keep the existing model and suppress repeated retries.
    for (bool atRegistration : {false, true}) {
        start(link); failStart = atRegistration; failSync = !atRegistration;
        activate(&link); ticks(link);
        assert(link.replacements == 0 && loads == 1 && !s_preload.archive);
        failStart = failSync = false;
        deactivate(&link, true); activate(&link); ticks(link);
        assert(same_archive(link.mArcName, "Mmdl"));
        finish(link);
    }

    // Unsafe native contexts defer the cosmetic transaction without pausing Link.
    for (bool* flag : {&link.event, &link.special, &link.riding, &paused}) {
        start(link); activate(&link); *flag = true;
        ticks(link); assert(loads == 0 && link.replacements == 0);
        *flag = false; ticks(link); assert(link.replacements == 1);
        finish(link);
    }

    // Actor deletion and save notification may precede I/O completion. Drain the
    // old request without writing the old outfit into the new save or actor.
    start(link); activate(&link); tick(link);
    void* deletion[] = {static_cast<leafdraw_class*>(&link)};
    before_player_delete(nullptr, deletion, nullptr, nullptr);
    currentLink = nullptr;
    savedClothes = equippedClothes = dItemNo_WEAR_ZORA_e;
    int writes = equipmentWrites;
    on_save_started(nullptr, 0, nullptr);
    for (int i = 0; i < 8; ++i) { ++frame; dispatch(&enemy); }
    assert(!s_preload.archive && equipmentWrites == writes);
    // Simulate vanilla releasing the old player, then binding a new one.
    dComIfG_deleteObjectResMain(link.mArcName);
    link = {}; link.id = 2; link.mArcName = "Zmdl"; currentLink = &link;
    resources["Zmdl"] = {1, frame, false};
    tick(link);
    assert(same_link(&link) && !s_state.active && s_state.meter == 0);
    finish(link);

    // Defensive path for a third-party loader that unexpectedly remains busy:
    // never run execute on freed model memory; resume ON the completion frame.
    start(link); loadDelay = 0; activate(&link);
    interfereWithNativeLoad = true;
    assert(dispatch(&link) == HOOK_SKIP_ORIGINAL && !link.modelAlive);
    assert(fierce_deity_model_reload_active());
    interfereWithNativeLoad = false;
    tick(link);
    assert(link.modelAlive && !s_preload.archive);
    finish(link);
}
'''

if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="dawnlight-fierce-lifecycle-") as directory:
        cpp = Path(directory) / "test.cpp"
        cpp.write_text(fixture + state + preload_state + callbacks + checks)
        for platform in ([], ["-D__APPLE__"]):
            exe = Path(directory) / ("apple" if platform else "execute")
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra",
                            "-Werror", *platform, str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
    print("Fierce Deity seamless lifecycle tests passed (Execute and Apple Method dispatch)")
