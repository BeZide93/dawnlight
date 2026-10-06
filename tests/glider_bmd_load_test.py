"""Exercise production BMD selection/lifetime with a real bundled BMD and fake host APIs."""
from pathlib import Path
import hashlib
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
bmd = ROOT / 'res/DawnlightGlider.bmd'
assert hashlib.sha256(bmd.read_bytes()).hexdigest() == (
    'c03615be7649b8e1bc53ddbeecb0775e801da007f4be48452a7fc3d4bd984aa9')
source = (ROOT / 'src/glider_bmd.cpp').read_text()
production = source[source.index('namespace dawnlight {'):source.index('bool draw_glider_bmd(')]
production += source[source.index('void shutdown_glider_bmd()'):]
fixture = r'''
#include "glider_bmd_format.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <vector>
using u8=uint8_t; using u32=uint32_t; using s32=int32_t;
constexpr int MOD_OK=0, kJ3DError_Success=0;
void* mod_ctx=nullptr;
struct JKRHeap {
 static JKRHeap* current;
 JKRHeap* becomeCurrentHeap(){auto* p=current; current=this; return p;}
 static JKRHeap* getRootHeap(){static JKRHeap root;return &root;}
};
JKRHeap* JKRHeap::current=JKRHeap::getRootHeap();
int heaps=0;
struct JKRExpHeap:JKRHeap {
 u8* memory; size_t size,used=0;
 static JKRExpHeap* create(void* p,u32 n,JKRHeap*,bool){
  assert(uintptr_t(p)%32==0); ++heaps;
  auto* h=new JKRExpHeap;h->memory=static_cast<u8*>(p);h->size=n;return h;
 }
 void* alloc(u32 n,int alignment){
  used=(used+alignment-1)&~size_t(alignment-1);
  assert(used+n<=size);auto* p=memory+used;used+=n;return p;
 }
 void destroy(){assert(current!=this);--heaps;delete this;}
};
struct HeapObject {
 static void* operator new(size_t n){
  assert(JKRHeap::current!=JKRHeap::getRootHeap());
  return static_cast<JKRExpHeap*>(JKRHeap::current)->alloc(n,32);
 }
};
#define JKR_NEW new
std::vector<u8> bundled,overlay;
int opened=0,closed=0,loads=0,frees=0,j3dLoads=0,failModels=0;
bool overlayExists=false,shortRead=false,bundleMissing=false;
const void* retained=nullptr; size_t retainedSize=0;
struct ModelData {
 unsigned getJointNum(){return 1;} unsigned getShapeNum(){return 1;}
 unsigned getMaterialNum(){return 3;}
} data;
struct J3DModelLoaderDataBase {
 static ModelData* load(void* p,u32){
  ++j3dLoads; assert(uintptr_t(p)%32==0);
  assert(p!=bundled.data()&&p!=overlay.data());
  if(failModels){--failModels;return nullptr;}
  retained=p;retainedSize=bundled.size();return &data;
 }
};
struct J3DModel:HeapObject {int entryModelData(ModelData*,int,int){return 0;}};
struct J3DDrawBuffer:HeapObject {int allocBuffer(int){return 0;}void setZSort(){}};
struct DVDFileInfo{u32 length;};
bool DVDOpen(const char* p,DVDFileInfo* f){
 assert(std::string(p)==dawnlight::kGliderBmdPath);++opened;
 f->length=overlay.size();return overlayExists;
}
s32 DVDReadPrio(DVDFileInfo*,void* p,u32 n,int,int){
 assert(uintptr_t(p)%32==0&&n%32==0&&n>=overlay.size());
 std::memcpy(p,overlay.data(),overlay.size());
 return shortRead?0:static_cast<s32>(overlay.size());
}
void DVDClose(DVDFileInfo*){++closed;}
struct ResourceBuffer{u32 struct_size;void* data;size_t size;};
#define RESOURCE_BUFFER_INIT {sizeof(ResourceBuffer),nullptr,0}
struct ResourceService{
 int load(void*,const char* p,ResourceBuffer* b){
  assert(std::string(p)=="DawnlightGlider.bmd");++loads;
  if(bundleMissing)return 1;
  b->data=new u8[bundled.size()];b->size=bundled.size();
  std::memcpy(b->data,bundled.data(),b->size);return 0;
 }
 void free(void*,ResourceBuffer* b){
  assert(b->data!=retained);++frees;
  std::memset(b->data,0xdd,b->size);delete[] static_cast<u8*>(b->data);
 }
} resources;
auto* svc_resource=&resources;
struct HookService{int (*resolve)(void*,const char*,void**,void*);};
HookService* svc_hook=nullptr;
struct LogService{void info(void*,const char*){} void warn(void*,const char*){}} logs;
auto* svc_log=&logs;
// PRODUCTION
void reset(){
 dawnlight::shutdown_glider_bmd();
 assert(!heaps&&!dawnlight::s_heapStorage);
 assert(JKRHeap::current==JKRHeap::getRootHeap());
 opened=closed=loads=frees=j3dLoads=failModels=0;
 overlayExists=shortRead=bundleMissing=false;retained=nullptr;
 overlay=bundled;
}
void verify(const std::vector<u8>& expected){
 assert(dawnlight::s_model&&heaps==1);
 assert(retained&&std::memcmp(retained,expected.data(),retainedSize)==0);
 assert(JKRHeap::current==JKRHeap::getRootHeap());
 const int attempts=opened;dawnlight::prepare_glider_bmd();assert(opened==attempts);
}
int main(int argc,char** argv){
 assert(argc==2);std::ifstream f(argv[1],std::ios::binary);
 bundled.assign(std::istreambuf_iterator<char>(f),{});
 assert(dawnlight::valid_glider_bmd(bundled.data(),bundled.size()));
 reset();dawnlight::prepare_glider_bmd();verify(bundled);
 assert(opened==1&&closed==0&&loads==1&&frees==1);
 reset();overlayExists=true;overlay[16]^=1; // distinct, valid container
 dawnlight::prepare_glider_bmd();verify(overlay);
 assert(closed==1&&loads==0);
 reset();overlayExists=true;overlay[0]=0;
 dawnlight::prepare_glider_bmd();verify(bundled);assert(loads==1&&frees==1&&closed==1);
 reset();overlayExists=true;shortRead=true;
 dawnlight::prepare_glider_bmd();verify(bundled);assert(closed==1&&loads==1);
 reset();overlayExists=true;failModels=1;
 dawnlight::prepare_glider_bmd();verify(bundled);assert(j3dLoads==2&&heaps==1);
 reset();bundleMissing=true;dawnlight::prepare_glider_bmd();
 assert(!dawnlight::s_model&&!heaps&&loads==1&&!frees);
 reset();failModels=1;dawnlight::prepare_glider_bmd();
 assert(!dawnlight::s_model&&!heaps&&frees==1&&!dawnlight::s_heapStorage);
 reset();auto good=bundled;bundled[0]=0;dawnlight::prepare_glider_bmd();
 assert(!dawnlight::s_model&&!heaps&&frees==1&&j3dLoads==0);
 bundled=good;reset();dawnlight::prepare_glider_bmd();verify(bundled);reset();
}
'''.replace('// PRODUCTION', production)
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe), str(bmd)], check=True)
print('Glider BMD: exact asset, overlay priority, missing/invalid/read/load failures, '
      'private retained copy, cleanup and reload passed')
