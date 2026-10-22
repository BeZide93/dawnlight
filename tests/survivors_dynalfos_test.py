"""Owned Dynalfos readiness/guard through the SDK's actual chase and guard."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1] if len(sys.argv)>1 else root/'dusklight')
text=(sdk/'src/d/actor/d_a_e_mf.cpp').read_text()
def extract(signature):
 start=text.index(signature);brace=text.index('{',start);depth=0
 for end in range(brace,len(text)):
  depth+=(text[end]=='{')-(text[end]=='}')
  if not depth:return text[start:end+1]
 raise AssertionError(signature)
native='\n'.join(extract(sig) for sig in ['static BOOL player_way_check(', 'static void e_mf_fight_run(', 'static void e_mf_guard('])
symbols=sorted(set(re.findall(r'\b(?:ANM_\w+|Z2SE_EN_MF_\w+)\b',native)))
fixture=r'''
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <map>
#include <vector>
using s16=short;using s8=signed char;using u8=unsigned char;using u32=unsigned;using f32=float;using BOOL=int;
constexpr BOOL TRUE=1,FALSE=0;struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,unsigned n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}}
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz v)const{return{x+v.x,y+v.y,z+v.z};}cXyz operator-(cXyz v)const{return{x-v.x,y-v.y,z-v.z};}
 cXyz operator*(float n)const{return{x*n,y*n,z*n};}float absXZ()const{return std::hypot(x,z);}};
struct fopAc_ac_c{unsigned id=1;int health=100;struct{cXyz pos;struct{s16 y=0;}angle;}current;
 struct{s16 y=0;}shape_angle;cXyz scale{2,2,2},speed;float speedF=0,gravity=0;};using fopEn_enemy_c=fopAc_ac_c;
struct dBgS_Acch{fopAc_ac_c* actor=nullptr;auto* getMyAc(){return actor;}bool ChkWallHit(){return false;}bool ChkGroundHit(){return true;}};
struct Morph{float frame=0,rate=1;bool stopped=false;float getFrame(){return frame;}void setPlaySpeed(float n){rate=n;}bool isStop(){return stopped;}};
struct e_mf_class{fopEn_enemy_c actor;int mAction=3,field_0x5b4=2;float field_0x72c=0,mPlayerDistance=250,field_0x6b8=1.e9f,field_0x6e8=0;
 s16 mYAngleToPlayer=0,mXAngleToPlayer=0,field_0x6ec=0,field_0x806=0,field_0x6c0[4]{};
 int field_0x6ca=0,field_0x6c8=0,field_0x6ac=1,field_0x80c=0,field_0x80e=0,field_0x10c4=0,field_0x6d4=0,field_0x6d1=0,field_0x6e0=0,field_0x6e1=0;
 cXyz field_0x5c8,field_0x5bc;dBgS_Acch mObjAcch{&actor};Morph morph;Morph* mpModelMorf=&morph;
 struct{void startCreatureVoice(int,int){}void startCreatureSound(int,int,int){}}mSound;};
unsigned cut=0;struct Player:fopAc_ac_c{enum{CUT_TYPE_JUMP=1,CUT_TYPE_GUARD_ATTACK=2};bool checkHookshotShootReturnMode(){return false;}
 bool checkSpinnerRide(){return false;}int getCutAtFlg(){return cut!=0;}int getCutType(){return 0;}}player;using daPy_py_c=Player;
Player* daPy_getPlayerActorClass(){return &player;}Player* dComIfGp_getPlayer(int){return &player;}
Player* daAlink_getAlinkActorClass(){return &player;}unsigned cc_pl_cut_bit_get(){return cut;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}constexpr int fpcNm_E_MF_e=1;
struct Enemy{unsigned id=1;int profile=1;bool elite=true,stagger=false,dormant=false;}enemy;
Enemy* tracked(unsigned id){return id==enemy.id?&enemy:nullptr;}
bool boss_tactics_owned(const Enemy& e){return e.elite&&!e.dormant;}
bool playing=true,paused=false,blocked=false,pathValid=true,recognized=true;
bool playable(){return playing;}bool boss_attack_paused(const Enemy&){return paused;}
bool fopAcM_otherBgCheck(fopAc_ac_c*,fopAc_ac_c*){return blocked;}
bool fopAcM_wayBgCheck(fopAc_ac_c*,float,float){return true;}
enum class Phase{Ready,Running};struct{Phase phase=Phase::Running;unsigned rolls=0,value=1;unsigned next_random(){++rolls;return value;}}s_run;
struct Tactics{unsigned dynalfosGuardCooldown=0,dynalfosGuardHold=0;cXyz origin;};std::map<unsigned,Tactics>s_bossTactics;
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
float cM_ssin(s16 a){return std::sin(a*3.14159265/32768);}float cM_scos(s16 a){return std::cos(a*3.14159265/32768);}
void boss_ground_step(fopAc_ac_c* a,const cXyz& target,float n){auto d=target-a->current.pos;d.y=0;const float len=d.absXZ();if(pathValid&&len>1)a->current.pos=a->current.pos+d*(std::min(n,len)/len);}
void anm_init(e_mf_class* a,int,float,int,float rate){a->morph.frame=0;a->morph.rate=rate;}
float randomFraction=0;float cM_rndF(float n){return n*randomFraction;}float cM_rndFX(float){return 0;}
void cLib_addCalc0(float* v,float,float n){*v+=std::clamp(-*v,-n,n);}
void cLib_addCalc2(float* v,float to,float,float n){*v+=std::clamp(to-*v,-n,n);}
void cLib_addCalcAngleS2(s16* v,s16 to,int,int){*v=to;}
struct{float dash_speed=28,battle_init_range=300,attack_init_range=350;}l_HIO;
int pl_check(e_mf_class*,float,s16){return recognized;}
bool move_gake_check(e_mf_class*,float,int){return false;}int way_check(e_mf_class*){return 0;}
void shot_s_sub(){}void* fpcM_Search(void(*)(),e_mf_class*){return nullptr;}
void* bomb_view_check(e_mf_class*){return nullptr;}
int dComIfGp_checkPlayerStatus1(int,unsigned){return 0;}int dComIfGp_checkPlayerStatus0(int,unsigned){return 1;}
void OS_REPORT(const char*){}
'''
main=r'''
void tick(e_mf_class& a){
 std::any args[]={&a};before_dynalfos_action(nullptr,args,nullptr,nullptr);
 a.mPlayerDistance=(player.current.pos-a.actor.current.pos).absXZ(); // native refresh
 before_dynalfos_readiness(nullptr,args,nullptr,nullptr);
 if(a.mAction==3)e_mf_fight_run(&a);else if(a.mAction==7)e_mf_guard(&a);
 after_dynalfos_action(nullptr,args,nullptr,nullptr);
}
int main(){
 player.current.pos={0,0,500};player.shape_angle.y=s16(-0x8000);
 // Reproduce the native priority inversion: continuous sword input always guards.
 e_mf_class native;native.mPlayerDistance=250;cut=1;e_mf_fight_run(&native);assert(native.mAction==7);
 for(float selection:{0.f,.8f})for(s16 yaw:{s16(0),s16(-0x8000),s16(0x4000)}){
  e_mf_class a;player.shape_angle.y=yaw;randomFraction=selection;s_bossTactics.clear();s_run.value=1;
  tick(a);assert(a.mAction==(selection<.65f?5:6)&&a.field_0x5b4==0&&a.field_0x72c==0);
  assert(a.mPlayerDistance==250&&a.field_0x6c0[2]>=30);
 }
 // Failed rolls keep their cooldown even with sustained automatic attacks.
 e_mf_class a;randomFraction=0;s_bossTactics.clear();s_run.rolls=0;s_run.value=1;
 for(int i=0;i<60;++i){a.mAction=3;a.field_0x5b4=2;a.field_0x6c0[2]=100;tick(a);assert(a.mAction==3);}
 assert(s_run.rolls==1);s_run.value=0;tick(a);assert(a.mAction==7&&s_run.rolls==2&&a.field_0x5b4==6&&a.field_0x6c8==0);
 // Native sustained guard would hold forever; owned hold is capped at six ticks.
 a.field_0x5b4=7;a.morph.frame=8;a.morph.rate=0;
 for(int i=0;i<6;++i){tick(a);assert(a.field_0x6c8==0);}
 assert(a.field_0x5b4==8&&a.morph.rate==1);
 a.morph.stopped=true;tick(a);assert(a.mAction==3&&a.field_0x6c8==0);
 // A guard attempt never cancels an existing attack, even when its timer expires.
 for(int action:{5,6}){a.mAction=action;s_bossTactics[1].dynalfosGuardCooldown=0;const auto rolls=s_run.rolls;tick(a);assert(a.mAction==action&&s_run.rolls==rolls);}
 // Preserve native entry waits; walls, range, pitch and lost targets forbid selection.
 cut=0;for(int gate=0;gate<4;++gate){e_mf_class b;b.field_0x6ca=0;
  blocked=gate==0;recognized=gate!=3;b.mXAngleToPlayer=gate==1?0x1000:0;player.current.pos.z=gate==2?800:500;
  tick(b);assert(b.mAction!=5&&b.mAction!=6);}
 blocked=false;recognized=true;player.current.pos.z=500;
 e_mf_class entering;entering.field_0x5b4=1;tick(entering);assert(entering.mAction==3&&entering.field_0x6c0[2]==30);
 // Back out safely from overlap; path clipping happens after arbitrary speed boosts.
 e_mf_class close;close.actor.current.pos={0,0,400};tick(close);assert(close.actor.current.pos.z==382);
 for(int action:{3,5})for(float end:{400.f,3000.f}){
  e_mf_class mover;mover.mAction=action;s_bossTactics[1].origin={0,0,-900};player.current.pos={};
  mover.actor.current.pos={0,17,end};std::any args[]={&mover.mObjAcch};before_dynalfos_path(nullptr,args,nullptr,nullptr);
  assert(std::abs(mover.actor.current.pos.z+(action==3?500:380))<.1f&&mover.actor.current.pos.y==17);
 }
 // Foreign/nested actors and inactive owners do not leak temporary masks.
 e_mf_class owned,foreign;foreign.actor.id=2;std::any ownArgs[]={&owned},foreignArgs[]={&foreign};
 before_dynalfos_action(nullptr,ownArgs,nullptr,nullptr);before_dynalfos_action(nullptr,foreignArgs,nullptr,nullptr);
 after_dynalfos_action(nullptr,foreignArgs,nullptr,nullptr);assert(s_dynalfosActionFrames.size()==1);
 after_dynalfos_action(nullptr,ownArgs,nullptr,nullptr);assert(s_dynalfosActionFrames.empty());
 for(int gate=0;gate<5;++gate){enemy={};playing=true;e_mf_class b;
  if(gate==0)enemy.elite=false;if(gate==1)enemy.dormant=true;if(gate==2)enemy.stagger=true;if(gate==3)playing=false;if(gate==4)b.actor.health=0;
  std::any args[]={&b};before_dynalfos_action(nullptr,args,nullptr,nullptr);before_dynalfos_readiness(nullptr,args,nullptr,nullptr);
  assert(s_dynalfosActionFrames.empty()&&b.field_0x72c==0&&b.field_0x6c0[2]==0);
 }
}
'''
hooks=(root/'src/survivors_roster_hooks.inc').read_text()
assert hooks.index('PRE(RosterMF,before_dynalfos_readiness)')<hooks.index('PRE(RosterMF,before_roster_native_damage)')
registration=(root/'src/survivors.cpp').read_text()
assert registration.index('PRE(SurvivorsEnemySpeed, before_enemy_speed)')<registration.index('PRE(SurvivorsEnemySpeed,before_dynalfos_path)')
code=fixture+'enum{'+','.join(symbols)+'};\n'+(root/'src/survivors_dynalfos.inc').read_text()
code+='\n#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored "-Wunused-variable"\n'+native+'\n#pragma GCC diagnostic pop\n'+main
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-implicit-fallthrough',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Dynalfos: native chase/guard, scaled spacing, boosted path clamp, cooldown, attack selection and restoration passed')
