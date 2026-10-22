"""Exercise production contact lifetime guards with freed and reused peers."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()
assert source.index('PRE(SurvivorsProcess, before_contact_owner_delete)') < source.index('PRE(SurvivorsProcess, before_process)')
assert source.index('PRE(SurvivorsBlock, before_player_contacts)') < source.index('PRE(SurvivorsBlock, before_shield_block)')
assert 'PRE(SurvivorsPlayer, before_player_contacts)' in source
assert 'POST(SurvivorsConcurrentHit, after_player_contact)' in source
assert 'clear_player_contacts();' in source
# Keep dispatch tied to production registration: the pre-fix code has no
# observer at Link's native callback boundary and must fail the incoming case.
native_hit_pre = re.findall(r'PRE\(SurvivorsPlayerHit,\s*(\w+)\)', source)
program = r'''
#include <array>
#include <cassert>
#include <any>
#include <unordered_map>
#include <memory>
using ActorId = unsigned;
constexpr ActorId kNone = ~0u;
struct ModContext {};
enum HookAction {HOOK_CONTINUE, HOOK_SKIP_ORIGINAL};
namespace mods {
template<class T> T arg(void* args, unsigned n) { return std::any_cast<T>(static_cast<std::any*>(args)[n]); }
}
using process_method_func = int(*)(void*);
int destroy(void*) {return 1;}
int execute(void*) {return 1;}
struct process_method_class {process_method_func delete_method=destroy;};
process_method_class methods;
struct fopAc_ac_c {ActorId id; void* sub_method=&methods; virtual ~fopAc_ac_c()=default;};
std::unordered_map<ActorId,fopAc_ac_c*> actors;
fopAc_ac_c* actor_by_id(ActorId id) {auto i=actors.find(id);return i==actors.end()?nullptr:i->second;}
bool is_actor_process(void*) {return true;}
ActorId fopAcM_GetID(fopAc_ac_c* actor) {return actor->id;}
struct cCcD_Obj;
struct cCcD_ObjHitInf {
    cCcD_Obj* attack=nullptr; cCcD_Obj* target=nullptr;
    cCcD_Obj* GetAtHitObj() {return attack;}
    cCcD_Obj* GetTgHitObj() {return target;}
};
struct cCcD_Obj : cCcD_ObjHitInf {
    fopAc_ac_c* owner=nullptr;
    virtual ~cCcD_Obj()=default;
    virtual fopAc_ac_c* GetAc() {return owner;}
};
struct dCcD_GObjInf : cCcD_Obj {
    bool shield=false; unsigned atResets=0,tgResets=0;
    void ResetAtHit() {attack=nullptr;shield=false;++atResets;}
    void ResetTgHit() {target=nullptr;++tgResets;}
};
struct daAlink_c : fopAc_ac_c {
    std::array<dCcD_GObjInf,3> mAtCps{},mTgCyls{};
    dCcD_GObjInf mAtSph,mAtCyl,mGuardAtCps;
};
daAlink_c* player=nullptr;
daAlink_c* daAlink_getAlinkActorClass(){return player;}
ActorId s_player=1;
bool arena=true;
bool in_arena(){return arena;}
struct Attack {dCcD_GObjInf collider;};
std::array<Attack,2> s_attacks;
#include "survivors_contacts.inc"
void dispatch_native_hit(void*);
void native_incoming(dCcD_GObjInf& attack,dCcD_GObjInf& target) {
    // dCcS::SetAtTgGObjInf invokes the installed target callback after SetTgHit.
    // No outer SetAtTgCommonHitInf post detour is required for a native hit.
    target.target=&attack;
    std::any args[]{target.owner,&target,attack.owner,&attack};
    dispatch_native_hit(args);
}
void remember(dCcD_GObjInf& attack, dCcD_GObjInf& target, bool at=true, bool tg=false) {
    if(at) attack.attack=&target;
    if(tg) target.target=&attack;
    std::any args[]{std::any{},static_cast<cCcD_Obj*>(&attack),static_cast<cCcD_Obj*>(&target)};
    after_player_contact(nullptr,args,nullptr,nullptr);
}
void remove_actor(fopAc_ac_c& actor, bool deletion=true) {
    std::any args[]{process_method_func(deletion?destroy:execute),static_cast<void*>(&actor)};
    before_contact_owner_delete(nullptr,args,nullptr,nullptr);
}
void validate() {
    std::any args[]{player}; before_player_contacts(nullptr,args,nullptr,nullptr);
}
int main() {
    daAlink_c link;link.id=1;player=&link;actors[1]=&link;
    visit_player_contacts([&](dCcD_GObjInf& c){c.owner=&link;});
    fopAc_ac_c enemy;enemy.id=2;actors[2]=&enemy;
    auto target=std::make_unique<dCcD_GObjInf>();target->owner=&enemy;
    // Incoming native hits survive both pre-Execute and pre-damage validation,
    // starting with no remembered contact. Covers every human body cylinder,
    // the wolf target, repeat hits and changing attacker/collider identities.
    int life=12;
    for(auto* body:{&link.mTgCyls[0],&link.mTgCyls[1],&link.mTgCyls[2],&link.mAtSph}) {
        native_incoming(*target,*body);validate();validate();
        assert(body->target==target.get());
        if(body->target)--life;
        body->ResetTgHit();
    }
    assert(life==8);
    fopAc_ac_c projectile;projectile.id=9;actors[9]=&projectile;
    auto missile=std::make_unique<dCcD_GObjInf>();missile->owner=&projectile;
    native_incoming(*target,link.mTgCyls[0]);
    native_incoming(*missile,link.mTgCyls[0]);validate();
    assert(link.mTgCyls[0].target==missile.get());
    // A later callback which does not own the actual accepted raw contact
    // cannot overwrite its identity, nor can a foreign recipient.
    std::any mismatch[]{static_cast<fopAc_ac_c*>(&link),&link.mTgCyls[0],&enemy,target.get()};
    dispatch_native_hit(mismatch);validate();
    assert(link.mTgCyls[0].target==missile.get());
    clear_player_contacts();
    link.mTgCyls[0].target=target.get();
    std::any foreign[]{&enemy,&link.mTgCyls[0],&enemy,target.get()};
    dispatch_native_hit(foreign);validate();assert(!link.mTgCyls[0].target);
    arena=false;
    native_incoming(*target,link.mTgCyls[0]);assert(s_contactPeers.empty());
    arena=true;link.mTgCyls[0].ResetTgHit();
    native_incoming(*missile,link.mTgCyls[0]);
    remove_actor(projectile);missile.reset();actors.erase(9);validate();
    assert(!link.mTgCyls[0].target);
    // Native-callback capture still rejects a peer freed without deletion hook.
    native_incoming(*target,link.mTgCyls[0]);actors.erase(2);target.reset();validate();
    assert(!link.mTgCyls[0].target);
    actors[2]=&enemy;target=std::make_unique<dCcD_GObjInf>();target->owner=&enemy;
    clear_player_contacts();
    // Live contacts retain electrical checks, shield flags and combo contact.
    remember(link.mAtCps[0],*target);link.mAtCps[0].shield=true;
    validate();assert(link.mAtCps[0].attack==target.get()&&link.mAtCps[0].shield);
    remove_actor(enemy,false);assert(link.mAtCps[0].attack==target.get());
    // Actor disappears without a delete callback: never dereference its freed collider.
    actors.erase(2);target.reset();validate();assert(!link.mAtCps[0].attack&&!link.mAtCps[0].shield);
    // Reused ID with a different actor address must not validate an old peer.
    actors[2]=&enemy;target=std::make_unique<dCcD_GObjInf>();target->owner=&enemy;
    remember(link.mAtCps[1],*target);
    fopAc_ac_c replacement;replacement.id=2;actors[2]=&replacement;
    target.reset();validate();assert(!link.mAtCps[1].attack);
    // Same actor address under a new ID is also rejected.
    actors[2]=&enemy;target=std::make_unique<dCcD_GObjInf>();target->owner=&enemy;
    remember(link.mAtCps[2],*target);actors.erase(2);enemy.id=3;actors[3]=&enemy;
    target.reset();validate();assert(!link.mAtCps[2].attack);
    // Untracked armor/projectile actors: clear both directions BEFORE destruction.
    auto armor=std::make_unique<dCcD_GObjInf>();armor->owner=&enemy;
    remember(link.mAtSph,*armor);remember(link.mAtCyl,*armor);
    remember(link.mGuardAtCps,*armor);remember(s_attacks[0].collider,*armor);
    remember(*armor,link.mTgCyls[0],false,true);
    fopAc_ac_c other;other.id=4;actors[4]=&other;
    dCcD_GObjInf otherTarget;otherTarget.owner=&other;
    remember(link.mAtCps[0],otherTarget);link.mAtCps[0].shield=true;
    remove_actor(enemy);
    assert(!link.mAtSph.attack&&!link.mAtCyl.attack&&!link.mGuardAtCps.attack);
    assert(!s_attacks[0].collider.attack&&!link.mTgCyls[0].target);
    assert(link.mAtCps[0].attack==&otherTarget&&link.mAtCps[0].shield);
    armor.reset();actors.erase(3);validate();
    // Unknown/corrupted raw pointers are compared, never dereferenced.
    link.mAtCps[0].attack=reinterpret_cast<cCcD_Obj*>(1);
    link.mTgCyls[1].target=reinterpret_cast<cCcD_Obj*>(14);
    validate();assert(!link.mAtCps[0].attack&&!link.mTgCyls[1].target);
    // A rejected later contact cannot replace the identity of an accepted one.
    remember(link.mAtCps[0],otherTarget);
    dCcD_GObjInf rejected;rejected.owner=&other;
    remember(link.mAtCps[0],rejected,false,false);
    validate();assert(link.mAtCps[0].attack==&otherTarget);
    // Story mode is untouched by the validation gate.
    arena=false;actors.erase(4);validate();assert(link.mAtCps[0].attack==&otherTarget);
    arena=true;validate();assert(!link.mAtCps[0].attack);
    // Reset/unload clears native hit pointers as well as the tracking state.
    actors[4]=&other;remember(link.mAtCps[0],otherTarget);
    remember(otherTarget,link.mTgCyls[0],false,true);
    clear_player_contacts();
    assert(s_contactPeers.empty()&&!link.mAtCps[0].attack&&!link.mTgCyls[0].target);
    // Deleting Link discards source addresses before they can be reused.
    actors[4]=&other;remember(link.mAtCps[0],otherTarget);remove_actor(link);
    assert(s_contactPeers.empty());
}
'''
program += '\nvoid dispatch_native_hit(void* args){(void)args;\n'+''.join(
    f'assert({callback}(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);\n' for callback in native_hit_pre)+'}\n'
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'contacts.cpp';exe=Path(temp)/'contacts';cpp.write_text(program)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-fno-omit-frame-pointer','-g',
                    '-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors contact lifetime regressions passed')
