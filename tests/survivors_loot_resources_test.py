"""Production treasure model allocation/teardown and shared chest animation ownership."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_loot.inc').read_text()
treasure=(root/'src/survivors_treasure.inc').read_text()
draw=treasure[treasure.index('HookAction before_treasure_draw'):]


def extract(name):
    match=re.search(r'^(?:bool|void) '+name+r'\(',source,re.M)
    assert match
    start=source.index('{',match.start())
    depth=0
    for end in range(start,len(source)):
        depth+=(source[end]=='{')-(source[end]=='}')
        if not depth:return source[match.start():end+1]
    raise AssertionError(name)


stubs=r'''
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using s16=short;using u32=unsigned;
constexpr int MOD_OK=0,TRUE=1,cPhs_ERROR_e=-1,cPhs_COMPLEATE_e=4;
constexpr unsigned dRes_ID_ALANM_BMD_AL_IB_e=0x311,dRes_ID_ALANM_BMD_AL_BOW_e=0x312,
 dRes_ID_ALANM_BCK_BARELORD_e=1,dRes_ID_ALANM_BCK_BASHOOT_e=2;
struct ModContext{};ModContext* mod_ctx=nullptr;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
    cXyz operator+(cXyz p)const{return {x+p.x,y+p.y,z+p.z};}
    float absXZ()const{return std::sqrt(x*x+z*z);}};
s16 cM_atan2s(float a,float b){return short(std::atan2(a,b)*32768/3.14159265f);}
struct GXColor{int r,g,b,a;};void dDbVw_drawLineXlu(cXyz&,cXyz&,GXColor,int,int){}
struct J3DJoint {void* callback=reinterpret_cast<void*>(2);
    void* getCallBack(){return callback;}void setCallBack(void* cb){callback=cb;}void* calculator=reinterpret_cast<void*>(1);void* getMtxCalc(){return calculator;}
    void setMtxCalc(void* p){calculator=p;}};
struct J3DModelData{J3DJoint root;J3DJoint* getJointNodePointer(int){return &root;}}data[5];
struct J3DAnmTransform{}lidData;
struct JKRSolidHeap {std::vector<void*>allocations;};
JKRSolidHeap defaultHeap,*currentHeap=&defaultHeap;
unsigned forgets=0,heapCreates=0,heapDestroys=0,draws=0,modelsMade=0;
int failModel=-1;bool failHeap=false,missingLid=false,waiting=false,resourceError=false;
struct J3DModel {J3DModelData* data;bool forgotten=false;
    explicit J3DModel(J3DModelData* d):data(d){}
    J3DModelData* getModelData(){return data;}
    void* matrix=nullptr;cXyz scale{};
    void setBaseTRMtx(void* m){matrix=m;}void setBaseScale(cXyz s){scale=s;}
    void* getBaseTRMtx(){return matrix;}cXyz* getBaseScale(){return &scale;}
};
std::vector<J3DModel*>liveModels;
void forget(J3DModel* m){assert(!m->forgotten);m->forgotten=true;++forgets;}
struct Hooks {int (*resolve)(ModContext*,const char*,void**,void*);};
int resolve(ModContext*,const char*,void** out,void*){*out=reinterpret_cast<void*>(&forget);return MOD_OK;}
Hooks hooks{resolve};Hooks* svc_hook=&hooks;
struct Log{void warn(ModContext*,const char*){}}logService;Log* svc_log=&logService;
struct request_of_phase_process_class {bool held=false;};unsigned references=0;
int dComIfG_resLoad(request_of_phase_process_class* p,const char*){
    if(!p->held){p->held=true;++references;}return resourceError?cPhs_ERROR_e:waiting?1:cPhs_COMPLEATE_e;}
void dComIfG_resDelete(request_of_phase_process_class* p,const char*){if(p->held){p->held=false;--references;}}
void* dComIfG_getObjectRes(const char* arc,int id){
    if(std::strcmp(arc,"Dalways")==0){if(id==7)return missingLid?nullptr:&lidData;assert(id==12);return &data[0];}
    if(id==0x13)return &lidData;
    if(id==0x34)return &data[0];
    assert(id==0x1c || id==0x1f || id==0x1e);return &data[id==0x1c?1:id==0x1f?2:3];
}
void* dComIfGp_getAnmArchive(){return &lidData;}
JKRSolidHeap* mDoExt_createSolidHeapFromGame(unsigned,int){if(failHeap)return nullptr;++heapCreates;return new JKRSolidHeap;}
JKRSolidHeap* mDoExt_setCurrentHeap(JKRSolidHeap* p){auto* old=currentHeap;currentHeap=p;return old;}
void mDoExt_adjustSolidHeap(JKRSolidHeap*){}
void mDoExt_destroySolidHeap(JKRSolidHeap* heap){
    assert(currentHeap!=heap);
    for(auto* m:liveModels){assert(m->forgotten);delete m;}liveModels.clear();
    for(auto* p:heap->allocations)::operator delete(p);
    delete heap;++heapDestroys;
}
J3DModel* mDoExt_J3DModel__create(J3DModelData* d,unsigned,unsigned){
    assert(currentHeap!=&defaultHeap);if(int(modelsMade++)==failModel)return nullptr;
    auto* m=new J3DModel(d);liveModels.push_back(m);return m;
}
namespace dRes_info_c{void offWarpMaterial(J3DModelData* d){assert(d==&data[4]);}}
struct J3DFrameCtrl{enum{EMode_NONE=0};};
struct mDoExt_bckAnm {
    static void* operator new(size_t size){auto* p=::operator new(size);currentHeap->allocations.push_back(p);return p;}
    bool init(J3DAnmTransform* a,int,int,float,int,int,bool){return a;}
    void entry(J3DModelData* d,float){d->root.setMtxCalc(this);}
    float getEndFrame(){return 30;}
};
#define JKR_NEW new
J3DModel nativeSword(&data[4]);
struct daAlink_c {struct{cXyz pos;}current;J3DModel* mpSwMModel=&nativeSword;
    static const char* getAlinkArcName(){return "Alink";}
    J3DModelData* loadAramBmd(unsigned id,unsigned size){assert(currentHeap!=&defaultHeap && ((id==0x311 && size==0x2800)||(id==0x312&&size==0x4c00)));return &data[4];}
    void* loadAram(unsigned,unsigned size){assert(size==0x4000);return &lidData;}
}player;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
namespace mDoMtx_stack_c{void transS(cXyz){}void YrotM(s16){}void XrotM(s16){}void* get(){return nullptr;}}
struct dKy_tevstr_c{};
struct Light{void settingTevStruct(int,cXyz*,dKy_tevstr_c*){}void setLightTevColorType_MAJI(J3DModel*,dKy_tevstr_c*){}}g_env_light;
void mDoExt_modelUpdateDL(J3DModel*){++draws;}
struct Attack{bool active=false,exploding=false;unsigned weapon=0,age=0;cXyz position,velocity;};
std::array<Attack,projectile_capacity>s_attacks;Run s_run;
void cancel_treasure_demo(){}
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct leafdraw_class{int id=42;};
int fopAcM_GetID(leafdraw_class* p){return p->id;}
namespace mods{template<class T>T arg(void* a,int){return static_cast<T>(a);}}
struct {bool retiring=false,started=true;int item=42;}s_treasure;
J3DModel placeholder(&data[0]);
struct Demo {J3DModel* mpModel=&placeholder;bool dead=false,visible=true;struct{cXyz pos;}current;
    bool chkDead(){return dead;}bool chkDraw(){return visible;}}prop;
Demo* treasure_actor(){return &prop;}bool owns_treasure_event(){return true;}

'''
globals=source[source.index('constexpr unsigned kChestCount'):source.index('void clear_loot()')]
main=r'''
int main(){
    waiting=true;assert(!prepare_loot(&player) && references==2 && heapCreates==0);
    clear_loot();assert(references==0 && heapDestroys==0);
    waiting=false;resourceError=true;assert(!prepare_loot(&player) && s_loot.failed && references==0);
    resourceError=false;clear_loot();failHeap=true;assert(!prepare_loot(&player) && references==0);
    failHeap=false;clear_loot();missingLid=true;assert(!prepare_loot(&player) && references==0 && liveModels.empty());
    missingLid=false;clear_loot();failModel=int(modelsMade)+3;
    assert(!prepare_loot(&player) && references==0 && liveModels.empty());
    failModel=-1;clear_loot();assert(prepare_loot(&player) && references==2 && liveModels.size()==25);
    assert(currentHeap==&defaultHeap);
    const auto creations=heapCreates;assert(prepare_loot(&player) && heapCreates==creations);
    s_chests[0].active=true;s_chests[1].active=true;s_chests[1].opening=true;s_chests[1].age=45;
    const auto before=data[0].root.getMtxCalc();draw_loot();
    assert(draws==2 && data[0].root.getMtxCalc()==before);
    s_attacks[12].active=true;s_attacks[12].weapon=unsigned(Upgrade::Boomerang);s_attacks[12].age=40;
    auto oldCallback=data[0].root.getCallBack();draw_loot();
    assert(data[0].root.getCallBack()==oldCallback&&data[0].root.getMtxCalc()==before);
    s_selectedChest=0;s_chests[0].reward=ChestReward::MasterSword;
    leafdraw_class process;int result=0;auto beforeDraw=draws;
    placeholder.matrix=reinterpret_cast<void*>(123);placeholder.scale={0.5f,0.5f,0.5f};
    assert(before_treasure_draw(nullptr,&process,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
    assert(draws==beforeDraw+1&&s_loot.masterSword->matrix==placeholder.matrix&&s_loot.masterSword->scale.x==0.5f);
    assert(nativeSword.matrix==nullptr); // Never mutate Link's equipment instance.
    process.id=43;assert(before_treasure_draw(nullptr,&process,&result,nullptr)==HOOK_CONTINUE);
    process.id=42;prop.visible=false;beforeDraw=draws;
    assert(before_treasure_draw(nullptr,&process,&result,nullptr)==HOOK_SKIP_ORIGINAL&&draws==beforeDraw);
    clear_loot();assert(references==0 && liveModels.empty() && !s_loot.ready && !s_chests[0].active);
    const auto destroys=heapDestroys;clear_loot();assert(heapDestroys==destroys);
    assert(prepare_loot(&player));clear_loot();assert(heapCreates==heapDestroys);
    std::cout<<"Survivors loot resources: async load, failures, heap ownership, shared animation and cleanup passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(stubs+globals+'\n'.join(extract(n) for n in ['clear_loot','prepare_loot','draw_loot_model','draw_loot'])+draw+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
