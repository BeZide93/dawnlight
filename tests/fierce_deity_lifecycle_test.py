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
enum HookAction { HOOK_CONTINUE };
struct leafdraw_class { int name = fpcNm_ALINK_e; };
struct fopAc_ac_c : leafdraw_class { int group = fopAc_ENEMY_e; };
struct daAlink_c : fopAc_ac_c { fpc_ProcID id = 1; uint16_t setID = 0; };
struct dCcU_AtInfo { daAlink_c* mpActor; bool sword = true; };
daAlink_c* currentLink = nullptr;
daAlink_c* daAlink_getAlinkActorClass() { return currentLink; }
fpc_ProcID fopAcM_GetID(daAlink_c* link) { return link->id; }
int fpcM_GetName(leafdraw_class* process) { return process->name; }
int fopAcM_GetGroup(fopAc_ac_c* actor) { return actor->group; }
bool fierce_deity_enabled() { return true; }
bool is_sword_attack(const dCcU_AtInfo* attack) { return attack && attack->sword; }
namespace mods {
template <class T> T arg(void* args, int index) {
    return static_cast<T>(static_cast<void**>(args)[index]);
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
    "restore_equipment_selection", "reset_for_link", "same_link",
    "on_save_started", "before_player_delete", "after_damage_check",
))

checks = r'''
int main() {
    daAlink_c link;
    currentLink = &link;
    fopAc_ac_c enemy;
    dCcU_AtInfo attack{&link};
    void* hit[] = {&enemy, &attack};
    void* deletion[] = {static_cast<leafdraw_class*>(&link)};

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
            reset_for_link(&link);
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
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Fierce Deity save reload, actor reuse, charging and equipment isolation: passed")
