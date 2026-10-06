"""Exercise the production deferred-hit hook without running enemy damage logic."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/bullet_time.cpp').read_text()


def function(signature):
    start = source.index(signature + '(')
    return source[start:source.index('\n}', start) + 2]


fixture = r'''
#include <cassert>
#include <cstdint>
using u8 = unsigned char;
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
struct cXyz { float x, y, z; };
struct fopAc_ac_c { int id; bool enemy; unsigned status = 0; };
constexpr unsigned fopAcStts_UNK_0x40000000_e = 0x40000000;
bool fopAcM_CheckStatus(fopAc_ac_c* a, unsigned f) { return (a->status & f) != 0; }
void fopAcM_OffStatus(fopAc_ac_c* a, unsigned f) { a->status &= ~f; }
void fopAcM_OnStatus(fopAc_ac_c* a, unsigned f) { a->status |= f; }
int fopAcM_GetID(fopAc_ac_c* a) { return a->id; }
struct daAlink_c { static bool checkEnemyGroup(fopAc_ac_c* a) { return a->enemy; } };
struct cCcD_GStts {};
struct dCcD_GStts : cCcD_GStts {};
struct cCcD_Stts {
    dCcD_GStts global;
    bool valid = true;
    cCcD_GStts* GetGStts() { return valid ? &global : nullptr; }
};
struct dCcD_GObjInf { bool shield = false, noHitmark = false; };
struct cCcD_Obj {
    fopAc_ac_c* actor;
    dCcD_GObjInf info;
    cCcD_Stts status;
    u8 power = 1;
    bool sword = true;
    fopAc_ac_c* GetAc() { return actor; }
    dCcD_GObjInf* GetGObjInf() { return &info; }
    cCcD_Stts* GetStts() { return &status; }
    u8 GetAtAtp() { return power; }
};
struct Collision {
    int effects = 0;
    bool lastShield = false;
    cXyz lastPosition{};
    bool ChkShield(cCcD_Obj*, cCcD_Obj*, dCcD_GObjInf*, dCcD_GObjInf* t, cXyz*) {
        return t->shield;
    }
    void ProcAtTgHitmark(bool, bool, cCcD_Obj*, cCcD_Obj* t,
        dCcD_GObjInf* aInfo, dCcD_GObjInf* tInfo, cCcD_Stts*, cCcD_Stts*,
        dCcD_GStts*, dCcD_GStts*, cXyz* p, bool shield) {
        if (aInfo->noHitmark || tInfo->noHitmark) return;
        if (fopAcM_CheckStatus(t->actor, fopAcStts_UNK_0x40000000_e)) return;
        fopAcM_OnStatus(t->actor, fopAcStts_UNK_0x40000000_e);
        ++effects; lastShield = shield; lastPosition = *p;
    }
} collision;
Collision* dComIfG_Ccsp() { return &collision; }
namespace mods {
template<class T> T arg(void* args, int i) { return static_cast<T>(static_cast<void**>(args)[i]); }
}
bool s_flurryRushActive = true;
std::uint64_t s_flurrySwordAttackSerial = 1;
fopAc_ac_c* s_flurryRushOwner;
fopAc_ac_c* s_flurryRushTarget;
// STATE
DeferredFlurryDamage s_deferredFlurryDamage{};
bool is_flurry_sword_collider(cCcD_Obj* a) { return a->sword; }
int attackHits = 0;
void preserve_flurry_attack_hit(cCcD_Obj*, cCcD_Obj*, cXyz*) { ++attackHits; }
// FUNCTIONS
int main() {
    fopAc_ac_c link{1, false}, enemy{2, true}, other{3, true};
    s_flurryRushOwner = &link; s_flurryRushTarget = &enemy;
    cCcD_Obj sword{&link}, secondSwordCollider{&link}, target{&enemy};
    cXyz impact{12, 34, 56};
    void* args[]{nullptr, &sword, &target, &impact};
    auto hit = [&] { return before_common_at_tg_hit(nullptr, args, nullptr, nullptr); };
    assert(hit() == HOOK_SKIP_ORIGINAL);
    assert(collision.effects == 1 && !collision.lastShield);
    assert(collision.lastPosition.x == 12 && collision.lastPosition.y == 34);
    assert(s_deferredFlurryDamage.pending && s_deferredFlurryDamage.targetActor == &enemy);
    assert(s_deferredFlurryDamage.lastAttackSerial == 1 && attackHits == 1);
    // Repeated contact and multiple sword capsules must not duplicate the flash.
    for (int i = 0; i < 10; ++i) assert(hit() == HOOK_SKIP_ORIGINAL);
    args[1] = &secondSwordCollider;
    assert(hit() == HOOK_SKIP_ORIGINAL && collision.effects == 1);
    // A new swing must flash even if the slowed enemy has not cleared its flag.
    ++s_flurrySwordAttackSerial; target.info.shield = true; impact.z = 78;
    assert(hit() == HOOK_SKIP_ORIGINAL && collision.effects == 2);
    assert(collision.lastShield && collision.lastPosition.z == 78);
    assert(fopAcM_CheckStatus(&enemy, fopAcStts_UNK_0x40000000_e));
    assert(s_deferredFlurryDamage.lastAttackSerial == 2);
    // Keep native particle suppression; recording deferred damage is independent.
    ++s_flurrySwordAttackSerial; target.info.noHitmark = true;
    assert(hit() == HOOK_SKIP_ORIGINAL && collision.effects == 2);
    assert(s_deferredFlurryDamage.lastAttackSerial == 3);
    target.info.noHitmark = false; target.status.valid = false;
    ++s_flurrySwordAttackSerial;
    assert(hit() == HOOK_SKIP_ORIGINAL && collision.effects == 2);
    target.status.valid = true;
    // Everything outside an eligible deferred sword contact keeps vanilla handling.
    ++s_flurrySwordAttackSerial; s_flurryRushActive = false;
    assert(hit() == HOOK_CONTINUE);
    s_flurryRushActive = true; target.actor = &other; assert(hit() == HOOK_CONTINUE);
    target.actor = &enemy; args[3] = nullptr; assert(hit() == HOOK_CONTINUE);
    args[3] = &impact; args[1] = &sword; sword.power = 0; assert(hit() == HOOK_CONTINUE);
    sword.power = 1; sword.sword = false; assert(hit() == HOOK_CONTINUE);
    assert(collision.effects == 2 && s_deferredFlurryDamage.lastAttackSerial == 4);
}
'''
start = source.index('struct DeferredFlurryDamage {')
fixture = fixture.replace('// STATE', source[start:source.index('\n};', start) + 3])
fixture = fixture.replace('// FUNCTIONS', function('void show_flurry_hit_effect') + '\n' +
                          function('HookAction before_common_at_tg_hit'))
# The fixture deliberately provides no target-hit, damage or hit-callback API.
fixture = fixture.replace('using u8 = unsigned char;',
                          'using u8 = unsigned char; using fpc_ProcID = int;\n'
                          'constexpr int fpcM_ERROR_PROCESS_ID_e = -1;')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-missing-field-initializers', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Flurry hit effects passed: contact position, per-swing deduplication, slow actor flags, '
      'shield effects, native suppression and deferred-hit isolation')
