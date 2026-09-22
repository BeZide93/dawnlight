"""Run the actual lifecycle callbacks with a small fake game environment."""

from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/fierce_deity.cpp").read_text()


def function(name):
    start = source.rfind("\n", 0, source.index(f" {name}(")) + 1
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


fixture = r'''
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <initializer_list>
using u8 = uint8_t;
using fpc_ProcID = uint32_t;
constexpr fpc_ProcID fpcM_ERROR_PROCESS_ID_e = 0xffffffff;
constexpr u8 dItemNo_NONE_e = 0xff;
constexpr int fpcNm_ALINK_e = 1, fopAc_ENEMY_e = 2;
constexpr float kMeterGainPerAttack = 5.0f;
using Clock = std::chrono::steady_clock;
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
using process_method_func = int(*)(void*);
struct process_method_class { process_method_func execute_method; };
int vanillaExecute(void*) { return 1; }
int outerExecute(void*) { return 1; }
process_method_class playerMethods{vanillaExecute};
process_method_class outerMethods{outerExecute};
struct leafdraw_class { int name = fpcNm_ALINK_e; };
struct fopAc_ac_c : leafdraw_class { int group = fopAc_ENEMY_e; };
struct daAlink_c : fopAc_ac_c {
    fpc_ProcID id = 1;
    uint16_t setID = 0;
    const process_method_class* sub_method = &playerMethods;
    int mClothesChangeWaitTimer = 0;
    int reloadCalls = 0;
    void loadModelDVD() { ++reloadCalls; --mClothesChangeWaitTimer; }
    void setClothesChange(int) { mClothesChangeWaitTimer = 3; }
};
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
int spinUpdates = 0, drainUpdates = 0;
void update_spin_activation(daAlink_c*) { ++spinUpdates; }
void update_drain(daAlink_c*) { ++drainUpdates; }
void deactivate(daAlink_c*, bool);
bool enabled = true;
daAlink_c* currentLink = nullptr;
daAlink_c* daAlink_getAlinkActorClass() { return currentLink; }
fpc_ProcID fopAcM_GetID(daAlink_c* link) { return link->id; }
int fpcM_GetName(leafdraw_class* process) { return process->name; }
int fopAcM_GetGroup(fopAc_ac_c* actor) { return actor->group; }
bool fierce_deity_enabled() { return enabled; }
namespace mods {
template <class T> T arg(void* args, int index) {
    return reinterpret_cast<T>(static_cast<void**>(args)[index]);
}
}
u8 savedClothes = 7, equippedClothes = 7;
int equipmentWrites = 0;
void dComIfGs_setSelectEquipClothes(u8 value) { savedClothes = value; ++equipmentWrites; }
void dComIfGp_setSelectEquipClothes(u8 value) { equippedClothes = value; ++equipmentWrites; }
'''

state = source[source.index("enum class ModelSwapState"):
               source.index("SaveObserverHandle s_saveObserver")]
callbacks = "".join(function(name) for name in (
    "is_sword_attack", "restore_equipment_selection", "reset_for_link", "same_link",
    "on_save_started", "before_player_delete", "after_damage_check",
    "service_model_swap", "dispatched_player", "before_player_execute", "after_player_execute",
))

