"""Compile the actual warp runtime with instrumented archive/model/particle APIs."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/heroes_shade_encounter.cpp').read_text()
start=source.index('HookAction draw_warp(')
draw_hook=source[start:source.index('\n}\n',start)+3]
assert 'uninstall<ShadeDrawHook>' in source
fixture=r'''
#include "heroes_shade_cinema.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
namespace shade=dawnlight::shade;
using u32=unsigned;using u16=unsigned short;
#define JKR_NEW new
struct cXyz {float x=0,y=0,z=0;void set(float a,float b,float c){x=a;y=b;z=c;}};
struct Tev {int stages=1;int getTevStageNum(){return stages;}int getTexNo(int){return 0xffff;}};
struct Tex {int getTexGenNum(){return 1;}};
struct Material {Tev tev;Tex tex;auto getTevBlock(){return &tev;}auto getTexGenBlock(){return &tex;}};
struct Shape {cXyz max{0,300,0};auto getMax(){return &max;}};
struct Data {
    int joints=2;Material material;Shape shape;float scroll=0,cutoff=0;bool warp=false;
    int getMaterialNum(){return 1;}auto getMaterialNodePointer(int){return &material;}
    int getJointNum(){return joints;}int getShapeNum(){return 1;}
    auto getShapeNodePointer(int){return &shape;}
};
struct Buffer {
    std::array<int,2> flags{1,1};
    void setScaleFlag(int i,int flag){flags.at(i)=flag;}int getScaleFlag(int i){return flags.at(i);}
};
struct J3DModel {
    J3DModel(Data* d):data(d){}
    Data* data;Buffer buffer;std::array<int,2> matrices{41,42};int base=100,envelopes=0,draws=0;
    cXyz scale{1,1,1};
    auto getModelData(){return data;}void setBaseTRMtx(int m){base=m;}int getBaseTRMtx(){return base;}
    void setBaseScale(cXyz s){scale=s;}auto getBaseScale(){return &scale;}
    void setAnmMtx(int i,int m){matrices.at(i)=m;}int getAnmMtx(int i){return matrices.at(i);}
    auto getMtxBuffer(){return &buffer;}void calcWeightEnvelopeMtx(){++envelopes;}
};
struct Morf {J3DModel model;auto getModel(){return &model;}};
struct daNpc_Kn_c {
    std::array<Morf*,2> mpModelMorf;
    cXyz scale{1,1,1},field_0x16f4;
    int field_0x170c=0,field_0x170d=0,tevStr=0,shape_angle=0;
    bool mNoDraw=false;struct {cXyz pos{10,20,30};}current;
};
struct JKRSolidHeap {} parent,owned;using JKRHeap=JKRSolidHeap;
JKRSolidHeap* current=&parent;
int allocations=0,destroys=0,adjusts=0,reads=0,volumes=0,decoded=0;
bool failHeap=false,failRead=false,mismatch=false;
auto mDoExt_createSolidHeapFromGame(unsigned,unsigned){++allocations;return failHeap ? nullptr : &owned;}
auto mDoExt_setCurrentHeap(JKRSolidHeap* h){auto* old=current;current=h;return old;}
void mDoExt_destroySolidHeap(JKRSolidHeap*){assert(current==&parent && volumes==0);++destroys;}
void mDoExt_adjustSolidHeap(JKRSolidHeap*){assert(current==&parent);++adjusts;}
constexpr int EXPAND_SWITCH_UNKNOWN1=1,JKRMEMBREAK_FLAG_UNKNOWN0=0;
struct Raw {bool decoded=false;} raw[2];
struct JKRDvdRipper {
    enum {ALLOC_DIRECTION_FORWARD=1};
    static void* loadToMainRAM(const char* path,void*,int,unsigned,JKRSolidHeap* heap,int,int,void*,u32* size){
        assert(std::strcmp(path,"/res/Object/KN_a.arc")==0 && heap==current && current==&owned);
        ++reads;*size=sizeof(raw);raw[0]={};raw[1]={};return failRead ? nullptr : raw;
    }
};
struct JKRMemArchive {
    JKRMemArchive(void* data,u32,int){assert(data==raw);++volumes;}
    ~JKRMemArchive(){--volumes;}
    bool isMounted(){return true;}int countFile(){return 50;}bool isFileEntry(unsigned){return true;}
    void* getIdxResource(unsigned i){assert(i==47 || i==48);return &raw[i-47];}
};
struct dRes_info_c {
    static Data* loaderBasicBmd(u32 tag,void* bytes){
        assert(tag==0x424d5745 && current==&owned);
        auto* b=static_cast<Raw*>(bytes);assert(!b->decoded);b->decoded=true;++decoded;
        auto* d=new Data;d->warp=true;if(mismatch)d->joints=3;return d;
    }
    static void setWarpSRT(Data* data,cXyz,float scroll,float cutoff){
        assert(data->warp);data->scroll=scroll;data->cutoff=cutoff;
    }
};
auto mDoExt_J3DModel__create(Data* data,u32 flags,u32 diff){
    assert(data->warp && flags==0x80000 && diff==0x13020684);
    return new J3DModel{data};
}
void mDoExt_modelEntryDL(J3DModel* m){assert(m->envelopes>0);++m->draws;}
struct mDoExt_invisibleModel {
    J3DModel* model=nullptr;
    bool create(J3DModel* m,int){model=m;return true;}
    void entryDL(int){mDoExt_modelEntryDL(model);}
};
struct Light {void settingTevStruct(int,cXyz*,int*){}void setLightTevColorType_MAJI(J3DModel*,int*){}} g_env_light;
struct Emitter {int killed=0,cleared=0,finished=0;void stopCreateParticle(){++finished;}void becomeInvalidEmitter(){++killed;}void deleteAllParticle(){++cleared;}} effect;
int emissions=0,eventMoves=0,sounds=0,lastSound=0;u16 lastEffect=0;cXyz lastPos;
constexpr int Z2SE_AL_WARP_OUT=1,Z2SE_AL_WARP_IN_TATE=2;
constexpr u16 ID_ZI_J_LK_WARP_APP_A=0x9f3,ID_ZI_J_LK_WARP_DISAPP_A=0x9f4;
auto dComIfGp_particle_getEmitter(u32 id){return id ? &effect : nullptr;}
u32 dComIfGp_particle_set(u32,u16 id,cXyz* p,int*,int*,void*,int,void*,int,void*,void*,void*){
    lastEffect=id;lastPos=*p;++emissions;return 7;
}
void dComIfGp_particle_levelEmitterOnEventMove(u32 id){assert(id==7);++eventMoves;}
int dComIfGp_getReverb(int){return 0;}
void mDoAud_seStart(int id,cXyz*,int,int){++sounds;lastSound=id;}
int warnings=0;struct Log {void warn(void*,const char*){++warnings;}} logService;
auto* svc_log=&logService;void* mod_ctx=nullptr;
#include "heroes_shade_warp.inc"
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int){return *static_cast<T*>(args);}}
struct Fighter {bool divide=false,deleting=false;} entry;
bool registered=true;
Fighter* fighter(daNpc_Kn_c*){return registered ? &entry : nullptr;}
shade::Cinema sCinema;
// DRAW_HOOK
HookAction draw_after_native_execute(daNpc_Kn_c* actor){
    actor->mNoDraw=false; // twilight() runs after our pre-execute tick
    int result=0;
    auto action=draw_warp(nullptr,&actor,&result,nullptr);
    if(action==HOOK_SKIP_ORIGINAL) assert(result==1);
    return action;
}
int main(){
    Data original[2];Morf morph[2]{{{&original[0]}},{{&original[1]}}};
    daNpc_Kn_c actor{};actor.mpModelMorf={&morph[0],&morph[1]};
    sCinema.begin(false);
    assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL); // before sound/particles
    assert(sounds==0 && emissions==0);
    registered=false;assert(draw_after_native_execute(&actor)==HOOK_CONTINUE);registered=true;
    entry.divide=true;assert(draw_after_native_execute(&actor)==HOOK_CONTINUE);entry.divide=false;
    sCinema.begin(true);assert(draw_after_native_execute(&actor)==HOOK_CONTINUE); // victory recovery stays visible
    sCinema.begin(false);sCinema.enter(shade::Shot::Arrival);
    actor.field_0x16f4={0,0,0};begin_shade_warp(&actor,true);
    assert(sShadeWarp.ready && sShadeWarp.active && actor.mNoDraw);
    assert(allocations==1 && adjusts==1 && reads==1 && decoded==2 && current==&parent && volumes==1);
    assert(actor.field_0x16f4.x==1 && actor.field_0x170c==0 && lastSound==Z2SE_AL_WARP_OUT);
    float previous=-0.5f;
    for(int tick=0;tick<=145;++tick){
        tick_shade_warp(&actor,tick);
        assert(lastEffect==0x9f3 && lastPos.y==actor.current.pos.y);
        if(tick<=45) assert(sShadeWarp.cutoff==-0.5f);
        else assert(sShadeWarp.cutoff>previous);
        previous=sShadeWarp.cutoff;
        const int count=emissions;
        const int draws=sShadeWarp.models[0]->draws;
        assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL);
        assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL); // draw cannot advance time/effects
        assert(sShadeWarp.models[0]->draws==draws+(tick<=45 ? 0 : 2));
        assert(emissions==count && sShadeWarp.cutoff==previous);
        for(int i=0;i<2;++i){
            assert(!original[i].warp && original[i].scroll==0 && original[i].cutoff==0);
            auto* model=sShadeWarp.models[i];assert(model->data!=&original[i]);
            if(tick>45){
                assert(model->matrices==morph[i].model.matrices && model->base==100);
                assert(model->data->cutoff==sShadeWarp.cutoff);
            }
        }
    }
    assert(std::abs(sShadeWarp.cutoff-(sShadeWarp.height/30-0.5f))<0.001f);
    end_shade_warp();assert(!sShadeWarp.active && !sShadeWarp.draw(&actor) && !sShadeWarp.emitter);
    sCinema.enter(shade::Shot::Words1);
    assert(draw_after_native_execute(&actor)==HOOK_CONTINUE);
    int stops=effect.killed;end_shade_warp();assert(effect.killed==stops);
    begin_shade_warp(&actor,false);
    assert(reads==1 && decoded==2 && lastSound==Z2SE_AL_WARP_IN_TATE && !actor.mNoDraw);
    sCinema.enter(shade::Shot::Depart);
    previous=sShadeWarp.height;
    for(int tick=0;tick<=100;++tick){
        tick_shade_warp(&actor,tick);
        assert(lastEffect==0x9f4 && lastPos.y<=previous+actor.current.pos.y);
        previous=lastPos.y-actor.current.pos.y;
        const int draws=sShadeWarp.models[0]->draws;
        assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL);
        assert(sShadeWarp.models[0]->draws==draws+(tick==100 ? 0 : 1));
    }
    assert(sShadeWarp.cutoff==-0.5f && lastPos.y==actor.current.pos.y);
    stops=effect.killed;finish_shade_warp();
    assert(!sShadeWarp.active && sShadeWarp.emitter==7 && effect.finished==1 && effect.killed==stops);
    sCinema.enter(shade::Shot::Afterglow);
    assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL);
    sCinema={};entry.deleting=true;
    assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL);
    entry.deleting=false;assert(draw_after_native_execute(&actor)==HOOK_CONTINUE);
    sShadeWarp.destroy();assert(!sShadeWarp.ready && !sShadeWarp.heap && volumes==0 && destroys==1);
    sShadeWarp.destroy();assert(destroys==1);
    // Failure never blocks the cinematic, re-parses native data or retries every frame.
    for(int failure=0;failure<4;++failure){
        failHeap=failure==0;failRead=failure==1;mismatch=failure==2;
        original[0].material.tev.stages=failure==3 ? 4 : 1;
        begin_shade_warp(&actor,true);assert(sShadeWarp.failed && !sShadeWarp.ready && sShadeWarp.active);
        for(int tick=0;tick<=145;++tick){
            tick_shade_warp(&actor,tick);
            assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL);
        }
        int loads=reads;end_shade_warp();
        assert(draw_after_native_execute(&actor)==HOOK_CONTINUE);
        begin_shade_warp(&actor,false);assert(reads==loads);
        tick_shade_warp(&actor,0);assert(draw_after_native_execute(&actor)==HOOK_CONTINUE);
        tick_shade_warp(&actor,50);assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL);
        tick_shade_warp(&actor,100);assert(draw_after_native_execute(&actor)==HOOK_SKIP_ORIGINAL);
        sShadeWarp.destroy();assert(current==&parent && volumes==0);
    }
    assert(warnings==4 && eventMoves==emissions);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'warp.cpp';exe=Path(tmp)/'warp'
    cpp.write_text(fixture.replace('// DRAW_HOOK',draw_hook))
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Hero's Shade draw gating, private warp models, native particles, pacing, replay and cleanup: passed")
