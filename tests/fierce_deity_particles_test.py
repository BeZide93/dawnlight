"""Exercise the actual native warp emitter callbacks and isolated 2x stepping."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/fierce_deity_transition.inc').read_text()


def function(name):
    start = source.rfind('\n', 0, source.index(f' {name}(')) + 1
    opening = source.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


fixture = r'''
#include <cassert>
#include <cmath>
#include "fierce_deity_wipe.hpp"
using dawnlight::FierceWarpWipe;
using u16=unsigned short; using u32=unsigned;
constexpr unsigned JPAEmtrStts_StopCalc=2;
constexpr u16 ID_ZI_J_LK_WARP_APP_A=0x9f3, ID_ZI_J_LK_WARP_DISAPP_A=0x9f4;
struct ModContext {};
namespace mods { template<class T> T arg(void* args,int i) {return static_cast<T>(static_cast<void**>(args)[i]);} }
struct Vec { float x=0,y=0,z=0; }; using cXyz=Vec;
struct JPABaseEmitter {
    unsigned tick=0, lifetime=1000, updates=0;
    bool stopped=false, hidden=false, deletedParticles=false, paused=false;
    bool checkStatus(unsigned) {return paused;}
    void stopCreateParticle() {stopped=true;}
    void stopDrawParticle() {hidden=true;}
    void deleteAllParticle() {deletedParticles=true;}
};
JPABaseEmitter own, foreign;
unsigned warnings=0, spawnCalls=0; u16 selected=0; Vec lastPosition;
bool controllerAlive=true, managerAlive=true, spawnFails=false;
u32 activeKey=77;
struct {
    bool committed=false;
    u32 particleKey=0;
    bool particleFailureReported=false;
    FierceWarpWipe wipe;
} s_transition;
bool s_extraWarpParticleStep=false;
void warp_log(const char*) {++warnings;}
struct { struct { void* getParticle() {return controllerAlive?this:nullptr;} } play; } g_dComIfG_gameInfo;
struct dPa_control_c {static void* getEmitterManager() {return managerAlive?&own:nullptr;} };
JPABaseEmitter* dComIfGp_particle_getEmitter(u32 key) {
    assert(controllerAlive && managerAlive);
    return key==activeKey?&own:nullptr;
}
struct daAlink_c {
    struct {Vec pos;} current;
    int shape_angle=0;
    JPABaseEmitter* setEmitter(u32* key,u16 id,const Vec* position,const int*) {
        ++spawnCalls; selected=id; lastPosition=*position;
        if(spawnFails) {*key=0; return nullptr;}
        *key=activeKey; return &own;
    }
};
struct JPAEmitterWorkData {};
struct JPAResource {bool calc(JPAEmitterWorkData*,JPABaseEmitter*);};
'''
callbacks = ''.join(function(name) for name in (
    'warp_particle_emitter', 'stop_warp_particles', 'update_warp_particles', 'after_warp_particle_calc'))
checks = r'''
bool JPAResource::calc(JPAEmitterWorkData* work,JPABaseEmitter* emitter) {
    // The game invokes the post hook for both the original and the extra call.
    ++emitter->updates;
    if(!emitter->paused) ++emitter->tick;
    bool finished=emitter->tick>=emitter->lifetime;
    void* args[]={this,work,emitter};
    after_warp_particle_calc(nullptr,args,&finished,nullptr);
    return finished;
}
int main() {
    daAlink_c link; link.current.pos={12,100,34};
    s_transition.committed=true;
    update_warp_particles(&link);
    assert(selected==ID_ZI_J_LK_WARP_DISAPP_A && selected<0x8000);
    assert(s_transition.particleKey==activeKey);
    assert(lastPosition.x==12 && lastPosition.y==280 && lastPosition.z==34);
    const float firstHeight=lastPosition.y;
    s_transition.wipe.advance(); link.current.pos.x+=10; link.current.pos.y+=5;
    update_warp_particles(&link);
    assert(lastPosition.x==22 && std::fabs(lastPosition.y-(firstHeight+5-3.6f))<0.0001f);
    assert(s_transition.particleKey==activeKey); // refresh the same emitter, no per-frame bursts
    s_transition.wipe.entering=false;
    update_warp_particles(&link);
    assert(selected==ID_ZI_J_LK_WARP_APP_A && selected<0x8000);
    assert(lastPosition.y==link.current.pos.y); // native assembly resource is authored at the feet

    JPAResource resource; JPAEmitterWorkData work;
    assert(!resource.calc(&work,&own));
    assert(own.tick==2 && own.updates==2 && !s_extraWarpParticleStep);
    assert(!resource.calc(&work,&foreign)); // even another emitter of the SAME resource stays native
    assert(foreign.tick==1 && foreign.updates==1);
    own.paused=true;
    assert(!resource.calc(&work,&own));
    assert(own.tick==2 && own.updates==3); // native pause gets no second callback
    own.paused=false;
    own.lifetime=3;
    assert(resource.calc(&work,&own)); // first step terminates: never advance a dead emitter
    assert(own.tick==3 && own.updates==4);
    own.tick=0; own.lifetime=2;
    assert(resource.calc(&work,&own)); // second step's termination must reach the manager
    assert(own.tick==2 && own.updates==6);
    own={};
    for(unsigned i=0;i<50;++i) assert(!resource.calc(&work,&own));
    assert(own.tick==100 && own.updates==100);

    stop_warp_particles();
    assert(own.stopped && own.hidden && own.deletedParticles && s_transition.particleKey==0);
    assert(!foreign.stopped && !foreign.hidden && !foreign.deletedParticles);
    stop_warp_particles(); // idempotent
    assert(!resource.calc(&work,&own));
    assert(own.tick==101); // untracked emitters cannot be accelerated
    s_transition.particleKey=activeKey+1;
    assert(warp_particle_emitter()==nullptr); // expired generation key
    controllerAlive=false;
    s_transition.particleKey=activeKey;
    stop_warp_particles(); // safe after particle system teardown
    update_warp_particles(&link);
    controllerAlive=true; managerAlive=false;
    update_warp_particles(&link);
    assert(s_transition.particleKey==0);
    managerAlive=true; spawnFails=true;
    update_warp_particles(&link); update_warp_particles(&link);
    assert(warnings==1 && s_transition.particleKey==0); // graceful failure, no per-frame log flood
}
'''
with tempfile.TemporaryDirectory(prefix='dawnlight-warp-particles-') as temp:
    cpp, exe = Path(temp)/'test.cpp', Path(temp)/'test'
    cpp.write_text(fixture + callbacks + checks)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(root/'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Fierce Deity native warp particles, positions, isolated 2x stepping and cleanup: passed')
