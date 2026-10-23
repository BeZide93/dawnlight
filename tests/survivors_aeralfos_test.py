"""Execute owned Aeralfos cadence, altitude, landing and native-state adapters."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include "survivors_field_boss_rules.hpp"
using namespace dawnlight::survivors;
using s16=short;
constexpr int fpcNm_B_GG_e=7;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct cXyz{float x=0,y=0,z=0;};
struct Sphere{bool at=true,hit=true;void OffAtSetBit(){at=false;}void ResetAtHit(){hit=false;}};
struct daB_GG_c{
 struct{cXyz pos;}current,home;
 cXyz speed;float speedF=0,gravity=0,field_0x5c0=0,field_0x5bc=0,field_0x648=0;
 int mAction=0,mSubAction=1,mMode=1,mAnm=14,field_0x65a=1;s16 mTimers[4]{};
 struct{float floor=0;float GetGroundH(){return floor;}}mAcch;
 Sphere mCcSph[3];
 void SpeedClear(){speedF=field_0x5bc=speed.y=field_0x5c0=0;}
 void SetAction(int a,int s,int m){mAction=a;mSubAction=s;mMode=m;}
 void SetAnm(int a,int,float,float){mAnm=a;}
};
struct Enemy{bool elite=true,initialized=true,retiring=false,credited=false;int profile=fpcNm_B_GG_e,stagger=0;AeralfosCycle aeralfosCycle;}owner;
bool owned=true,running=true,playing=true,dry=true;float floorY=0;
Enemy* encounter_owner(daB_GG_c*){return owned?&owner:nullptr;}
bool encounter_running(const Enemy& e){return e.initialized&&!e.retiring&&!e.credited&&running;}
bool playable(){return playing;}
bool loot_floor_position(cXyz& p){p.y=floorY;return dry;}
'''
main=r'''
int main(){
 daB_GG_c actor;actor.current.pos.y=300;std::any args[]={&actor};
 auto pre=[&]{return before_aeralfos_action(nullptr,args,nullptr,nullptr);};
 auto post=[&]{after_aeralfos_action(nullptr,args,nullptr,nullptr);};
 auto matrix=[&]{before_aeralfos_matrix(nullptr,args,nullptr,nullptr);};
 // Three extra ticks quarter idle, orbit and preparation delays, never a dive.
 for(auto state:{std::array{0,0,1,14},std::array{0,1,1,14},std::array{0,2,10,11},std::array{1,1,2,30}}){
  owner={};actor.SetAction(state[0],state[1],state[2]);actor.mAnm=state[3];actor.mTimers[0]=100;
  for(int i=0;i<25;++i){--actor.mTimers[0];pre();}
  assert(actor.mTimers[0]==0);
  for(int remaining=0;remaining<=3;++remaining){actor.mTimers[0]=remaining;pre();assert(actor.mTimers[0]==0);}
 }
 actor.SetAction(0,2,1);actor.mTimers[0]=80;pre();assert(actor.mTimers[0]==80);
 // Flight control halves the original 500..800 band above varied terrain.
 owner={};actor.SetAction(0,1,1);
 for(float terrain:{-500.f,0.f,1800.f}){
  actor.mAcch.floor=terrain;actor.current.pos.y=terrain+200;actor.field_0x5c0=8;post();assert(actor.field_0x5c0==3);
  actor.current.pos.y=terrain+450;actor.speed.y=60;post();matrix();
  assert(actor.current.pos.y==terrain+400&&actor.speed.y==0&&actor.field_0x5c0<=0);
 }
 // Recovery can finish at the lowered ceiling, preserving its native return.
 actor.mAcch.floor=0;actor.current.pos.y=400;actor.SetAction(0,2,2);actor.mAnm=7;
 post();assert(actor.mAction==0&&actor.mSubAction==1&&actor.mMode==0&&actor.mAnm==14);
 for(auto& s:actor.mCcSph)assert(!s.at&&!s.hit);
 // Every 300 active updates alternates; descent is smooth and requires dry ground.
 owner={};actor.SetAction(0,1,1);actor.current.pos.y=300;
 for(int i=0;i<299;++i)pre();assert(!owner.aeralfosCycle.ground);
 dry=false;pre();assert(owner.aeralfosCycle.ground&&actor.mAction==0&&actor.current.pos.y==300);
 dry=true;assert(pre()==HOOK_SKIP_ORIGINAL&&actor.speed.y==-15&&actor.gravity==0);
 for(int i=0;i<20&&actor.mAction==0;++i){actor.current.pos.y+=actor.speed.y;pre();}
 assert(actor.mAction==1&&actor.mSubAction==1&&actor.mMode==2&&actor.current.pos.y==0);
 actor.mTimers[2]=0;actor.current.pos.y=-10;pre();assert(actor.mTimers[2]==2&&actor.field_0x648==-10);
 actor.SetAction(1,1,4);pre();assert(actor.mMode==2); // Native timeout cannot take off early.
 owner.aeralfosCycle.ticks=299;pre();assert(!owner.aeralfosCycle.ground&&actor.mMode==4);
 // Freeze cadence and readiness on pause, teardown and stagger.
 for(int kind=0;kind<4;++kind){owner={};actor.mTimers[0]=80;
  if(kind==0)playing=false;if(kind==1)running=false;if(kind==2)owner.stagger=1;if(kind==3)owner.retiring=true;
  assert(pre()==HOOK_SKIP_ORIGINAL&&owner.aeralfosCycle.ticks==0&&actor.mTimers[0]==80);
  playing=running=true;
 }
 // Non-owned and ordinary Aeralfos keep their native height/cadence.
 owner={};owned=false;actor.current.pos.y=900;assert(pre()==HOOK_CONTINUE);matrix();assert(actor.current.pos.y==900);
 owned=true;owner.elite=false;pre();matrix();assert(actor.current.pos.y==900&&owner.aeralfosCycle.ticks==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_aeralfos.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Aeralfos: half-height terrain cap/recovery, quarter readiness, 10s phases, landing, pause and isolation passed')
