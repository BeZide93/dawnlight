"""Native Stalhound wind-up must complete with the Survivors facing/aim hooks."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'dusklight')

def function(path, signature):
    text = path.read_text()
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 0
    for end in range(brace, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if not depth:
            return text[start:end + 1]
    raise AssertionError(signature)

ai = root / 'src/survivors_extended_ai.inc'
production = function(ai, 'bool committed_enemy_motion(') + function(ai, 'void face_extended_enemy(')
production += function(root / 'src/survivors_boss_tactics.inc', 'void prepare_stalhound_pounce(')
production += function(root / 'src/survivors_boss_tactics.inc', 'void after_boss_aim(')
native = function(sdk / 'src/d/actor/d_a_e_sh.cpp', 'static void e_sh_attack(')
profiles = sorted(set(re.findall(r'fpcNm_\w+_e', production)))
types = sorted(set(re.findall(r'reinterpret_cast<(?:const )?(\w+)\*>', production)))
fixture = r'''
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <map>
using s16=short;using ActorId=unsigned;using ProfileName=int;
struct ModContext{};
namespace mods {template<class T>T arg(void* p,unsigned i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct cXyz {float x=0,y=0,z=0;cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 float absXZ()const{return std::hypot(x,z);}};
struct fopAc_ac_c {ActorId id=1;struct{cXyz pos;struct{s16 y=0;}angle;}current;
 struct{s16 y=0;}shape_angle;cXyz scale{2,2,2},speed;float speedF=0;};
struct Morph {unsigned frame=0,end=10;bool isStop(){return frame>=end;}};
struct Native:fopAc_ac_c {
 fopAc_ac_c& enemy=*this;
 int field_0x676=3,field_0x678=0,field_0x6aa=0,field_0x674=0,field_0xcea=1,field_0xce8=0;
 s16 field_0x68c=0,field_0x688=0,field_0x698[4]{};float field_0x690=300;
 Morph morph;Morph* mAnm_p=&morph;
 struct{bool ChkAtShieldHit(){return false;}}mSph2;
 struct{bool ground=false;bool ChkGroundHit(){return ground;}}mObjAcch;
 struct{void startCreatureVoice(int,int){}}mSound;
 int animation=0;int action=0,mode=0,mAction=0,mMode=0,mActionMode=0,mMoveMode=0;
 int mCurAction=0,mActionPhase=0,mActionMode1=0;
 s16 angle_target=0,mTargetAngleY=0,mCurrentAngleYTarget=0,field_0x6c0=0;
};
'''
fixture += 'enum{' + ','.join(profiles) + '};\n'
fixture += '\n'.join(f'using {t}=Native;' for t in types) + '\n'
fixture += r'''
constexpr int ACT_ATTACK=1,ACTION_ATTACK=1,Z2SE_EN_SH_V_BREATH=1,Z2SE_EN_SH_V_ATTACK=2;
enum class Species{Stalhound,KingBulblin};
struct Enemy{ActorId id=1;bool elite=true;int profile=fpcNm_E_SH_e;unsigned stagger=0,species=unsigned(Species::Stalhound);}owner;
struct Tactics{bool attacking=true;unsigned phaseTick=0;s16 yaw=0;bool targetLocked=false;cXyz target;}tactics;
std::map<ActorId,Tactics>s_bossTactics;
fopAc_ac_c player;bool arena=true,owned=true;
bool in_arena(){return arena;}auto* daAlink_getAlinkActorClass(){return &player;}
auto* dComIfGp_getPlayer(int){return &player;}
ActorId fopAcM_GetID(const fopAc_ac_c* a){return a->id;}
Enemy* tracked(ActorId id){return owned&&id==1?&owner:nullptr;}
bool boss_tactics_owned(const Enemy& e){return e.elite;}
bool extended_enemy(int){return true;}
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793);}
cXyz boss_lead_target(float){return player.current.pos;}
s16 boss_target_yaw(fopAc_ac_c* actor,const cXyz& target){return cM_atan2s(target.x-actor->current.pos.x,target.z-actor->current.pos.z);}
int TREG_F(int){return 0;}int JREG_F(int){return 0;}float cM_rndF(float){return 0;}
struct{float mCheck3=800;}l_HIO;
void anm_init(Native* a,int animation,float,int,float){a->animation=animation;a->morph.frame=0;}
void cLib_addCalc0(float* value,float,float step){*value=std::max(0.f,*value-step);}
void cLib_addCalcAngleS2(s16* angle,s16 target,int divisor,int step){
 const int delta=s16(target-*angle);int change=delta/divisor;
 if(delta&&!change)change=delta>0?1:-1;
 *angle=s16(*angle+std::clamp(change,-step,step));
}
'''
main = r'''
s16 aim(Native& hound){
 s16 yaw=cM_atan2s(player.current.pos.x-hound.current.pos.x,player.current.pos.z-hound.current.pos.z);
 std::any args[]={static_cast<const fopAc_ac_c*>(&hound),static_cast<const fopAc_ac_c*>(&player)};
 after_boss_aim(nullptr,args,&yaw,nullptr);return yaw;
}
void tick(Native& hound){
 // Production ordering: general facing, native action's searchPlayerAngle,
 // native wind-up, then generic post-movement facing. The director has already
 // sampled a different direction; moving Link must not deadlock the alignment.
 face_extended_enemy(&hound,owner);
 auto& t=s_bossTactics[hound.id];++t.phaseTick;
 if(hound.field_0x698[0]>0)--hound.field_0x698[0];
 hound.field_0x68c=aim(hound);
 e_sh_attack(&hound);
 prepare_stalhound_pounce(owner,&hound);
 face_extended_enemy(&hound,owner);
 ++hound.morph.frame;
}
int main(){
 Native hound;player.current.pos={500,0,0};s_bossTactics[1]={true,10,0,false,{}};
 // Red regression: old aim hook uses yaw 0 after ten ticks while general
 // facing resets yaw to 0x4000. Native phase 1 never passes its <0x800 test.
 for(int i=0;i<60 && hound.field_0x678<2;++i)tick(hound);
 assert(hound.field_0x678==2&&hound.animation==5);
 assert(s_bossTactics[1].yaw==0x4000&&hound.current.angle.y==0x4000&&hound.shape_angle.y==0x4000);
 player.current.pos={0,0,500}; // Dodging after the tell must not steer the airborne attack.
 // The tell still plays. Then the native launch and bite really run.
 assert(hound.speed.y==0);
 for(int i=0;i<20 && hound.field_0x678<3;++i)tick(hound);
 assert(hound.field_0x678==3&&hound.speedF==40&&hound.speed.y==20&&hound.animation==6);
 tick(hound);assert(hound.field_0xce8==1);
 hound.mObjAcch.ground=true;tick(hound);assert(hound.field_0x678==4&&hound.animation==7);
 for(int i=0;i<20 && hound.field_0x676==3;++i)tick(hound);
 assert(hound.field_0x676==2&&hound.field_0x678==0);
 player.current.pos={500,0,0};s_bossTactics[1].yaw=0;
 // Wind-up follows Link; telegraph/flight preserve the sampled direction.
 for(int mode=0;mode<=4;++mode){hound.field_0x676=3;hound.field_0x678=mode;
  hound.current.angle.y=123;hound.shape_angle.y=456;
  face_extended_enemy(&hound,owner);
  assert(hound.current.angle.y==(mode<2?0x4000:123));
  assert(aim(hound)==(mode<2?0x4000:0));
 }
 owner.elite=false;hound.field_0x678=1;assert(aim(hound)==0x4000);owner.elite=true;
 arena=false;hound.field_0x678=3;assert(aim(hound)==0x4000);arena=true;
 owned=false;assert(aim(hound)==0x4000);owned=true;
 owner.stagger=1;hound.current.angle.y=123;hound.field_0x678=1;
 face_extended_enemy(&hound,owner);assert(hound.current.angle.y==123);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture + production + native + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-variable', '-Wno-implicit-fallthrough', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Stalhound: native wind-up, telegraph, launch, bite, landing and facing/aim ownership passed')
