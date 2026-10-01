"""Run the actual two-model ownership callbacks with checked native heap stubs."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
transition = (root / 'src/fierce_deity_transition.inc').read_text()
visual = (root / 'src/fierce_deity_visual.cpp').read_text()


def function(source, name):
    start = source.rfind('\n', 0, source.index(f' {name}(')) + 1
    opening = source.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


fixture = r'''
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "fierce_deity_wipe.hpp"
using dawnlight::FierceWarpWipe;
using u8=unsigned char; using fpc_ProcID=unsigned;
constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
struct Heap {
    virtual ~Heap() = default;
    bool alive=true;
    unsigned getHeapSize() const {return 0x40000;}
    Heap* getParent() {return nullptr;}
    void destroy() {assert(alive); alive=false;}
};
std::vector<std::unique_ptr<Heap>> heaps;
bool failModels=false, failScratch=false, failPin=false, compatible=true;
struct JKRSolidHeap : Heap {
    static JKRSolidHeap* create(unsigned,Heap*,bool) {
        if(failModels) return nullptr;
        auto p=std::make_unique<JKRSolidHeap>(); auto* result=p.get();
        heaps.push_back(std::move(p)); return result;
    }
};
struct JKRExpHeap : Heap {
    static JKRExpHeap* create(unsigned,Heap*,bool) {
        if(failScratch) return nullptr;
        auto p=std::make_unique<JKRExpHeap>(); auto* result=p.get();
        heaps.push_back(std::move(p)); return result;
    }
};
struct J3DModel {};
struct daAlink_c {
    unsigned id=1;
    const char* mArcName="Kmdl";
    JKRExpHeap* mpArcHeap=nullptr;
    struct {JKRSolidHeap* mAnimeHeap=nullptr;} mAnmHeap3;
    J3DModel *mpLinkModel=nullptr,*mpLinkFaceModel=nullptr,*mpLinkHatModel=nullptr,
        *mpLinkHandModel=nullptr,*mpLinkBootModels[2]{};
    struct {J3DModel* mModel=nullptr;} field_0x2e44;
    bool wolf=false,dead=false,scene=false,event=false;
    bool checkWolf() {return wolf;}
    bool checkDeadHP() {return dead;}
    bool checkSceneChangeAreaStart() {return scene;}
    bool checkEventRun() {return event;}
};
std::map<std::string,int> refs;
int dComIfG_setObjectRes(const char* arc,u8,void*) {
    if(failPin) return 0;
    ++refs[arc]; return 1;
}
int dComIfG_deleteObjectResMain(const char* arc) {
    assert(refs[arc]>0); --refs[arc]; return 1;
}
daAlink_c* current=nullptr;
daAlink_c* daAlink_getAlinkActorClass() {return current;}
unsigned fopAcM_GetID(daAlink_c* p) {return p->id;}
using OutfitModels=std::array<J3DModel*,6>;
bool warp_compatible(const OutfitModels&,daAlink_c*) {return compatible;}
'''
state = transition[transition.index('struct WarpLayer'):transition.index('bool transition_owner')]
callbacks = ''.join(function(transition, name) for name in (
    'outfit_models', 'transition_owner', 'restore_archive_heap', 'discard_transition', 'prepare_transition'))
callbacks += ''.join(function(visual, name) for name in (
    'fierce_deity_transition_busy', 'fierce_deity_transition_prepare', 'fierce_deity_transition_commit',
    'fierce_deity_transition_tick', 'fierce_deity_transition_cancel'))
checks = r'''
int main() {
    // Native human range 6.0 / native rate 0.06 = 100 ticks. Exactly twice
    // as fast means 50 simulation ticks, in either direction, not 50 draws.
    for(bool entering : {false,true}) {
        FierceWarpWipe wipe; wipe.entering=entering;
        assert(wipe.height()==(entering?5.5f:-0.5f));
        for(unsigned tick=1;tick<=50;++tick) {
            const float previous=wipe.height();
            assert(wipe.advance()==(tick==50));
            assert(std::fabs(std::fabs(wipe.height()-previous)-0.12f)<0.00001f);
            assert(wipe.inverse(false)!=wipe.inverse(true));
            assert(wipe.scroll()>=0 && wipe.scroll()<1);
        }
        assert(wipe.height()==(entering?-0.5f:5.5f));
    }
    daAlink_c link; current=&link;
    J3DModel oldModel, newModel;
    auto setup=[&]() {
        assert(!fierce_deity_transition_busy());
        heaps.clear(); refs.clear();
        failModels=failScratch=failPin=false; compatible=true;
        link={}; link.mpLinkModel=&oldModel; link.field_0x2e44.mModel=&oldModel;
        link.mAnmHeap3.mAnimeHeap=JKRSolidHeap::create(0,nullptr,false);
        link.mpArcHeap=JKRExpHeap::create(0,nullptr,false);
        refs["Kmdl"]=1;
    };
    auto nativeSwap=[&](const char* target) {
        // Mirror the native ownership operations; its temporary freeAll must
        // touch neither the outgoing archive storage nor its model instances.
        assert(link.mpArcHeap!=s_transition.archiveHeap);
        assert(link.mAnmHeap3.mAnimeHeap!=s_transition.modelHeap);
        assert(s_transition.archiveHeap->alive && s_transition.modelHeap->alive);
        --refs[link.mArcName]; ++refs[target];
        link.mArcName=target; link.mpLinkModel=&newModel;
        fierce_deity_transition_commit(&link);
        assert(link.field_0x2e44.mModel==&newModel); // no dangling joint collision
    };
    for(const char* target : {"Kmdl","Mmdl"}) {
        for(bool entering : {false,true}) {
            setup();
            auto* archiveHeap=link.mpArcHeap;
            auto* modelHeap=link.mAnmHeap3.mAnimeHeap;
            fierce_deity_transition_prepare(&link,false,true,entering);
            assert(fierce_deity_transition_busy() && refs["Kmdl"]==2);
            auto* newHeap=link.mAnmHeap3.mAnimeHeap;
            auto* scratch=link.mpArcHeap;
            nativeSwap(target);
            assert(!scratch->alive && link.mpArcHeap==archiveHeap);
            for(int i=0;i<49;++i) {
                fierce_deity_transition_tick(&link);
                assert(modelHeap->alive && archiveHeap->alive && newHeap->alive);
            }
            fierce_deity_transition_tick(&link);
            assert(!fierce_deity_transition_busy() && !modelHeap->alive);
            assert(archiveHeap->alive && newHeap->alive && refs[target]==1);
            assert(refs["Kmdl"]==(std::strcmp(target,"Kmdl")==0?1:0));
        }
    }
    // Allocation/capability failures leave all player heap pointers unchanged.
    for(int fail=0;fail<4;++fail) {
        setup();
        failModels=fail==0; failScratch=fail==1; failPin=fail==2; compatible=fail!=3;
        auto* arc=link.mpArcHeap; auto* model=link.mAnmHeap3.mAnimeHeap;
        fierce_deity_transition_prepare(&link,false,true,true);
        assert(!fierce_deity_transition_busy() && refs["Kmdl"]==1);
        assert(link.mpArcHeap==arc && link.mAnmHeap3.mAnimeHeap==model);
        for(const auto& heap:heaps)
            if(heap.get()!=arc && heap.get()!=model) assert(!heap->alive);
    }
    // Deletion, cutscenes and wolf changes must retire the outgoing references
    // before the game can clear Link's original archive heap.
    for(int reason=0;reason<4;++reason) {
        setup();
        fierce_deity_transition_prepare(&link,false,true,true);
        auto* old=s_transition.modelHeap;
        nativeSwap("Mmdl");
        if(reason==0) fierce_deity_transition_cancel(&link);
        else {
            if(reason==1) link.wolf=true;
            if(reason==2) link.scene=true;
            if(reason==3) link.event=true;
            fierce_deity_transition_tick(&link);
        }
        assert(!fierce_deity_transition_busy() && !old->alive && refs["Kmdl"]==0);
        assert(link.mpArcHeap->alive && refs["Mmdl"]==1);
    }
    // Unsupported incoming geometry still rebinds collision and frees only
    // retained outgoing data; the already-built target remains fully usable.
    setup(); fierce_deity_transition_prepare(&link,false,true,true);
    auto* old=s_transition.modelHeap;
    compatible=false; nativeSwap("Mmdl");
    assert(!old->alive && !fierce_deity_transition_busy() && refs["Mmdl"]==1);
}
'''
with tempfile.TemporaryDirectory(prefix='dawnlight-warp-') as temp:
    cpp, exe = Path(temp)/'test.cpp', Path(temp)/'test'
    cpp.write_text(fixture + state + callbacks + checks)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(root/'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Fierce Deity warp timing, heap/reference lifetime, cancellation and collision rebinding: passed')
