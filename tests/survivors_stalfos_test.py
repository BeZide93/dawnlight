"""Run actual SDK Stalfos facing, fight-run and guard code through owned adapters."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1] if len(sys.argv)>1 else root/'dusklight')
text=(sdk/'src/d/actor/d_a_e_sf.cpp').read_text()
def extract(signature):
 start=text.index(signature);brace=text.index('{',start);depth=0
 for end in range(brace,len(text)):
  depth+=(text[end]=='{')-(text[end]=='}')
  if not depth:return text[start:end+1]
 raise AssertionError(signature)
enums=extract('enum Action {')+';'+extract('enum Action_Phase {')+';'
way=extract('static BOOL player_way_check(')
native=extract('static void e_sf_fight_run(')+extract('static int e_sf_guard(')
start=text.index('    if (i_this->arg3 != 0xFF && unkFlag1')
end=text.index('    cLib_addCalcAngleS2(&a_this->shape_angle.y',start)
selector='void native_guard_selector(e_sf_class* i_this){bool unkFlag1=true;'+text[start:end]+'}'
symbols=sorted(set(re.findall(r'\b(?:BCK_SF_\w+|Z2SE_EN_SF_\w+)\b',native)))
fixture=r'''
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <map>
#include <vector>
using s16=short;using u8=unsigned char;using u16=unsigned short;using u32=unsigned;using f32=float;using BOOL=int;
constexpr BOOL TRUE=1,FALSE=0;struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,unsigned i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct cXyz{float x=0,y=0,z=0;void set(float a,float b,float c){x=a;y=b;z=c;}cXyz operator-(cXyz v)const{return{x-v.x,y-v.y,z-v.z};}float absXZ()const{return std::hypot(x,z);}};
struct fopAc_ac_c{unsigned id=1;int health=100;struct{cXyz pos;struct{s16 y=0;}angle;}current;
 struct{s16 y=0;}shape_angle;cXyz speed,scale{2,2,2};float speedF=0;};using fopEn_enemy_c=fopAc_ac_c;
struct Morph{float frame=0,speed=1;bool stopped=false;float getFrame(){return frame;}void setPlaySpeed(float v){speed=v;}bool isStop(){return stopped;}};
struct e_sf_class{fopEn_enemy_c actor;u8 arg3=0;int mAnm=0,mAction=3,mActionPhase=0,mType=0,mInvulnerabilityTimer=0;
 float mPlayerDistanceXZ=250,mRecognizeDist=1.e9f;s16 mPlayerAngleY=0,mPlayerAngleX=0,mTimers[4]{};
 int field_0x6ab=0,field_0x6ae=0;Morph morph;Morph* mpModelMorf=&morph;
 struct{bool ChkGroundHit(){return true;}}mBgc;struct{void startCreatureVoice(int,int){}}mSound;};
struct Player:fopAc_ac_c{}player;Player* daAlink_getAlinkActorClass(){return &player;}Player* dComIfGp_getPlayer(int){return &player;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
constexpr int fpcNm_E_SF_e=1;
struct Enemy{unsigned id=1;int profile=1;bool elite=true,initialized=true,retiring=false,credited=false,dormant=false,stagger=false,idleAnimation=false;}owner;
Enemy* tracked(unsigned id){return id==owner.id?&owner:nullptr;}
bool boss_tactics_owned(const Enemy& e){return e.elite&&e.initialized&&!e.retiring&&!e.credited&&!e.dormant;}
bool arenaActive=true;bool in_arena(){return arenaActive;}
unsigned idleAnimations=0;void encounter_animation(Morph*,const char*,int animation){assert(animation==0x0c);++idleAnimations;}
bool playing=true,blocked=false,paused=false;bool playable(){return playing;}bool boss_attack_paused(const Enemy&){return paused;}
BOOL fopAcM_otherBgCheck(fopAc_ac_c*,fopAc_ac_c*){return blocked;}
enum class Phase{Running,Ready};struct{Phase phase=Phase::Running;unsigned roll=0,calls=0;unsigned next_random(){++calls;return roll;}}s_run;
struct Tactics{unsigned stalfosGuardCooldown=0,stalfosGuardHold=0;};std::map<unsigned,Tactics>s_bossTactics;
u16 cut=0;u16 cc_pl_cut_bit_get(){return cut;}
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
struct J3DFrameCtrl{enum{EMode_LOOP=2,EMode_NONE=0};};
void anm_init(e_sf_class* a,int,float,int,float speed){a->morph.frame=0;a->morph.stopped=false;a->morph.speed=speed;}
float attackRoll=0;unsigned attackRolls=0;float cM_rndF(float n){if(n==1){++attackRolls;return attackRoll;}return 0;}float cM_rndFX(float){return 0;}int KREG_S(int){return 0;}
void cLib_addCalc0(float* value,float,float step){if(*value>0)*value=std::max(0.f,*value-step);else *value=std::min(0.f,*value+step);}
void cLib_addCalc2(float* value,float target,float,float step){*value+=std::clamp(target-*value,-step,step);}
void cLib_addCalcAngleS2(s16* value,s16 target,int,int){*value=target;}
struct{float charge_spd=10,move_spd=5,combat_start_range=250,attack_start_range=300,defense_recognize_range=300;}l_HIO;
bool recognized=true;int pl_check(e_sf_class*,float,s16){return recognized;}
static BOOL player_way_check(e_sf_class*);
'''
main=r'''
int main(){
 e_sf_class sf;player.current.pos={0,0,500};std::any args[]={&sf};
 owner.dormant=true;sf.actor.speedF=20;sf.actor.speed={1,2,3};
 assert(before_stalfos_action(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 after_stalfos_action(nullptr,args,nullptr,nullptr);
 assert(idleAnimations==1&&sf.mAction==0&&sf.actor.speedF==0&&sf.actor.speed.x==0);
 assert(s_stalfosActionFrames.empty());
 assert(before_stalfos_action(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&idleAnimations==1);
 arenaActive=false;assert(before_stalfos_action(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);arenaActive=true;
 owner.dormant=false;sf.mAction=3; // First player hit wakes the guard before its action hook.
 // Native attack selection fails with Link facing away despite valid range.
 for(s16 yaw:{s16(0),s16(0x4000),s16(-0x4000),s16(-0x8000)}){
  sf.mAction=3;sf.mActionPhase=1;sf.mTimers[2]=0;player.shape_angle.y=yaw;
  if(yaw==0)assert(!player_way_check(&sf));
  const auto rolls=attackRolls;owned_fight(&sf);assert(sf.mAction==5&&sf.mTimers[2]==10&&attackRolls==rolls+1);
 }
 blocked=true;sf.mAction=3;sf.mActionPhase=1;sf.mTimers[2]=0;owned_fight(&sf);assert(sf.mAction==3&&sf.mTimers[2]==0);blocked=false;
 player.current.pos.z=602;sf.mAction=3;sf.mTimers[2]=0;owned_fight(&sf);assert(sf.mAction==3);player.current.pos.z=500;
 sf.mTimers[2]=2;owned_fight(&sf);assert(sf.mAction==3&&sf.mTimers[2]==2);sf.mTimers[2]=0;
 // Pitch boundaries, both native weapon variants, and failed selection retries.
 for(s16 pitch:{s16(-0x1000),s16(0x1000)}){sf.mPlayerAngleX=pitch;owned_fight(&sf);assert(sf.mAction==3&&sf.mTimers[2]==0);}
 sf.mPlayerAngleX=0;sf.mType=1;owned_fight(&sf);assert(sf.mAction==4&&sf.mTimers[2]==10);sf.mType=0;
 sf.mAction=3;sf.mTimers[2]=0;attackRoll=0.75f;const auto rolls=attackRolls;
 owned_fight(&sf);assert(sf.mAction==3&&sf.mTimers[2]==10&&attackRolls==rolls+1);attackRoll=0;
 // Native loss-of-target transition must win; standalone damage hooks do nothing.
 recognized=false;sf.mAction=3;sf.mTimers[0]=sf.mTimers[2]=0;owned_fight(&sf);
 assert(sf.mAction==0&&sf.mTimers[2]==0);recognized=true;
 sf.mAction=3;before_stalfos_readiness(nullptr,args,nullptr,nullptr);assert(sf.mTimers[2]==0);
 // The actual native selector cancels active attacks; the scoped shield mask prevents it.
 player.shape_angle.y=s16(-0x8000);cut=1;sf.mAction=5;native_guard_selector(&sf);assert(sf.mAction==7);
 for(int action:{4,5}){sf.mAction=action;sf.mActionPhase=1;
  before_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.arg3==0xff);
  native_guard_selector(&sf);assert(sf.mAction==action);
  after_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.arg3==0&&!sf.mInvulnerabilityTimer);
 }
 // A failed roll still starts the full two-second retry cooldown.
 sf.mAction=3;sf.mActionPhase=0;s_run.roll=1;s_run.calls=0;s_bossTactics.clear();
 for(int i=0;i<60;++i){before_stalfos_action(nullptr,args,nullptr,nullptr);native_guard_selector(&sf);after_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.mAction==3);}
 assert(s_run.calls==1);
 s_run.roll=0;before_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.mAction==7&&s_run.calls==2);
 e_sf_guard(&sf);native_guard_selector(&sf);after_stalfos_action(nullptr,args,nullptr,nullptr);
 assert(sf.mActionPhase==1&&sf.arg3==0&&!sf.mInvulnerabilityTimer);
 // Native guard otherwise freezes forever against a sustained charged slash.
 cut=0x100;sf.mActionPhase=2;
 for(int i=0;i<12;++i){e_sf_guard(&sf);assert(sf.mActionPhase==2);}
 sf.morph.stopped=false;s_bossTactics[1].stalfosGuardHold=0;
 for(int i=0;i<6;++i){before_stalfos_action(nullptr,args,nullptr,nullptr);e_sf_guard(&sf);
  native_guard_selector(&sf);after_stalfos_action(nullptr,args,nullptr,nullptr);assert(!sf.mInvulnerabilityTimer);}
 assert(sf.mActionPhase==3&&sf.morph.speed==1);
 sf.morph.stopped=true;before_stalfos_action(nullptr,args,nullptr,nullptr);e_sf_guard(&sf);native_guard_selector(&sf);
 after_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.mAction==3); // cannot immediately reenter guard
 // Preserve native actor configuration through nested callbacks and foreign actors.
 e_sf_class foreign;foreign.actor.id=2;foreign.arg3=1;std::any foreignArgs[]={&foreign};
 before_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.arg3==0xff);
 before_stalfos_action(nullptr,foreignArgs,nullptr,nullptr);after_stalfos_action(nullptr,foreignArgs,nullptr,nullptr);
 assert(sf.arg3==0xff&&foreign.arg3==1);after_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.arg3==0&&s_stalfosActionFrames.empty());
 // Unowned/paused/dead/ordinary actors keep native behavior and no random rolls.
 for(int gate=0;gate<6;++gate){owner={};playing=true;sf.actor.health=100;sf.mAction=3;
  if(gate==0)owner.elite=false;if(gate==1)owner.dormant=true;if(gate==2)owner.stagger=true;
  if(gate==3)owner.retiring=true;if(gate==4)playing=false;if(gate==5)sf.actor.health=0;
  const auto calls=s_run.calls;
  sf.mTimers[2]=0;assert(before_stalfos_readiness(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE&&sf.mTimers[2]==0);
  before_stalfos_action(nullptr,args,nullptr,nullptr);assert(sf.arg3==0&&s_run.calls==calls&&s_stalfosActionFrames.empty());
 }
}
'''
source=(root/'src/survivors_stalfos.inc').read_text()
# Check the real registration order: our observer must run before the existing
# damage callback returns SKIP_ORIGINAL. Both symbols are already exported on iOS.
hooks=(root/'src/survivors_roster_hooks.inc').read_text()
assert hooks.index('PRE(RosterSF,before_stalfos_readiness)')<hooks.index('PRE(RosterSF,before_roster_native_damage)')
assert 'RosterSFPlayerWay' not in (root/'src/survivors_roster_native.inc').read_text()
action=extract('static void action(')
assert action.index('mPlayerAngleX =')<action.index('damage_check(i_this)')<action.index('e_sf_fight_run(i_this)')
wrapper=r'''void owned_fight(e_sf_class* a){
 std::any args[]={a};before_stalfos_action(nullptr,args,nullptr,nullptr);
 // Native action refreshes metrics before damage_check. The existing owned
 // damage adapter normalizes the distance for native chase after our observer.
 a->mPlayerDistanceXZ=(player.current.pos-a->actor.current.pos).absXZ();
 before_stalfos_readiness(nullptr,args,nullptr,nullptr);
 a->mPlayerDistanceXZ/=a->actor.scale.x;
 e_sf_fight_run(a);native_guard_selector(a);
 after_stalfos_action(nullptr,args,nullptr,nullptr);
}'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+enums+'enum{'+','.join(symbols)+'};'+source+way+native+selector+wrapper+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-variable','-Wno-misleading-indentation','-Wno-implicit-fallthrough',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Stalfos: native facing/readiness, guard cancellation/hold, chance cooldown, nested restoration and scope passed')