checks = r'''
void deactivate(daAlink_c*, bool) { s_state.meter = 0; s_state.active = false; }
void* dispatch_target(const process_method_class& methods) {
#ifdef __APPLE__
    return reinterpret_cast<void*>(methods.execute_method);
#else
    return const_cast<process_method_class*>(&methods);
#endif
}
int run_dispatch(void* target, void* actor) {
    void* args[] = {target, actor};
    int result = 0;
    bool skipped = before_player_execute(nullptr, args, &result, nullptr) == HOOK_SKIP_ORIGINAL;
    after_player_execute(nullptr, args, &result, nullptr);
    if (skipped) assert(result == 1);
    return skipped;
}

int main() {
    daAlink_c link;
    currentLink = &link;
    fopAc_ac_c enemy;
    Collider collider;
    dCcU_AtInfo attack{&link, &collider};
    void* hit[] = {&enemy, &attack};
    void* deletion[] = {static_cast<leafdraw_class*>(&link)};

    // Save loaded, owner absent, then actual actor dispatch
    // without ever calling daAlink_c::execute's entry hook. Only the innermost
    // Link execute dispatch may bind the owner or advance the state machine.
    on_save_started(nullptr, 0, nullptr);
    assert(s_state.link == nullptr);
    void* target = dispatch_target(playerMethods);
    void* outer = dispatch_target(outerMethods);
    assert(run_dispatch(target, &enemy) == 0);
    assert(run_dispatch(outer, &link) == 0);
    assert(s_state.link == nullptr && spinUpdates == 0 && drainUpdates == 0);
    link.sub_method = nullptr;
    assert(run_dispatch(target, &link) == 0 && s_state.link == nullptr);
    link.sub_method = &playerMethods;
    currentLink = nullptr;
    assert(run_dispatch(target, &link) == 0 && s_state.link == nullptr);
    currentLink = &link;
    assert(run_dispatch(target, &link) == 0);
    assert(same_link(&link) && spinUpdates == 1 && drainUpdates == 1);
    for (int i = 0; i < 6; ++i) after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 30.0f); // six valid sword hits
    assert(run_dispatch(outer, &link) == 0);
    assert(spinUpdates == 1 && drainUpdates == 1); // no duplicate post tick

    // Critical model frames must still skip the actual player callback, even
    // if its member-function hook is bypassed. Exercise the real swap service.
    s_state.modelSwapState = ModelSwapState::Activating;
    link.mClothesChangeWaitTimer = 3;
    for (int remaining = 2; remaining >= 0; --remaining) {
        assert(run_dispatch(outer, &link) == 0);
        assert(run_dispatch(target, &enemy) == 0);
        assert(run_dispatch(target, &link) == 1);
        assert(link.mClothesChangeWaitTimer == remaining);
        assert(spinUpdates == 1);
    }
    assert(link.reloadCalls == 3 && s_state.modelSwapped);
    assert(s_state.modelSwapState == ModelSwapState::None);
    assert(run_dispatch(target, &link) == 0);
    assert(!s_state.modelReloadFrame && spinUpdates == 2);
    on_save_started(nullptr, 0, nullptr);
    assert(run_dispatch(target, &link) == 0);
    assert(same_link(&link) && s_state.meter == 0);
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 5.0f);

    // Exercise the actual sword predicate. Ordon
    // and wooden swords use NORMAL only; the Master Sword adds MASTER.
    for (unsigned sword : {AT_TYPE_NORMAL_SWORD,
                           AT_TYPE_NORMAL_SWORD | AT_TYPE_MASTER_SWORD}) {
        reset_for_link(&link);
        collider.type = sword;
        for (int hitIndex = 0; hitIndex < 25; ++hitIndex) {
            after_damage_check(nullptr, hit, nullptr, nullptr);
            assert(s_state.meter == std::min(100.0f, (hitIndex + 1) * 5.0f));
        }
    }
    reset_for_link(&link);
    collider.type = 0x20; // non-sword attacks do not charge
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 0);
    collider.type = AT_TYPE_NORMAL_SWORD;
    attack.mHitType = 16; // stun classification is deliberately rejected
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 0);
    attack.mHitType = HIT_TYPE_LINK_NORMAL_ATTACK;
    attack.mpCollider = nullptr;
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 0);
    attack.mpCollider = &collider;
    enabled = false;
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 0);
    enabled = true;
    s_state.active = true;
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 0);
    s_state.active = false;
    enemy.group = 0;
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 0);
    enemy.group = fopAc_ENEMY_e;
    daAlink_c otherLink;
    attack.mpActor = &otherLink;
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 0);
    attack.mpActor = &link;

    // Both the address and stage placement ID can survive a new player spawn.
    reset_for_link(&link);
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 5.0f);
    ++link.id;
    assert(!same_link(&link));
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 5.0f); // do not charge the departing player
    reset_for_link(&link);
    after_damage_check(nullptr, hit, nullptr, nullptr);
    assert(s_state.meter == 5.0f);
    ++link.setID;
    assert(same_link(&link)); // placement ID is not the actor identity

    // Save notifications also occur for same-slot reloads and new games. They
    // must clear all pending model phases without writing into the loaded save.
    for (auto phase : {ModelSwapState::None, ModelSwapState::Activating,
                       ModelSwapState::RestoreRequested, ModelSwapState::Restoring}) {
        for (uint32_t slot : {0, 0, 1, 2}) {
            s_state.meter = 85;
            s_state.active = true;
            s_state.spinChargeArmed = true;
            s_state.equipmentOverridden = true;
            s_state.originalClothes = 3;
            s_state.modelSwapped = true;
            s_state.modelReloadFrame = true;
            s_state.modelSwapState = phase;
            s_state.lastDrainTime = Clock::now();
            savedClothes = equippedClothes = 7;
            equipmentWrites = 0;
            on_save_started(nullptr, slot, nullptr);
            assert(!same_link(&link) && s_state.link == nullptr);
            assert(s_state.meter == 0 && !s_state.active && !s_state.spinChargeArmed);
            assert(!s_state.modelSwapped && !s_state.modelReloadFrame);
            assert(s_state.modelSwapState == ModelSwapState::None);
            assert(!s_state.equipmentOverridden && s_state.originalClothes == dItemNo_NONE_e);
            assert(s_state.lastDrainTime == Clock::time_point{});
            assert(equipmentWrites == 0 && savedClothes == 7 && equippedClothes == 7);
            // A late delete of the old actor must not restore its old outfit.
            before_player_delete(nullptr, deletion, nullptr, nullptr);
            assert(equipmentWrites == 0);
            assert(run_dispatch(target, &link) == 0);
            after_damage_check(nullptr, hit, nullptr, nullptr);
            assert(s_state.meter == 5.0f); // new session can charge immediately
        }
    }

    // Normal player teardown restores a temporary outfit before discarding it.
    s_state.equipmentOverridden = true;
    s_state.originalClothes = 3;
    leafdraw_class unrelated;
    unrelated.name = 99;
    void* otherDeletion[] = {&unrelated};
    before_player_delete(nullptr, otherDeletion, nullptr, nullptr);
    assert(s_state.meter == 5.0f && equipmentWrites == 0);
    before_player_delete(nullptr, deletion, nullptr, nullptr);
    assert(equipmentWrites == 2 && savedClothes == 3 && equippedClothes == 3);
    assert(s_state.meter == 0 && s_state.link == nullptr);
    before_player_delete(nullptr, deletion, nullptr, nullptr);
    assert(equipmentWrites == 2); // repeated teardown is harmless
}
'''

with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "fierce_lifecycle.cpp"
    exe = Path(tmp) / "fierce_lifecycle"
    cpp.write_text(fixture + state + callbacks + checks)
    for defines in ([], ["-D__APPLE__"]):
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        *defines, str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
print("Fierce Deity dispatch (Execute + Apple Method), model reload, save lifecycle and charging: passed")
