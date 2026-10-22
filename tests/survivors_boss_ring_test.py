"""Boss Rush barrier reuse: real native visibility/collision and run ownership."""
from pathlib import Path
import os, subprocess, tempfile, sys
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1]) if len(sys.argv)>1 else root/'dusklight'
def function(text, signature):
    start=text.index(signature);body=text.index('{',start);depth=0
    for end in range(body,len(text)):
        depth+=(text[end]=='{')-(text[end]=='}')
        if not depth:return text[start:end+1]
    raise AssertionError(signature)
native=function((sdk/'src/d/actor/d_a_obj_gb.cpp').read_text(),'static int daObj_Gb_Execute(')
shared=(root/'src/ganondorf_barrier.hpp').read_text().replace('#pragma once','').replace('#include "d/actor/d_a_obj_gb.h"','').replace('#include "m_Do/m_Do_ext.h"','')
terrain=(root/'src/survivors_boss_ring_terrain.inc').read_text()
production=(root/'src/survivors_boss_ring.inc').read_text()
core=(root/'src/survivors.cpp').read_text()
assert 'if(id==s_bossRing.id)return process_boss_ring(actor,args,result);' in function(core,'HookAction before_process(')
assert 'retire_boss_ring();' in function(core,'void retire_enemies(')
assert 'cleanup_boss_ring();' in function(core,'void cleanup_enemies(')
assert 'update_boss_ring();' in function(core,'void update_survivors(')
assert 'shutdown_boss_ring();' in function(core,'void shutdown_survivors(')
hub=(root/'src/new_save_modes.cpp').read_text()
assert 'configure_ganondorf_barrier(barrier);' in function(hub,'void arm_ganondorf_barrier(obj_gb_class* barrier) {')
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <limits>
#include <set>
using ActorId=unsigned;using u32=unsigned;using s16=short;using u16=unsigned short;using s8=signed char;using u8=unsigned char;constexpr ActorId kNone=~0u;
constexpr int MOD_OK=0,MOD_UNAVAILABLE=1,fpcNm_OBJ_GB_e=10;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator-(const cXyz& p)const{return{x-p.x,y-p.y,z-p.z};}float absXZ()const{return std::hypot(x,z);}};
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,unsigned i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
using process_method_func=int;struct process_method_class{int execute_method=1,delete_method=2;}methods;
struct fopAc_ac_c{ActorId id=kNone;int profile=0,health=100;void* sub_method=&methods;
 struct{cXyz pos;struct{int y=0;}angle;}current;cXyz scale{1,1,1};};
using daAlink_c=fopAc_ac_c;
struct Matrix{float x=1,y=1,z=1;};
namespace mDoMtx_stack_c{Matrix matrix;void transS(float,float,float){matrix={};}void transM(float,float,float){}
 void YrotM(int){}void scaleM(float x,float y,float z){matrix.x*=x;matrix.y*=y;matrix.z*=z;}
 Matrix* get(){return &matrix;}}
void MTXCopy(Matrix* a,Matrix& b){b=*a;}

constexpr int GX_F32=4,GX_S16=3,GX_U16=2,GX_S8=1,GX_U8=0,GX_VA_POS=9;
const std::array<cXyz,8> sourcePoints{{{0,0,1000},{0,300,1000},{1000,0,0},{1000,300,0},
 {0,0,-1000},{0,300,-1000},{-1000,0,0},{-1000,300,0}}};
