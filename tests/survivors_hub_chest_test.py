"""Exercise the actual hub actor creation, draw, pending spawn and resource release."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/heroes_shade_encounter.cpp').read_text()
def extract(pattern):
 m=re.search(pattern,source,re.M);assert m,pattern
 a=source.index('{',m.start());depth=0
 for i in range(a,len(source)):
  depth+=(source[i]=='{')-(source[i]=='}')
  if not depth:return source[m.start():i+1]
fixture=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include "fierce_deity_equipment_scope.hpp"
namespace dark_link=dawnlight::dark_link;
using s16=int16_t;using u16=uint16_t;using ActorId=unsigned;
constexpr int MOD_OK=0,TRUE=1,cPhs_COMPLEATE_e=4,cPhs_ERROR_e=5;
constexpr const char* kSwordArchive="MstrSword";
constexpr int dRes_INDEX_MSTRSWORD_BMD_O_AL_SWM_e=1,dRes_INDEX_MSTRSWORD_BTK_O_AL_SWM_e=2,dRes_INDEX_MSTRSWORD_BRK_O_AL_SWM_e=3;
struct cXyz{float x=0,y=0,z=0;};
struct fopAc_ac_c{unsigned param=2;struct{cXyz pos;}current;struct{s16 y=0;}shape_angle;struct{int flags=0;}attention_info;};
unsigned fopAcM_GetParam(fopAc_ac_c* p){return p->param;}
#define fopAcM_ct(p,t) ((void)0)
struct request_of_phase_process_class{bool loaded=false;};
int references=0,phaseResult=4,modelCreates=0,draws=0,forgets=0,heapCreates=0;
bool heapFail=false,modelFail=false,animationFail=false;
std::string archive;
int dComIfG_resLoad(request_of_phase_process_class* p,const char* a){archive=a;if(!p->loaded){p->loaded=true;++references;}return phaseResult;}
void dComIfG_resDelete(request_of_phase_process_class* p,const char* a){assert(archive==a);if(p->loaded){p->loaded=false;--references;}}
struct Joint{void* calc=reinterpret_cast<void*>(123);void* getMtxCalc(){return calc;}void setMtxCalc(void* p){calc=p;}};
struct Shape{cXyz min{0,-10,0};cXyz* getMin(){return &min;}};
struct J3DModelData{Joint joint;Shape shape;unsigned getShapeNum(){return 1;}Shape* getShapeNodePointer(int){return &shape;}Joint* getJointNodePointer(int){return &joint;}}modelData;
struct J3DModel{J3DModelData* getModelData(){return &modelData;}void setBaseTRMtx(void*){}void* getBaseTRMtx(){return this;}}model;
struct J3DAnmTransform{}anm;struct J3DAnmTextureSRTKey{};struct J3DAnmTevRegKey{};
void* dComIfG_getObjectRes(const char* a,int n){assert(std::string(a)=="Dalways");assert(n==13||n==8);return n==13?static_cast<void*>(&modelData):static_cast<void*>(&anm);}
J3DModel* mDoExt_J3DModel__create(J3DModelData*,unsigned,unsigned){++modelCreates;return modelFail?nullptr:&model;}
namespace J3DFrameCtrl{enum{EMode_NONE,EMode_LOOP};}
struct mDoExt_bckAnm{bool init(J3DAnmTransform*,int,int,int,int,int,bool){return !animationFail;}void entry(J3DModelData* d,int frame){assert(frame==0);d->joint.calc=this;}};
struct mDoExt_btkAnm{template<class... T>bool init(T...){return true;}};using mDoExt_brkAnm=mDoExt_btkAnm;
struct PlinthPacket{cXyz position;};struct EnergyPacket{};
struct dCcD_Stts{void Init(int,int,fopAc_ac_c*){}};
struct dCcD_Cyl{int height=0;void SetStts(dCcD_Stts*){}void SetCoSPrm(int){}void SetC(cXyz){}void SetR(int){}void SetH(int h){height=h;}};
struct ShadeTrials{void init(fopAc_ac_c*){assert(false);}};ShadeTrials* sTrials=nullptr;
namespace mDoMtx_stack_c{float y=0;int yaw=0;void transS(float,float height,float){y=height;}void YrotM(s16 a){yaw=a;}void* get(){return nullptr;}}
void fopAcM_SetMtx(fopAc_ac_c*,void*){}
int fopAcM_entrySolidHeap(fopAc_ac_c* a,int(*fn)(fopAc_ac_c*),int size){assert(size==0x10000);++heapCreates;return !heapFail&&fn(a);}
void* mod_ctx=nullptr;
void forget(J3DModel* p){assert(p==&model);++forgets;}
int resolveSymbol(void*,const char* name,void** p,void*){assert(std::string(name)=="J3DModel::forgetMtx");*p=reinterpret_cast<void*>(&forget);return MOD_OK;}
struct Hook{decltype(&resolveSymbol) resolve;}hook{resolveSymbol};auto* svc_hook=&hook;
void mDoExt_modelUpdateDL(J3DModel*){assert(dark_link::WorldModelDrawScope::active());assert(modelData.joint.calc!=reinterpret_cast<void*>(123));++draws;}
constexpr unsigned kNone=~0u;unsigned sSurvivorsChest=kNone,sRegistration=1,sProfile=794;
bool hub=true,pending=false;fopAc_ac_c* live=nullptr;unsigned spawns=0,removes=0;
bool survivors_hub_active(){return hub;}
bool pending_or_live(unsigned id){return id!=kNone&&(pending||live);}
fopAc_ac_c* actor_by_id(unsigned id){return id!=kNone&&!pending?live:nullptr;}
void remove_actor(unsigned){pending=false;live=nullptr;sSurvivorsChest=kNone;++removes;}
int spawn(unsigned profile,cXyz p,s16 yaw,unsigned param,unsigned& id){assert(profile==794&&p.x==500&&yaw==7&&param==2);++spawns;id=100;pending=true;return MOD_OK;}
'''
main=r'''
int main(){
 cXyz anchor{500,40,0},out;
 hub=false;update_survivors_equipment_chest(true,anchor,7);assert(!spawns);
 hub=true;update_survivors_equipment_chest(true,anchor,7);assert(spawns==1&&pending);
 update_survivors_equipment_chest(true,anchor,7);assert(spawns==1&&!survivors_equipment_chest_anchor(out));
 update_survivors_equipment_chest(false,{},0);assert(!pending&&removes==1);
 update_survivors_equipment_chest(true,anchor,7);assert(spawns==2);
 {
  Pedestal p;p.current.pos=anchor;p.shape_angle.y=7;live=&p;pending=false;
  assert(!survivors_equipment_chest_anchor(out));
  phaseResult=1;assert(create_pedestal(&p)==1&&!p.ready&&references==1&&heapCreates==0);
  phaseResult=4;assert(create_pedestal(&p)==4&&p.ready&&p.equipment);
  assert(modelCreates==1&&heapCreates==1&&p.collision.height==120&&!sTrials);
  assert(mDoMtx_stack_c::y==50&&mDoMtx_stack_c::yaw==7);
  assert(survivors_equipment_chest_anchor(out)&&out.x==500&&out.y==40);
  assert(create_pedestal(&p)==4&&modelCreates==1);
  // The actor draw runs without Link, loot state or native chest actor/events.
  assert(draw_equipment_chest(&p)==1&&draws==1);
  assert(!dark_link::WorldModelDrawScope::active()&&modelData.joint.calc==reinterpret_cast<void*>(123));
  update_survivors_equipment_chest(true,anchor,7);assert(spawns==2);
  update_survivors_equipment_chest(false,{},0);assert(!survivors_equipment_chest_anchor(out));
 }
 assert(references==0&&forgets==1);
 for(int failure=0;failure<4;++failure){
  heapFail=failure==0;modelFail=failure==1;animationFail=failure==2;phaseResult=failure==3?5:4;
  {Pedestal p;assert(create_pedestal(&p)==5&&!p.ready);}
  assert(references==0&&forgets==1);
 }
}
'''
code=fixture+extract(r'^class Pedestal :')+';\n'
for name in ['create_pedestal','draw_equipment_chest','update_survivors_equipment_chest','survivors_equipment_chest_anchor']:
 code+=extract(r'^(?:int|void|bool) '+name+r'\(')+'\n'
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(code+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors hub actor: async creation, independent draw, anchor, pending cancellation and resource failures passed')
