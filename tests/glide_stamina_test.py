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

state = source[source.index('using Clock'):source.index('bool s_lazyTweaksDetected')]
state = state.replace('using Clock = std::chrono::steady_clock;', '')
fixture = r'''
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <vector>
#include "stamina_settings.hpp"
using namespace dawnlight;
struct ModContext {};
enum HookAction {HOOK_CONTINUE, HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
struct dCcD_GObjInf {bool shield=true;bool ChkTgShieldHit(){return shield;}};
std::array<int,kStaminaSettings.size()> settings{};
void defaults(){for(size_t i=0;i<settings.size();++i)settings[i]=kStaminaSettings[i].standard;}
int stamina_setting(StaminaSetting key){return settings[static_cast<size_t>(key)];}
void setting(StaminaSetting key,int value){settings[static_cast<size_t>(key)]=value;}
int maxLife=15;
bool progression=false;
int dComIfGs_getMaxLife(){return maxLife;}
bool progression_system_enabled(){return progression;}
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
    defaults();progression=false;maxLife=15;s_guardEvents.clear();
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
    glide = false; step(0.2); near(s_state.stamina, 100); assert(stamina_meter_visible());
    // Glide adds its own cost if bullet time overlaps; sprint cost stays intact.
    reset(); set_glide_stamina_active(true); step(0.2, true); near(s_state.stamina, 96);
    reset(); mark_sprint_stamina_active(); step(0.2); near(s_state.stamina, 99);
    // Empty stamina gates redeployment until normal exhaustion recovery completes.
    reset(); s_state.stamina = 0.5f; set_glide_stamina_active(true); step(0.2);
    near(s_state.stamina, 0); assert(s_state.exhausted && !stamina_available_for_glide());
    set_glide_stamina_active(false);
    for (int i = 0; i < 39; ++i) step(0.25);
    assert(!stamina_available_for_glide()); step(0.25);
    near(s_state.stamina, 50); assert(stamina_available_for_glide());

    // Maximum points are independent of costs and rendering percentages.
    reset(); setting(StaminaSetting::Amount, 500); reset_for_link(&link);
    assert(consume_flurry_rush_stamina()); near(s_state.stamina,450);
    assert(consume_great_spin_stamina()); near(s_state.stamina,410);
    setting(StaminaSetting::Amount,50);current_link();near(s_state.stamina,50);
    // Progression derives from complete max hearts, never current life or a global counter.
    reset();progression=true;
    for(int life=0;life<=100;++life) {
        maxLife=life;near(maximum_stamina(),100+std::max(0,life/5-3)*10);
    }
    maxLife=19;near(maximum_stamina(),100);maxLife=20;near(maximum_stamina(),110);
    maxLife=100;setting(StaminaSetting::Amount,500);near(maximum_stamina(),670);
    progression=false;near(maximum_stamina(),500);
    progression=true;maxLife=15;++link.setID;current_link();near(s_state.stamina,500);
    // Normal and exhausted recovery are separate; threshold scales with capacity.
    reset();setting(StaminaSetting::Amount,200);setting(StaminaSetting::Recovery,12);
    setting(StaminaSetting::ExhaustRecovery,40);setting(StaminaSetting::ExhaustThreshold,25);
    s_state.stamina=80;step(.25);near(s_state.stamina,83);
    s_state.stamina=0;s_state.exhausted=true;
    for(int i=0;i<4;++i)step(.25);
    near(s_state.stamina,40);assert(s_state.exhausted);step(.25);
    near(s_state.stamina,50);assert(!s_state.exhausted);
    for(int threshold:{0,100}) {
        reset();setting(StaminaSetting::ExhaustThreshold,threshold);setting(StaminaSetting::ExhaustRecovery,100);
        s_state.stamina=0;s_state.exhausted=true;step(.25);
        assert(s_state.exhausted==(threshold==100));
        if(threshold==100){for(int i=0;i<3;++i)step(.25);assert(!s_state.exhausted);}
    }
    // Zero means free, including while other actions have exhausted the meter.
    reset();s_state.stamina=0;s_state.exhausted=true;
    for(auto key:{StaminaSetting::Sprint,StaminaSetting::WolfSprint,StaminaSetting::Glide,
                 StaminaSetting::BulletTime,StaminaSetting::FlurryRush,StaminaSetting::GreatSpin,
                 StaminaSetting::ShieldAttack,StaminaSetting::BackSlice,StaminaSetting::HelmSplitter,
                 StaminaSetting::MidnaAttack})setting(key,0);
    assert(stamina_available_for_bullet_time()&&stamina_available_for_sprint()&&stamina_available_for_wolf_sprint());
    assert(stamina_available_for_glide()&&consume_flurry_rush_stamina()&&consume_great_spin_stamina());
    int success=1,failed=0;
    assert(before_guard_attack(nullptr,nullptr,&success,nullptr)==HOOK_CONTINUE);
    after_guard_attack(nullptr,nullptr,&success,nullptr);near(s_state.stamina,0);
    assert(before_midna_charge(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
    // Continuous costs are points per real second; wolf ticks charge once.
    reset();setting(StaminaSetting::WolfSprint,17);
    mark_wolf_sprint_stamina_active();mark_wolf_sprint_stamina_active();step(.2);near(s_state.stamina,96.6f);
    setting(StaminaSetting::Sprint,20);mark_sprint_stamina_active();step(.2);near(s_state.stamina,92.6f);
    setting(StaminaSetting::BulletTime,50);setting(StaminaSetting::Glide,20);
    set_glide_stamina_active(true);step(.2,true);near(s_state.stamina,78.6f);
    // Paid skills require enough points, charge once on successful init, and respect Off.
    reset();setting(StaminaSetting::ShieldAttack,33);
    after_guard_attack(nullptr,nullptr,&failed,nullptr);near(s_state.stamina,100);
    assert(before_guard_attack(nullptr,nullptr,&success,nullptr)==HOOK_CONTINUE);
    after_guard_attack(nullptr,nullptr,&success,nullptr);near(s_state.stamina,67);
    after_back_slice(nullptr,nullptr,&success,nullptr);near(s_state.stamina,47);
    after_helm_splitter(nullptr,nullptr,&success,nullptr);near(s_state.stamina,27);
    assert(before_guard_attack(nullptr,nullptr,&success,nullptr)==HOOK_SKIP_ORIGINAL&&success==0);
    assert(before_midna_charge(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    enabled=false;assert(before_guard_attack(nullptr,nullptr,&success,nullptr)==HOOK_CONTINUE);
    success=1;after_guard_attack(nullptr,nullptr,&success,nullptr);near(s_state.stamina,27);
    enabled=true;reset();assert(before_midna_charge(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
    near(s_state.stamina,50);
    // Block or Guard Break charges once; ordinary damage / armor SE never count.
    daAlink_c* owner=&link;dCcD_GObjInf hit,*hitPtr=&hit;
    void* damageArgs[]={&owner};void* hitArgs[]={&owner,&hitPtr};
    for(int kind=0;kind<4;++kind) {
        reset();before_damage(nullptr,damageArgs,nullptr,nullptr);
        if(kind!=0) {hit.shield=kind!=3;after_block(nullptr,hitArgs,nullptr,nullptr);}
        if(kind==2)after_guard_break(nullptr,damageArgs,&success,nullptr);
        after_damage(nullptr,damageArgs,nullptr,nullptr);
        near(s_state.stamina,kind==1?90:kind==2?40:100);assert(s_guardEvents.empty());
    }
    reset();s_state.stamina=3;consume_defense(StaminaSetting::Block);
    near(s_state.stamina,0);assert(s_state.exhausted);
    reset();after_guard_break(nullptr,damageArgs,&failed,nullptr);near(s_state.stamina,100);
    after_guard_break(nullptr,damageArgs,&success,nullptr);near(s_state.stamina,40);
    setting(StaminaSetting::Block,0);consume_defense(StaminaSetting::Block);near(s_state.stamina,40);
    enabled=false;consume_defense(StaminaSetting::GuardBreak);near(s_state.stamina,40);
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
production = function('maximum_stamina') + '\n' + state + '\n' + source[source.index('struct GuardEvent {'):source.index('HookAction before_damage')] + '\n' + '\n'.join(function(name) for name in (
    'same_link', 'reset_for_link', 'current_link', 'can_consume', 'try_consume',
    'consume_defense', 'before_damage', 'after_block', 'after_guard_break', 'after_damage',
    'before_skill', 'after_skill', 'before_guard_attack', 'after_guard_attack',
    'before_back_slice', 'after_back_slice', 'before_helm_splitter', 'after_helm_splitter',
    'before_midna_charge', 'stamina_available_for_bullet_time', 'stamina_available_for_sprint',
    'stamina_available_for_wolf_sprint', 'mark_wolf_sprint_stamina_active',
    'consume_flurry_rush_stamina', 'consume_great_spin_stamina',
    'stamina_meter_visible', 'stamina_available_for_glide',
    'set_glide_stamina_active', 'mark_sprint_stamina_active', 'update_stamina'))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture.replace('// PRODUCTION', production))
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Stamina runtime passed: configurable costs, guard accounting, recovery, progression, zero-cost actions and lifecycle')
