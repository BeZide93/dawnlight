"""Shadow Beast spacing with the SDK's actual chase, wind-up and attack windows."""
from pathlib import Path
import os,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1] if len(sys.argv)>1 else root/'dusklight')
def function(text,signature):
 start=text.index(signature);brace=text.index('{',start);depth=0
 for end in range(brace,len(text)):
  depth+=(text[end]=='{')-(text[end]=='}')
  if not depth:return text[start:end+1]
 raise AssertionError(signature)
source=(root/'src/survivors_boss_tactics.inc').read_text()
helpers='\n'.join(function(source,sig) for sig in ['void boss_ground_step(', 'void prepare_shadow_beast_melee(', 'void limit_shadow_beast_approach('])
native=(sdk/'src/d/actor/d_a_e_s1.cpp').read_text()
code='\n'.join(function(native,sig) for sig in ['static BOOL pl_at_check(', 'static void e_s1_fight_run(', 'static void e_s1_fight('])
# The decompiled SDK retains four unused local vectors. Suppress that warning
# only for the verbatim native functions; production helpers and test code keep
# all -Werror checks under both GCC and Clang.
code='\n#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored "-Wunused-variable"\n'+code+'\n#pragma GCC diagnostic pop\n'
fixture=r'''
#include <algorithm>
#include <cassert>
#include <cmath>
using s16=short;using f32=float;using BOOL=int;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}float absXZ()const{return std::hypot(x,z);}};
struct fopAc_ac_c{struct{cXyz pos;struct{s16 y=0;}angle;}current;struct{s16 y=0;}shape_angle;
 cXyz scale{2,2,2},speed;float speedF=0;};
struct Morph{float frame=0;float getFrame(){return frame;}void setFrame(float f){frame=f;}bool isStop(){return frame>40;}};
struct e_s1_class:fopAc_ac_c{int mAction=2,mMode=0,mTimers[4]{},mIsSlowAttack=0,field_0x6b9=0,mAnm=0;
 float mSearchRange=100000;Morph morph;Morph* mpMorf=&morph;
 struct{void startCreatureVoice(int,int){}void startCreatureSound(int,int,int){}}mSound;};
fopAc_ac_c player;bool hasPlayer=true,pathValid=true,scaledDistance=true;
auto* daAlink_getAlinkActorClass(){return hasPlayer?&player:nullptr;}
float cM_ssin(s16 a){return std::sin(a*3.141592653589793/32768);}
float cM_scos(s16 a){return std::cos(a*3.141592653589793/32768);}
s16 fopAcM_searchPlayerAngleY(fopAc_ac_c* a){const auto d=player.current.pos-a->current.pos;return s16(std::atan2(d.x,d.z)*32768/3.141592653589793);}
float fopAcM_searchPlayerDistance(e_s1_class* a){return (player.current.pos-a->current.pos).absXZ()/(scaledDistance?a->scale.x:1.f);}
bool walkable_spawn_path(cXyz,cXyz&,float){return pathValid;}
void cLib_addCalcAngleS2(s16* a,s16 target,int,int){*a=target;}
void cLib_addCalc2(float* a,float target,float,float step){*a+=std::clamp(target-*a,-step,step);}
float roll=0;float cM_rndF(float n){return n*roll;}
struct{float mDashSpeed=20,field_0x18=300;}l_HIO;
struct J3DFrameCtrl{enum{EMode_LOOP,EMode_NONE};};
enum{ANM_DASH_01=8,ANM_WAIT_01=29,ANM_ATTACK=5,ANM_ATTACK_02=6,ACT_WAIT=0,ACT_FIGHT_RUN=2,ACT_FIGHT=3};
enum{Z2SE_EN_NS_V_ATTACK,Z2SE_EN_NS_V_ATTACK2,Z2SE_EN_NS_UDEHURI,Z2SE_EN_NS_HAND_L};
void anm_init(e_s1_class* a,int animation,float,int,float){a->mAnm=animation;a->morph.frame=0;}
bool pl_check(e_s1_class*,float){return true;}
'''
main=r'''
int main(){
 for(bool scaling:{false,true})for(float speed:{1.f,3.f,30.f,300.f})for(float start:{0.f,50.f,350.f,550.f,700.f,2000.f})
 for(s16 angle:{s16(0),s16(0x4000),s16(-0x4000),s16(0x7000)})for(float selection:{0.f,.8f}){
  scaledDistance=scaling;roll=selection;e_s1_class boss;player.current.pos={1234,20,5678};
  boss.current.pos=player.current.pos-cXyz(cM_ssin(angle)*start,0,cM_scos(angle)*start);boss.shape_angle.y=angle;
  boss.speedF=20;unsigned active=0;bool started=false;cXyz attackStart;
  for(unsigned tick=0;tick<150;++tick){
   prepare_shadow_beast_melee(&boss);const auto origin=boss.current.pos;
   if(boss.mAction==3&&boss.mMode==1&&boss.mTimers[0]>0)--boss.mTimers[0];
   for(auto& timer:boss.mTimers)if(timer>0)--timer;
   boss.field_0x6b9=0;
   if(boss.mAction==2)e_s1_fight_run(&boss);else if(boss.mAction==3)e_s1_fight(&boss);
   const float step=boss.speedF*1.3f*1.5f*speed;
   boss.current.pos=boss.current.pos+cXyz(cM_ssin(boss.current.angle.y)*step,0,cM_scos(boss.current.angle.y)*step);
   limit_shadow_beast_approach(&boss,origin);
   if(boss.mAction==3&&boss.mMode==2){
    if(!started){started=true;attackStart=boss.current.pos;}
    const float distance=(player.current.pos-boss.current.pos).absXZ();assert(distance>=439.9f&&distance<=600.1f);
    assert((boss.current.pos-attackStart).absXZ()<.001f);
    if(boss.field_0x6b9)++active;
   }else if(started)break;
   ++boss.morph.frame;
  }
  assert(started&&active==(selection==0?10u:9u)); // Native windows remain intact.
 }
 // Clamp the first crossing even when a boosted step ends beyond Link.
 e_s1_class boss;player.current.pos={0,0,0};const cXyz origin{0,0,-900};
 boss.current.pos={0,17,3000};limit_shadow_beast_approach(&boss,origin);
 assert(std::abs(boss.current.pos.z+500)<.01f&&boss.current.pos.y==17);
 // Sideways movement, moving away, elevated targets and active swings are untouched.
 for(int reason=0;reason<4;++reason){boss={};boss.current.pos={900,0,-900};player.current.pos={0,0,0};
  if(reason==1)boss.current.pos={0,0,-1000};if(reason==2)player.current.pos.y=500;
  if(reason==3){boss.mAction=3;boss.mMode=2;boss.current.pos={0,0,3000};}
  const auto before=boss.current.pos;limit_shadow_beast_approach(&boss,origin);assert((before-boss.current.pos).absXZ()==0);
 }
 for(int action:{0,4,5,9,10,11,20}){boss={};boss.mAction=action;boss.speedF=20;
  const auto before=boss.current.pos;prepare_shadow_beast_melee(&boss);assert(boss.mAction==action&&boss.speedF==20&&(before-boss.current.pos).absXZ()==0);}
 // If Link leaves during wind-up, chase again; blocked retreat never stalls readiness.
 boss={};boss.mAction=3;boss.mMode=1;boss.current.pos={0,0,-900};player.current.pos={};
 prepare_shadow_beast_melee(&boss);assert(boss.mAction==2&&boss.mMode==0);
 boss.current.pos={};boss.mAction=3;boss.mMode=1;boss.mTimers[0]=0;pathValid=false;
 prepare_shadow_beast_melee(&boss);assert(boss.mTimers[0]==0&&boss.current.pos.absXZ()==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(fixture+helpers+code+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Shadow Beast: native chase/braking, overlap recovery, boosted-step clamp, attack windows and scoped states passed')
