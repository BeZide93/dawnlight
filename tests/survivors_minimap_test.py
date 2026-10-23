"""Production minimap hooks: camera heading, native bug styling and run isolation."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <iostream>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u8=unsigned char;using s16=int16_t;
struct Vec{float x=0,y=0,z=0;Vec operator-(Vec b)const{return {x-b.x,y-b.y,z-b.z};}};using cXyz=Vec;
#define BE(T) T
struct ModContext{};enum HookAction{HOOK_CONTINUE};
namespace mods{template<class T>T& arg_ref(void* a,int n){return std::any_cast<T&>(static_cast<std::any*>(a)[n]);}template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
constexpr unsigned kNone=~0u;
struct Enemy{unsigned id=kNone;bool elite=true,initialized=true,credited=false,retiring=false;};
std::array<Enemy,enemy_capacity>s_enemies;
struct Actor{struct{cXyz pos;}current;int health=100,room=0;};
std::array<Actor,enemy_capacity>actors;
Actor* actor_by_id(unsigned id){return id<actors.size()?&actors[id]:nullptr;}
int fopAcM_GetRoomNo(Actor* a){return a->room;}
Run s_run;bool arenaActive=true,changingStage=false;
bool in_arena(){return arenaActive;}bool transitioning(){return changingStage;}
struct Arena{int room=0;}currentArena;const Arena& arena(){return currentArena;}
namespace dMapInfo_n{void correctionOriginPos(int room,Vec* pos){
 assert(room==currentArena.room);const float x=pos->x;pos->x=pos->z+100;pos->z=-x+200;
}}

s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768.0/3.141592653589793);}
struct Link{int field_0x317c=2; s16 modelAngle=1234;}link;
bool playerAvailable=true,cameraAvailable=true,roomAvailable=true;
Link* daAlink_getAlinkActorClass(){return playerAvailable?&link:nullptr;}
struct CameraActor{struct{Vec mEye{},mCenter{0,0,100};}mCamera;}camera;
CameraActor* dComIfGp_getCamera(int index){assert(index==2);return cameraAvailable?&camera:nullptr;}
struct Room{int field_0x1c=0;}room;
int dComIfGp_roomControl_getStayNo(){return 3;}
struct dStage_roomControl_c{static Room* getFileList2(int index){assert(index==3);return roomAvailable?&room:nullptr;}};
int textureObject=0;bool textureAvailable=true;
struct TextureGroup{int* getTexObjPointer(int type){assert(type==4);return textureAvailable?&textureObject:nullptr;}};
namespace dMpath_n{TextureGroup m_texObjAgg;}
struct renderingAmap_c{
 int mRoomNoSingle=0;bool icons=true,textured=false;float zoom=50;
 std::vector<Vec> drawn;unsigned setups=0,restores=0;
 bool isRendIcon(){return icons;}
 bool isDrawAreaCheck(Vec p){return p.x>-10000&&p.x<10000&&p.z>-10000&&p.z<10000;}
 float getIconSize(int group){assert(group==4);return 0.08f;}
 float getCmPerTexel(){return zoom;}
 void setTevSettingIntensityTextureToCI(){textured=true;++setups;}
 void setTevSettingNonTextureDirectColor(){textured=false;++restores;}
 void drawIconSingle(Vec p,float x,float y){assert(textured&&x==0.08f*zoom&&y==x);drawn.push_back(p);}
};
enum{GX_VA_POS,GX_VA_TEX0,GX_DIRECT,GX_INDEX8,GX_VTXFMT0,GX_POS_XY,GX_F32,GX_TEX_ST,GX_U8,GX_TEXMAP0,GX_TEVREG1,GX_TEVREG2};
struct GXColor{u8 r,g,b,a;};
unsigned gxChanges=0;
void GXClearVtxDesc(){++gxChanges;}
void GXSetVtxDesc(int,int){}
void GXSetVtxAttrFmt(int,int,int,int,int){}
void GXSETARRAY(int attribute,const u8* data,unsigned size,int stride,bool){
 assert(attribute==GX_VA_TEX0&&size==8&&stride==2);
 const u8 native[]{0,1,1,1,1,0,0,0};for(unsigned i=0;i<size;++i)assert(data[i]==native[i]);
}
void GXInvalidateTexAll(){}
void GXLoadTexObj(int* texture,int slot){assert(texture==&textureObject&&slot==GX_TEXMAP0);}
void GXSetTevColor(int reg,GXColor c){assert(c.r==(reg==GX_TEVREG1?132:136)&&c.g==0&&c.b==0&&c.a==0);}
'''
main = r'''
int main(){

 // Only the live player cursor inside the minimap follows the active camera.
 renderingAmap_c cursorMap;std::any mapArgs[]={&cursorMap};
 std::any cursorArgs[]={&cursorMap,Vec{},s16(1234),int(0x1e),float(25)};
 const auto cursor=[&]{cursorArgs[2]=s16(1234);assert(before_survivors_minimap_cursor(nullptr,cursorArgs,nullptr,nullptr)==HOOK_CONTINUE);return std::any_cast<s16>(cursorArgs[2]);};
 assert(cursor()==1234); // Full-screen map / other draw passes remain native.
 before_survivors_minimap(nullptr,mapArgs,nullptr,nullptr);
 for(const Vec direction:{Vec{0,0,100},Vec{100,90,0},Vec{0,-90,-100},Vec{-100,0,0}}){
  camera.mCamera.mEye={20,40,60};camera.mCamera.mCenter={20+direction.x,40+direction.y,60+direction.z};
  for(int rotation:{0,8192,30000,-30000}){
   room.field_0x1c=rotation;
   const s16 expected=s16(cM_atan2s(direction.x,direction.z)+rotation);
   assert(cursor()==expected&&link.modelAngle==1234);
  }
 }
 room.field_0x1c=8192;camera.mCamera.mEye={};camera.mCamera.mCenter={100,0,0};
 roomAvailable=false;assert(cursor()==16384);roomAvailable=true;
 for(int marker:{0x1f,0x20,4}){cursorArgs[3]=marker;assert(cursor()==1234);}cursorArgs[3]=int(0x1e);
 for(unsigned reason=0;reason<7;++reason){
  arenaActive=reason!=0;changingStage=reason==1;playerAvailable=reason!=2;cameraAvailable=reason!=3;
  camera.mCamera.mCenter=reason==4?Vec{0,100,0}:reason==5?Vec{std::numeric_limits<float>::quiet_NaN(),0,1}:reason==6?Vec{1,0,std::numeric_limits<float>::infinity()}:Vec{100,0,0};
  assert(cursor()==1234);
 }
 arenaActive=true;changingStage=false;playerAvailable=cameraAvailable=true;camera.mCamera.mCenter={100,0,0};
 before_survivors_minimap(nullptr,mapArgs,nullptr,nullptr);assert(s_minimapDrawDepth==2);
 after_elite_minimap(nullptr,mapArgs,nullptr,nullptr);assert(s_minimapDrawDepth==1&&cursor()==24576);
 after_elite_minimap(nullptr,mapArgs,nullptr,nullptr);assert(s_minimapDrawDepth==0&&cursor()==1234);
 assert(std::any_cast<int>(cursorArgs[3])==0x1e&&std::any_cast<float>(cursorArgs[4])==25);
 s_run.start(42);renderingAmap_c map;std::any args[]={&map};
 const auto draw=[&]{map.drawn.clear();before_survivors_minimap(nullptr,args,nullptr,nullptr);after_elite_minimap(nullptr,args,nullptr,nullptr);assert(!map.textured);};
 draw();assert(map.drawn.empty()&&gxChanges==0);
 // A dormant guard and an aggro/scheduled elite both use their actor positions.
 s_enemies[0].id=0;actors[0].current.pos={25,0,50};
 s_enemies[1].id=1;actors[1].current.pos={-30,0,70};
 draw();assert(map.drawn.size()==2&&map.drawn[0].x==150&&map.drawn[0].z==175);
 actors[0].current.pos={40,0,80};map.zoom=100;draw();assert(map.drawn[0].x==180&&map.drawn[0].z==160);
 // No marker for dying/deleted/uninitialized/ordinary/wrong-room/off-map actors.
 for(unsigned reason=0;reason<8;++reason){
  s_enemies[0]={0};actors[0]={};
  if(reason==0)s_enemies[0].credited=true;
  if(reason==1)s_enemies[0].retiring=true;
  if(reason==2)s_enemies[0].initialized=false;
  if(reason==3)s_enemies[0].elite=false;
  if(reason==4)actors[0].health=0;
  if(reason==5)actors[0].room=99;
  if(reason==6)actors[0].current.pos.x=20000;
  if(reason==7)s_enemies[0].id=kNone-1;
  draw();assert(map.drawn.size()==1);
 }
 // Replacement actors appear at their new position; there is no cached marker.
 s_enemies[0]={2};actors[2].current.pos={900,0,600};draw();assert(map.drawn[0].x==700&&map.drawn[0].z==-700);
 for(auto phase:{Phase::Upgrade,Phase::TreasureChoice,Phase::Treasure}){s_run.phase=phase;draw();assert(map.drawn.size()==2);}
 for(auto phase:{Phase::Loading,Phase::Ready,Phase::Victory,Phase::Defeat}){s_run.phase=phase;draw();assert(map.drawn.empty());}
 s_run.phase=Phase::Running;
 for(unsigned reason=0;reason<5;++reason){const unsigned before=gxChanges;
  arenaActive=reason!=0;changingStage=reason==1;textureAvailable=reason!=2;map.icons=reason!=3;map.mRoomNoSingle=reason==4?-1:0;
  draw();assert(map.drawn.empty()&&gxChanges==before);
 }
 assert(map.setups==map.restores);
 std::cout<<"Survivors minimap: camera heading, room rotation, fallback, cursor isolation and elite markers passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp = Path(temp) / 'test.cpp'
    exe = Path(temp) / 'test'
    cpp.write_text(fixture + (root / 'src/survivors_minimap.inc').read_text() + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
