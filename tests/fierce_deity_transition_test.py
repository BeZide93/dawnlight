"""Exercise native outfit swaps and independent tint instances with checked heap stubs."""
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


gate_fixture = r'''
#include <cassert>
#include <cstdint>
using u32=uint32_t; using u16=uint16_t;
constexpr unsigned GX_TG_POS=0;
struct Texture {
    void* image=nullptr;
    unsigned getNum() { return 1; }
    void* getImgDataPtr(unsigned i) { assert(i==0); return image; }
};
struct Tev {
    unsigned type='TVB4', count=1, texNo=0;
    unsigned getType() { return type; }
    unsigned getTevStageNum() { return count; }
    unsigned getTexNo(unsigned i) {
        assert(i==3 && (type=='TVB4'||type=='TVPT'||type=='TV16'));
        return texNo;
    }
};
struct Matrix { struct { unsigned mInfo=8; } info;
    auto& getTexMtxInfo() { return info; }
};
struct Coord { unsigned src=GX_TG_POS;
    unsigned getTexGenSrc() { return src; }
};
struct Gen {
    unsigned count=1; Matrix matrix; Coord coord;
    unsigned getTexGenNum() { return count; }
    Matrix* getTexMtx(unsigned i) { assert(i==count && i<4); return &matrix; }
    Coord* getTexCoord(unsigned i) { assert(i==count && i<4); return &coord; }
};
struct J3DMaterial {
    Tev tev; Gen gen;
    Tev* getTevBlock() { return &tev; }
    Gen* getTexGenBlock() { return &gen; }
};
struct Data { Texture tex; Texture* getTexture() { return &tex; } };
struct J3DModel { Data data; Data* getModelData() { return &data; } };
struct daAlink_c { void* mpWarpTexData=nullptr; };
'''
gate_checks = r'''
int main() {
    int warpImage, foreignImage;
    daAlink_c link{&warpImage}; J3DModel model; J3DMaterial material;
    model.data.tex.image=&warpImage;
    // BMWR's dormant slot follows the active generators/stages, including
    // the three-stage eye materials in the current Ichigo face replacements.
    for(unsigned type : {unsigned('TVB4'),unsigned('TVPT'),unsigned('TV16')}) {
        material.tev.type=type;
        for(unsigned count=1;count<=3;++count) {
            material.tev.count=material.gen.count=count;
            assert(native_warp_coord(&model,&material,&link)==int(count));
            material.gen.matrix.info.mInfo=0x88; // native Maya SRT flag
            assert(native_warp_coord(&model,&material,&link)==int(count));
        }
    }
    material.tev.type='TVB1'; // must reject BEFORE indexing slot 3
    assert(native_warp_coord(&model,&material,&link)<0);
    material.tev.type='TVB4'; material.tev.texNo=0xFFFF;
    assert(native_warp_coord(&model,&material,&link)<0);
    material.tev.texNo=0; model.data.tex.image=&foreignImage;
    assert(native_warp_coord(&model,&material,&link)<0);
    model.data.tex.image=&warpImage; material.gen.count=4;
    assert(native_warp_coord(&model,&material,&link)<0);
    material.gen.count=3; material.gen.coord.src=4;
    assert(native_warp_coord(&model,&material,&link)<0);
    material.gen.coord.src=0; material.gen.matrix.info.mInfo=0;
    assert(native_warp_coord(&model,&material,&link)<0);
}
'''

