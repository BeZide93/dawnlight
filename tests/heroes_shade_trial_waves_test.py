"""Regression: a valid sound ID still needs its scene's resident wave samples."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/heroes_shade_encounter.cpp').read_text()
start = source.index('void update_heroes_shade_audio()')
audio_update = source[start:source.index('\n}', start)+2]
fixture = r'''
#include <array>
#include <algorithm>
#include <cstdint>
#include "heroes_shade_battle.hpp"
namespace shade=dawnlight::shade;
using Trial=shade::Trial;
#include <cassert>
#include <cstdio>
#include <type_traits>
using u32=unsigned;
struct JASHeap {
    uintptr_t base=0x4000;
    unsigned size=0xa00000,available=0xa00000,children=0;
    void* getBase(){return reinterpret_cast<void*>(base);}
    unsigned getSize(){return size;}
    unsigned getFreeSize(){return available;}
    void initRootHeap(void* p,unsigned n){base=reinterpret_cast<uintptr_t>(p);size=available=n;}
    void* getFirstChild(){return children ? this : nullptr;}
    void* getEndChild(){return nullptr;}
    void free(){assert(!children);}
} audioHeap;
namespace JASKernel {JASHeap* getAramHeap(){return &audioHeap;}}
struct JASWaveArcLoader {static JASHeap* getRootHeap(){return &audioHeap;}};
unsigned blocks=0;
struct JKRAramBlock {
    unsigned address,size;
    unsigned getAddress(){return address;}
    unsigned getSize(){return size;}
    ~JKRAramBlock(){--blocks;}
};
struct JKRAramHeap {
    enum {TAIL};
    bool fail=false;
    unsigned next=0xe00000;
    JKRAramBlock* alloc(unsigned size,int){
        if(fail)return nullptr;
        ++blocks;next-=size;return new JKRAramBlock{next,size};
    }
} graphHeap;
struct JKRAram {
    static JKRAram* getManager(){static JKRAram a;return &a;}
    static JKRAramHeap* getAramHeap(){return &graphHeap;}
};
#define JKR_NEW_ARGS(...) new
#define JKR_DELETE(p) delete (p)
struct Arc {
    int status=0,loads=0,erases=0,binds=0,mEntryNum=42,_5a=0;
    unsigned bytes=0x10000;
    JASHeap* parent=nullptr;
    bool fail=false,bound=false;
    int getStatus(){return status;}
    unsigned getFileSize(){return bytes;}
    void onLoadDone(){++binds;bound=true;}
    bool load(JASHeap* heap){
        ++loads;
        if(status || fail || mEntryNum<0 || bytes>heap->available)return false;
        heap->available-=bytes;++heap->children;parent=heap;
        status=1;++_5a;return true;
    }
    void erase(){
        if(parent){parent->available+=bytes;--parent->children;parent=nullptr;}
        status=0;++erases;
    }
};
using JASWaveArc=Arc;
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
        return bank.arcs.at(id).load(&audioHeap);
    }
    bool eraseSeWave(unsigned id){
        auto& a=bank.arcs.at(id);assert(a.status==2);a.erase();
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
    int warnings=0;
    void warn(void*,const char*){++warnings;}
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
shade::Battle sBattle;
int sRegistration=1;
bool arena(){return true;}
bool dComIfGp_isEnableNextStage(){return false;}
void suspend_trial(){}
// ACTUAL_AUDIO_UPDATE
void finish(unsigned id){auto& a=bank.arcs.at(id);assert(a.status==1);--a._5a;a.status=2;a.onLoadDone();}
void finish_all(){for(auto id:ShadeTrialWaves::ids)if(bank.arcs[id].status==1)finish(id);}
void reset(bool borrowedVoice=true){
    finish_all();scene.loadedSeWave_1=scene.loadedSeWave_2=scene.loadedDemoWave=0;
    scene.requestSeWave_1=scene.requestSeWave_2=scene.requestDemoWave=0;
    for(auto& a:bank.arcs)if(a.parent)a.erase();
    sTrialWaves.release();assert(blocks==0);
    sTrialWaves={};bank={};scene={};testLog={};audioHeap={};graphHeap={};
    // Existing trial-specific cases isolate their own allocations; voice
    // ownership, pressure and retirement are exercised separately below.
    if(borrowedVoice)bank.arcs[0x4a].status=bank.arcs[0x5d].status=2;
}
int main(){
    scene.exists=false;
    assert(!sTrialWaves.prepare(Trial::Shield));assert(bank.arcs[0x22].loads==0);
    scene.exists=true;
    assert(!sTrialWaves.prepare(Trial::Shield));
    assert(bank.arcs[0x08].loads==0 && bank.arcs[0x16].loads==0);
    for(int i=0;i<90;++i)assert(!sTrialWaves.prepare(Trial::Shield));
    assert(bank.arcs[0x22].loads==1);
    finish_all();assert(sTrialWaves.prepare(Trial::Shield));
    assert(sTrialWaves.prepare(Trial::Shield));
    bank.arcs[0x15].status=2;bank.arcs[0x15].bound=true;
    sTrialWaves.release();assert(bank.arcs[0x22].erases==1);
    assert(bank.arcs[0x15].status==2 && bank.arcs[0x15].bound);
    sTrialWaves.release();assert(bank.arcs[0x22].erases==1);

    // Full normal audio heap: native load fails; reserved storage makes the
    // same production request playable without evicting Darknut/music samples.
    reset();audioHeap.available=0;
    assert(!sTrialWaves.prepare(Trial::Shield));
    assert(testLog.warnings==0 && blocks==4);
    for(auto id:{0x1d,0x1f,0x21,0x22})assert(bank.arcs[id].status==1);
    sTrialWaves.release();assert(blocks==4); // pending writes must keep storage
    finish_all();assert(sTrialWaves.prepare(Trial::Shield));
    sTrialWaves.release();assert(blocks==0);

    // Phase prefetch only needs its own groups and gives old one-shots a tail.
    reset();audioHeap.available=0;
    sTrialWaves.prepare(Trial::Shield);finish_all();
    sTrialWaves.prepare(Trial::Fire);finish_all();
    assert(bank.arcs[0x08].status==2 && bank.arcs[0x22].status==2);
    for(int i=0;i<60;++i)sTrialWaves.prepare(Trial::Fire);
    assert(bank.arcs[0x22].status==0 && bank.arcs[0x08].status==2 && blocks==1);
    sTrialWaves.prepare(Trial::Eyes);finish_all();
    for(int i=0;i<60;++i)sTrialWaves.prepare(Trial::Eyes);
    assert(bank.arcs[0x08].status==0 && bank.arcs[0x16].status==2 && blocks==1);
    for(int i=0;i<60;++i)sTrialWaves.prepare(Trial::Wind);
    assert(blocks==0);

    // Borrowed archives are never erased.
    reset();bank.arcs[0x1d].status=2;
    sTrialWaves.prepare(Trial::Shield);finish_all();assert(sTrialWaves.prepare(Trial::Shield));
    assert(!sTrialWaves.owns(0x1d));sTrialWaves.release();
    assert(bank.arcs[0x1d].status==2 && bank.arcs[0x1d].erases==0);

    // Native destination adopts an overflow preload, including its storage.
    reset();audioHeap.available=0;sTrialWaves.prepare(Trial::Eyes);
    scene.requestSeWave_2=0x16;sTrialWaves.release();
    Args args{&scene,0x16};bool result=false;
    assert(before_trial_wave_load(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result);
    finish_all();bank.arcs[0x16].bound=false;
    assert(before_trial_wave_load(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(bank.arcs[0x16].bound);
    scene.loadedSeWave_2=0x16;sTrialWaves.release();
    assert(!sTrialWaves.owns(0x16) && bank.arcs[0x16].erases==0 && blocks==1);
    scene.eraseSeWave(0x16);sTrialWaves.release();assert(blocks==0);
    args.id=0x58;assert(before_trial_wave_load(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);

    // Native erase can detach an adopted heap while a DVD write is pending.
    reset();audioHeap.available=0;sTrialWaves.prepare(Trial::Eyes);
    scene.loadedSeWave_2=0x16;sTrialWaves.release();
    bank.arcs[0x16].erase();sTrialWaves.release();assert(blocks==1);
    --bank.arcs[0x16]._5a;sTrialWaves.release();assert(blocks==0);

    // Exhaustion and an overlapping ARAM partition fail safely and retry.
    reset();audioHeap.available=0;graphHeap.fail=true;
    for(int i=0;i<200;++i)assert(!sTrialWaves.prepare(Trial::Eyes));
    assert(testLog.warnings==1 && bank.arcs[0x16].loads<5 && blocks==0);
    graphHeap.fail=false;graphHeap.next=0x900000; // overlaps live audio root
    for(int i=0;i<62;++i)sTrialWaves.prepare(Trial::Eyes);
    assert(blocks==0 && bank.arcs[0x16].status==0);
    graphHeap.next=0xe00000;
    for(int i=0;i<62;++i)sTrialWaves.prepare(Trial::Eyes);
    finish_all();assert(sTrialWaves.prepare(Trial::Eyes));assert(blocks==1);

    // The full native Z2SCENE_SHADES_REALM branch loads BOTH se_wave1=0x4a
    // (also used by Hyrule Field) and se_wave2=0x5d. Loading only 0x4a used to
    // report "voice ready" while KN's voice set was never requested.
    reset(false);audioHeap.available=0;
    assert(!sTrialWaves.prepare(Trial::None));
    assert(bank.arcs[0x4a].status==1 && bank.arcs[0x5d].status==1);
    assert(blocks==2 && testLog.warnings==0);
    assert(sTrialWaves.owns(0x4a) && sTrialWaves.owns(0x5d));
    sTrialWaves.release();assert(blocks==2); // both writes still pending
    finish(0x4a);
    assert(!sTrialWaves.prepare(Trial::None)); // the missing second set matters
    finish(0x5d);assert(sTrialWaves.prepare(Trial::None));
    for(auto trial:{Trial::Shield,Trial::Fire,Trial::Eyes,Trial::Wind,Trial::None}) {
        sTrialWaves.prepare(trial);finish_all();
        for(int i=0;i<120;++i)sTrialWaves.prepare(trial);
        for(auto id:{0x4a,0x5d})
            assert(bank.arcs[id].status==2 && bank.arcs[id].erases==0);
    }
    assert(blocks==2);sTrialWaves.release();assert(blocks==0);
    assert(bank.arcs[0x4a].erases==1 && bank.arcs[0x5d].erases==1);
    // Missing 0x5d must keep the archive pair unavailable.
    reset(false);bank.arcs[0x5d].mEntryNum=-1;
    sTrialWaves.prepare(Trial::None);finish_all();
    assert(!sTrialWaves.prepare(Trial::None) && testLog.warnings==1);
    // A borrowed base archive remains untouched; only the added set is freed.
    reset(false);bank.arcs[0x4a].status=2;
    sTrialWaves.prepare(Trial::None);finish_all();assert(sTrialWaves.prepare(Trial::None));
    sTrialWaves.release();
    assert(bank.arcs[0x4a].status==2 && bank.arcs[0x4a].erases==0);
    assert(bank.arcs[0x5d].status==0 && bank.arcs[0x5d].erases==1);

    // Missing disc entries and queue failures must not be hidden by fallback.
    reset();audioHeap.available=0;bank.arcs[0x16].mEntryNum=-1;
    sTrialWaves.prepare(Trial::Eyes);assert(blocks==0 && testLog.warnings==1);
    reset();bank.arcs[0x16].fail=true;
    sTrialWaves.prepare(Trial::Eyes);assert(blocks==0 && testLog.warnings==1);
    reset();audioHeap.available=0;bank.arcs[0x16].fail=true;
    sTrialWaves.prepare(Trial::Eyes);assert(blocks==0 && testLog.warnings==1);
    reset();
    // Exercise production prefetch for every valid fire-before-wind order.
    std::array<Trial,4> order{Trial::Shield,Trial::Fire,Trial::Eyes,Trial::Wind};
    unsigned validOrders=0;
    do {
        if(std::find(order.begin(),order.end(),Trial::Wind)<
           std::find(order.begin(),order.end(),Trial::Fire)) continue;
        ++validOrders;
        reset();sBattle={};sBattle.trial_order=order;
        for(unsigned i=0;i<order.size();++i) {
            sBattle.trial=Trial::None;sBattle.trials_started=i;
            update_heroes_shade_audio();
            assert(sTrialWaves.selected==order[i]);
            finish_all();update_heroes_shade_audio();assert(sTrialWaves.ready);
            // Once started, preload must keep the active set, not jump ahead.
            sBattle.trial=order[i];sBattle.trials_started=i+1;
            update_heroes_shade_audio();assert(sTrialWaves.selected==order[i]);
        }
        sBattle.trial=Trial::None;
        update_heroes_shade_audio();assert(sTrialWaves.selected==Trial::None);
    } while(std::next_permutation(order.begin(),order.end()));
    assert(validOrders==12);
    reset();
}

'''
fixture = fixture.replace('// ACTUAL_AUDIO_UPDATE', audio_update)
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'waves.cpp'
    exe = Path(tmp) / 'waves'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

assert 'PRE(ShadeWaveLoadHook,before_trial_wave_load)' in source
assert 'uninstall<ShadeWaveLoadHook>' in source
print('Shade phase waves, bounded audio memory, reserved ARAM, async cleanup, adoption and retry: passed')
