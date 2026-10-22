"""Execute production Zelda fan/burst scheduling, collision and lifecycle code."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_zelda_orbs.inc').read_text()
fixture=r'''
#include <array>
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
struct Actor{int health=100;struct{cXyz pos;}current;cXyz eyePos;};
struct e_hzelda_class:Actor{int mAction=3,mMode=3,field_0x6e4=2;cXyz mBallPos{0,100,0},mBallMove{0,0,35};};
struct Enemy{ActorId id=1;bool retiring=false,credited=false;unsigned stagger=0;};
Actor player;Actor* daAlink_getAlinkActorClass(){return &player;}
struct SourceHit{struct{int type,power,flags;}at;struct{int type,flags;}tg;int co;};
struct dCcD_SrcSph{struct{struct{int flags;SourceHit hit;}obj;int at[5],tg[5],co[1];}info;struct{struct{cXyz pos;float radius;}sph;}attr;};
constexpr int AT_TYPE_CSTATUE_SWING=1,dCcD_SE_13=13,dCcD_SE_NONE=0,dCcD_MTRL_ELECTRIC=2;
struct dCcD_Stts{Actor* actor=nullptr;void Init(int,int,Actor* a){actor=a;}};
struct dCcD_Sph{bool active=false,hit=false,shield=false;int power=0,material=0;float radius=0;dCcD_Stts* status=nullptr;cXyz position;
 void Set(const dCcD_SrcSph& s){power=s.info.obj.hit.at.power;radius=s.attr.sph.radius;hit=shield=false;}
 void SetStts(dCcD_Stts* s){status=s;}void SetAtMtrl(int m){material=m;}
 void StartCAt(cXyz p){position=p;}void MoveCAt(cXyz p){position=p;}
 void OnAtSetBit(){active=true;}void OffAtSetBit(){active=false;}
 void ResetAtHit(){hit=false;}void OffAtShieldHit(){shield=false;}bool ChkAtHit(){return hit;}bool ChkAtShieldHit(){return shield;}};
using cCcD_Obj=dCcD_Sph;
struct Emitter{bool stopped=false;void stopCreateParticle(){stopped=true;}void quitImmortalEmitter(){}void becomeInvalidEmitter(){}};
std::map<u32,Emitter>emitters;u32 nextEmitter=1;
auto* dComIfGp_particle_getEmitter(u32 id){return &emitters[id];}
u32 dComIfGp_particle_set(u32 id,u16 effect,const cXyz*,void*,void*){assert(effect>=0x8918&&effect<=0x891a);if(!id)id=nextEmitter++;emitters[id].stopped=false;return id;}
struct {unsigned ticks=0;enum Phase{Running,Upgrade};Phase phase=Running;}s_run;
using Phase=decltype(s_run)::Phase;
bool running=true,arena=true;bool in_arena(){return arena;}bool playable(){return running;}
struct Tactics{unsigned zeldaVolley=0;bool zeldaBallFlying=false;};std::map<ActorId,Tactics>s_bossTactics;
struct HitSource{EnemyAttackCycle cycle;EnemyAttackUsePtr magic;};std::map<ActorId,HitSource>s_enemyHitSources;
void observe_enemy_attack(Enemy& e,e_hzelda_class* a){s_enemyHitSources[e.id].cycle.observe(a->mAction,a->mAction,a->mMode);}
bool wall=false;struct dBgS_LinChk{void Set(const cXyz*,const cXyz*,Actor*){}};
struct BG{bool LineCross(dBgS_LinChk*){return wall;}}bg;auto& dComIfG_Bgsp(){return bg;}
std::vector<unsigned>registrations;
struct CC{void Set(dCcD_Sph* s){assert(s->active&&s->status->actor&&s->radius==15&&s->power==4&&s->material==dCcD_MTRL_ELECTRIC);registrations.push_back(s_run.ticks);}}cc;
auto* dComIfG_Ccsp(){return &cc;}
'''
main=r'''
unsigned live(){unsigned n=0;for(auto& o:s_zeldaLightOrbs)n+=o.owner!=kNone;return n;}
void reset(){clear_zelda_light_orbs(kNone);s_bossTactics.clear();s_enemyHitSources.clear();s_run.ticks=0;
 s_run.phase=Phase::Running;arena=running=true;wall=false;registrations.clear();player.eyePos={0,140,1200};}
int main(){
 reset();e_hzelda_class zelda;Enemy owner;
 update_zelda_light_volley(&zelda,owner);
 assert(live()==2&&registrations.size()==2&&s_bossTactics[1].zeldaVolley==1);
 auto& left=s_zeldaLightOrbs[0];auto& right=s_zeldaLightOrbs[1];
 assert(left.velocity.x<0&&right.velocity.x>0&&std::abs(left.velocity.x+right.velocity.x)<.001f);
 assert(std::abs(left.velocity.abs()-35)<.001f&&left.velocity.z==right.velocity.z);
 assert(left.use==right.use&&left.use==s_enemyHitSources[1].magic);
 assert(zelda_light_orb_use(&left.collider)==left.use);
 dCcD_Sph unrelated;assert(!zelda_light_orb_use(&unrelated));
 const cXyz fixed=left.velocity;player.eyePos={1000,200,0};
 ++s_run.ticks;update_zelda_light_volley(&zelda,owner);assert(live()==2&&s_bossTactics[1].zeldaVolley==1);
 assert((left.velocity-fixed).abs()==0); // No homing, no duplicate release per frame.
 left.collider.hit=true;++s_run.ticks;update_zelda_light_volley(&zelda,owner);assert(live()==1);
 right.collider.shield=true;++s_run.ticks;update_zelda_light_volley(&zelda,owner);assert(live()==0);
 for(auto& [id,e]:emitters){(void)id;assert(e.stopped);}
 // Reflections and native recovery cannot create another volley.
 zelda.field_0x6e4=3;update_zelda_light_volley(&zelda,owner);
 zelda.field_0x6e4=2;update_zelda_light_volley(&zelda,owner);assert(live()==0&&s_bossTactics[1].zeldaVolley==1);
 // Second cast: two delayed additions at exactly +9 and +18 active ticks.
 zelda.field_0x6e4=1;update_zelda_light_volley(&zelda,owner);
 zelda.field_0x6e4=2;zelda.mMode=4;s_run.ticks=100;registrations.clear();update_zelda_light_volley(&zelda,owner);
 assert(live()==2&&registrations.empty()&&s_bossTactics[1].zeldaVolley==2);
 // Menu time and stagger cannot advance pending shots or particles.
 const auto delay=left.delay;const auto position=left.position;
 s_run.phase=Phase::Upgrade;for(int i=0;i<30;++i)update_zelda_light_volley(&zelda,owner);
 assert(left.delay==delay&&(left.position-position).abs()==0);s_run.phase=Phase::Running;
 owner.stagger=1;update_zelda_light_volley(&zelda,owner);assert(left.delay==delay);owner.stagger=0;
 for(unsigned t=101;t<=118;++t){s_run.ticks=t;zelda.current.pos.x+=2;update_zelda_light_volley(&zelda,owner);}
 assert(registrations.front()==109&&registrations.size()==11);
 assert(right.collider.active&&left.collider.active); // Third native+extra shot at +18.
 assert(std::abs(left.velocity.abs()-35)<.001f&&std::abs(right.velocity.abs()-35)<.001f);
 // Wall collisions, timeout and owner teardown release particles/colliders.
 wall=true;++s_run.ticks;update_zelda_light_volley(&zelda,owner);assert(live()==0);wall=false;
 zelda.field_0x6e4=1;update_zelda_light_volley(&zelda,owner);zelda.field_0x6e4=2;
 ++s_run.ticks;update_zelda_light_volley(&zelda,owner);assert(registrations.back()==s_run.ticks&&live()==2);
 assert(s_bossTactics[1].zeldaVolley==3); // Third cast is a fan again.
 for(int i=0;i<121;++i){++s_run.ticks;update_zelda_light_volley(&zelda,owner);}assert(live()==0);
 zelda.field_0x6e4=1;update_zelda_light_volley(&zelda,owner);zelda.field_0x6e4=2;
 ++s_run.ticks;update_zelda_light_volley(&zelda,owner);assert(live()==2);
 owner.credited=true;update_zelda_light_volley(&zelda,owner);assert(live()==0);owner.credited=false;
 // Separate bosses alternate independently and targeted cleanup spares others.
 reset();Enemy second;second.id=2;e_hzelda_class other;
 update_zelda_light_volley(&zelda,owner);update_zelda_light_volley(&other,second);
 assert(live()==4&&s_bossTactics[1].zeldaVolley==1&&s_bossTactics[2].zeldaVolley==1);
 clear_zelda_light_orbs(1);assert(live()==2);clear_zelda_light_orbs(kNone);assert(live()==0);
 for(auto& o:s_zeldaLightOrbs)assert(!o.collider.active&&!o.use);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
 cpp.write_text(fixture+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Zelda light orbs: alternating fan/burst, exact delays, cast budget, pause, collision and cleanup passed')