fixture = r'''
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <map>
#include <memory>
#include <cstdint>
#include <new>
#include <utility>
#include <string>
#include <vector>
#include "fierce_deity_wipe.hpp"
using dawnlight::FierceWarpWipe;
enum class FierceDeityTint { None, Dark, White, Gold };
using u8=unsigned char; using u32=unsigned; using fpc_ProcID=unsigned;
constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
struct Heap;
Heap* currentHeap=nullptr;
struct Heap {
    virtual ~Heap() = default;
    bool alive=true;
    Heap* becomeCurrentHeap() {auto* old=currentHeap; currentHeap=this; return old;}
    unsigned getHeapSize() const {return 0x40000;}
    Heap* getParent() {return nullptr;}
    void destroy() {assert(alive); alive=false;}
};
std::vector<std::unique_ptr<Heap>> heaps;
bool failModels=false, failScratch=false, failPin=false, compatible=true;
unsigned scratchRequested=0;
void warp_warning(const char*) {}
void stop_warp_particles() {}
struct JKRSolidHeap : Heap {
    static JKRSolidHeap* create(unsigned,Heap*,bool) {
        if(failModels) return nullptr;
        auto p=std::make_unique<JKRSolidHeap>(); auto* result=p.get();
        heaps.push_back(std::move(p)); return result;
    }
};
struct JKRHeap {static Heap* getRootHeap() {return nullptr;}};
struct JKRExpHeap : Heap {
    static JKRExpHeap* create(void*,unsigned size,Heap* parent,bool flag) {return create(size,parent,flag);}
    static JKRExpHeap* create(unsigned size,Heap*,bool) {
        scratchRequested=size;
        if(failScratch) return nullptr;
        auto p=std::make_unique<JKRExpHeap>(); auto* result=p.get();
        heaps.push_back(std::move(p)); return result;
    }
};
constexpr u32 Z2SE_AL_WARP_OUT=0x20098, Z2SE_AL_WARP_IN_TATE=0x20096;

#define JKR_NEW new
struct J3DModelData {
    bool alive=true;
    int geometry=42;
    unsigned getJointNum() {assert(alive); return 3;}
    unsigned getModelDataType() {return 0;}
};
constexpr unsigned J3DMdlFlag_UseSharedDL=0x40000, J3DMdlFlag_UseSingleDL=0x80000,
    J3DMdlFlag_DifferedDLBuffer=0x1000000;
constexpr int kJ3DError_Success=0;
struct J3DModel {
    int entryModelData(J3DModelData*,unsigned,unsigned);
    int newDifferedDisplayList(unsigned diff) {mDiffFlag=diff;return 0;}
    void lock() {}
    J3DModelData ownData;
    J3DModelData* data=&ownData;
    unsigned mFlags=0x80000, mDiffFlag=0x02000400;
    float base=1, scale=1;
    bool ownsData=false;
    ~J3DModel() {if(ownsData) delete data;}
    J3DModelData* getModelData() {assert(ownData.alive);return data;}
    float* getBaseTRMtx() {return &base;}
    float* getBaseScale() {return &scale;}
    void setBaseTRMtx(float* m) {base=*m;}
    void setBaseScale(float s) {scale=s;}
};
std::vector<std::unique_ptr<J3DModel>> copies;
int copyAttempts=0, failCopy=-1, forgotten=0, entries=0, poses=0;
int J3DModel::entryModelData(J3DModelData* incoming,unsigned flags,unsigned count) {
    assert(count==1 && currentHeap && currentHeap->alive);
    data=incoming;ownsData=true;mFlags=flags;
    copies.emplace_back(this);
    return copyAttempts++==failCopy ? -1 : kJ3DError_Success;
}
void forget_matrices(J3DModel* model) {
    assert(model->getModelData()->getJointNum()==3);++forgotten;
}
void copy_outgoing_pose(J3DModel* outgoing,J3DModel* incoming) {
    assert(outgoing!=incoming);assert(incoming->getModelData()->alive);
    assert(outgoing->getModelData()->geometry==incoming->getModelData()->geometry);++poses;
}
void mDoExt_modelEntryDL(J3DModel*) {++entries;}
struct Environment {void setLightTevColorType_MAJI(J3DModel*,int*) {}} g_env_light;

struct daAlink_c {
    unsigned soundCalls=0; u32 lastSound=0;
    void seStartOnlyReverb(u32 sound) {++soundCalls;lastSound=sound;}
    unsigned id=1;
    const char* mArcName="Kmdl";
    JKRExpHeap* mpArcHeap=nullptr;
    struct {JKRSolidHeap* mAnimeHeap=nullptr;} mAnmHeap3;
    J3DModel *mpLinkModel=nullptr,*mpLinkFaceModel=nullptr,*mpLinkHatModel=nullptr,
        *mpLinkHandModel=nullptr,*mpLinkBootModels[2]{};
    struct {J3DModel* mModel=nullptr;} field_0x2e44;
    int mClothesChangeWaitTimer=0, tevStr=0;
    bool noDraw=false,heavy=false;
    bool checkPlayerNoDraw() {return noDraw;}
    bool checkEquipHeavyBoots() {return heavy;}
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
void update_warp_particles(daAlink_c*) {}
daAlink_c* daAlink_getAlinkActorClass() {return current;}
unsigned fopAcM_GetID(daAlink_c* p) {return p->id;}
using OutfitModels=std::array<J3DModel*,6>;
bool warp_compatible(const OutfitModels&,daAlink_c*) {return compatible;}
bool player_model(daAlink_c* link, J3DModel* model) {return model && model==link->mpLinkModel;}
'''
state = transition[transition.index('struct WarpLayer'):transition.index('void warp_warning')]
callbacks = ''.join(function(transition, name) for name in (
    'outfit_models', 'transition_owner', 'restore_archive_heap', 'discard_transition', 'tint_sources_live', 'separate_outgoing', 'prepare_tint_transition', 'prepare_transition', 'warp_layer', 'draw_outgoing'))
