"""Verify the production Stalhound hooks isolate and restore the native clock."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/cave_randomizer.cpp").read_text()

def function(name):
    start = source.index(name + "(")
    start = source.rfind("\n", 0, start) + 1
    body = source.index("{", start)
    depth = 0
    for end in range(body, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError(name)

stubs = r'''
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using fpc_ProcID = unsigned;
struct ModContext {};
enum HookAction { HOOK_CONTINUE };
using process_method_func = int (*)(void*);
int execute(void*) { return 1; }
int other_method(void*) { return 1; }
struct process_method_class { process_method_func execute_method = execute; } methods;
constexpr int fpcNm_E_SH_e = 42;
struct fopAc_ac_c {
    unsigned id;
    int name = fpcNm_E_SH_e;
    const process_method_class* sub_method = &methods;
    bool isActor = true;
};
using Actor = fopAc_ac_c;
bool fopAcM_IsActor(void* process) { return static_cast<Actor*>(process)->isActor; }
int fopAcM_GetName(void* actor) { return static_cast<Actor*>(actor)->name; }
struct Environment { float daytime; } environment;
Environment* dKy_getEnvlight() { return &environment; }
bool cave = true;
bool in_cave() { return cave; }
unsigned fopAcM_GetID(const Actor* actor) { return actor->id; }
std::unordered_map<unsigned, Actor*> actors;
Actor* fopAcM_SearchByID(unsigned id) {
    auto it = actors.find(id);
    return it == actors.end() ? nullptr : it->second;
}
std::unordered_set<unsigned> creating, s_stalhounds;
bool fpcM_IsCreating(unsigned id) { return creating.contains(id); }
std::unordered_map<unsigned, int> s_pending;
namespace mods {
template<typename T> T arg(void* args, int n) {
    return *static_cast<T*>(static_cast<void**>(args)[n]);
}
}
'''
frame = source[source.index("struct StalhoundTimeFrame {"):source.index("bool in_cave() {")]
tests = r'''
int main() {
    Actor replacement{1}, native{2};
    void* owned = &replacement;
    void* other = &native;
    process_method_func method = execute;
    void* ownedArgs[] = {&method, &owned};
    void* otherArgs[] = {&method, &other};
    s_stalhounds.insert(1);
    actors[1] = &replacement;

    for (unsigned hour = 0; hour < 24; ++hour) {
        const float realTime = hour * 15.0f + 0.25f;
        environment.daytime = realTime;
        before_stalhound_process(nullptr, otherArgs, nullptr, nullptr);
        assert(environment.daytime == realTime && s_stalhoundTimeFrames.empty());
        after_stalhound_process(nullptr, otherArgs, nullptr, nullptr);

        before_stalhound_process(nullptr, ownedArgs, nullptr, nullptr);
        assert(environment.daytime == 0); // both native day checks see night
        // Post without a matching pre must not pop the outer actor's frame.
        after_stalhound_process(nullptr, otherArgs, nullptr, nullptr);
        assert(environment.daytime == 0 && s_stalhoundTimeFrames.size() == 1);
        // A nested native Stalhound sees real time and restores the outer scope.
        before_stalhound_process(nullptr, otherArgs, nullptr, nullptr);
        assert(environment.daytime == realTime);
        after_stalhound_process(nullptr, otherArgs, nullptr, nullptr);
        assert(environment.daytime == 0);
        after_stalhound_process(nullptr, ownedArgs, nullptr, nullptr);
        assert(environment.daytime == realTime && s_stalhoundTimeFrames.empty());
    }

    // Do not treat the outer actor wrapper, creation/deletion, other profiles,
    // incomplete actor setup or non-actor processes as Stalhound execution.
    environment.daytime = 180;
    for (int scenario = 0; scenario < 5; ++scenario) {
        method = scenario == 0 ? other_method : execute;
        replacement.name = scenario == 1 ? 7 : fpcNm_E_SH_e;
        replacement.sub_method = scenario == 2 ? nullptr : &methods;
        replacement.isActor = scenario != 3;
        owned = scenario == 4 ? nullptr : &replacement;
        before_stalhound_process(nullptr, ownedArgs, nullptr, nullptr);
        assert(environment.daytime == 180 && s_stalhoundTimeFrames.empty());
        after_stalhound_process(nullptr, ownedArgs, nullptr, nullptr);
    }
    method = execute; replacement.name = fpcNm_E_SH_e;
    replacement.sub_method = &methods; replacement.isActor = true; owned = &replacement;

    // Failed/pending creation, actor deletion, room unload and stage exit.
    s_stalhounds.insert(3); creating.insert(3);
    update_cave_randomizer();
    assert(s_stalhounds.contains(1) && s_stalhounds.contains(3));
    creating.clear(); actors.clear();
    update_cave_randomizer();
    assert(s_stalhounds.empty());
    s_stalhounds.insert(1); actors[1] = &replacement;
    cave = false;
    environment.daytime = 180;
    before_stalhound_process(nullptr, ownedArgs, nullptr, nullptr);
    assert(environment.daytime == 180 && s_stalhoundTimeFrames.empty());
    update_cave_randomizer();
    assert(s_stalhounds.empty());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "stalhound.cpp"
    binary = Path(tmp) / "stalhound-test"
    cpp.write_text(stubs + frame + function("before_stalhound_process") +
                   function("after_stalhound_process") + function("update_cave_randomizer") + tests)
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Cave Stalhound clock isolation and lifecycle tests passed")
