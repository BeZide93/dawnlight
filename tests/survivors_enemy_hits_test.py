"""Production incoming-hit adapter: one accepted contact per enemy attack use."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors_enemy_hits.inc').read_text()
profiles = sorted(set(re.findall(r'fpcNm_\w+_e', source)) | {'fpcNm_E_MM_MT_e', 'fpcNm_E_TH_BALL_e'})
types = sorted((set(re.findall(r'(?:static_cast|reinterpret_cast)<(\w+)\*>', source)) | set(re.findall(r'HIT_STATE\(fpcNm_\w+,([\w]+),', source))) - {'fopAc_ac_c', 'process_method_class', 'ActorId', 'T'})
fixture = r'''
#include <any>
#include <array>
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "survivors_enemy_hits.hpp"
using namespace dawnlight::survivors;
using ActorId=int;using s16=short;using ProfileName=int;
constexpr int kNone=-1;enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};struct ModContext{};
namespace mods{template<class T>T arg(void* p,unsigned n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}}
using process_method_func=int;struct process_method_class{int execute_method=1,delete_method=2;};
process_method_class methods;
struct fopAc_ac_c;
struct cCcD_Obj {fopAc_ac_c* actor=nullptr;cCcD_Obj* hit=nullptr;unsigned power=1;
 auto* GetAc(){return actor;}auto* GetTgHitObj(){return hit;}bool ChkTgHit(){return hit;}
 unsigned GetAtAtp(){return power;}};
struct fopAc_ac_c {int id=0,profile=0;void* sub_method=&methods;
 int mActionMode=0,mOcState=0,action=0,mode=0,mAction=0,field_0x5b4=0,mMode=0,mCurAction=0,
 field_0x672=0,field_0x676=0,field_0x678=0,mActionPhase=0,mActionMode1=0,mActionMode2=0,
 mMoveMode=0,mSubAction=0,field_0x68c=0;
 cCcD_Obj mMagicSphs[2],field_0xe64,mAtSph,mBallSphAt;
 static constexpr int ACTION_OPACI_ATTACK_e=16,ACTION_OPACI_FLY_e=12,ACT_B2_F_MOVE=2,ACT_ATTACK=4,ACTION_ATTACK_e=3;
};
int fopAcM_GetID(fopAc_ac_c* a){return a->id;}
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
bool is_actor_process(void*){return true;}
struct Enemy{int id=-1,profile=0;bool elite=true,initialized=true,retiring=false,credited=false;
 unsigned specialCooldown=0,summonCooldown=240,encounterTick=0;cCcD_Obj extraAttack;} enemies[4];
std::unordered_map<int,fopAc_ac_c*> actors;
std::unordered_map<int,int> children;
bool arena=true,playing=true;int s_player=1;
constexpr unsigned ticks_per_second=30;
enum class Phase{Running,Ready};struct{Phase phase=Phase::Running;unsigned ticks=0;}s_run;
bool in_arena(){return arena;}bool playable(){return playing;}
fopAc_ac_c link;
auto* daAlink_getAlinkActorClass(){return &link;}
Enemy* tracked(int id){for(auto& e:enemies)if(e.id==id)return &e;return nullptr;}
fopAc_ac_c* actor_by_id(int id){auto i=actors.find(id);return i==actors.end()?nullptr:i->second;}
Enemy* enemy_child_root(int id){if(auto* e=tracked(id))return e;auto i=children.find(id);return i==children.end()?nullptr:tracked(i->second);}
bool active_enemy_arrow(int id){return children.count(id);}
'''
fixture += 'enum {' + ','.join(profiles) + '};\n'
fixture += '\n'.join('using ' + name + '=fopAc_ac_c;' for name in types) + '\n'
fixture += r'''
bool native_boss_profile(int p){return p==fpcNm_B_TN_e||p==fpcNm_E_PZ_e||p==fpcNm_B_GND_e;}
bool structural_enemy_child(int p){return p==fpcNm_E_MM_MT_e||p==fpcNm_E_TH_BALL_e||p==fpcNm_E_RDY_e;}
'''
fixture += '\nEnemyAttackUsePtr zelda_light_orb_use(cCcD_Obj*){return {};}\nvoid boss_record_contact(cCcD_Obj*){}\n'
main = r'''
fopAc_ac_c boss,other,projectileA,projectileB;
cCcD_Obj target,secondTarget,weapon,secondWeapon;
int damage=0,nativeContacts=0;bool nativeAccept=true;
bool contact(cCcD_Obj& at,cCcD_Obj& tg){
 tg.hit=nullptr;std::any args[]={nullptr,&at,&tg};
 const auto decision=before_enemy_attack_contact(nullptr,args,nullptr,nullptr);
 if(decision==HOOK_CONTINUE){++nativeContacts;if(nativeAccept){tg.hit=&at;damage+=int(at.power);}}
 after_enemy_attack_contact(nullptr,args,nullptr,nullptr);
 return decision==HOOK_CONTINUE;
}
void observe(){observe_enemy_attack(enemies[0],&boss);}
void reset(int profile){
 s_enemyHitSources.clear();s_enemyContactFrames.clear();actors.clear();children.clear();
 for(auto& e:enemies)e={};
 enemies[0].id=10;enemies[0].profile=profile;
 boss={};boss.id=10;boss.profile=profile;other={};other.id=11;
 link={};link.id=s_player=1;actors[10]=&boss;actors[11]=&other;
 target={&link};secondTarget={&link};weapon={&boss};secondWeapon={&boss};
 enemies[0].extraAttack.actor=&boss;
 arena=playing=nativeAccept=true;s_run.phase=Phase::Running;s_run.ticks=0;damage=nativeContacts=0;
}
void spawn(int id,int profile,int parent=10){
 children[id]=10;std::any args[]={s16(profile),parent};
 after_enemy_attack_spawn(nullptr,args,&id,nullptr);
}
int main(){
 // Ordinary King Bulblin swings retain one accepted contact per attack,
 // across colliders and repeated animation frames.
 reset(fpcNm_E_RDB_e);boss.mAction=3;boss.mMode=0;observe();
 for(int frame=0;frame<102;++frame){boss.mMode=frame?2:1;observe();contact(weapon,target);contact(secondWeapon,secondTarget);}
 assert(damage==1&&nativeContacts==1);
 boss.mAction=1;boss.mMode=0;observe();boss.mAction=3;observe();assert(contact(weapon,target)&&damage==2);
 // New attack without an idle Execute; unchanged action with a fresh mode 0.
 boss.mMode=2;observe();boss.mMode=0;observe();assert(contact(weapon,target)&&damage==3);
 // Active spin has a per-owner 15-tick window shared across Link's colliders.
 reset(fpcNm_E_RDB_e);boss.field_0xe64.actor=&boss;auto& axe=boss.field_0xe64;
 boss.mAction=4;boss.mMode=1;observe();
 assert(contact(axe,target)&&!contact(axe,secondTarget)); // Wind-up stays limited.
 boss.mMode=2;observe();
 for(unsigned frame=0;frame<102;++frame){
  s_run.ticks=frame;
  assert(contact(axe,target)==(frame%15==0));
  assert(!contact(axe,secondTarget)); // Never double-hit in the same tick.
  assert(!contact(weapon,secondTarget)); // Other collider keeps its attack budget.
 }
 assert(damage==8&&nativeContacts==8); // Wind-up + ticks 0,15,...,90.
 // Rejected native contacts do not start or extend the cooldown. The first
 // accepted contact can occur long after the eligible overlap was observed.
 s_run.ticks=105;nativeAccept=false;assert(contact(axe,target)&&damage==8);
 s_run.ticks=120;nativeAccept=true;assert(contact(axe,target)&&damage==9);
 for(unsigned tick=120;tick<135;++tick){s_run.ticks=tick;assert(!contact(axe,target));}
 s_run.ticks=135;assert(contact(axe,target)&&damage==10);
 assert(s_enemyContactFrames.empty());
 // Another King has an independent cooldown, not global player immunity.
 enemies[1].id=11;enemies[1].profile=fpcNm_E_RDB_e;other.mAction=4;other.mMode=2;
 other.field_0xe64.actor=&other;
 assert(contact(other.field_0xe64,target)&&!contact(other.field_0xe64,secondTarget));
 // Starting another spin cannot bypass the half-second since the last hit.
 boss.mMode=0;observe();boss.mMode=2;observe();assert(!contact(axe,target));
 s_run.ticks=150;assert(contact(axe,target));
 boss.mMode=3;observe();assert(contact(axe,target)&&!contact(axe,secondTarget));
 boss.mAction=3;boss.mMode=2;observe();
 assert(contact(axe,target)&&!contact(axe,secondTarget)); // Normal axe swing.
 // Darknut heavy throw is independent of his simultaneous sword swing.
 reset(fpcNm_B_TN_e);boss.mActionMode1=4;boss.mActionMode2=1;
 enemies[0].specialCooldown=90;observe();
 assert(contact(enemies[0].extraAttack,target));assert(!contact(enemies[0].extraAttack,secondTarget));
 assert(contact(weapon,target));assert(!contact(secondWeapon,target));assert(damage==2);
 for(unsigned cooldown=89;cooldown>0;--cooldown){enemies[0].specialCooldown=cooldown;observe();assert(!contact(enemies[0].extraAttack,target));}
 enemies[0].specialCooldown=90;observe();assert(contact(enemies[0].extraAttack,target)&&damage==3);
 assert(!contact(weapon,target)); // New throw must not rearm the old melee attack.
 // Structural Helmasaur shell follows each dash, not shell actor lifetime.
 reset(fpcNm_E_MM_e);boss.action=1;boss.mode=3;projectileA={};projectileA.id=20;actors[20]=&projectileA;
 spawn(20,fpcNm_E_MM_MT_e);cCcD_Obj shell{&projectileA};
 assert(contact(shell,target)&&!contact(weapon,secondTarget));
 boss.mode=5;observe();boss.mode=0;observe();assert(contact(shell,target));
 // Darkhammer's outbound and return ball share a throw; spinning is separate.
 reset(fpcNm_E_TH_e);boss.mAction=2;boss.mMode=3;
 projectileA.id=20;actors[20]=&projectileA;spawn(20,fpcNm_E_TH_BALL_e);shell.actor=&projectileA;
 assert(contact(shell,target));boss.mAction=3;boss.mMode=0;observe();assert(contact(shell,target));
 boss.mAction=4;observe();assert(!contact(shell,target));boss.mMode=2;observe();assert(!contact(shell,target));
 // Darkhammer: only the rotating ball's attack sphere repeats, every 15
 // playable ticks. Collision probes and rejected contacts never spend a hit.
 reset(fpcNm_E_TH_e);boss.mAction=2;boss.mMode=3;
 projectileA={};projectileA.id=20;projectileA.profile=fpcNm_E_TH_BALL_e;
 projectileA.mAction=1;projectileA.mMode=1;projectileA.mAtSph.actor=&projectileA;
 actors[20]=&projectileA;spawn(20,fpcNm_E_TH_BALL_e);
 auto& ballAttack=projectileA.mAtSph;
 for(unsigned tick=0;tick<=60;++tick){
  s_run.ticks=tick;
  assert(contact(ballAttack,target)==(tick%15==0));
  assert(!contact(ballAttack,secondTarget));
 }
 assert(damage==5&&nativeContacts==5);
 s_run.ticks=75;nativeAccept=false;assert(contact(ballAttack,target)&&damage==5);
 s_run.ticks=78;nativeAccept=true;assert(contact(ballAttack,target)&&damage==6);
 s_run.ticks=92;assert(!contact(ballAttack,target));
 s_run.ticks=93;assert(contact(ballAttack,target)&&damage==7);
 // Re-entering spin or changing its subphase cannot reset the cooldown.
 projectileA.mAction=0;boss.mAction=0;observe();
 projectileA.mAction=1;boss.mAction=2;observe();
 for(int mode=1;mode<=3;++mode){projectileA.mMode=mode;assert(!contact(ballAttack,target));}
 // The owner's body and defensive sphere keep the ordinary single-use budget.
 assert(contact(weapon,target)&&!contact(secondWeapon,secondTarget));
 shell.actor=&projectileA;assert(!contact(shell,target));
 // Another Darkhammer has an independent spin clock.
 enemies[1].id=11;enemies[1].profile=fpcNm_E_TH_e;other.mAction=2;other.mMode=3;
 projectileB={};projectileB.id=21;projectileB.profile=fpcNm_E_TH_BALL_e;
 projectileB.mAction=1;projectileB.mMode=2;projectileB.mAtSph.actor=&projectileB;
 actors[21]=&projectileB;children[21]=11;
 assert(contact(projectileB.mAtSph,target)&&!contact(projectileB.mAtSph,secondTarget));
 // Outbound and return still share one throw, even after half a second.
 boss.mAction=3;boss.mMode=0;projectileA.mAction=2;observe();
 s_run.ticks=108;assert(contact(ballAttack,target)&&!contact(ballAttack,secondTarget));
 boss.mAction=4;projectileA.mAction=3;observe();s_run.ticks=123;
 assert(!contact(ballAttack,target));
 // Whole Zelda triangle/energy-ball sequence, including later animation stages.
 reset(fpcNm_E_HZELDA_e);boss.mAction=2;
 assert(contact(weapon,target));for(int mode=1;mode<=3;++mode){boss.mMode=mode;assert(!contact(weapon,target));}
 boss.mAction=0;observe();boss.mAction=2;observe();assert(contact(weapon,target));
 // Death Sword keeps both inline magic colliders on the original cast,
 // including if his next sword attack starts before an orb disappears.
 reset(fpcNm_E_VT_e);boss.mAction=12;boss.mMode=10;observe();
 for(auto& orb:boss.mMagicSphs)orb.actor=&boss;
 assert(contact(boss.mMagicSphs[0],target));assert(!contact(boss.mMagicSphs[1],target));
 boss.mAction=15;observe();boss.mAction=16;boss.mMode=0;observe();
 assert(contact(weapon,target)&&!contact(boss.mMagicSphs[1],target));
 boss.mAction=12;boss.mMode=10;observe();assert(contact(boss.mMagicSphs[1],target));
 assert(!contact(boss.mMagicSphs[0],target));
 // Multi-projectile casts share their original use, even after another cast
 // starts or the root disappears. Nested Phantom portals retain that use.
 for(int profile:{fpcNm_B_DS_e,fpcNm_E_PZ_e}){
  reset(profile);boss.mAction=profile==fpcNm_E_VT_e?12:2;boss.mActionMode=3;boss.mMode=11;
  projectileA={};projectileA.id=20;projectileB={};projectileB.id=21;actors[20]=&projectileA;actors[21]=&projectileB;
  const int childProfile=profile==fpcNm_B_DS_e?fpcNm_B_DS_e:fpcNm_E_PZ_e;
  spawn(20,childProfile);spawn(21,childProfile,20);
  cCcD_Obj first{&projectileA},second{&projectileB};
  assert(contact(first,target)&&!contact(second,target));
  boss.mAction=0;boss.mActionMode=0;observe();boss.mAction=profile==fpcNm_E_VT_e?12:2;boss.mActionMode=3;observe();
  assert(!contact(second,target));
  assert(contact(weapon,target)); // new cast is allowed
  enemies[0].id=-1;actors.erase(10);assert(!contact(first,target));
 }
 // Five ghost riders are one summon, independent of Ganondorf's sword.
 reset(fpcNm_B_GND_e);boss.mActionMode=11;boss.mMoveMode=10;enemies[0].summonCooldown=0;
 projectileA.id=20;projectileB.id=21;spawn(20,fpcNm_E_FK_e);spawn(21,fpcNm_E_FK_e);
 cCcD_Obj ghostA{&projectileA},ghostB{&projectileB};
 assert(contact(ghostA,target)&&!contact(ghostB,target)&&contact(weapon,target));
 enemies[0].summonCooldown=240;observe();enemies[0].summonCooldown=0;spawn(22,fpcNm_E_FK_e);
 other.id=22;ghostB.actor=&other;assert(contact(ghostB,target));
 assert(!contact(ghostA,target));
 // Mounted archer remains independent from the Carrier claw attack.
 reset(fpcNm_E_YC_e);boss.mAction=4;spawn(20,fpcNm_E_ARROW_e);spawn(21,fpcNm_E_ARROW_e);
 projectileA.id=20;projectileB.id=21;ghostA.actor=&projectileA;ghostB.actor=&projectileB;
 assert(contact(weapon,target)&&contact(ghostA,target)&&contact(ghostB,target));
 assert(!contact(weapon,target)&&!contact(ghostA,target)&&!contact(ghostB,target));
 // No blanket immunity: other elites and ordinary enemies can still hit.
 reset(fpcNm_E_S1_e);boss.mAction=3;assert(contact(weapon,target));
 enemies[1].id=11;enemies[1].profile=fpcNm_E_S1_e;other.mAction=3;secondWeapon.actor=&other;
 assert(contact(secondWeapon,target));enemies[1].elite=false;assert(contact(secondWeapon,target));
 // Native rejected contacts do not spend a use; blocks/dodges are one attempt.
 reset(fpcNm_E_RDB_e);boss.mAction=4;nativeAccept=false;assert(contact(weapon,target));
 nativeAccept=true;assert(contact(weapon,target)&&damage==1);assert(!contact(weapon,target));
 // Player outgoing attacks and objects, non-run modes, pauses are unaffected.
 target.actor=&other;assert(contact(weapon,target));target.actor=&link;
 playing=false;assert(contact(weapon,target));playing=true;arena=false;assert(contact(weapon,target));arena=true;
 s_run.phase=Phase::Ready;assert(contact(weapon,target));s_run.phase=Phase::Running;
 weapon.power=0;assert(contact(weapon,target));weapon.power=1;assert(!contact(weapon,target));
 // All ordinary promoted elite attack states reset for a new use.
 for(int profile:{fpcNm_E_OC_e,fpcNm_E_DN_e,fpcNm_E_MF_e,fpcNm_E_RD_e,
                  fpcNm_E_RDY_e,fpcNm_E_KR_e,fpcNm_E_SH_e,fpcNm_E_SF_e,fpcNm_E_S1_e}){
  reset(profile);boss.mActionMode=4;boss.action=5;boss.mAction=5;boss.mCurAction=3;boss.field_0x676=3;
  if(profile==fpcNm_E_RD_e)boss.action=4;
  if(profile==fpcNm_E_RDY_e)boss.mAction=4;
  if(profile==fpcNm_E_S1_e)boss.mAction=3;
  assert(contact(weapon,target));assert(!contact(secondWeapon,secondTarget));
  boss.mOcState=boss.mode=boss.field_0x5b4=boss.mMode=boss.field_0x672=boss.field_0x678=boss.mActionPhase=1;observe();
  boss.mOcState=boss.mode=boss.field_0x5b4=boss.mMode=boss.field_0x672=boss.field_0x678=boss.mActionPhase=0;observe();
  assert(contact(weapon,target));
  assert(!contact(secondWeapon,secondTarget));
 }
 // Delete methods release source ownership, including reused process IDs.
 std::any deletion[]={methods.delete_method,static_cast<void*>(&boss)};
 before_enemy_attack_process(nullptr,deletion,nullptr,nullptr);assert(!s_enemyHitSources.count(10));
 assert(contact(weapon,target));assert(s_enemyContactFrames.empty());
 // Every timed elite/native boss and stationary guard has a typed state path.
 std::cout<<"Survivors enemy attacks: repeated colliders, next uses, throws, shell/ball, volleys, portals, riders, independent attackers and lifecycle passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(re.sub(r'\blink\b', 'player', fixture) + source + re.sub(r'\blink\b', 'player', main))
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