callbacks += ''.join(function(visual, name) for name in (
    'fierce_deity_transition_busy', 'fierce_deity_transition_prepare', 'fierce_deity_transition_commit',
    'fierce_deity_transition_tick', 'fierce_deity_transition_cancel', 'fierce_deity_transition_tint'))
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
        heaps.clear(); refs.clear(); copies.clear();
        oldModel.ownData.alive=true;
        copyAttempts=0; failCopy=-1; forgotten=entries=poses=0;
        s_forgetWarpMatrices=forget_matrices;
        failModels=failScratch=failPin=false; compatible=true;
        link={}; link.mpLinkModel=&oldModel; link.field_0x2e44.mModel=&oldModel;
        link.mAnmHeap3.mAnimeHeap=JKRSolidHeap::create(0,nullptr,false);
        link.mpArcHeap=JKRExpHeap::create(0,nullptr,false);
        currentHeap=link.mpArcHeap;
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
            fierce_deity_transition_prepare(&link,FierceDeityTint::None,FierceDeityTint::Dark,entering);
            assert(fierce_deity_transition_busy() && refs["Kmdl"]==2);
            assert(scratchRequested<=0x1000); // preloaded resource needs no duplicate archive storage
            auto* newHeap=link.mAnmHeap3.mAnimeHeap;
            auto* scratch=link.mpArcHeap;
            nativeSwap(target);
            assert(!scratch->alive && link.mpArcHeap==archiveHeap);
            assert(link.soundCalls==1);
            assert(link.lastSound==(entering?Z2SE_AL_WARP_OUT:Z2SE_AL_WARP_IN_TATE));
            fierce_deity_transition_commit(&link); // repeated commit cannot replay sound
            assert(link.soundCalls==1);
            for(int i=0;i<49;++i) {
                fierce_deity_transition_tick(&link);
                assert(modelHeap->alive && archiveHeap->alive && newHeap->alive);
            }
            fierce_deity_transition_tick(&link);
            assert(!fierce_deity_transition_busy() && !modelHeap->alive);
            assert(archiveHeap->alive && newHeap->alive && refs[target]==1);
            assert(link.soundCalls==1); // no per-tick or completion replay
            assert(refs["Kmdl"]==(std::strcmp(target,"Kmdl")==0?1:0));
        }
    }
    // Allocation/capability failures leave all player heap pointers unchanged.
    for(int fail=0;fail<4;++fail) {
        setup();
        failModels=fail==0; failScratch=fail==1; failPin=fail==2; compatible=fail!=3;
        auto* arc=link.mpArcHeap; auto* model=link.mAnmHeap3.mAnimeHeap;
        fierce_deity_transition_prepare(&link,FierceDeityTint::None,FierceDeityTint::Dark,true);
        assert(!fierce_deity_transition_busy() && refs["Kmdl"]==1);
        fierce_deity_transition_commit(&link);
        assert(link.soundCalls==0); // no sound without a visual transition
        assert(link.mpArcHeap==arc && link.mAnmHeap3.mAnimeHeap==model);
        for(const auto& heap:heaps)
            if(heap.get()!=arc && heap.get()!=model) assert(!heap->alive);
    }
    // Deletion, cutscenes and wolf changes must retire the outgoing references
    // before the game can clear Link's original archive heap.
    for(int reason=0;reason<4;++reason) {
        setup();
        fierce_deity_transition_prepare(&link,FierceDeityTint::None,FierceDeityTint::Dark,true);
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
    // Both layers retain their exact palette, including same-outfit changes.
    for(auto from : {FierceDeityTint::None,FierceDeityTint::Dark,FierceDeityTint::White,FierceDeityTint::Gold})
    for(auto to : {FierceDeityTint::None,FierceDeityTint::Dark,FierceDeityTint::White,FierceDeityTint::Gold}) {
        setup(); auto* old=link.mpLinkModel; J3DModel incoming;
        fierce_deity_transition_prepare(&link,from,to,true);
        link.mpLinkModel=&incoming; fierce_deity_transition_commit(&link);
        assert(warp_layer(old).tint==from && warp_layer(old).active);
        assert(warp_layer(&incoming).tint==to && warp_layer(&incoming).active);
        assert(warp_layer(old).inverse!=warp_layer(&incoming).inverse);
        fierce_deity_transition_cancel(&link);
    }

    // Shared lists must stay borrowed; converting them to double buffering
    // would put live material storage in the disposable transition heap.
    for(unsigned flags : {J3DMdlFlag_UseSharedDL,J3DMdlFlag_DifferedDLBuffer}) {
        setup();oldModel.mFlags=flags;
        fierce_deity_transition_tint(&link,FierceDeityTint::None,FierceDeityTint::Dark,true);
        auto* copy=s_transition.outgoing[0];assert(copy);
        if(flags==J3DMdlFlag_UseSharedDL) assert(copy->mFlags & J3DMdlFlag_UseSingleDL);
        if(flags==J3DMdlFlag_DifferedDLBuffer) assert(copy->mDiffFlag==oldModel.mDiffFlag);
        fierce_deity_transition_cancel(&link);
    }
    // Cached TE models must remain live, with independent packets for BOTH
    // wipe layers, on entry and exit. No native heap swap or archive reference.
    for(bool entering : {false,true}) {
        setup(); J3DModel face, boot;
        link.mpLinkFaceModel=&face;link.mpLinkBootModels[0]=&boot;
        auto* modelHeap=link.mAnmHeap3.mAnimeHeap;auto* arc=link.mpArcHeap;
        fierce_deity_transition_tint(&link,entering?FierceDeityTint::None:FierceDeityTint::Dark,
            entering?FierceDeityTint::Dark:FierceDeityTint::None,entering);
        assert(s_transition.committed && s_transition.cloneHeap);
        auto* owned=s_transition.cloneHeap;
        assert(link.mpLinkModel==&oldModel && link.field_0x2e44.mModel==&oldModel);
        assert(link.mpArcHeap==arc && link.mAnmHeap3.mAnimeHeap==modelHeap && currentHeap==arc);
        assert(refs["Kmdl"]==1 && s_transition.modelHeap==nullptr && copies.size()==3);
        auto* copy=s_transition.outgoing[0];
        assert(copy!=&oldModel && copy->getModelData()!=oldModel.getModelData());
        assert(copy->getModelData()->geometry==oldModel.getModelData()->geometry);
        assert(warp_layer(copy).active && warp_layer(&oldModel).active);
        assert(warp_layer(copy).inverse!=warp_layer(&oldModel).inverse);
        draw_outgoing(&link);assert(entries==2 && poses==2); // unequipped boots skipped
        link.heavy=true;draw_outgoing(&link);assert(entries==5);
        link.noDraw=true;draw_outgoing(&link);assert(entries==5);
        for(int tick=0;tick<50;++tick) fierce_deity_transition_tick(&link);
        assert(!fierce_deity_transition_busy() && !owned->alive && forgotten==3);
        assert(modelHeap->alive && arc->alive && oldModel.ownData.alive && refs["Kmdl"]==1);
    }
    // Abort on source replacement (including in-place data swaps), clothing
    // reload, deletion and scene changes. Never read a retired borrowed header
    // even while clearing the host's per-instance interpolation history.
    for(int reason=0;reason<7;++reason) {
        setup();J3DModelData replacementData;
        fierce_deity_transition_tint(&link,FierceDeityTint::None,FierceDeityTint::Dark,true);
        auto* owned=s_transition.cloneHeap;
        if(reason==0) {link.mpLinkModel=&newModel;oldModel.ownData.alive=false;}
        if(reason==1) {oldModel.data=&replacementData;}
        if(reason==2) link.mClothesChangeWaitTimer=4;
        if(reason==3) link.scene=true;
        if(reason==4) link.wolf=true;
        if(reason==5) link.event=true;
        if(reason<3) {draw_outgoing(&link);assert(entries==0);assert(!warp_layer(link.mpLinkModel).active);}
        if(reason==6) fierce_deity_transition_cancel(&link);
        else fierce_deity_transition_tick(&link);
        assert(!owned->alive && forgotten==1 && !fierce_deity_transition_busy());
        assert(link.mpArcHeap->alive && link.mAnmHeap3.mAnimeHeap->alive);
        oldModel.data=&oldModel.ownData;
    }
    // Partial allocation failure and unsupported geometry are all-or-nothing.
    for(int reason=0;reason<4;++reason) {
        setup();J3DModel face;link.mpLinkFaceModel=&face;
        if(reason==0) failScratch=true;
        if(reason==1) failCopy=0;
        if(reason==2) failCopy=1;
        if(reason==3) compatible=false;
        fierce_deity_transition_tint(&link,FierceDeityTint::None,FierceDeityTint::Dark,true);
        assert(!fierce_deity_transition_busy() && currentHeap==link.mpArcHeap);
        assert(link.mpLinkModel==&oldModel && refs["Kmdl"]==1 && link.soundCalls==0);
        for(const auto& heap:heaps)
            if(heap.get()!=link.mpArcHeap && heap.get()!=link.mAnmHeap3.mAnimeHeap) assert(!heap->alive);
    }
    // Defensive coverage for the native armor path: a third-party changeLink
    // callback may reinstall an outgoing model in ANY incoming slot.
    for(bool differentSlot : {false,true}) {
        setup();fierce_deity_transition_prepare(&link,FierceDeityTint::None,FierceDeityTint::Dark,true);
        if(differentSlot) {link.mpLinkModel=&newModel;link.mpLinkHatModel=&oldModel;}
        fierce_deity_transition_commit(&link);
        assert(!fierce_deity_transition_busy());draw_outgoing(&link);assert(entries==0);
    }
    setup(); fierce_deity_transition_prepare(&link,FierceDeityTint::None,FierceDeityTint::Dark,true);
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
    cpp.write_text('#include <initializer_list>\n' + gate_fixture +
                   function(transition, 'native_warp_coord') + gate_checks)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-multichar',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Fierce Deity warp layout, tint instances, borrowed-data lifetime, allocation failures and native swaps: passed')
