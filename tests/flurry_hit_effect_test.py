"""Exercise production Flurry contact, recovery, end and edge-render hooks."""
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
#include <array>
#include <cassert>
#include <cstdint>
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
using fpc_ProcID = unsigned;
constexpr auto fpcM_ERROR_PROCESS_ID_e = ~0U;
constexpr int fpcNm_ALINK_e = 1, fpcNm_ARROW_e = 2;
struct fopAc_ac_c { bool enemy; unsigned id; int name = 0; };
unsigned fopAcM_GetID(fopAc_ac_c* a) { return a->id; }
int fopAcM_GetName(fopAc_ac_c* a) { return a->name; }
struct daAlink_c : fopAc_ac_c {
    static bool checkEnemyGroup(fopAc_ac_c* a) { return a->enemy; }
    void setBStatus(int) {}
};
struct daArrow_c : fopAc_ac_c {};
bool arrow_flight_was_initialized(daArrow_c*) { return true; }
void reset_actor_clock(fopAc_ac_c*, bool) {}
bool actor_is_exempt(fopAc_ac_c* a) { return !a || !a->enemy; }
bool enemy_uses_continuous_slow(fopAc_ac_c*) { return false; }
struct dCcD_GObjInf {
    bool shield = false;
    bool ChkTgShieldHit() { return shield; }
};
struct cCcD_Obj {
    fopAc_ac_c* actor;
    int power = 1;
    bool sword = true;
    cCcD_Obj* hit = nullptr;
    dCcD_GObjInf info{};
    fopAc_ac_c* GetAc() { return actor; }
    int GetAtAtp() { return power; }
    bool ChkTgHit() { return hit != nullptr; }
    cCcD_Obj* GetTgHitObj() { return hit; }
    dCcD_GObjInf* GetGObjInf() { return &info; }
};
struct SlowActorClockEntry { int pendingTicks = 0; } clock;
fopAc_ac_c* scheduledActor = nullptr;
SlowActorClockEntry* find_actor_clock(fopAc_ac_c* a, bool) {
    scheduledActor = a;
    return &clock;
}
bool consume_actor_tick(fopAc_ac_c*) {
    if (clock.pendingTicks == 0) return false;
    --clock.pendingTicks;
    return true;
}
namespace mods {
template<class T> T arg(void* args, int i) { return static_cast<T>(static_cast<void**>(args)[i]); }
}
bool is_flurry_sword_collider(cCcD_Obj* a) { return a->sword; }
bool s_flurryRushActive = true, s_flurryMeleePositioned = true, s_flurryLinkSlowed = false;
bool s_bulletTimeActive = false;
daAlink_c* s_flurryRushOwner = nullptr;
fopAc_ac_c* s_flurryRushTarget = nullptr;
fpc_ProcID s_flurryHitTargetId = fpcM_ERROR_PROCESS_ID_e;
struct HitActorEntry { fopAc_ac_c* actor = nullptr; std::uint64_t frame = 0; };
std::array<HitActorEntry, 2> s_hitActors{};
std::uint64_t s_slowFrame = 0;
constexpr std::uint64_t kHitExecuteGraceFrames = 6;
constexpr int kFlurryRushActionStatus = 1, BUTTON_STATUS_NONE = 0;
int dComIfGp_getAStatus() { return 0; }
void sync_slow_motion_controllers() {}
bool combat_slow_active() { return s_flurryRushActive || s_bulletTimeActive; }
void clear_combat_time_caches() { clock = {}; s_hitActors = {}; }
struct Controller {
    float strength = 1.0f;
    float time_scale() { return 0.1f; }
    float edge_strength() { return strength; }
} s_enemySlowMotion;
struct GfxStageContext {};
struct view_class {} view;
view_class* dComIfGd_getView() { return &view; }
int edgeDraws = 0;
void drawSlowMotionEdges(view_class*, float) { ++edgeDraws; }
// EDGE_FLAG
// FUNCTIONS
int main() {
    daAlink_c link; link.enemy = false; link.id = 1; link.name = fpcNm_ALINK_e;
    fopAc_ac_c enemy{true, 2}, other{true, 3};
    cCcD_Obj sword{&link}, target{&enemy};
    s_flurryRushOwner = &link; s_flurryRushTarget = &enemy;
    void* args[]{nullptr, &sword, &target};
    int hp = 20, effects = 0, callbackCalls = 0, cooldown = 0;
    bool blocked = false;
    auto contact = [&] {
        target.hit = nullptr; target.info.shield = blocked;
        auto action = before_common_at_tg_hit(nullptr, args, nullptr, nullptr);
        // Native collision/actor damage stand-in: a shield can register contact
        // without damage; post-hit invulnerability lasts ten native updates.
        if (action == HOOK_CONTINUE && cooldown == 0) {
            target.hit = &sword;
            if (!blocked) { hp -= 2; ++effects; ++callbackCalls; cooldown = 10; }
        }
        after_common_at_tg_hit(nullptr, args, nullptr, nullptr);
        return action;
    };
    assert(should_skip_actor(&enemy));
    // Blocked first contact must not release the target from slow motion.
    blocked = true;
    assert(contact() == HOOK_CONTINUE && hp == 20);
    assert(!actor_has_hit_grace(&enemy));
    blocked = false;
    for (int i = 0; i < 4; ++i) {
        clock.pendingTicks = 0;
        sword.power = (i + 1) * 10;
        assert(contact() == HOOK_CONTINUE);
        assert(hp == 18 - 2 * i && effects == i + 1 && callbackCalls == i + 1);
        assert(clock.pendingTicks == 1 && scheduledActor == &enemy);
        assert(actor_has_hit_grace(&enemy));
        assert(enemy_slow_motion_scale(&enemy) == 1.0f);
        assert(!actor_uses_visual_slowdown(&enemy));
        // Duplicate contacts in the same swing do not bypass native immunity.
        contact(); assert(hp == 18 - 2 * i);
        for (int frame = 0; frame < 12; ++frame) {
            ++s_slowFrame;
            clock.pendingTicks = frame % 10 == 0 ? 1 : 0;
            if (!should_skip_actor(&enemy) && cooldown > 0) --cooldown;
        }
        assert(cooldown == 0); // Recovery must not stretch to 100 frames.
        clock.pendingTicks = 0;
        assert(should_skip_actor(&other));
        assert(enemy_slow_motion_scale(&other) == 0.1f);
        assert(actor_uses_visual_slowdown(&other));
    }
    clock.pendingTicks = 3; blocked = true; contact();
    assert(clock.pendingTicks == 3 && hp == 12);
    // Reused addresses must not inherit the accepted target's recovery.
    ++enemy.id; assert(!actor_has_hit_grace(&enemy)); --enemy.id;
    auto unchanged = [&] {
        s_flurryHitTargetId = fpcM_ERROR_PROCESS_ID_e;
        target.hit = &sword; target.info.shield = false;
        scheduledActor = nullptr; clock.pendingTicks = 0;
        assert(before_common_at_tg_hit(nullptr, args, nullptr, nullptr) == HOOK_CONTINUE);
        after_common_at_tg_hit(nullptr, args, nullptr, nullptr);
        assert(scheduledActor == nullptr && clock.pendingTicks == 0);
        assert(s_flurryHitTargetId == fpcM_ERROR_PROCESS_ID_e);
    };
    target.actor = &other; unchanged(); target.actor = &enemy;
    sword.sword = false; unchanged(); sword.sword = true;
    sword.power = 0; unchanged(); sword.power = 1;
    sword.actor = &other; unchanged(); sword.actor = &link;
    args[1] = nullptr; unchanged(); args[1] = &sword;
    args[2] = nullptr; unchanged(); args[2] = &target;
    target.hit = nullptr;
    after_common_at_tg_hit(nullptr, args, nullptr, nullptr);
    assert(s_flurryHitTargetId == fpcM_ERROR_PROCESS_ID_e);
    // No edge geometry during the rush, the fade-out or standalone Bullet Time.
    draw_slow_motion_edges(nullptr, nullptr, nullptr);
    s_flurryHitTargetId = enemy.id;
    stop_flurry_rush();
    assert(!s_flurryRushActive && !s_flurryRushOwner && !s_flurryRushTarget);
    assert(s_flurryHitTargetId == fpcM_ERROR_PROCESS_ID_e);
    assert(hp == 12 && effects == 4 && callbackCalls == 4);
    s_enemySlowMotion.strength = 0.5f;
    draw_slow_motion_edges(nullptr, nullptr, nullptr);
    s_bulletTimeActive = true;
    draw_slow_motion_edges(nullptr, nullptr, nullptr);
    assert(edgeDraws == 0);
    assert(enemy_slow_motion_scale(&enemy) == 0.1f);
    unchanged(); stop_flurry_rush();
}
'''
flag = next(line for line in source.splitlines()
            if line.startswith('constexpr bool kTestDisableSlowMotionEdges'))
fixture = fixture.replace('// EDGE_FLAG', flag)
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(signature) for signature in [
    'bool actor_has_hit_grace', 'float enemy_slow_motion_scale',
    'bool actor_uses_visual_slowdown', 'bool should_skip_actor',
    'HookAction before_common_at_tg_hit', 'void after_common_at_tg_hit',
    'void stop_flurry_rush', 'void draw_slow_motion_edges',
]))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Flurry diagnostic passed: successive native hits after recovery, shield/immunity '
      'preserved, target-only recovery, lifetime/reset safety and no edge geometry')