struct VertexData {
 std::array<unsigned char,128> source{};unsigned count=8,stride=16,frac=0;int type=GX_F32;
 VertexData(){load(false);}void load(bool shortFormat){type=shortFormat?GX_S16:GX_F32;stride=shortFormat?8:16;frac=shortFormat?2:0;
 source.fill(0x57);for(unsigned i=0;i<count;++i){const float v[3]={sourcePoints[i].x,sourcePoints[i].y,sourcePoints[i].z};
 for(unsigned a=0;a<3;++a){if(shortFormat){s16 n=s16(v[a]*4);std::memcpy(source.data()+i*stride+a*2,&n,2);}
 else std::memcpy(source.data()+i*stride+a*4,&v[a],4);}}}
 void* getVtxPosArray()const{return const_cast<unsigned char*>(source.data());}
 unsigned getVtxNum()const{return count;}unsigned getVtxArrNum(int)const{return count;}
 unsigned getVtxArrStride(int)const{return stride;}unsigned getVtxPosFrac()const{return frac;}int getVtxPosType()const{return type;}
};
struct ModelData{VertexData vertex;const VertexData& getVertexData(){return vertex;}};
struct VertexBuffer{void* current=nullptr;void* original=nullptr;void setCurrentVtxPos(void* p){current=p;}void frameInit(){current=original;}};
struct J3DModel;using J3DCalcCallBack=void(*)(J3DModel*,u32);
struct J3DModel{ModelData data;VertexBuffer buffer;J3DCalcCallBack mCalcCallBack=nullptr;
 J3DModel(){buffer.original=data.vertex.source.data();buffer.frameInit();}
 void setBaseTRMtx(Matrix*){}ModelData* getModelData(){return &data;}VertexBuffer* getVertexBuffer(){return &buffer;}
 void calc(){buffer.frameInit();if(mCalcCallBack)mCalcCallBack(this,0);}
};using Model=J3DModel;

struct Btk{void play(){}};struct Brk{float frame=0;void setFrame(float f){frame=f;}};

struct Bg{struct{const cXyz* m_v_tbl=sourcePoints.data();}source;
 std::array<cXyz,8> vertices=sourcePoints;unsigned moves=0;bool locked=false,noCalc=false;int mNeedsFullTransform=0;
 void Move(){if(locked)return;++moves;if(noCalc){assert(mNeedsFullTransform==1);return;}
 for(unsigned i=0;i<vertices.size();++i){vertices[i]=sourcePoints[i];vertices[i].y*=mDoMtx_stack_c::matrix.y;}}
 int GetVtxNum(){return int(vertices.size());}cXyz* GetVtxTbl(){return vertices.data();}auto* GetBgd(){return &source;}
 void SetNoCalcVtx(){noCalc=true;}void SetLock(){locked=true;}
};

struct obj_gb_class:fopAc_ac_c{int field_0x58e=0,field_0x58a[2]{},mIsFinalBattle=1,field_0x57c=0;
 int mSw1=0,mSw2=0,mColorAlpha=0;float mBrkFrame=0;cXyz field_0x594[20];Matrix mBgMtx;
 Model model;Btk btk;Brk brk;Bg bg;Model* mModel=&model;Btk* mBtk=&btk;Brk* mBrk=&brk;Bg* mpBgW=&bg;};

unsigned groundQueries=0;int groundMode=0;
struct dBgS_GndChk{ActorId ignored=kNone;cXyz pos;void SetActorPid(ActorId id){ignored=id;}void SetPos(cXyz* p){pos=*p;}};
struct GroundService{float GroundCross(dBgS_GndChk* check){++groundQueries;assert(check->ignored!=kNone);
 if(groundMode==1)return 20.f+(check->pos.x-100)*0.5f; // Steep hill: 1,500 units across the diameter.
 if(groundMode==2)return check->pos.x>100?-1.e9f:120.f; // Cliff / missing half-circle.
 if(groundMode==3)return std::numeric_limits<float>::quiet_NaN();
 return 20.f;}}groundService;
