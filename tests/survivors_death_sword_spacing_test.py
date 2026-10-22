"""Death Sword's production spacing against native chase/braking decisions."""
from pathlib import Path
import os, subprocess, sys, tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1] if len(sys.argv)>1 else root/'dusklight')
def function(text, signature):
    start=text.index(signature);brace=text.index('{',start);depth=0
    for end in range(brace,len(text)):
        depth+=(text[end]=='{')-(text[end]=='}')
        if not depth:return text[start:end+1]
    raise AssertionError(signature)
helper=function((root/'src/survivors_bosses.inc').read_text(),'void prepare_death_sword_melee(')
native=function((sdk/'src/d/actor/d_a_e_vt.cpp').read_text(),'void daE_VA_c::executeOpaciChase()')
# Arena adapter uses only native chase modes 12/13. Execute their exact SDK
# statements; unrelated dungeon orbit states require the dungeon's world origin.
cases=native[native.index('    case 12:'):native.rindex('    }')]
fixture=r'''
#include <algorithm>
#include <cassert>
#include <cmath>
using s16=short;
struct cXyz{float x=0,y=0,z=0;cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 float absXZ()const{return std::hypot(x,z);}};
struct Player{cXyz pos;bool checkNowWolf(){return false;}struct{cXyz pos;}current;}player;
auto* daAlink_getAlinkActorClass(){return &player;}
struct{double factor=1;double endless_factor(){return factor;}}s_run;
struct daE_VA_c{
 enum{ACTION_OPACI_CHASE_e=15,ACTION_OPACI_ATTACK_e=16};int mAction=15,mMode=12,field_0x1388=0;
 struct{cXyz pos;struct{s16 y=0;}angle;}current;struct{s16 y=0;}shape_angle;
 float speedF=0;
 struct{void OnTgSetBit(){}}mBodyCyls[2];struct{void startCreatureVoiceLevel(int,int){}}mSound;
 void setActionMode(int a,int m){mAction=a;mMode=m;}
 void executeOpaciChase();
};
constexpr int Z2SE_EN_VA_V_ROTATE=0;
struct{float mAttackRange=500;}l_HIO;
float fopAcM_searchPlayerDistance(daE_VA_c* a){return (player.current.pos-a->current.pos).absXZ();}
void cLib_addCalcAngleS(s16* a,s16 target,int,int,int){*a=target;}
bool cLib_chaseF(float* value,float target,float step){
 *value+=std::clamp(target-*value,-step,step);return *value==target;
}
'''
native='void daE_VA_c::executeOpaciChase(){auto* player=&::player;s16 angleY_to_player=0;switch(mMode){\n'+cases+'}}\n'
main=r'''
int main(){
 for(double factor:{1.,3.,10.})for(float start:{50.f,220.f,450.f,650.f,1500.f})for(float speed:{0.f,100.f}){
  s_run.factor=factor;player.current.pos={1234,0,6789};daE_VA_c boss;
  boss.current.pos={1234,0,6789-start};boss.speedF=speed;
  for(unsigned tick=0;tick<400&&boss.mAction==15;++tick){
   prepare_death_sword_melee(&boss);boss.executeOpaciChase();
   boss.current.pos.z+=boss.speedF*float(1.5*factor);
  }
  assert(boss.mAction==16&&boss.speedF==0);
  const float distance=fopAcM_searchPlayerDistance(&boss);assert(distance>=350&&distance<=500);
 }
 // Wind-up/active strikes and magic phases keep their native trajectory.
 daE_VA_c boss;boss.mAction=16;boss.mMode=1;boss.speedF=12;
 prepare_death_sword_melee(&boss);assert(boss.mAction==16&&boss.mMode==1&&boss.speedF==12);
 boss.mAction=12;prepare_death_sword_melee(&boss);assert(boss.mAction==12&&boss.speedF==12);
 // Link leaving range during braking returns to chase instead of an empty swing.
 boss.mAction=15;boss.mMode=13;boss.current.pos={1234,0,5789};boss.speedF=50;
 prepare_death_sword_melee(&boss);assert(boss.mMode==12);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'spacing.cpp';exe=Path(tmp)/'spacing'
    cpp.write_text(fixture+helper+native+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Death Sword: native chase/braking, close retreat, attack spacing, Endless speed and active-attack exclusion passed')
