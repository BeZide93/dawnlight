"""Run production stamina accounting with a deterministic clock and game fixture."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/stamina.cpp').read_text()

def function(name):
    start = re.search(r'^\w[^\n]*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M).start()
    return source[start:source.index('\n}', start) + 2]

state = source[source.index('constexpr float kMaximumStamina'):source.index('bool s_lazyTweaksDetected')]
state = state.replace('using Clock = std::chrono::steady_clock;', '')
fixture = r'''
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
using u16 = unsigned short;
struct Clock {
    using time_point = std::chrono::steady_clock::time_point;
    static inline time_point value{std::chrono::seconds(1)};
    static time_point now() { return value; }
};
struct daAlink_c {
    enum { PROC_WAIT, PROC_TIRED_WAIT, PROC_WOLF_TIRED_WAIT };
    u16 setID = 1;
    int mProcID = PROC_WAIT;
    bool dead = false, scene = false;
    bool checkDeadHP() { return dead; }
    bool checkSceneChangeAreaStart() { return scene; }
    void procWaitInit() { mProcID = PROC_WAIT; }
    void procWolfWaitInit() { mProcID = PROC_WAIT; }
} link;
daAlink_c* current = &link;
daAlink_c* daAlink_getAlinkActorClass() { return current; }
bool enabled = true, glide = true, paused = false;
bool stamina_enabled() { return enabled; }
bool glide_enabled() { return glide; }
bool bullet_time_enabled() { return false; }
bool flurry_rush_enabled() { return false; }
bool great_spin_projectile_enabled() { return false; }
bool sprint_enabled() { return false; }
bool menu_or_pause_active() { return paused; }
int dComIfGs_getLife() { return 20; }
// PRODUCTION
void near(float actual, float expected) { assert(std::fabs(actual - expected) < 0.002f); }
void step(double seconds, bool bullet = false) {
    Clock::value += std::chrono::duration_cast<Clock::time_point::duration>(std::chrono::duration<double>(seconds));
    update_stamina(bullet);
}
void reset() {
    link = {}; current = &link; enabled = glide = true; paused = false;
    reset_for_link(&link);
}
int main() {
    // A single attachment must remain active across all render updates.
    for (int fps : {30, 60, 120}) {
        reset(); assert(stamina_meter_visible()); set_glide_stamina_active(true);
        for (int i = 0; i < fps * 2; ++i) step(1.0 / fps);
        near(s_state.stamina, 90);
        set_glide_stamina_active(false);
        for (int i = 0; i < fps; ++i) step(1.0 / fps);
        near(s_state.stamina, 95);
    }
    reset(); set_glide_stamina_active(true);
    for (double seconds : {0.1, 0.2, 0.05, 0.25, 0.15, 0.25}) step(seconds);
    near(s_state.stamina, 95);
    // Pending deployment does not drain; pause does not drain or catch up.
    reset(); s_state.stamina = 80; step(0.2); near(s_state.stamina, 81);
    set_glide_stamina_active(true); paused = true; step(5); near(s_state.stamina, 81);
    paused = false; step(0.2); near(s_state.stamina, 80);
    // Stamina disabled means free gliding; turning it on resumes drain.
    enabled = false; step(0.2); near(s_state.stamina, 100);
    assert(stamina_available_for_glide() && !stamina_meter_visible());
    enabled = true; step(0.2); near(s_state.stamina, 99);
    glide = false; step(0.2); near(s_state.stamina, 100); assert(!stamina_meter_visible());
    // Glide adds its own cost if bullet time overlaps; sprint cost stays intact.
    reset(); set_glide_stamina_active(true); step(0.2, true); near(s_state.stamina, 95);
    reset(); mark_sprint_stamina_active(); step(0.2); near(s_state.stamina, 99);
    // Empty stamina gates redeployment until normal exhaustion recovery completes.
    reset(); s_state.stamina = 0.5f; set_glide_stamina_active(true); step(0.2);
    near(s_state.stamina, 0); assert(s_state.exhausted && !stamina_available_for_glide());
    set_glide_stamina_active(false);
    for (int i = 0; i < 39; ++i) step(0.25);
    assert(!stamina_available_for_glide()); step(0.25);
    near(s_state.stamina, 50); assert(stamina_available_for_glide());
    // New Link/death/scene reset cannot inherit an active glide drain.
    for (int reason : {0, 1, 2}) {
        reset(); set_glide_stamina_active(true);
        if (reason == 0) ++link.setID;
        if (reason == 1) link.dead = true;
        if (reason == 2) link.scene = true;
        step(0.2); near(s_state.stamina, 100); assert(!s_state.glideActive);
    }
}
'''
production = state + '\n' + '\n'.join(function(name) for name in (
    'same_link', 'reset_for_link', 'current_link', 'can_consume',
    'stamina_meter_visible', 'stamina_available_for_glide',
    'set_glide_stamina_active', 'mark_sprint_stamina_active', 'update_stamina'))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture.replace('// PRODUCTION', production))
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Glide stamina passed: 5/s across frame rates, pause, toggles, exhaustion, recovery and lifecycle')