auto& dComIfG_Bgsp(){return groundService;}
float cullTop=0;void fopAcM_setCullSizeBox(fopAc_ac_c*,float,float,float,float,float top,float){cullTop=top;}
struct Audio{template<class...T>void seStart(T...){}template<class...T>void seStartLevel(T...){}}audio;
Audio* Z2GetAudioMgr(){return &audio;}constexpr int Z2SE_OBJ_GANON_BARRIER_APPR=1,Z2SE_OBJ_GANON_BARRIER=2;
void cLib_addCalc2(float* value,float target,float,float step){*value+=std::clamp(target-*value,-step,step);}
fopAc_ac_c player;auto* dComIfGp_getPlayer(int){return &player;}auto* daAlink_getAlinkActorClass(){return &player;}
int fopAcM_GetRoomNo(fopAc_ac_c*){return 0;}ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
const char* stage="D_DLBR0";const char* dComIfGp_getStartStageName(){return stage;}
bool dComIfGs_isSwitch(int,int){return false;} // No switch writes are provided.
std::map<ActorId,fopAc_ac_c*> actors;std::set<ActorId> pending;
bool fpcM_IsCreating(ActorId id){return pending.contains(id);}
fopAc_ac_c* actor_by_id(ActorId id){return !pending.contains(id)&&actors.contains(id)?actors[id]:nullptr;}
struct Enemy{ActorId id=kNone;bool elite=false,initialized=false,dormant=false,retiring=false,credited=false;};
std::array<Enemy,4>s_enemies;
enum class Phase{Loading,Ready,Running,Upgrade,TreasureChoice,Treasure,Victory,Defeat};
struct{Phase phase=Phase::Running;}s_run;
bool active=true,s_hub=false,s_victory=false,s_deathPending=false,transition=false,playing=true,floorValid=true;
bool in_arena(){return active;}bool transitioning(){return transition;}bool playable(){return playing;}
bool loot_floor_position(cXyz& p){p.y=20;return floorValid;}
struct{int room=7;}map;const auto& arena(){return map;}
struct ActorSpawnParams{u32 parameters;int argument,room_num; cXyz position;
 struct{int x,y,z;}angle;cXyz scale;void* create_function;};
unsigned creates=0,deletes=0;ActorSpawnParams created{};bool failCreate=false;
int create_standalone_actor(int profile,const ActorSpawnParams& params,ActorId& id){
 assert(profile==fpcNm_OBJ_GB_e);++creates;created=params;
 if(failCreate)return MOD_UNAVAILABLE;
 id=100+creates;pending.insert(id);return MOD_OK;}
struct Service{void delete_actor(void*,ActorId id){assert(!pending.contains(id));++deletes;}}service;
auto* svc_actor=&service;void* mod_ctx=nullptr;
'''
main=r'''
void reset(){release_boss_ring_terrain();groundQueries=0;groundMode=0;s_bossRing={};s_enemies={};actors.clear();pending.clear();creates=deletes=0;
 active=playing=floorValid=true;s_hub=s_victory=s_deathPending=transition=failCreate=false;
 s_run.phase=Phase::Running;player.current.pos={100,300,200};}
unsigned previousCalls=0;
void previousCallback(J3DModel*,u32){++previousCalls;}
fopAc_ac_c boss;
void ownBoss(){boss.id=1;boss.health=100;actors[1]=&boss;s_enemies[0]={1,true,true,false,false,false};}
HookAction executeRing(obj_gb_class& ring){std::any args[]={process_method_func(1)};int result=0;
 auto decision=process_boss_ring(&ring,args,&result);if(decision==HOOK_CONTINUE)daObj_Gb_Execute(&ring);
 else assert(result==1);
 return decision;}
void deleted(obj_gb_class& ring){std::any args[]={process_method_func(2)};int result=0;
 assert(process_boss_ring(&ring,args,&result)==HOOK_CONTINUE);actors.erase(ring.id);}
