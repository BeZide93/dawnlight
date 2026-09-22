"""Regression: a valid sound ID still needs its scene's resident wave samples."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <array>
#include <cassert>
#include <cstdio>
#include <type_traits>
using u32=unsigned;
struct Arc {
    int status=0,loads=0,erases=0,binds=0;
    bool fail=false,bound=false;
    int getStatus(){return status;}
    void onLoadDone(){++binds;bound=true;}
};
struct Bank {
    std::array<Arc,256> arcs;
    Bank& getWaveBankTable(){return *this;}
    Bank* getWaveBank(int bank){assert(bank==0);return this;}
    unsigned getArcCount(){return arcs.size();}
    Arc* getWaveArc(unsigned id){return &arcs.at(id);}
} bank;
using JAUSectionHeap=Bank;
using JASWaveBank=Bank;
struct Z2SceneMgr {
    unsigned char loadedSeWave_1=0,loadedSeWave_2=0,loadedDemoWave=0;
    unsigned char requestSeWave_1=0,requestSeWave_2=0,requestDemoWave=0;
    bool exists=true;
    bool isSceneExist(){return exists;}
    int getSeLoadStatus(unsigned id){return bank.arcs.at(id).status;}
    bool loadSeWave(unsigned id){
        auto& a=bank.arcs.at(id);++a.loads;
        if(a.status || a.fail)return false;
        a.status=1;return true;
    }
    bool eraseSeWave(unsigned id){
        auto& a=bank.arcs.at(id);assert(a.status==2);++a.erases;a.status=0;
        // Model the native WSYS duplicate-ID hole on erase.
        for(auto& group:bank.arcs)group.bound=false;
        return true;
    }
};
struct SeqMgr {Bank* getSeqDataMgr(){return &bank;}};
struct SoundMgr {SeqMgr seq;SeqMgr* getSeqMgr(){return &seq;}};
struct Z2AudioMgr:Z2SceneMgr {SoundMgr mSoundMgr;} scene;
// Match mod linkage: the unexported template singleton is a null local copy,
// while Z2AudioMgr's explicitly imported pointer reaches the host scene.
Z2SceneMgr* Z2GetSceneMgr(){return nullptr;}
Z2AudioMgr* Z2GetAudioMgr(){return &scene;}
struct Log {
    int warnings=0,ready=0;
    void warn(void*,const char*){++warnings;}
    void info(void*,const char*){++ready;}
} testLog;
auto* svc_log=&testLog;void* mod_ctx=nullptr;
using ModContext=void;
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
#define DEFINE_HOOK(...)
struct Args {Z2SceneMgr* scene;u32 id;};
namespace mods {
    template<class T>T arg(void* ptr,int){
        auto* args=static_cast<Args*>(ptr);
        if constexpr(std::is_pointer_v<T>)return args->scene;
        else return args->id;
    }
}
#include "heroes_shade_trial_waves.inc"
void finish(unsigned id){auto& a=bank.arcs.at(id);assert(a.status==1);a.status=2;a.onLoadDone();}
void finish_all(){for(auto id:ShadeTrialWaves::ids)if(bank.arcs[id].status==1)finish(id);}
void reset(){sTrialWaves={};bank={};scene={};testLog={};}
int main(){
    scene.exists=false;
    assert(!sTrialWaves.prepare());assert(bank.arcs[0x16].loads==0);
    scene.exists=true;
    // Model/game archive readiness does not imply audio wave readiness.
    assert(!sTrialWaves.prepare());
    for(auto id:ShadeTrialWaves::ids)assert(bank.arcs[id].status==1);
    for(int i=0;i<90;++i)assert(!sTrialWaves.prepare());
    for(auto id:ShadeTrialWaves::ids)assert(bank.arcs[id].loads==1);
    finish_all();assert(sTrialWaves.prepare());assert(testLog.ready==1);
    assert(sTrialWaves.prepare());assert(testLog.ready==1);
    bank.arcs[0x15].status=2;bank.arcs[0x15].bound=true;
    sTrialWaves.release();
    for(auto id:ShadeTrialWaves::ids)assert(bank.arcs[id].erases==1);
    assert(bank.arcs[0x15].status==2 && bank.arcs[0x15].bound);
    sTrialWaves.release();assert(bank.arcs[0x16].erases==1);
    assert(!sTrialWaves.prepare());finish_all();assert(sTrialWaves.prepare());

    // Borrowed groups and the scene's music/base SE must not be erased.
    reset();bank.arcs[0x1d].status=2;
    sTrialWaves.prepare();finish_all();assert(sTrialWaves.prepare());
    assert(!sTrialWaves.owns(0x1d));sTrialWaves.release();
    assert(bank.arcs[0x1d].status==2 && bank.arcs[0x1d].erases==0);

    // Leave during asynchronous I/O: defer free, then clean up on a later tick.
    reset();sTrialWaves.prepare();sTrialWaves.release();
    assert(sTrialWaves.owns(0x16));assert(bank.arcs[0x16].erases==0);
    finish_all();sTrialWaves.release();assert(bank.arcs[0x16].erases==1);

    // Native destination load must succeed even for our existing preload.
    reset();sTrialWaves.prepare();scene.requestSeWave_2=0x16;
    sTrialWaves.release();Args args{&scene,0x16};bool result=false;
    assert(before_trial_wave_load(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result);
    finish_all();bank.arcs[0x16].bound=false; // old scene erased a duplicate
    assert(before_trial_wave_load(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(bank.arcs[0x16].bound);
    scene.loadedSeWave_2=0x16;sTrialWaves.release();
    assert(!sTrialWaves.owns(0x16) && bank.arcs[0x16].erases==0);
    args.id=0x58;assert(before_trial_wave_load(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);

    // A rejected allocation isn't marked ready or spammed every frame.
    reset();bank.arcs[0x22].fail=true;
    assert(!sTrialWaves.prepare());finish_all();
    for(int i=0;i<200;++i)assert(!sTrialWaves.prepare());
    assert(testLog.warnings==1 && bank.arcs[0x22].loads<5);
    bank.arcs[0x22].fail=false;
    for(int i=0;i<62;++i)sTrialWaves.prepare();
    finish_all();assert(sTrialWaves.prepare());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'waves.cpp'
    exe = Path(tmp) / 'waves'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

source = (root / 'src/heroes_shade_encounter.cpp').read_text()
assert 'PRE(ShadeWaveLoadHook,before_trial_wave_load)' in source
assert 'uninstall<ShadeWaveLoadHook>' in source
print('Shade wave readiness, async cleanup, borrowed banks, scene adoption and retry: passed')
