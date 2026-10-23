"""Slot presentation against native FileInfo draw, vertex transforms and text setter."""
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
struct Vec{float x=0,y=0,z=0;};
using Mtx=float[3][4];using MtxP=float(*)[4];
void MTXIdentity(Mtx m){for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)m[i][j]=i==j?1.f:0.f;}
void MTXScale(Mtx m,float x,float y,float z){MTXIdentity(m);m[0][0]=x;m[1][1]=y;m[2][2]=z;}
void MTXConcat(const Mtx a,const Mtx b,Mtx out){Mtx tmp{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j){for(unsigned k=0;k<3;++k)tmp[i][j]+=a[i][k]*b[k][j];if(j==3)tmp[i][j]+=a[i][3];}std::memcpy(out,tmp,sizeof(tmp));}
struct J2DGrafContext{Mtx mPosMtx;J2DGrafContext(){MTXIdentity(mPosMtx);}}graf;
J2DGrafContext* dComIfGp_getCurrentGrafPort(){return &graf;}
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
 u64 tag;Box bounds;Box& mBounds=bounds;float mTranslateX=0,mTranslateY=0;J2DPane* parent=nullptr;std::vector<J2DPane*> children;bool visible=true;u64 userInfo=0;u8 mAlpha=255,mColorAlpha=255;
 Mtx mGlobalMtx{{1,0,0,0},{0,1,0,0},{0,0,1,0}};
 float mPositionMtx[3][4]{{1,0,0,0},{0,1,0,0},{0,0,1,0}};
 J2DPane(u64 t=0,Box b={}):tag(t),bounds(b){++livePanes;}
 virtual ~J2DPane(){for(auto* child:children)delete child;--livePanes;}
 J2DPane* search(u64 t){if(tag==t)return this;for(auto* child:children)if(auto* p=child->search(t))return p;return nullptr;}
 bool appendChild(J2DPane* p){if(rejectAppend)return false;p->parent=this;children.push_back(p);return true;}
 J2DPane* getParentPane(){return parent;}
 Box& getBounds(){return bounds;}
 Vec getGlbVtx(u8)const;
 float getWidth()const{return bounds.getWidth();}float getHeight()const{return bounds.getHeight();}
 MtxP getGlbMtx(){return mGlobalMtx;}void setMtx(Mtx m){std::memcpy(mPositionMtx,m,sizeof(Mtx));}
 bool isVisible()const{return visible;}u64 getUserInfo()const{return userInfo;}void setUserInfo(u64 v){userInfo=v;}
 void renderTree(){if(parent)MTXConcat(parent->mGlobalMtx,mPositionMtx,mGlobalMtx);else std::memcpy(mGlobalMtx,mPositionMtx,sizeof(Mtx));
  mColorAlpha=parent?u8(unsigned(mAlpha)*parent->mColorAlpha/255):mAlpha;
  for(auto* child:children)child->renderTree();}
 void draw(float,float,J2DGrafContext*){renderTree();}
 void calcMtx(){mPositionMtx[0][3]=mTranslateX;mPositionMtx[1][3]=mTranslateY;}
 J2DPane* getFirstChildPane(){return children.empty()?nullptr:children.front();}
 J2DPane* getNextChildPane(){if(!parent)return nullptr;auto it=std::find(parent->children.begin(),parent->children.end(),this);return ++it==parent->children.end()?nullptr:*it;}
 void hide(){visible=false;}void show(){visible=true;}
};
enum J2DTextBoxHBinding{HBIND_CENTER,HBIND_RIGHT,HBIND_LEFT};
enum J2DTextBoxVBinding{VBIND_CENTER,VBIND_BOTTOM,VBIND_TOP};
// Execute the SDK's actual printReturn vertical binding/rounding. The font
// fixture supplies one-line glyph metrics; no GPU or game assets are required.
struct Font{float getAscent(){return 15;}float getCellHeight(){return 20;}}testFont;
u8 data_8045158C[1]{};
struct J2DPrint {
 struct TSize{};
 Font* mFont=&testFont;float mCursorH=0,mCursorV=0,field_0x24=0,field_0x28=0;
 float mLineSpacing=0,mFontSizeY=20;size_t mStrBuffSize=256;
 void initchar(){}
 float parse(u8*,size_t,float,u16* lines,TSize&,u8,bool){lines[0]=1;lines[1]=0xffff;return mFontSizeY/4;}
 void printReturn(const char*,f32,f32,J2DTextBoxHBinding,J2DTextBoxVBinding,f32,f32,u8);
};
struct J2DTextBox:J2DPane {
 struct TFontSize{float mSizeX=18,mSizeY=18;};TFontSize fontSize;
 unsigned draws=0;
 u8 mFlags=HBIND_LEFT<<2;float field_0x10c=0,field_0x110=0,charSpace=0,lineSpace=20;
 J2DTextBoxVBinding getVBinding()const{return J2DTextBoxVBinding(mFlags&3);}
 float getCharSpace()const{return charSpace;}void setCharSpace(float v){charSpace=v;}
 float getLineSpace()const{return lineSpace;}void setLineSpace(float v){lineSpace=v;}
 void* getFont()const{return font;}
 void drawSelf(float x,float y,Mtx* m){assert(x==0&&y==0&&m==&graf.mPosMtx&&!visible);++draws;}
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
struct dDlst_FileInfo_c{J2DPane* Scr;void* mFont;J2DPane* field_0x10;J2DPane* mBasePane=nullptr;void draw();};
struct dFile_info_c {dDlst_FileInfo_c mFileInfo;};
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
float baseline(J2DTextBox* box){
 J2DPrint print;print.mFontSizeY=box->fontSize.mSizeY;print.mLineSpacing=box->getLineSpace();
 print.mCursorV=box->mBounds.i.y;
 print.printReturn(box->getStringPtr(),box->getWidth(),box->getHeight(),HBIND_LEFT,
                   box->getVBinding(),box->field_0x10c,box->field_0x110,255);
 return box->mGlobalMtx[1][1]*print.mCursorV+box->mGlobalMtx[1][3];
}
struct Panel {
 J2DPane* screen=new J2DPane;
 J2DPane* bridge=new J2DPane(MULTI_CHAR('Nm_02'));
 J2DPane* base=new J2DPane(3,Box(0,0,800,400));
 J2DPane* data=new J2DPane(1,Box(-200,-100,200,100));
 J2DPane* group=new J2DPane(2);
 dFile_info_c info{{screen,reinterpret_cast<void*>(1),data,base}};
 float baseX=100,baseY=150;
 Panel(){
  screen->appendChild(bridge);bridge->appendChild(data);data->appendChild(group);
  group->mPositionMtx[0][0]=group->mPositionMtx[1][1]=2;
  group->mPositionMtx[0][3]=10;group->mPositionMtx[1][3]=20;
  data->appendChild(new J2DTextBox(MULTI_CHAR('w_ptim01'),{},nullptr,"",32,HBIND_LEFT,VBIND_CENTER));
  for(auto tag:{MULTI_CHAR('f_name01'),MULTI_CHAR('w_name01')}){
   auto* name=new J2DTextBox(tag,Box(40,10,140,34),nullptr,"Link",32,HBIND_LEFT,VBIND_CENTER);
   name->setFont(info.mFileInfo.mFont);name->setFontSize(20,20);name->field_0x110=1;
   data->appendChild(name);if(tag==MULTI_CHAR('w_name01'))name->hide();
  }
  for(unsigned i=0;i<20;++i){auto* heart=new J2DPane(kSaveHeartTags[i],Box(0,0,12,12));
   heart->mPositionMtx[0][3]=float(i%10)*14;heart->mPositionMtx[1][3]=float(i/10)*20;group->appendChild(heart);}
 }
 ~Panel(){delete screen;delete base;}
 void render(){
  MTXIdentity(base->mGlobalMtx);base->mGlobalMtx[0][3]=baseX;base->mGlobalMtx[1][3]=baseY;
  info.mFileInfo.draw(); // Actual native bridge/scaling, before the post-hook.
  auto* ptr=&info.mFileInfo;void* args[]={&ptr};after_survivors_save_slot_draw(nullptr,args,nullptr,nullptr);
 }
 J2DTextBox* label(){return static_cast<J2DTextBox*>(screen->search(kSaveMetaTag));}
 void refresh(u8 slot,int status=0){
  // Native setHeartCnt refreshes the heart visibility before the post-hook.
  for(unsigned i=0;i<20;++i){auto* p=screen->search(kSaveHeartTags[i]);if(i<3)p->show();else p->hide();}
  auto* ptr=&info;void* save=nullptr;int checksum=1;void* args[]={&ptr,&save,&checksum,&slot};
  after_survivors_save_slot(nullptr,args,&status,nullptr);render();
 }
 void replaced(){assert(label()&&label()->getUserInfo()&&!label()->visible&&label()->draws);for(auto t:kSaveHeartTags)assert(!screen->search(t)->visible);}
 void native(){assert(!label()||!label()->getUserInfo());assert(screen->search(kSaveHeartTags[0])->visible);}
};
void level(unsigned slot,unsigned n){Records r;r.total_xp=meta_threshold(n-1);const auto bytes=r.encode();slots[slot]={bytes.begin(),bytes.end()};}
int main(){
 level(0,17);level(1,250); // Slot 2 has no records: legacy/new progression starts at 1.
 {
  Panel a,b,c;a.refresh(0);b.refresh(1);c.refresh(2);
  assert(a.label()->text()=="Meta Level 17"&&b.label()->text()=="Meta Level 250"&&c.label()->text()=="Meta Level 1");
  a.replaced();b.replaced();c.replaced();assert(reads==3&&lastSlot==2);
  assert(a.label()->parent==a.screen&&a.label()->font==a.info.mFileInfo.mFont);
  assert(a.data->children.size()==4&&a.group->children.size()==20); // Cached alpha-manager subtree never grows.
  auto labelBox=[&]{const Vec tl=a.label()->getGlbVtx(0),br=a.label()->getGlbVtx(3);return Box(tl.x,tl.y,br.x,br.y);};
  auto box=labelBox();
  assert(box.i.x==120&&box.i.y==170&&box.f.x==672&&box.f.y==218&&a.label()->fontSize.mSizeY==20);
  // Move/resize the native slot after records were read: selection animation
  // and replacement layouts must reposition text without another save lookup.
  const auto readCount=reads;
  a.baseX=400;a.baseY=300;a.base->bounds=Box(0,0,400,200);
  a.group->mPositionMtx[0][3]=30;a.group->mPositionMtx[1][3]=45;
  a.render();box=labelBox();
  assert(box.i.x==430&&box.i.y==310&&box.f.x==706&&box.f.y==334);
  assert(reads==readCount&&a.label()->fontSize.mSizeY==20);
  for(const Box origin:{Box(0,0,400,200),Box(80,30,480,230),Box(-200,-100,200,100)}){
   a.data->bounds=origin;a.render();const auto placed=labelBox();
   assert(placed.i.x==430&&placed.i.y==310&&placed.f.x==706&&placed.f.y==334);
  }
  // The name and label must share the actual printed baseline, including
  // local nonzero bounds, print offsets and integer binding at fractional zoom.
  auto* name=static_cast<J2DTextBox*>(a.screen->search(MULTI_CHAR('f_name01')));
  auto* japanese=static_cast<J2DTextBox*>(a.screen->search(MULTI_CHAR('w_name01')));
  for(auto* reference:{name,japanese}){
   name->hide();japanese->hide();reference->show();
   reference->mBounds=Box(-10,-7,100,26);reference->field_0x110=2.5f;
   reference->setCharSpace(1);reference->setLineSpace(25);
   reference->mPositionMtx[1][3]=13;
   for(auto bind:{VBIND_TOP,VBIND_CENTER,VBIND_BOTTOM})for(float zoom:{1.f,1.125f,2.f}){
    reference->mFlags=(HBIND_RIGHT<<2)|bind;
    a.base->bounds=Box(0,0,400*zoom,200*zoom);a.render();
    assert(std::abs(baseline(a.label())-baseline(reference))<0.0001f);
    assert(a.label()->fontSize.mSizeY==reference->fontSize.mSizeY);
    assert(a.label()->getGlbVtx(0).x==a.group->getGlbVtx(0).x);
   }
   // Narrower replacement layouts fit long levels without lowering the line.
   a.group->mPositionMtx[0][0]=0.5f;a.render();
   assert(a.label()->fontSize.mSizeX<reference->fontSize.mSizeX);
   assert(std::abs(baseline(a.label())-baseline(reference))<0.0001f);
   a.group->mPositionMtx[0][0]=2;
  }
  japanese->hide();name->show();a.base->bounds=Box(0,0,400,200);
  assert(reads==readCount);
  a.data->mAlpha=64;a.render();assert(a.label()->mColorAlpha==64);a.data->mAlpha=255;
  unsigned drawn=a.label()->draws;a.data->hide();a.render();assert(a.label()->draws==drawn);a.data->show();
  auto* style=a.screen->search(MULTI_CHAR('f_name01'));
  style->hide();a.render();assert(a.label()->draws==drawn);style->show();
  a.group->mPositionMtx[0][3]=NAN;a.render();assert(a.label()->draws==drawn);a.group->mPositionMtx[0][3]=30;
  a.render();assert(a.label()->draws==drawn+1);
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
  panel.group->mPositionMtx[0][3]=NAN;panel.refresh(0);assert(panel.label()->draws==0);
 }
 assert(livePanes==0);
 std::puts("Survivors save slots: per-slot/legacy levels, copy/erase refresh, native layout/lifetime and isolation passed");
}
'''
# Compile actual SDK draw/vertex/string functions, including the native
# mBasePane-to-Nm_02 bridge that is only applied at render time.
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

native = native_function('dusklight/libs/JSystem/src/J2DGraph/J2DPane.cpp', 'Vec J2DPane::getGlbVtx(')
native += native_function('dusklight/libs/JSystem/src/J2DGraph/J2DPrint.cpp', 'void J2DPrint::printReturn(')
native += native_function('dusklight/src/d/d_file_sel_info.cpp', 'void dDlst_FileInfo_c::draw(')
native += native_function('dusklight/libs/JSystem/src/J2DGraph/J2DTextBox.cpp', 's32 J2DTextBox::setString(s16')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture + native + (root/'src/survivors_save_slot.inc').read_text() + main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-Wno-multichar','-Wno-overflow','-Wno-unused-variable','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
