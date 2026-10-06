"""Run the actual aura callbacks: bounded emitter ownership, looping and cleanup."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <cassert>
using u32 = unsigned; using u16 = unsigned short; using fpc_ProcID = unsigned;
constexpr auto fpcM_ERROR_PROCESS_ID_e = ~0u;
constexpr u16 ID_ZI_J_WTOA_B = 0x252;
constexpr unsigned JPAEmtrStts_StopCalc = 2;
enum class FierceDeityTint { None, Dark, White, Gold };
enum HookAction { HOOK_CONTINUE };
struct ModContext {};
namespace mods { template<class T> T arg(void* a, int i) {return static_cast<T>(static_cast<void**>(a)[i]);} }
struct Vec {float x=0,y=0,z=0;};
struct JPABaseEmitter {
    int mMaxFrame=8; u32 mTick=0, particles=3;
    bool stopped=false, hidden=false, paused=false;
    const void* matrix=nullptr;
    void stopCreateParticle() {stopped=true;}
    void playCreateParticle() {stopped=false;}
    void stopDrawParticle() {hidden=true;}
    void deleteAllParticle() {particles=0;}
    bool checkStatus(unsigned) {return paused;}
    u32 getAge() {return mTick;}
    void setGlobalRTMatrix(const void* m) {matrix=m;}
};
JPABaseEmitter own, foreign;
bool controller=true, manager=true, failed=false, active=true, busy=false;
unsigned warnings=0, allocations=0, updates=0;
u32 generation=10;
FierceDeityTint tint=FierceDeityTint::Dark;
bool fierce_deity_active() {return active;}
bool fierce_deity_transition_busy() {return busy;}
FierceDeityTint fierce_deity_displayed_tint() {return tint;}
void warp_warning(const char*) {++warnings;}
struct {struct {void* getParticle() {return controller?this:nullptr;}} play;} g_dComIfG_gameInfo;
struct dPa_control_c {static void* getEmitterManager() {return manager?&own:nullptr;}};
JPABaseEmitter* dComIfGp_particle_getEmitter(u32 key) {
    assert(controller && manager); return key==generation?&own:nullptr;
}
struct Model {
    unsigned joints=3;
    Model* getModelData() {return this;}
    unsigned getJointNum() {return joints;}
    void* getAnmMtx(int joint) {assert(joint==1); return this;}
};
struct daAlink_c {
    u32 id=1; Model model; Model* mpLinkModel=&model;
    bool wolf=false, dead=false, scene=false, event=false, hidden=false;
    struct {Vec pos;} current;
    bool checkWolf() {return wolf;}
    bool checkDeadHP() {return dead;}
    bool checkSceneChangeAreaStart() {return scene;}
    bool checkEventRun() {return event;}
    bool checkPlayerNoDraw() {return hidden;}
    JPABaseEmitter* setEmitter(u32* key,u16 effect,const Vec*,const void*) {
        assert(effect==ID_ZI_J_WTOA_B);
        ++updates;
        if(failed) {*key=0; return nullptr;}
        if(*key!=generation) {++allocations; own={};}
        *key=generation; return &own;
    }
};
u32 fopAcM_GetID(daAlink_c* p) {return p->id;}
'''
checks = r'''
int main() {
    daAlink_c link;
    update_dark_aura(&link);
    assert(allocations==1 && own.matrix==&link.model);
    void* args[]={nullptr,nullptr,&own};
    // Emulate native termination and keyed emission over many complete cycles.
    // Existing particles survive cycle boundaries; no additional emitter is made.
    for(int i=0;i<1000;++i) {
        update_dark_aura(&link);
        before_dark_aura_calc(nullptr,args,nullptr,nullptr);
        assert(own.mTick<8 && own.particles==3);
        ++own.mTick;
    }
    assert(allocations==1);
    foreign.mTick=12; args[2]=&foreign;
    before_dark_aura_calc(nullptr,args,nullptr,nullptr);
    assert(foreign.mTick==12); // includes native metamorphosis emitters
    args[2]=&own; own.mTick=8; own.paused=true;
    before_dark_aura_calc(nullptr,args,nullptr,nullptr);
    assert(own.mTick==8);
    own.paused=false; own.mMaxFrame=0;
    before_dark_aura_calc(nullptr,args,nullptr,nullptr);
    assert(own.mTick==8); // authored continuous effects keep their own clock
    own.mMaxFrame=8;
    before_dark_aura_calc(nullptr,args,nullptr,nullptr);
    assert(own.mTick==0 && !own.stopped);
    for(bool* condition : {&link.wolf,&link.dead,&link.scene,&link.event,&link.hidden,&busy}) {
        *condition=true; update_dark_aura(&link);
        assert(s_darkAura.particleKey==0 && own.stopped && own.hidden && own.particles==0);
        *condition=false; update_dark_aura(&link);
    }
    for(auto appearance : {FierceDeityTint::None,FierceDeityTint::White,FierceDeityTint::Gold}) {
        tint=appearance; update_dark_aura(&link); assert(s_darkAura.particleKey==0);
    }
    tint=FierceDeityTint::Dark; update_dark_aura(&link);
    active=false; update_dark_aura(&link); assert(s_darkAura.particleKey==0);
    active=true; update_dark_aura(&link);
    link.mpLinkModel=nullptr; update_dark_aura(&link); assert(s_darkAura.particleKey==0);
    link.mpLinkModel=&link.model; link.model.joints=1;
    update_dark_aura(&link); assert(s_darkAura.particleKey==0);
    link.model.joints=3; update_dark_aura(&link);
    ++link.id; update_dark_aura(&link); assert(s_darkAura.ownerId==link.id);
    ++generation; // scene table replaced; old key must not affect its new emitter
    own={}; stop_dark_aura(); assert(!own.stopped);
    update_dark_aura(&link); controller=false; stop_dark_aura();
    assert(s_darkAura.particleKey==0);
    update_dark_aura(&link); assert(s_darkAura.particleKey==0);
    controller=true; manager=false; update_dark_aura(&link);
    assert(s_darkAura.particleKey==0);
    manager=true; failed=true;
    update_dark_aura(&link); update_dark_aura(&link);
    assert(warnings==1 && s_darkAura.particleKey==0);
    failed=false; update_dark_aura(&link);
    update_dark_aura(nullptr); assert(s_darkAura.particleKey==0 && own.hidden);
    stop_dark_aura(); // idempotent cleanup, including shutdown
    assert(!foreign.stopped && !foreign.hidden);
}
'''
with tempfile.TemporaryDirectory(prefix='dawnlight-dark-aura-') as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text('#include <initializer_list>\n' + fixture +
                   (root / 'src/fierce_deity_aura.inc').read_text() + checks)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Dark Link aura: looping, ownership, appearance gating and cleanup passed')
