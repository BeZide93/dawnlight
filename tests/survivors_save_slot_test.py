"""Production slot presentation using actual SDK string/placement implementations."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u64=uint64_t;using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using s16=int16_t;using s32=int32_t;using f32=float;
using std::strlen;using std::strncpy;
constexpr u64 pane_tag(const char* p){u64 n=0;for(++p;p[0]&&p[1];++p)n=(n<<8)|u8(*p);return n;}
#define MULTI_CHAR(x) pane_tag(#x)
#define JKR_NEW new
#define JKR_DELETE(x) delete (x)
#define JKR_DELETE_ARRAY(x) delete[] (x)
#define JKR_NEW_ARRAY(T,n) new T[n]
namespace JGeometry {
template<class T>struct TBox2 {
 struct Point{T x=0,y=0;}i,f;
 TBox2()=default;TBox2(T a,T b,T c,T d):i{a,b},f{c,d}{}
 T getWidth()const{return f.x-i.x;}T getHeight()const{return f.y-i.y;}
};}
using Box=JGeometry::TBox2<float>;
bool rejectAppend=false;unsigned livePanes=0;
struct J2DPane {
 u64 tag;Box bounds;Box& mBounds=bounds;float mTranslateX=0,mTranslateY=0;J2DPane* parent=nullptr;std::vector<J2DPane*> children;bool visible=true;
 float mPositionMtx[3][4]{{1,0,0,0},{0,1,0,0},{0,0,1,0}};
 J2DPane(u64 t=0,Box b={}):tag(t),bounds(b){++livePanes;}
 virtual ~J2DPane(){for(auto* child:children)delete child;--livePanes;}
 J2DPane* search(u64 t){if(tag==t)return this;for(auto* child:children)if(auto* p=child->search(t))return p;return nullptr;}
 bool appendChild(J2DPane* p){if(rejectAppend)return false;p->parent=this;children.push_back(p);return true;}
 J2DPane* getParentPane(){return parent;}
 Box& getBounds(){return bounds;}
 void place(const Box&);
 void calcMtx(){mPositionMtx[0][3]=mTranslateX;mPositionMtx[1][3]=mTranslateY;}
 J2DPane* getFirstChildPane(){return children.empty()?nullptr:children.front();}
 J2DPane* getNextChildPane(){if(!parent)return nullptr;auto it=std::find(parent->children.begin(),parent->children.end(),this);return ++it==parent->children.end()?nullptr:*it;}
 void hide(){visible=false;}void show(){visible=true;}
};
enum{HBIND_LEFT,VBIND_CENTER};
struct J2DTextBox:J2DPane {
 struct TFontSize{float mSizeX=18,mSizeY=18;};TFontSize fontSize;
 void* font=nullptr;char* mStringPtr=nullptr;u16 mStringLength=0;bool inheritedAlpha=false;
 std::string text()const{return mStringPtr?mStringPtr:"";}
 J2DTextBox(u64 t,Box b,void*,const char* str,int,int,int):J2DPane(t,Box(0,0,b.getWidth(),b.getHeight())){mTranslateX=b.i.x;mTranslateY=b.i.y;calcMtx();setString(32,str);}
 ~J2DTextBox(){delete[] mStringPtr;}
 void setFont(void* p){font=p;}
 void setInfluencedAlpha(bool value,bool){inheritedAlpha=value;}
 void getFontSize(TFontSize& out){out=fontSize;}
 void setFontSize(float x,float y){fontSize={x,y};}
 int getCharColor(){return 0x123;}int getGradColor(){return 0x456;}
 void setFontColor(int a,int b){assert(a==0x123&&b==0x456);}
 s32 setString(s16,const char*,...);
 const char* getStringPtr(){return mStringPtr;}
};
struct dFile_info_c {struct {J2DPane* Scr;void* mFont;J2DPane* field_0x10;}mFileInfo;};
struct ModContext{};void* mod_ctx=nullptr;bool s_active=true;
constexpr const char* kBlob="records";
constexpr int MOD_OK=0,MOD_UNAVAILABLE=1,MOD_INVALID_ARGUMENT=2;
namespace mods{template<class T>T arg(void* a,int n){return *static_cast<T*>(static_cast<void**>(a)[n]);}}
std::array<std::vector<uint8_t>,3> slots;unsigned reads=0,lastSlot=99;bool oversize=false;int forcedResult=MOD_OK;
int peek(void* ctx,unsigned slot,const char* key,void* buf,size_t* size){
 assert(ctx==mod_ctx&&key==kBlob&&slot<3);++reads;lastSlot=slot;
 if(forcedResult!=MOD_OK)return forcedResult;
 if(slots[slot].empty())return MOD_UNAVAILABLE;
 assert(*size>=slots[slot].size());std::memcpy(buf,slots[slot].data(),slots[slot].size());
 *size=oversize?Records::byte_count+1:slots[slot].size();return MOD_OK;
}
struct SaveService{decltype(&peek)peek_blob=peek;}saveService;SaveService* svc_save=&saveService;
'''
main = r'''
struct Panel {
 J2DPane* screen=new J2DPane;
 J2DPane* data=new J2DPane(1,Box(-200,-100,200,100));
 J2DPane* group=new J2DPane(2);
 dFile_info_c info{{screen,reinterpret_cast<void*>(1),data}};
 Panel(){
  screen->appendChild(data);data->appendChild(group);
  data->mPositionMtx[0][3]=300; // Native animated slot transform must not be baked into the label.
  group->mPositionMtx[0][0]=group->mPositionMtx[1][1]=2;
  group->mPositionMtx[0][3]=10;group->mPositionMtx[1][3]=20;
  data->appendChild(new J2DTextBox(MULTI_CHAR('w_ptim01'),{},nullptr,"",32,HBIND_LEFT,VBIND_CENTER));
  for(unsigned i=0;i<20;++i){auto* heart=new J2DPane(kSaveHeartTags[i],Box(0,0,12,12));
   heart->mPositionMtx[0][3]=float(i%10)*14;heart->mPositionMtx[1][3]=float(i/10)*20;group->appendChild(heart);}
 }
 ~Panel(){delete screen;}
 J2DTextBox* label(){return static_cast<J2DTextBox*>(screen->search(kSaveMetaTag));}
 void refresh(u8 slot,int status=0){
  // Native setHeartCnt refreshes the heart visibility before the post-hook.
  for(unsigned i=0;i<20;++i){auto* p=screen->search(kSaveHeartTags[i]);if(i<3)p->show();else p->hide();}
  auto* ptr=&info;void* save=nullptr;int checksum=1;void* args[]={&ptr,&save,&checksum,&slot};
  after_survivors_save_slot(nullptr,args,&status,nullptr);
 }
 void replaced(){assert(label()&&label()->visible);for(auto t:kSaveHeartTags)assert(!screen->search(t)->visible);}
 void native(){assert(!label()||!label()->visible);assert(screen->search(kSaveHeartTags[0])->visible);}
};
void level(unsigned slot,unsigned n){Records r;r.total_xp=meta_threshold(n-1);const auto bytes=r.encode();slots[slot]={bytes.begin(),bytes.end()};}
int main(){
 level(0,17);level(1,250); // Slot 2 has no records: legacy/new progression starts at 1.
 {
  Panel a,b,c;a.refresh(0);b.refresh(1);c.refresh(2);
  assert(a.label()->text()=="Meta Level 17"&&b.label()->text()=="Meta Level 250"&&c.label()->text()=="Meta Level 1");
  a.replaced();b.replaced();c.replaced();assert(reads==3&&lastSlot==2);
  assert(a.label()->parent==a.data&&a.label()->inheritedAlpha&&a.label()->font==a.info.mFileInfo.mFont);
  Box box;bool first=true;assert(save_heart_bounds(a.label(),a.data,box,first));
  assert(box.i.x==10&&box.i.y==20&&box.f.x==286&&box.f.y==84&&a.label()->fontSize.mSizeY==18);
  // Native place() adds the parent's bounds origin. Verify both axes for
  // centered, zero-origin and positive-origin panels, including pane reuse.
  for(const Box origin:{Box(0,0,400,200),Box(80,30,480,230),Box(-200,-100,200,100)}){
   a.data->bounds=origin;a.refresh(0);Box placed;bool empty=true;
   assert(save_heart_bounds(a.label(),a.data,placed,empty));
   assert(placed.i.x==10&&placed.i.y==20&&placed.f.x==286&&placed.f.y==84);
   assert(a.label()->text()=="Meta Level 17");
  }
  const auto count=livePanes;auto* original=a.label();
  level(0,33);a.refresh(0);assert(a.label()==original&&livePanes==count&&a.label()->text()=="Meta Level 33");
  // A copy destination uses its passed slot, independent of pane identity/selection.
  slots[2]=slots[1];a.refresh(2);assert(a.label()->text()=="Meta Level 250");
  slots[2].clear();a.refresh(2,1);a.native();a.refresh(2,2);assert(a.label()->text()=="Meta Level 1");
  // Empty/corrupt game saves, invalid indices, disabled mode and absent service do not read records.
  const auto before=reads;
  a.refresh(0,1);a.native();a.refresh(0,-1);a.native();a.refresh(3);a.native();
  s_active=false;a.refresh(0);a.native();s_active=true;
  svc_save=nullptr;a.refresh(0);a.native();svc_save=&saveService;
  saveService.peek_blob=nullptr;a.refresh(0);a.native();saveService.peek_blob=peek;
  assert(reads==before);
  // A supported old record migrates through Records::decode without modifying storage.
  level(0,19);slots[0].resize(100);slots[0][4]=2;const auto legacy=slots[0];
  a.refresh(0);assert(a.label()->text()=="Meta Level 19"&&slots[0]==legacy);
  slots[0][0]=0;a.refresh(0);assert(a.label()->text()=="Meta Level ?");a.replaced();
  level(0,25);oversize=true;a.refresh(0);assert(a.label()->text()=="Meta Level ?");oversize=false;
  forcedResult=MOD_INVALID_ARGUMENT;a.refresh(0);assert(a.label()->text()=="Meta Level ?");forcedResult=MOD_OK;
  level(0,1+meta_levels(max_meta_xp));a.refresh(0);a.replaced();
  assert(a.label()->fontSize.mSizeX*float(a.label()->text().size())<=box.getWidth());
 }
 assert(livePanes==0); // Native screen ownership releases the added label as well.
 {
  Panel panel;rejectAppend=true;const auto count=livePanes;panel.refresh(0);panel.native();assert(livePanes==count);rejectAppend=false;
  panel.info.mFileInfo.mFont=nullptr;panel.refresh(0);panel.native();
  panel.info.mFileInfo.mFont=reinterpret_cast<void*>(1);
  panel.group->mPositionMtx[0][3]=NAN;panel.refresh(0);panel.native();
 }
 assert(livePanes==0);
 std::puts("Survivors save slots: per-slot/legacy levels, copy/erase refresh, native layout/lifetime and isolation passed");
}
'''
# Compile the actual pinned SDK functions: these APIs do not behave like printf
# or a raw bounds assignment. Their previous simplified stubs hid both bugs.
def native_function(path, signature):
    source = (root/path).read_text()
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

native = native_function('dusklight/libs/JSystem/src/J2DGraph/J2DPane.cpp', 'void J2DPane::place(')
native += native_function('dusklight/libs/JSystem/src/J2DGraph/J2DTextBox.cpp', 's32 J2DTextBox::setString(s16')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture + native + (root/'src/survivors_save_slot.inc').read_text() + main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-Wno-multichar','-Wno-overflow','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
