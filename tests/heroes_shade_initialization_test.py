"""Regression: archive mesh layout must not suppress sword/arena creation."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root/'src/heroes_shade_trials.inc').read_text()
encounter = (root/'src/heroes_shade_encounter.cpp').read_text()

def method(text, signature, end='\n    }'):
    start=text.index(signature)
    return text[start:text.index(end,start)+len(end)]

fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdint>
using u32=unsigned;using s16=std::int16_t;using ActorId=int;
constexpr int kNone=-1,cPhs_COMPLEATE_e=4,cPhs_ERROR_e=5;
constexpr u32 J3DMdlFlag_UseSharedDL=0x20000,J3DMdlFlag_DifferedDLBuffer=0x80000;
constexpr int kJ3DError_Success=0;
#define JKR_NEW_ARRAY(T,N) new T[N]
struct Joint {};
using J3DJointCallBack=int (*)(Joint*,int);
struct Shape {};
struct J3DMaterial {
    bool shared=false;Joint joint;Shape shape;
    auto getSharedDisplayListObj(){return shared ? this : nullptr;}
    auto getShape(){return &shape;}auto getJoint(){return &joint;}
};
struct Names {
    bool missing=false;
    const char* getName(unsigned i){return i==1 && !missing ? "bm6_eye" : "stone";}
};
struct J3DModelData {
    Names names;std::array<J3DMaterial,2> materials;bool locked=false;
    unsigned getJointNum(){return 7;} // no fixed six-joint layout
    unsigned getMaterialNum(){return materials.size();}
    auto getMaterialName(){return &names;}
    auto getMaterialNodePointer(unsigned i){return &materials.at(i);}
    bool isLocked(){return locked;}
    // Deliberately no getMesh/head-joint API: skeletal joints need no mesh.
};
struct J3DModel {
    u32 usedFlags=999;int differed=0;
    int entryModelData(J3DModelData* data,u32 flags,int){
        assert(flags!=J3DMdlFlag_DifferedDLBuffer || data->materials[0].shared);
        usedFlags=flags;return kJ3DError_Success;
    }
    int newDifferedDisplayList(u32){++differed;return kJ3DError_Success;}
    void lock(){}
};
struct TrialEyeModel:J3DModel {
    J3DJointCallBack* callbacks=nullptr;unsigned jointCount=0;J3DMaterial* eyeMaterial=nullptr;
    ~TrialEyeModel(){delete[] callbacks;}
'''
room = r'''
};
struct cXyz {
    float x=0,y=0,z=0;
    cXyz operator-(cXyz b)const{return {x-b.x,y-b.y,z-b.z};}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
struct ActorSpawnParams {
    int parameters,argument,room_num;cXyz position;struct{s16 x,y,z;}angle;
    cXyz scale;void* create_function;
};
int spawnCount=0;bool actorsReady=false;
constexpr int fpcNm_Obj_HsTarget_e=1;
s16 cLib_targetAngleY(const cXyz*,const cXyz*){return 0;}
bool pending_or_live(int id){return id!=kNone;}
void* actor_by_id(int id){return actorsReady && id!=kNone ? &spawnCount : nullptr;}
void create_standalone_actor(int,const ActorSpawnParams& p,int& id){
    assert(p.angle.x==0 && p.room_num==51);id=++spawnCount;
}
struct Room {
    std::array<bool,4> anchorPlaced{};std::array<bool,2> eyePlaced{};
    std::array<cXyz,4> anchorPos{};std::array<cXyz,2> eyePos{};
    std::array<int,4> anchors{kNone,kNone,kNone,kNone};
    bool radiusReady=false,missingEye=true,missingWall=true;float radius=0;cXyz center;
    bool wall(s16 angle,float height,float,cXyz& p){
        if(height==400 && missingEye) return false;
        if(height>=550 && missingWall && angle==0x2000) return false;
        p={1000,height,1000};return true;
    }
    bool find_wall(s16 a,float h,float i,cXyz& p){return wall(a,h,i,p);}
'''
assets = r'''
};
struct Heap {};
using JKRSolidHeap=Heap;
Heap rootHeap,allocatedHeap;Heap* currentHeap=&rootHeap;
int allocated=0,destroyed=0,adjusted=0;
auto mDoExt_createSolidHeapFromGame(unsigned,unsigned){++allocated;return &allocatedHeap;}
auto mDoExt_setCurrentHeap(Heap* p){auto* old=currentHeap;currentHeap=p;return old;}
void mDoExt_destroySolidHeap(Heap*){assert(currentHeap==&rootHeap);++destroyed;}
void mDoExt_adjustSolidHeap(Heap*){++adjusted;}
struct Logger {void warn(void*,const char*){}} logger;
auto* svc_log=&logger;void* mod_ctx=nullptr;
struct Effects {
    bool effectsReady=false,effectsFailed=false,heapSucceeds=true;
    int phase=0;Heap* effectHeap=nullptr;const char* failure="test";
    int load(){return phase;}bool heap(){assert(currentHeap==&allocatedHeap);return heapSucceeds;}
'''
checks = r'''
};
int main(){
    for(int shared=0;shared<2;++shared) for(int locked=0;locked<2;++locked){
        J3DModelData data;data.materials[0].shared=shared;data.locked=locked;
        TrialEyeModel eye;assert(eye.create(&data));
        assert(eye.eyeMaterial==&data.materials[1] && eye.jointCount==7 && eye.callbacks);
        assert(eye.usedFlags==(!shared ? 0 : locked ? J3DMdlFlag_UseSharedDL : J3DMdlFlag_DifferedDLBuffer));
        assert(eye.differed==(shared && !locked));
    }
    J3DModelData invalid;invalid.names.missing=true;TrialEyeModel eye;
    assert(!eye.create(&invalid));
    Room room;
    assert(!room.place() && spawnCount==3); // one missing wall/eyes don't hide the others
    assert(!room.place() && spawnCount==3); // pending actors aren't duplicated
    room.missingWall=false;
    assert(!room.place() && spawnCount==4);
    actorsReady=true;room.missingEye=false;
    assert(room.place() && spawnCount==4 && room.radiusReady);
    const auto radius=room.radius;
    assert(room.place() && spawnCount==4 && room.radius==radius);
    Effects effects;
    assert(!effects.prepare_effects() && allocated==0); // archive loading is asynchronous
    effects.phase=cPhs_COMPLEATE_e;
    assert(effects.prepare_effects() && allocated==1 && adjusted==1 && currentHeap==&rootHeap);
    assert(effects.prepare_effects() && allocated==1);
    Effects failed;failed.phase=cPhs_COMPLEATE_e;failed.heapSucceeds=false;
    assert(!failed.prepare_effects() && failed.effectsFailed && !failed.effectHeap);
    assert(currentHeap==&rootHeap && destroyed==1);
    assert(!failed.prepare_effects() && allocated==2); // no retry/allocation leak each tick
}
'''
code=(fixture+method(source,'    bool create(J3DModelData* data)')+room+
      method(source,'    bool place()')+assets+method(source,'    bool prepare_effects()')+checks)
# The arena's create transaction must depend only on the sword archive/heap.
create=method(encounter,'int create_pedestal(', '\n}')
heap=method(encounter[encounter.index('class Pedestal :'):], '    static int heap(')
assert 'trials.load(' not in create and 'trials.heap(' not in heap
assert '0x4000)' in create
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'initialization.cpp';exe=Path(tmp)/'initialization'
    cpp.write_text(code)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Hero's Shade BMDE initialization, independent target spawning and effect-heap lifetime: passed")
