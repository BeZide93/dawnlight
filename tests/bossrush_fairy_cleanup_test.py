"""Reproduce the hub fairy's missing-emitter departure/capture cleanup.

Runs the production hook around cleanup blocks extracted from native WaitAction.
No game assets required. The native null member call is routed through the same
pre-hook decision before dereferencing, as the host hook dispatcher does.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/new_save_modes.cpp').read_text()
native = (root / 'dusklight/src/d/actor/d_a_obj_yousei.cpp').read_text()
start = source.index('HookAction on_hub_particle_delete_pre(')
hook = source[start:source.index('\n}', start) + 2]
wait = native[native.index('void daObjYOUSEI_c::WaitAction()'):native.index('void daObjYOUSEI_c::LinkAction()')]
start = wait.index('JPABaseEmitter* emitter =')
depart = wait[start:wait.index('fopAcM_delete(this);', start) + len('fopAcM_delete(this);')]
start = wait.index('JPABaseEmitter* emitter =', start + 1)
capture = wait[start:wait.index('fopAcM_delete(this);', start) + len('fopAcM_delete(this);')]
for block in (depart, capture):
    assert 'emitter->deleteAllParticle();' in block
assert 'mods::hook_add_pre<HubParticleDeleteHook>' in source
assert 'uninstall_bossrush_hook<HubParticleDeleteHook>' in source
fixture = r'''
#include <cassert>
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int){return *static_cast<T*>(args);}}
bool bossRush=true,hub=true;
bool is_boss_rush(){return bossRush;}
bool is_boss_hub_stage_name(){return hub;}
struct JPABaseEmitter {int clears=0;void deleteAllParticle(){++clears;}} emitters[2];
JPABaseEmitter* slots[2]{};
JPABaseEmitter* dComIfGp_particle_getEmitter(int key){return slots[key];}
int moves=0,deletes=0,data_804D1830=1,data_804D1831=1;
void dComIfGp_particle_levelEmitterOnEventMove(int){++moves;}
void fopAcM_delete(void*){++deletes;}
// HOOK
void delete_particles(JPABaseEmitter* emitter){
    if(on_hub_particle_delete_pre(nullptr,&emitter,nullptr,nullptr)==HOOK_CONTINUE){
        assert(emitter); // This is the native dereference that crashed at +0xf0.
        emitter->deleteAllParticle();
    }
}
struct Fairy {
    int field_0x604=0,field_0x608=1;
    void depart(){int sp18=0; // DEPART
        assert(sp18==1);
    }
    void capture(){int sp18=0; // CAPTURE
        assert(sp18==1 && data_804D1830==0 && data_804D1831==0);
    }
};
int main(){
    Fairy fairy;
    // No particles, a single missing effect, and both valid: the native actor
    // deletion/capture must always complete; existing emitters still clean up.
    for(int mask=0;mask<4;++mask){
        emitters[0]={};emitters[1]={};moves=deletes=0;
        for(int i=0;i<2;++i) slots[i]=(mask&(1<<i)) ? &emitters[i] : nullptr;
        fairy.depart();fairy.capture();
        assert(deletes==2 && moves==3);
        assert(emitters[0].clears==((mask&1)?2:0));
        assert(emitters[1].clears==((mask&2)?1:0));
    }
    // No changes to cleanup outside this hub, or to any non-null emitter.
    for(bool mode:{false,true})for(bool room:{false,true}){
        bossRush=mode;hub=room;
        JPABaseEmitter* p=nullptr;
        assert(on_hub_particle_delete_pre(nullptr,&p,nullptr,nullptr)==
            (mode&&room ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE));
        p=&emitters[0];
        assert(on_hub_particle_delete_pre(nullptr,&p,nullptr,nullptr)==HOOK_CONTINUE);
    }
}
'''
fixture = '#include <initializer_list>\n' + fixture.replace('// HOOK', hook)
fixture = fixture.replace('// DEPART', depart).replace('// CAPTURE', capture)
# Route only the native snippets through the hook, never invoke a null member in C++.
for block in (depart,capture):
    fixture = fixture.replace(block, block.replace('emitter->deleteAllParticle()', 'delete_particles(emitter)'))
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'fairy.cpp';exe=Path(tmp)/'fairy'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Boss Rush fairy departure/capture with missing and valid emitters: passed')
