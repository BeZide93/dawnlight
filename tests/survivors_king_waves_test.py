"""King Bulblin sword-wave release, exact range, collision, pause and cleanup."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <array>
#include "survivors_field_boss_rules.hpp"
#include <cassert>
#include <cmath>
#include <map>
#include <vector>
#include "survivors_enemy_hits.hpp"
using namespace dawnlight::survivors;
using ActorId=int;using u32=unsigned;using u16=unsigned short;using s16=short;
constexpr int kNone=-1;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}
 cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}
 float abs()const{return std::sqrt(x*x+y*y+z*z);}};
float cM_ssin(s16 a){return std::sin(a*3.141592653589793/32768);}
float cM_scos(s16 a){return std::cos(a*3.141592653589793/32768);}
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793);}
struct csXyz{s16 x=0,y=0,z=0;csXyz()=default;csXyz(s16 a,s16 b,s16 c):x(a),y(b),z(c){}};
struct Actor{int health=100;struct{cXyz pos;csXyz angle;}current;csXyz shape_angle;int tevStr=0;cXyz eyePos;};
struct e_hzelda_class:Actor{int mAction=3,mMode=3,field_0x6e4=2;cXyz mBallPos{0,100,0},mBallMove{0,0,35};};
struct e_rdb_class{Actor enemy;int mAction=3,mMode=1,field_0x6c4=1;};
s16 aimYaw=0;s16 fopAcM_searchPlayerAngleY(Actor*){return aimYaw;}
struct Enemy{ActorId id=1;bool retiring=false,credited=false;unsigned stagger=0;KingWaveClock kingWaveClock;};
Actor player;Actor* daAlink_getAlinkActorClass(){return &player;}
void setAim(s16 yaw,float height=0){aimYaw=yaw;player.current.pos={cM_ssin(yaw)*1000,height,cM_scos(yaw)*1000};}
struct SourceHit{struct{int type,power,flags;}at;struct{int type,flags;}tg;int co;};
struct dCcD_SrcSph{struct{struct{int flags;SourceHit hit;}obj;int at[5],tg[5],co[1];}info;struct{struct{cXyz pos;float radius;}sph;}attr;};
constexpr int AT_TYPE_CSTATUE_SWING=1,dCcD_SE_METAL=7,dCcD_SE_NONE=0;
struct dCcD_Stts{Actor* actor=nullptr;void Init(int,int,Actor* a){actor=a;}};
struct dCcD_Sph{bool active=false,hit=false,shield=false;int power=0,material=0;float radius=0;dCcD_Stts* status=nullptr;cXyz position;
 void Set(const dCcD_SrcSph& s){power=s.info.obj.hit.at.power;radius=s.attr.sph.radius;hit=shield=false;}
 void SetStts(dCcD_Stts* s){status=s;}void SetAtMtrl(int m){material=m;}
 void StartCAt(cXyz p){position=p;}void MoveCAt(cXyz p){position=p;}
 void OnAtSetBit(){active=true;}void OffAtSetBit(){active=false;}
 void ResetAtHit(){hit=false;}void OffAtShieldHit(){shield=false;}bool ChkAtHit(){return hit;}bool ChkAtShieldHit(){return shield;}};
using cCcD_Obj=dCcD_Sph;
struct Emitter{int mMaxFrame=0,age=0;int getAge(){return age;}bool stopped=false;void stopCreateParticle(){stopped=true;}void quitImmortalEmitter(){}void becomeInvalidEmitter(){}};
std::map<u32,Emitter>emitters;u32 nextEmitter=1;
auto* dComIfGp_particle_getEmitter(u32 id){return &emitters[id];}
constexpr u16 ID_ZI_J_KAITENGIRI_A=1,ID_ZI_J_KAITENGIRI_B=2;
u32 dComIfGp_particle_set(u32 id,u16 effect,const cXyz*,int*,const csXyz*,const cXyz* scale,int,void*,int,void*,void*,void*){
 assert(effect==1||effect==2);assert(scale->x==.35f);if(!id)id=nextEmitter++;emitters[id].stopped=false;return id;}

struct {unsigned random=123;unsigned next_random(){return random=random*1664525+1013904223;}unsigned ticks=0;enum Phase{Running,Upgrade};Phase phase=Running;}s_run;
using Phase=decltype(s_run)::Phase;
bool running=true,arena=true;bool in_arena(){return arena;}bool playable(){return running;}
struct Tactics{unsigned zeldaVolley=0;bool zeldaBallFlying=false;};std::map<ActorId,Tactics>s_bossTactics;
struct HitSource{EnemyAttackCycle cycle;EnemyAttackUsePtr magic;};std::map<ActorId,HitSource>s_enemyHitSources;
void observe_enemy_attack(Enemy& e,e_hzelda_class* a){s_enemyHitSources[e.id].cycle.observe(a->mAction,a->mAction,a->mMode);}
bool wall=false;struct dBgS_LinChk{void Set(const cXyz*,const cXyz*,Actor*){}};
struct BG{bool LineCross(dBgS_LinChk*){return wall;}}bg;auto& dComIfG_Bgsp(){return bg;}
std::vector<unsigned>registrations;
struct CC{void Set(dCcD_Sph* s){assert(s->active&&s->status->actor&&s->radius==90&&s->power==12);registrations.push_back(s_run.ticks);}}cc;
auto* dComIfG_Ccsp(){return &cc;}
'''
main=r'''
unsigned live(){unsigned count=0;for(auto& w:s_kingWaves)count+=w.owner!=kNone;return count;}
void reset(){clear_king_waves(kNone);emitters.clear();nextEmitter=1;s_run.ticks=0;s_run.phase=Phase::Running;running=arena=true;wall=false;registrations.clear();}
int main(){
 reset();e_rdb_class king;Enemy owner;
 setAim(0x4000);update_king_waves(&king,owner);
 assert(live()==1&&std::abs(s_kingWaves[0].velocity.x-35)<.001&&std::abs(s_kingWaves[0].velocity.z)<.001);
 assert(king.enemy.shape_angle.y==aimYaw&&s_kingWaves[0].remaining==2*boss_ring_radius-35);
 auto& first=s_kingWaves[0];assert(king_wave_use(&first.collider)==first.use);
 for(int i=0;i<9;++i)update_king_waves(&king,owner);assert(live()==1); // One per native strike window.
 king.field_0x6c4=0;update_king_waves(&king,owner);king.field_0x6c4=1;update_king_waves(&king,owner);assert(live()==2);
 // Every full second emits five waves: one freshly aimed at Link and four random.
 reset();owner={};king.mAction=4;king.mMode=1;king.field_0x6c4=0;
 update_king_waves(&king,owner);assert(live()==0);
 setAim(0x2000);king.mMode=2;king.field_0x6c4=2;update_king_waves(&king,owner);assert(live()==5);
 assert(s_kingWaves[0].rotation.y==aimYaw);
 for(unsigned i=1;i<5;++i)assert(s_kingWaves[i].use==s_kingWaves[0].use);
 assert(s_kingWaves[0].rotation.y!=s_kingWaves[1].rotation.y&&s_kingWaves[1].rotation.y!=s_kingWaves[2].rotation.y);
 assert(s_kingWaves[0].use==s_kingWaves[1].use&&s_kingWaves[1].use==s_kingWaves[2].use);
 for(int i=0;i<29;++i)update_king_waves(&king,owner);assert(live()==5);
 auto remaining=s_kingWaves[0].remaining;running=false;
 for(int i=0;i<90;++i)update_king_waves(&king,owner);
 assert(live()==5&&s_kingWaves[0].remaining==remaining&&owner.kingWaveClock.spinTicks==0);
 setAim(-0x4000);running=true;update_king_waves(&king,owner);assert(live()==10&&s_kingWaves[5].use!=s_kingWaves[0].use);
 assert(s_kingWaves[5].rotation.y==aimYaw&&s_kingWaves[0].rotation.y==0x2000);
 assert(std::abs(s_kingWaves[5].velocity.x+35)<.001&&std::abs(s_kingWaves[5].velocity.z)<.001);
 king.mMode=3;king.field_0x6c4=0;update_king_waves(&king,owner);assert(live()==10);
 // Exact diameter travel, including the final partial frame, then removal.
 reset();owner={};king.mAction=3;king.mMode=1;king.field_0x6c4=1;setAim(0);
 update_king_waves(&king,owner);king.mAction=1;king.field_0x6c4=0;
 for(int i=1;i<86;++i)update_king_waves(&king,owner);
 assert(s_kingWaves[0].position.z==2*boss_ring_radius&&s_kingWaves[0].remaining==0);
 update_king_waves(&king,owner);assert(live()==0);
 for(const auto& [id,e]:emitters)assert(e.stopped);
 // Aim normal strikes and the first spin wave uphill/downhill at release.
 for(bool spin:{false,true})for(float height:{630.f,-630.f}){
  reset();owner={};king.mAction=spin?4:3;king.mMode=spin?2:1;king.field_0x6c4=spin?2:1;
  setAim(0,height);player.current.pos.z=840;
  update_king_waves(&king,owner);auto& wave=s_kingWaves[0];
  assert(std::abs(wave.velocity.abs()-35)<.001&&wave.velocity.y*height>0);
  assert((height>0&&wave.rotation.x<0)||(height<0&&wave.rotation.x>0));
  if(spin)for(unsigned i=1;i<5;++i)assert(s_kingWaves[i].velocity.y==0);
  const auto velocity=wave.velocity;king.mAction=1;king.field_0x6c4=0;
  player.current.pos={5000,5000,5000}; // Released waves do not home after launch.
  for(int i=1;i<30;++i)update_king_waves(&king,owner);
  assert(std::abs(wave.position.y-(65+height))<.01&&std::abs(wave.position.z-840)<.01);
  assert(wave.velocity.y==velocity.y);
  for(int i=30;i<86;++i)update_king_waves(&king,owner);
  assert(std::abs((wave.position-cXyz(0,65,0)).abs()-2*boss_ring_radius)<.02&&wave.remaining==0);
 }
 // Degenerate target at the origin keeps a finite forward velocity.
 reset();owner={};king.mAction=1;const cXyz coincident(0,65,0);
 launch_king_wave(&king,owner,0,new_enemy_attack_use(),&coincident);
 assert(s_kingWaves[0].velocity.z==35&&s_kingWaves[0].velocity.y==0);
 // Walls, accepted hits, blocks and owner teardown remove all effects.
 for(int kind=0;kind<4;++kind){reset();owner={};king.mAction=3;king.field_0x6c4=1;update_king_waves(&king,owner);
  king.mAction=1;king.field_0x6c4=0;
  if(kind==0)wall=true;if(kind==1)s_kingWaves[0].collider.hit=true;
  if(kind==2)s_kingWaves[0].collider.shield=true;if(kind==3)owner.credited=true;
  update_king_waves(&king,owner);assert(live()==0);
 }
 reset();owner={};king.mAction=1;
 auto use=new_enemy_attack_use();launch_king_wave(&king,owner,0,use);
 Enemy other;other.id=2;launch_king_wave(&king,other,0,use);
 clear_king_waves(1);assert(live()==1);clear_king_waves(kNone);assert(live()==0);
 // Bounded stable storage and a fresh effect after a particle expires.
 for(int i=0;i<40;++i)launch_king_wave(&king,owner,0,use);assert(live()==32);
 update_king_waves(&king,owner);auto id=s_kingWaves[0].emitters[0];emitters[id].mMaxFrame=1;emitters[id].age=1;
 update_king_waves(&king,owner);assert(emitters[id].stopped&&s_kingWaves[0].emitters[0]!=id);
 clear_king_waves(kNone);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_king_waves.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('King waves: native strike release, 1s five-wave salvos with fresh 3D aim, diameter range, contact budget, walls, pause and teardown passed')
