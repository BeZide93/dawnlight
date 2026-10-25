"""Run production room ownership/render/cleanup code against a bounded fake host."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/survivors_roundtable.cpp').read_text()
production=source[source.index('namespace dawnlight {'):]
fixture=r'''
#include "generated/roundtable_layout.hpp"
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <vector>
using s16=short;using u8=unsigned char;using u16=unsigned short;using u32=unsigned;using ActorId=unsigned;
constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
constexpr int fpcNm_BG_e=1,cPhs_COMPLEATE_e=4,MOD_OK=0;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}}
using process_method_func=int(*)(void*);
struct process_method_class{process_method_func create_method,delete_method,execute_method,is_delete_method;};
struct leafdraw_method_class{process_method_class base;process_method_func draw_method;};
struct fopAc_ac_c{int name=1;unsigned id=1,param=51;const void* sub_method=nullptr;};
struct dBgW_Base{bool used=false;enum PRIORITY{PRIORITY_0};void SetPriority(PRIORITY){};};
struct cBgD_t{};
struct cBgW{enum{GLOBAL_e};};
int liveHeaps=0;bool allocationFails=false,collisionFails=false;
struct JKRHeap{
 static JKRHeap* current;
 std::vector<void*> blocks;std::vector<JKRHeap*> children;
 virtual ~JKRHeap(){for(auto* child:children)delete child;for(auto* b:blocks)std::free(b);}
 JKRHeap* becomeCurrentHeap(){auto* p=current;current=this;return p;}
 void* alloc(unsigned size,int){if(allocationFails)return nullptr;void* p=std::malloc(size);blocks.push_back(p);return p;}
 static JKRHeap* getRootHeap();
};
JKRHeap rootHeap;JKRHeap* JKRHeap::current=&rootHeap;JKRHeap* JKRHeap::getRootHeap(){return &rootHeap;}
struct JKRExpHeap:JKRHeap{
 static JKRExpHeap* create(void*,unsigned,JKRHeap*,bool){++liveHeaps;return new JKRExpHeap;}
 void destroy(){--liveHeaps;delete this;}
};
struct JKRSolidHeap:JKRHeap{
 static JKRSolidHeap* create(unsigned,JKRHeap* parent,bool){auto* p=new JKRSolidHeap;parent->children.push_back(p);return p;}
};
struct dBgW:dBgW_Base{
 static void* operator new(size_t n){return JKRHeap::current->alloc(n,32);}
 bool Set(cBgD_t* p,int,void*){assert(p);return collisionFails;}
};
struct dBgWKCol:dBgW_Base{};
struct cBgS{static void* ConvDzb(void* p){return p;}};
struct daBg_c:fopAc_ac_c{dBgW* mpBgW=nullptr;dBgWKCol* mpKCol=nullptr;};
std::map<unsigned,fopAc_ac_c*> actors;
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetName(fopAc_ac_c* a){return a->name;}
unsigned fopAcM_GetParam(fopAc_ac_c* a){return a->param;}
void* fopAcM_SearchByID(unsigned id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
bool is_actor_process(void* p){assert(p!=reinterpret_cast<void*>(1));return p!=nullptr;}
struct Registry{
 std::set<dBgW_Base*> entries;bool fail=false;
 bool Regist(dBgW_Base* bg,fopAc_ac_c*){if(fail)return true;assert(!bg->used);bg->used=true;entries.insert(bg);return false;}
 void Release(dBgW_Base* bg){assert(bg&&bg->used);bg->used=false;assert(entries.erase(bg)==1);}
} registry;
Registry& dComIfG_Bgsp(){return registry;}
dBgW_Base* roomCollision=nullptr;
struct dStage_roomControl_c{static void setBgW(int room,dBgW_Base* p){assert(room==51);roomCollision=p;}};
struct ResourceBuffer{void* data=nullptr;size_t size=0;};
#define RESOURCE_BUFFER_INIT {}
#define JKR_NEW new
bool missing=false;int loads=0,frees=0;std::string assetRoot;
struct Resources{
 int load(void*,const char* path,ResourceBuffer* out){
  ++loads;if(missing)return 1;
  std::ifstream file(assetRoot+"/"+path,std::ios::binary);assert(file);
  std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});
  out->size=bytes.size();out->data=std::malloc(bytes.size());std::memcpy(out->data,bytes.data(),bytes.size());return 0;
 }
 void free(void*,ResourceBuffer* b){++frees;std::free(b->data);*b={};}
} resources;
auto* svc_resource=&resources;void* mod_ctx=nullptr;
struct Logs{void info(void*,const char*){}void error(void*,const char*){}} logs;auto* svc_log=&logs;
struct cXyz{float x,y,z;cXyz(float a=0,float b=0,float c=0):x(a),y(b),z(c){}void set(float a,float b,float c){x=a;y=b;z=c;}};
struct Color{int r,g,b,a;};
struct LIGHT_INFLUENCE{cXyz mPosition;Color mColor;float mPow,mFluctuation;};
int lights=0,particles=0;
void dKy_plight_set(LIGHT_INFLUENCE*){++lights;}void dKy_plight_cut(LIGHT_INFLUENCE*){assert(lights>0);--lights;}
struct Emitter{void setGlobalPrmColor(u8 r,u8 g,u8 b){assert(r==255&&g==225&&b==120);}void setGlobalEnvColor(u8 r,u8 g,u8 b){assert(r==255&&g==130&&b==18);}void stopCreateParticle(){}void quitImmortalEmitter(){}void becomeInvalidEmitter(){--particles;}} emitter;
Emitter* dComIfGp_particle_getEmitter(unsigned id){return id?&emitter:nullptr;}
template<class... Args>unsigned dComIfGp_particle_set(unsigned id,Args...){if(id)return id;++particles;return particles;}
struct J3DPacket{
 J3DPacket* next=nullptr;virtual ~J3DPacket()=default;virtual void draw(){}
 J3DPacket* getNextPacket(){return next;}void setNextPacket(J3DPacket* p){next=p;}
};
struct J3DDrawBuffer{
 J3DPacket* entries[2]{};J3DPacket** mpBuffer=entries;
 unsigned getEntryTableSize(){return 2;}
 void entryImm(J3DPacket* p,unsigned i){p->setNextPacket(entries[i]);entries[i]=p;}
} drawList,shadeList;
struct dDlst_list_c{enum{DB_XLU_LIST=0};J3DDrawBuffer* mDrawBuffers[1]{&shadeList};};
struct{dDlst_list_c drawlist;}g_dComIfG_gameInfo;
J3DDrawBuffer* dComIfGd_getOpaListBG(){return &drawList;}
J3DDrawBuffer* dComIfGd_getOpaList(){assert(false && "Room must precede native shadows, not render with actors");return nullptr;}
struct J3DSys{void reinitGX(){}void* getViewMtx(){return nullptr;}}j3dSys;
struct GXTexObj{};
struct Lighting{void settingTevStruct(int,void*,void*){}}g_env_light;
void* dComIfGp_roomControl_getTevStr(unsigned){return nullptr;}
unsigned submitted=0,batches=0;int zTest=0,zWrite=0,blendMode=0,cullMode=0;
// Mirror Aurora's shared, fixed-size per-frame arena, including other actors.
constexpr size_t frameCapacity=5*1024*1024,otherSceneBytes=1536*1024;
size_t frameBytes=otherSceneBytes;
void stream(size_t bytes){frameBytes+=bytes;assert(frameBytes<=frameCapacity && "Shared Aurora vertex arena overflow");}
std::map<int,std::pair<int,int>> formats;
void GXSetVtxAttrFmt(int,int attr,int,int type,int frac){formats[attr]={type,frac};}
std::vector<std::array<float,3>> positions;std::vector<std::array<unsigned,4>> colors;
void GXPosition3f32(float x,float y,float z){stream(12);positions.push_back({x,y,z});}
void GXPosition3s16(s16 x,s16 y,s16 z){stream(6);positions.push_back({x/8.f,y/8.f,z/8.f});}
void GXTexCoord2f32(float,float){stream(8);}
void GXTexCoord2u16(u16,u16){stream(4);}
void GXColor4u8(u8 r,u8 g,u8 b,u8 a){stream(4);colors.push_back({r,g,b,a});}
void GXSetZMode(int test,int,int write){zTest=test;zWrite=write;}
void GXSetBlendMode(int mode,int,int,int){blendMode=mode;}
void GXSetCullMode(int mode){cullMode=mode;}
void GXBegin(int,int,unsigned count){assert(count<=60000&&count%3==0);submitted+=count;++batches;}
'''
constants=sorted(set(re.findall(r'\bGX_[A-Z0-9a-z_]+',production)) | {'GX_S16','GX_U16'})
fixture+='\nenum {'+','.join(constants)+'};\n'
for fn in sorted(set(re.findall(r'\b(GX\w+)\(',production))):
    if fn not in {'GXBegin','GXPosition3f32','GXColor4u8','GXSetZMode','GXSetBlendMode','GXSetCullMode','GXSetVtxAttrFmt','GXPosition3s16','GXTexCoord2u16','GXTexCoord2f32'}:fixture+=f'template<class... T>void {fn}(T...){{}}\n'
tests=r'''
int create(void*){return 4;}int remove(void*){return 1;}int execute(void*){return 1;}int draw(void*){return 1;}
leafdraw_method_class methods{{create,remove,execute,nullptr},draw};
void dispatch(daBg_c& bg,process_method_func method,bool hub,int result=4,bool poison=false){
 std::any args[]{method,static_cast<void*>(&bg)};
 dawnlight::before_roundtable_process(args,&result,hub);
 if(poison)args[1]=reinterpret_cast<void*>(1);
 dawnlight::after_roundtable_process(args,&result);
}
int main(int argc,char** argv){
 assert(argc==2);assetRoot=argv[1];
 for(int cycle=0;cycle<30;++cycle){
  daBg_c bg;bg.id=unsigned(cycle+1);bg.sub_method=&methods;actors[bg.id]=&bg;
  dBgWKCol native;bg.mpKCol=&native;registry.Regist(&native,&bg);roomCollision=&native;
  // Normal combat/story room and partial create must remain native.
  dispatch(bg,create,false);assert(bg.mpKCol==&native&&liveHeaps==0);
  dispatch(bg,create,true,0);assert(bg.mpKCol==&native&&liveHeaps==0);
  // Roll back failure without releasing the only live collision.
  if(cycle==0){
   missing=true;dispatch(bg,create,true);missing=false;assert(bg.mpKCol==&native&&liveHeaps==0);
   collisionFails=true;dispatch(bg,create,true);collisionFails=false;assert(bg.mpKCol==&native&&liveHeaps==0);
   registry.fail=true;dispatch(bg,create,true);registry.fail=false;assert(bg.mpKCol==&native&&liveHeaps==0);
  }
  dispatch(bg,create,true);assert(bg.mpBgW&&bg.mpKCol==nullptr&&liveHeaps==1&&!native.used);

  auto* custom=bg.mpBgW;assert(custom==roomCollision&&registry.entries.count(custom));
  dispatch(bg,create,true);assert(bg.mpBgW==custom&&liveHeaps==1);
  // Mode selection is now false: this outgoing actor must keep its own room.
  J3DPacket other,otherShade;drawList.entryImm(&other,0);shadeList.entryImm(&otherShade,0);
  dispatch(bg,draw,false);auto* packet=drawList.entries[0];assert(packet!=&other);
  for(int i=0;i<5;++i)dispatch(bg,draw,false);
  assert(packet==drawList.entries[0]&&packet->getNextPacket()==&other&&other.getNextPacket()==nullptr);
  std::ifstream mesh(assetRoot+"/survivors_roundtable/hall.mesh",std::ios::binary);
  unsigned char header[8]{};mesh.read(reinterpret_cast<char*>(header),8);assert(mesh);
  const unsigned expected=unsigned(header[4])|(unsigned(header[5])<<8)|
      (unsigned(header[6])<<16)|(unsigned(header[7])<<24);
  submitted=batches=0;frameBytes=otherSceneBytes;positions.clear();packet->draw();
  assert(frameBytes==otherSceneBytes+size_t(expected)*18);
  assert(formats[GX_VA_POS]==std::make_pair(int(GX_S16),3));
  assert(formats[GX_VA_TEX0]==std::make_pair(int(GX_U16),16));
  assert(formats[GX_VA_TEX1]==std::make_pair(int(GX_U16),16));
  if(cycle==0){
   for(unsigned i=0;i<expected;++i){
    float v[7];unsigned char rgba[4];mesh.read(reinterpret_cast<char*>(v),28);mesh.read(reinterpret_cast<char*>(rgba),4);assert(mesh);
    const auto& packed=dawnlight::s_vertices[i];
    for(unsigned axis=0;axis<3;++axis)assert(std::abs(positions[i][axis]-v[axis])<=.0625f);
    const u16 uv[]{packed.u,packed.v,packed.lu,packed.lv};
    for(unsigned axis=0;axis<4;++axis)assert(std::abs(uv[axis]/65536.f-v[axis+3])*1024<=1.f/128);
   }
  }
  assert(submitted==expected&&batches==(expected+59999)/60000);
  auto* shade=shadeList.entries[0];assert(shade!=&otherShade&&shade->getNextPacket()==&otherShade);
  submitted=batches=0;positions.clear();colors.clear();shade->draw();
  assert(frameBytes==otherSceneBytes+size_t(expected)*18+36*16);
  assert(submitted==36&&batches==6&&zTest==GX_TRUE&&zWrite==GX_FALSE&&blendMode==GX_BM_BLEND&&cullMode==GX_CULL_BACK);
  for(unsigned i=0;i<colors.size();++i){
   assert(colors[i][0]==0&&colors[i][1]==0&&colors[i][2]==0&&colors[i][3]>0);
   assert(positions[i][2]>dawnlight::roundtable_layout::exitZ);
   if(i)assert(positions[i][2]<=positions[i-1][2]);
  }
  assert(colors.front()[3]==255&&colors.back()[3]==45);
  dispatch(bg,execute,false);assert(lights==2&&particles==18);
  dispatch(bg,execute,false);assert(lights==2&&particles==18);
  if(cycle%2==0){
   dispatch(bg,remove,false,1,true); // Post-delete must not inspect the poisoned actor.
  }else dawnlight::shutdown_roundtable();
  assert(liveHeaps==0&&lights==0&&particles==0);
  assert(bg.mpKCol==&native&&!bg.mpBgW&&native.used&&roomCollision==&native);
  assert(drawList.entries[0]==&other&&other.getNextPacket()==nullptr);
  assert(shadeList.entries[0]==&otherShade&&otherShade.getNextPacket()==nullptr);
  registry.Release(&native);actors.clear();drawList.entries[0]=nullptr;shadeList.entries[0]=nullptr;
 }
 dawnlight::shutdown_roundtable();assert(loads>=frees&&JKRHeap::current==&rootHeap&&registry.entries.empty());
 std::cout<<"Roundtable runtime: repeated creation, mode switches, deferred drawing, fire and cleanup passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+production+tests)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',str(ROOT/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe),str(ROOT/'res')],check=True)