int main(){
 // No ring for ordinary enemies, dormant guards, or failed/pending boss creation.
 for(int gate=0;gate<5;++gate){reset();ownBoss();
  if(gate==0)s_enemies[0].elite=false;
  if(gate==1)s_enemies[0].dormant=true;
  if(gate==2)s_enemies[0].initialized=false;
  if(gate==3)s_enemies[0].credited=true;
  if(gate==4)pending.insert(1);
  update_boss_ring();assert(creates==0);
 }
 reset();ownBoss();update_boss_ring();auto id=s_bossRing.id;
 assert(id!=kNone&&pending.contains(id)&&created.parameters==0xF01E9600&&created.room_num==7);
 assert(created.position.x==100&&created.position.y==20&&created.position.z==200);
 assert(inside_boss_ring({1600,20,200})&&!inside_boss_ring({1601,20,200}));
 player.current.pos={600,900,900};for(int i=0;i<60;++i){cleanup_boss_ring();update_boss_ring();}
 assert(creates==1&&s_bossRing.center.x==100); // Native asynchronous load owns one ID.
 obj_gb_class ring;ring.id=id;ring.profile=fpcNm_OBJ_GB_e;actors[id]=&ring;pending.erase(id);
 for(int i=0;i<3;++i){assert(executeRing(ring)==HOOK_SKIP_ORIGINAL);assert(groundQueries==24u*(i+1));}
 assert(executeRing(ring)==HOOK_CONTINUE&&groundQueries==96);
 assert(cullTop>=1600.f&&ring.scale.y==5.f);
 // Native Execute would fade on other stages with both switches off. The shared
 // Hub configuration keeps the real model AND moving-background collision up.
 for(const char* name:{"D_DLBR0","F_SP121","F_SP124","D_MN08","D_MN09C"}){
  stage=name;for(int tick=0;tick<3;++tick){assert(executeRing(ring)==HOOK_CONTINUE);
   assert(ring.mBrkFrame==29&&ring.brk.frame==29&&ring.mBgMtx.y==5);
   assert(ring.scale.x==1.5f&&ring.mColorAlpha==0xF0);}
 }
 auto moves=ring.bg.moves;playing=false;update_boss_ring();assert(executeRing(ring)==HOOK_SKIP_ORIGINAL);
 assert(ring.bg.moves==moves&&deletes==0);playing=true;
 for(auto phase:{Phase::Upgrade,Phase::TreasureChoice,Phase::Treasure}){
  s_run.phase=phase;update_boss_ring();assert(executeRing(ring)==HOOK_SKIP_ORIGINAL&&deletes==0);
 }
 s_run.phase=Phase::Running;
 // Overlapping bosses share the circle; it ends only with the last live boss.
 fopAc_ac_c second;second.id=2;actors[2]=&second;s_enemies[1]={2,true,true,false,false,false};
 boss.health=0;update_boss_ring();assert(deletes==0&&creates==1);
 second.health=0;update_boss_ring();assert(deletes==1&&s_bossRing.retiring);
 cleanup_boss_ring();assert(deletes==2); // Failed/asynchronous deletion is retried.
 deleted(ring);assert(s_bossRing.id==kNone);
 // Each subsequent encounter anchors to Link's NEW grounded position.
 boss.health=100;update_boss_ring();assert(creates==2&&created.position.x==600&&created.position.y==20);
 // Run exit while still loading: retain ID and delete once initialization finishes.
 auto pendingId=s_bossRing.id;active=false;update_boss_ring();assert(s_bossRing.id==pendingId&&s_bossRing.retiring);
 cleanup_boss_ring();assert(deletes==2);
 obj_gb_class late;late.id=pendingId;late.profile=fpcNm_OBJ_GB_e;pending.erase(pendingId);actors[pendingId]=&late;
 cleanup_boss_ring();assert(deletes==3);assert(executeRing(late)==HOOK_SKIP_ORIGINAL);
 deleted(late);assert(s_bossRing.id==kNone);
 // Restart/death/victory/transition/hub cleanup never reaches an unrelated actor.
 for(int gate=0;gate<5;++gate){reset();ownBoss();update_boss_ring();
  if(gate==0)s_hub=true;
  if(gate==1)s_victory=true;
  if(gate==2)s_deathPending=true;
  if(gate==3)transition=true;
  if(gate==4)s_run.phase=Phase::Loading;
  update_boss_ring();assert(s_bossRing.retiring);
 }
 reset();ownBoss();failCreate=true;update_boss_ring();for(int i=0;i<30;++i)update_boss_ring();assert(creates==1);
 failCreate=false;update_boss_ring();assert(creates==2); // Bounded retry, same anchor.
 reset();ownBoss();floorValid=false;update_boss_ring();assert(creates==0&&!s_bossRing.anchored);
 reset();ownBoss();update_boss_ring();auto stale=s_bossRing.id;pending.erase(stale);
 fopAc_ac_c unrelated;unrelated.id=stale;actors[stale]=&unrelated;cleanup_boss_ring();assert(deletes==0&&s_bossRing.id==kNone);
 // Height and collision follow a steep slope, including signed fixed-point
 // vertex layouts. Archive geometry and per-vertex padding stay unchanged.
 for(bool shortFormat:{false,true}) {
  reset();groundMode=1;ownBoss();update_boss_ring();
  obj_gb_class hill;hill.id=s_bossRing.id;hill.profile=fpcNm_OBJ_GB_e;
  hill.model.data.vertex.load(shortFormat);const auto archive=hill.model.data.vertex.source;
  pending.erase(hill.id);actors[hill.id]=&hill;
  for(int i=0;i<4;++i)executeRing(hill);
  assert(s_bossRingTerrain.ready&&groundQueries==96&&hill.current.pos.y==-770.f);
  hill.model.calc();assert(hill.model.buffer.current==s_bossRingTerrain.positions.get());
  const auto& data=hill.model.data.vertex;const auto* vertices=static_cast<unsigned char*>(hill.model.buffer.current);
  for(unsigned i=0;i<8;++i) {
   const auto& v=sourcePoints[i];const float expected=20.f+v.x*1.5f*0.5f-40.f+v.y*5.f;
   const float modelY=hill.current.pos.y+boss_ring_component(vertices+i*data.stride+(shortFormat?2:4),data.type,data.frac)*5.f;
   assert(std::abs(modelY-expected)<1.f&&std::abs(hill.bg.vertices[i].y-expected)<1.f);
   assert(hill.bg.vertices[i].x==100+v.x*1.5f&&hill.bg.vertices[i].z==200+v.z*1.5f);
   assert(vertices[i*data.stride+data.stride-1]==0x57);
  }
  assert(hill.model.data.vertex.source==archive);
  const auto builds=hill.bg.moves;
  for(int i=0;i<300;++i){executeRing(hill);hill.model.calc();}
  assert(groundQueries==96&&hill.bg.moves==builds&&hill.bg.vertices[3].y==2230.f);
  assert(hill.model.data.vertex.source==archive);
  retire_boss_ring();assert(s_bossRingTerrain.positions); // Native Delete still owns the model.
  deleted(hill);assert(!s_bossRingTerrain.positions&&hill.model.mCalcCallBack==nullptr);
  assert(hill.model.buffer.current==hill.model.data.vertex.source.data());
 }
 // Missing ground never lowers a section to the native sentinel or NaN.
 for(int mode:{2,3}) {
  reset();groundMode=mode;ownBoss();update_boss_ring();obj_gb_class cliff;
  cliff.id=s_bossRing.id;cliff.profile=fpcNm_OBJ_GB_e;pending.erase(cliff.id);actors[cliff.id]=&cliff;
  for(int i=0;i<4;++i)executeRing(cliff);
  assert(s_bossRingTerrain.ready);
  for(float y:s_bossRingTerrain.floor)assert(std::isfinite(y)&&std::abs(y-(mode==2?120.f:20.f))<0.01f);
  deleted(cliff);
 }
 // Unloading restores a prior callback even when native deletion is delayed.
 reset();ownBoss();update_boss_ring();obj_gb_class unloading;
 unloading.id=s_bossRing.id;unloading.profile=fpcNm_OBJ_GB_e;pending.erase(unloading.id);actors[unloading.id]=&unloading;
 unloading.model.mCalcCallBack=previousCallback;
 for(int i=0;i<4;++i)executeRing(unloading);
 unloading.model.calc();assert(previousCalls==1);
 shutdown_boss_ring();assert(unloading.model.mCalcCallBack==previousCallback&&!s_bossRingTerrain.positions);
 unloading.model.calc();assert(previousCalls==2&&unloading.model.buffer.current==unloading.model.data.vertex.source.data());
 // Hub setup keeps its original flat height and never installs our callback.
 obj_gb_class hubRing;configure_ganondorf_barrier(&hubRing);assert(hubRing.scale.y==1.f&&hubRing.model.mCalcCallBack==nullptr);

}
'''
with tempfile.TemporaryDirectory() as td:
    cpp=Path(td)/'test.cpp';exe=Path(td)/'test'
    cpp.write_text(fixture+shared+'\nusing namespace dawnlight;\n'+terrain+production+
        '\n#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored "-Wunused-variable"\n'+native+
        '\n#pragma GCC diagnostic pop\n'+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors boss ring: terrain model/collision, 5x height, bounded sampling, immutable archives, formats, cliffs, pauses, lifecycle and unload passed')
