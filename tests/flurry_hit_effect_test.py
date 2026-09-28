"""Exercise production contact/end hooks with a fixed-damage enemy callback."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/bullet_time.cpp').read_text()


def function(signature):
    start = source.index(signature + '(')
    return source[start:source.index('\n}', start) + 2]


fixture = r'''
#include <algorithm>
#include <cassert>
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
struct fopAc_ac_c { bool enemy; };
struct daAlink_c : fopAc_ac_c {
    static bool checkEnemyGroup(fopAc_ac_c* a) { return a->enemy; }
    void setBStatus(int) {}
};
struct cCcD_Obj {
    fopAc_ac_c* actor;
    int power = 1;
    bool sword = true;
    fopAc_ac_c* GetAc() { return actor; }
    int GetAtAtp() { return power; }
};
struct SlowActorClockEntry { int pendingTicks = 0; } clock;
fopAc_ac_c* scheduledActor = nullptr;
SlowActorClockEntry* find_actor_clock(fopAc_ac_c* a, bool) {
    scheduledActor = a;
    return &clock;
}
namespace mods {
template<class T> T arg(void* args, int i) { return static_cast<T>(static_cast<void**>(args)[i]); }
}
bool is_flurry_sword_collider(cCcD_Obj* a) { return a->sword; }
bool s_flurryRushActive = true, s_flurryMeleePositioned = true, s_flurryLinkSlowed = false;
daAlink_c* s_flurryRushOwner = nullptr;
fopAc_ac_c* s_flurryRushTarget = nullptr;
constexpr int kFlurryRushActionStatus = 1, BUTTON_STATUS_NONE = 0;
int dComIfGp_getAStatus() { return 0; }
void sync_slow_motion_controllers() {}
bool combat_slow_active() { return s_flurryRushActive; }
void clear_combat_time_caches() { clock = {}; }
// FUNCTIONS
int main() {
    daAlink_c link; link.enemy = false;
    fopAc_ac_c enemy{true}, other{true};
    cCcD_Obj sword{&link}, target{&enemy};
    s_flurryRushOwner = &link; s_flurryRushTarget = &enemy;
    void* args[]{nullptr, &sword, &target};
    int hp = 20, effects = 0, callbackCalls = 0;
    bool blocked = false;
    auto contact = [&] {
        auto action = before_common_at_tg_hit(nullptr, args, nullptr, nullptr);
        // Stand-in for an enemy that consumes each accepted native hit in its
        // callback, without ever calling cc_at_check or at_power_check.
        if (action == HOOK_CONTINUE && !blocked) {
            hp -= 2; ++effects; ++callbackCalls;
        }
        return action;
    };
    for (int i = 0; i < 4; ++i) {
        clock.pendingTicks = 0;
        sword.power = (i + 1) * 10;
        assert(contact() == HOOK_CONTINUE);
        assert(hp == 18 - 2 * i && effects == i + 1 && callbackCalls == i + 1);
        assert(clock.pendingTicks == 1 && scheduledActor == &enemy);
    }
    // The observer leaves native blocking and damage amounts alone.
    blocked = true; clock.pendingTicks = 3;
    assert(contact() == HOOK_CONTINUE && hp == 12 && effects == 4);
    assert(clock.pendingTicks == 3); // Don't discard an already due update.
    // Unrelated contacts must not accelerate an actor or be swallowed.
    auto unchanged = [&] {
        scheduledActor = nullptr; clock.pendingTicks = 0;
        assert(before_common_at_tg_hit(nullptr, args, nullptr, nullptr) == HOOK_CONTINUE);
        assert(scheduledActor == nullptr && clock.pendingTicks == 0);
    };
    target.actor = &other; unchanged(); target.actor = &enemy;
    sword.sword = false; unchanged(); sword.sword = true;
    sword.power = 0; unchanged(); sword.power = 1;
    sword.actor = &other; unchanged(); sword.actor = &link;
    args[1] = nullptr; unchanged(); args[1] = &sword;
    args[2] = nullptr; unchanged(); args[2] = &target;
    // Ending/cancelling the rush cannot dispatch another collision or effect.
    stop_flurry_rush();
    assert(!s_flurryRushActive && !s_flurryRushOwner && !s_flurryRushTarget);
    assert(hp == 12 && effects == 4 && callbackCalls == 4);
    unchanged(); stop_flurry_rush();
}
'''
fixture = fixture.replace('// FUNCTIONS', function('HookAction before_common_at_tg_hit') + '\n' +
                          function('void stop_flurry_rush'))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Direct Flurry hits passed: fixed-damage callbacks, immediate per-hit effects, '
      'native blocking, pending actor updates and no end-of-rush hit')
