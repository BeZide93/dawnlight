"""Passive Stalhound device probe: real sphere/cylinder gaps and bounded logging."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
state=(root/'src/survivors_boss_tactics_state.inc').read_text().split('std::unordered_map')[0]
probe=(root/'src/survivors_stalhound_diagnostic.inc').read_text()
fixture=r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using ActorId=unsigned;using s16=short;using u32=unsigned;constexpr ActorId kNone=~0u;
struct cXyz{float x=0,y=0,z=0;cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 float abs2()const{return x*x+y*y+z*z;}float absXZ()const{return std::hypot(x,z);}};
struct csXyz{};struct BossWatchdog{};struct BossStagger{};
struct AttackUse{bool contacted=false;};using EnemyAttackUsePtr=std::shared_ptr<AttackUse>;
struct fopAc_ac_c{unsigned id=1;int health=100;float speedF=40;struct{cXyz pos;}current;cXyz scale{2,2,2};};
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
struct cCcD_Obj{};
struct Sphere:cCcD_Obj{cXyz center;float radius=70;bool enabled=true,hit=false;fopAc_ac_c* victim=nullptr;
 cXyz GetC()const{return center;}float GetR()const{return radius;}bool ChkAtSet()const{return enabled;}
 bool ChkAtHit()const{return hit;}fopAc_ac_c* GetAtHitAc(){return victim;}};
struct Cylinder{cXyz center;float radius=30,height=120;bool enabled=true;
 cXyz GetC()const{return center;}float GetR()const{return radius;}float GetH()const{return height;}bool ChkTgSet()const{return enabled;}};
struct Player:fopAc_ac_c{Cylinder mTgCyls[3];}player;
struct e_sh_class{fopAc_ac_c enemy;int field_0x676=3,field_0x678=3;Sphere mSph2;
 struct{bool ChkGroundHit(){return false;}}mObjAcch;};
[[maybe_unused]] constexpr int fpcNm_E_SH_e=1;
struct Enemy{unsigned id=1;int profile=1;bool elite=true,initialized=true,retiring=false,credited=false,dormant=false;};
bool playing=true,present=true;bool playable(){return playing;}Player* daAlink_getAlinkActorClass(){return present?&player:nullptr;}
struct ProbeRun{unsigned ticks=100;};[[maybe_unused]] ProbeRun s_run;unsigned s_map=2;void* mod_ctx=nullptr;
struct Log{std::vector<std::string> lines;void info(void*,const char* s){lines.emplace_back(s);}}logger;auto* svc_log=&logger;
struct Source{struct{EnemyAttackUsePtr use;}cycle;};std::unordered_map<ActorId,Source>s_enemyHitSources;
'''
main=r'''
int main(){
#if DAWNLIGHT_SURVIVORS_DEBUG
 Enemy e;e_sh_class h;auto& t=s_bossTactics[e.id];auto& p=t.stalhoundProbe;
 // A hidden native sphere keeps its At bit but must never look like a bite.
 h.mSph2.center={15000,-14000,23000};trace_stalhound_frame(e,&h.enemy);
 assert(p.frames==1&&p.visible==0&&p.enabled==0&&logger.lines.size()==1);
 // Known actual cylinder geometry: vertical miss of 20, then real overlap.
 h.mSph2.center={0,210,0};trace_stalhound_frame(e,&h.enemy);assert(p.gap==20&&p.overlaps==0);
 h.mSph2.center={0,180,0};trace_stalhound_frame(e,&h.enemy);assert(p.gap==-10&&p.overlaps==1&&p.radius==70);
 assert(p.target.y==0&&p.height==120&&p.cylinderRadius==30);
 h.mSph2.enabled=false;trace_stalhound_frame(e,&h.enemy);assert(p.visible==3&&p.enabled==2);
 // Sample the exact bite only; safety and already-spent budget remain separate.
 Sphere other;trace_stalhound_contact(e,&h.enemy,&other);assert(!p.attempts);
 t.recovery=12;trace_stalhound_contact(e,&h.enemy,&h.mSph2);assert(p.attempts==1&&p.safety==1&&!p.spent);
 t.recovery=0;s_enemyHitSources[e.id].cycle.use=std::make_shared<AttackUse>();
 auto use=s_enemyHitSources[e.id].cycle.use;use->contacted=true;
 trace_stalhound_contact(e,&h.enemy,&h.mSph2);assert(p.attempts==2&&p.safety==1&&p.spent==1);
 h.mSph2.hit=true;h.mSph2.victim=&player;trace_stalhound_frame(e,&h.enemy);
 assert(p.nativeHits==1&&p.accepted);
 // Passive: no enable/position/radius/HP/action/budget mutation.
 assert(!h.mSph2.enabled&&h.mSph2.center.y==180&&h.mSph2.radius==70&&h.enemy.health==100);
 assert(h.field_0x676==3&&h.field_0x678==3&&use->contacted&&s_enemyHitSources[e.id].cycle.use==use);
 h.field_0x676=2;h.field_0x678=0;trace_stalhound_frame(e,&h.enemy);
 assert(p.reports==1&&p.frames==0&&logger.lines.size()==2);
 assert(logger.lines.back().find("gap=-10.00")!=std::string::npos);
 assert(logger.lines.back().find("attempts=2 safety=1 spent=1 accepted=1")!=std::string::npos);
 // Frozen/story/dormant/dead/non-boss actors are excluded.
 for(int gate=0;gate<8;++gate){e={};playing=present=true;h.enemy.health=100;
  if(gate==0)e.elite=false;if(gate==1)e.retiring=true;if(gate==2)e.dormant=true;
  if(gate==3)e.initialized=false;if(gate==4)e.credited=true;if(gate==5)playing=false;
  if(gate==6)present=false;if(gate==7)h.enemy.health=0;
  trace_stalhound_frame(e,&h.enemy);assert(!p.frames);
 }
 e={};playing=present=true;h.enemy.health=100;h.mSph2.hit=false;h.mSph2.enabled=true;
 // Horizontal miss and height are not conflated; diagonal sphere/cylinder gap.
 h.mSph2.center={100,120,0};trace_stalhound_frame(e,&h.enemy);assert(p.gap==0);
 // Idle or stuck attacks still produce bounded reports without any contacts.
 for(int i=0;i<2000;++i)trace_stalhound_frame(e,&h.enemy);
 assert(p.reports==8&&logger.lines.size()==9);
 const auto count=logger.lines.size();trace_stalhound_frame(e,&h.enemy);assert(logger.lines.size()==count);
 // Existing boss-state cleanup removes all probe history, including reused IDs.
 s_bossTactics.clear();s_bossTactics[e.id]={};trace_stalhound_frame(e,&h.enemy);
 assert(s_bossTactics[e.id].stalhoundProbe.reports==0&&logger.lines.size()==count+1);
#endif
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+state+'std::unordered_map<ActorId,BossTactics>s_bossTactics;\n'+probe+main)
 for enabled in (0,1):
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}',str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Stalhound device probe: geometric gaps, native sentinel, filters, passivity, lifecycle and log cap passed')
