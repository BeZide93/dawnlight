"""Exercise the production actor scopes and native chase acceleration without game assets."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/enemy_hard_mode_runtime.cpp').read_text()

def function(name):
    match = re.search(r'^\w[^\n]*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    start = match.start()
    pos = source.index('{', start)
    depth, end = 1, pos + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
using s16 = int16_t;
struct ModContext {};
enum HookAction { HOOK_CONTINUE };
namespace mods {
template<class T> T arg(void* p, int i) {return *static_cast<T*>(static_cast<void**>(p)[i]);}
template<class T> T& arg_ref(void* p, int i) {return *static_cast<T*>(static_cast<void**>(p)[i]);}
}
using process_method_func = void(*)();
void execute() {}
void draw() {}
struct process_method_class {process_method_func execute_method = execute;} methods;
struct fopAc_ac_c {
    int kind = 1;
    bool eligible = true;
    void* sub_method = &methods;
    float speedF = 0;
    struct {struct {s16 y = 0;} angle;} current;
    struct {s16 y = 0;} shape_angle;
};
bool fopAcM_IsActor(void* actor) {return actor != nullptr;}
struct EnemyHardModeStep {
    fopAc_ac_c* actor = nullptr;
    int profileName = -1, action = 0, subaction = 0;
    bool chaseSpeed = false, chaseCurrentYaw = false, chaseShapeYaw = false;
};
struct HardModeProfile {
    int name;
    bool (*eligible)(fopAc_ac_c*);
    void (*snapshot)(EnemyHardModeStep&);
    void (*install)();
};
bool eligible(fopAc_ac_c* a) {return a->eligible;}
void snapshot(EnemyHardModeStep& step) {
    step.chaseSpeed = step.chaseCurrentYaw = step.chaseShapeYaw = true;
}
void typed_install() {}
const HardModeProfile profiles[]{{1, eligible, snapshot, nullptr}, {2, eligible, snapshot, typed_install}};
const HardModeProfile* find_profile(fopAc_ac_c* a) {
    return a && a->kind >= 1 && a->kind <= 2 ? &profiles[a->kind - 1] : nullptr;
}
std::array<EnemyHardModeStep, 8> s_steps{};
size_t s_depth = 0;
std::array<bool, 64> s_processFrames{};
size_t s_processDepth = 0;
bool enabled = true;
int preparations = 0, finishes = 0;
bool enemy_hard_mode_applies(int) {return enabled;}
void prepare_enemy_hard_mode(EnemyHardModeStep&) {++preparations;}
void finish_enemy_hard_mode(EnemyHardModeStep&) {++finishes;}
float enemy_hard_mode_chase_scale(const EnemyHardModeStep&, const float*) {return 1.4f;}
float enemy_hard_mode_turn_scale(const EnemyHardModeStep&) {return 1.25f;}
namespace fire_toadpoli {
bool is_toadpoli_profile(int) {return false;}
void after_execute(EnemyHardModeStep&) {assert(false);}
}
// FUNCTIONS
void enter(fopAc_ac_c* actor, process_method_func method = execute) {
    void* process = actor;
    void* args[]{&method, &process};
    before_process_method(nullptr, args, nullptr, nullptr);
}
void leave() {after_process_method(nullptr, nullptr, nullptr, nullptr);}
int main() {
    fopAc_ac_c parent, foreign, typed;
    foreign.kind = 99; typed.kind = 2;
    enter(&parent);
    assert(current_enemy_hard_mode_step()->actor == &parent && preparations == 1);
    enter(&foreign); assert(!current_enemy_hard_mode_step()); leave();
    assert(current_enemy_hard_mode_step()->actor == &parent && finishes == 0);
    enter(&parent, draw); assert(current_enemy_hard_mode_step()->actor == &parent); leave();
    assert(preparations == 1 && finishes == 0);
    enter(&typed); assert(!current_enemy_hard_mode_step());
    fopAc_ac_c* typedActor = &typed;
    void* typedArgs[]{&typedActor};
    before_enemy_hard_mode_execute(nullptr, typedArgs, nullptr, nullptr);
    assert(current_enemy_hard_mode_step()->actor == &typed && preparations == 2);
    after_enemy_hard_mode_execute(nullptr, nullptr, nullptr, nullptr);
    assert(!current_enemy_hard_mode_step()); leave();
    assert(current_enemy_hard_mode_step()->actor == &parent && finishes == 1);
    // Native acceleration only changes the active actor's registered fields.
    float unrelated = 0, target = 1, rate = 2, maximum = 3;
    float* value = &unrelated;
    void* chase[]{&value, &target, &rate, &maximum};
    before_chase_target(nullptr, chase, nullptr, nullptr);
    assert(rate == 2 && maximum == 3);
    value = &parent.speedF;
    before_chase_target(nullptr, chase, nullptr, nullptr);
    assert(std::abs(rate - 2.8f) < .001f && std::abs(maximum - 4.2f) < .001f);
    current_enemy_hard_mode_step()->chaseSpeed = false;
    rate = 2; maximum = 3;
    before_chase_target(nullptr, chase, nullptr, nullptr);
    assert(rate == 2 && maximum == 3);
    assert(owns_chase_angle(current_enemy_hard_mode_step(), &parent.current.angle.y));
    assert(!owns_chase_angle(current_enemy_hard_mode_step(), &foreign.current.angle.y));
    leave(); assert(!current_enemy_hard_mode_step() && finishes == 2);
    enabled = false; enter(&parent); assert(!current_enemy_hard_mode_step()); leave();
    enabled = true; parent.eligible = false;
    enter(&parent); assert(!current_enemy_hard_mode_step()); leave();
    assert(preparations == 2 && finishes == 2);
    // Overflow suppresses child scopes, then recovers the original parent.
    parent.eligible = true; enter(&parent);
    for (int i = 0; i < 12; ++i) enter(&foreign);
    assert(!current_enemy_hard_mode_step());
    for (int i = 0; i < 12; ++i) leave();
    assert(current_enemy_hard_mode_step()->actor == &parent);
    leave(); assert(s_depth == 0 && s_processDepth == 0 && preparations == finishes);
}
'''
names = ['current_enemy_hard_mode_step', 'before_enemy_hard_mode_execute',
         'after_enemy_hard_mode_execute', 'before_process_method', 'after_process_method',
         'owns_chase_float', 'owns_chase_angle', 'before_chase_target']
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'runtime.cpp', Path(tmp) / 'runtime'
    cpp.write_text(fixture.replace('// FUNCTIONS', '\n'.join(function(n) for n in names)))
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Enemy Hard Mode runtime passed: native/typed dispatch, nested isolation, overflow, eligibility and chase ownership')
