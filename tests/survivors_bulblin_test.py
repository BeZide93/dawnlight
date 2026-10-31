"""Bulblin damage hooks against the SDK's actual one-shot decision block."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
# Shared collision/effect boundaries, without executing the status test itself.
context={'__file__':str(root/'tests/survivors_status_test.py')}
source=(root/'tests/survivors_status_test.py').read_text()
exec(compile(source[:source.index("source = (root/'src/survivors.cpp')")],'<status fixture>','exec'),context)
fixture=context['fixture']
fixture=fixture.replace('fopAc_ac_c{int id=2,health=50;', 'fopAc_ac_c{int id=2,health=50;float speedF=8;')
fixture=fixture.replace('struct Collider{unsigned power=0;', 'struct Collider{unsigned flags=0;bool ChkAtType(unsigned f){return (flags&f)!=0;} unsigned power=0;')
fixture=fixture.replace('constexpr int fpcNm_E_RD_e=4;\nstruct e_rd_class{fopAc_ac_c enemy;Collider cc_sph[3];dCcU_AtInfo at_info;s16 damage_timer=0;};',r'''
using s8=signed char;
constexpr int fpcNm_E_RD_e=4,HIT_TYPE_ARROW=15,AT_TYPE_SLINGSHOT=2,RIDE_MODE_OFF=0,ACTION_DROP=22,ACTION_A_DAMAGE=23,Z2SE_EN_RD_V_DAMAGE=0;
#define OS_REPORT(...) ((void)0)
struct csXyz{s16 x=0,y=0,z=0;};
struct J3DAnmTransform{} originalAnimation,damageAnimation;
struct J3DFrameCtrl{float frame=12,rate=1;int getAttribute()const{return 2;}float getRate()const{return rate;}
 float getStart()const{return 0;}float getEnd()const{return 30;}};
struct mDoExt_McaMorfSO{
 J3DAnmTransform* animation=&originalAnimation;J3DFrameCtrl mFrameCtrl;float mCurMorf=1,mPrevMorf=1,mMorfStep=0;
 J3DAnmTransform* getAnm(){return animation;}
 void setAnm(J3DAnmTransform* a,int,float,float rate,float start,float){animation=a;mFrameCtrl={start,rate};}
};
struct e_rd_class{
 fopAc_ac_c enemy;Collider cc_sph[3];dCcU_AtInfo at_info;
 s16 damage_timer=0,action=5,mode=2,old_action=0,timer[4]{1,2,3,4},field_0x9f4=0,field_0x9f6=0;
 int anm=0x32,actor_set=0,ride_mode=0,one_hit_kill=1;
 float jump_z=0,field_0x9f0=0;csXyz jump_angle;s8 damage_flag=0;
 mDoExt_McaMorfSO body,bow;mDoExt_McaMorfSO* anm_p=&body; mDoExt_McaMorfSO* bow_anm=&bow;
 struct Sound{void startCreatureVoice(int,int){}}sound;
};
''')
core=(root/'src/survivors.cpp').read_text()
def function(name):
 m=re.search(r'^(?:void|bool) '+name+r'\([^;{}]*\)\s*\{',core,re.M);start=core.index('{',m.start());depth=0
 for end in range(start,len(core)):
  depth+=(core[end]=='{')-(core[end]=='}')
  if not depth:return core[m.start():end+1]
native=(root/'dusklight/src/d/actor/d_a_e_rd.cpp').read_text()
start=native.index('                    s16 cur_health = enemy->health;')
end=native.index('                    if (daPy_getPlayerActorClass()->getCutType()',start)
# Keep the engine block verbatim: both one_hit_kill and mHitType==15 branches.
kill_block=native[start:end]
engine=r'''
unsigned powerCalls=0,hitEffects=0;
void at_power_check(dCcU_AtInfo* info){++powerCalls;info->mAttackPower=3;info->mHitType=HIT_TYPE_ARROW;
 Args args{info,nullptr};after_power(nullptr,&args,nullptr,nullptr);}
void cc_at_check(fopAc_ac_c* enemy,dCcU_AtInfo* info){Args feedbackArgs{enemy,info};begin_damage_feedback(&feedbackArgs,enemy,info);at_power_check(info);enemy->health=std::max(0,enemy->health-int(info->mAttackPower));
 end_damage_feedback(&feedbackArgs);++hitEffects;record_status_hit(enemy,info);}
void native_hit(e_rd_class* i_this){auto* enemy=&i_this->enemy;
 if(i_this->damage_timer)return;
 for(auto& sphere:i_this->cc_sph)if(sphere.ChkTgHit()){
 i_this->at_info.mpCollider=sphere.GetTgHitObj();at_power_check(&i_this->at_info);
'''+kill_block+r'''
 (void)cur_health;
 // Native reaction/animation boundary, exercised before the production post hook.
 i_this->action=enemy->health?20:21;i_this->mode=0;i_this->old_action=5;i_this->timer[1]=1;
 i_this->damage_timer=1000;enemy->speedF=0;enemy->speed.y=25;i_this->jump_z=-20;i_this->field_0x9f0=20;
 i_this->body.animation=&damageAnimation;i_this->body.mFrameCtrl.frame=0;
 i_this->bow.mFrameCtrl.frame=0; // Same bow animation can be restarted by native damage_check.
 break;
 }}
void hit(e_rd_class& enemy){Args args{&enemy,nullptr};before_bulblin_hit(nullptr,&args,nullptr,nullptr);
 native_hit(&enemy);after_bulblin_hit(nullptr,&args,nullptr,nullptr);}
'''
main=r'''
int main(){
 s_run.start(1);player.id=1;e_rd_class enemy;enemy.enemy.id=2;enemy.enemy.health=40;
 actors[2]=&enemy.enemy;s_enemies[0]={2,fpcNm_E_RD_e,true};
 Collider arrow;arrow.owner=&player;enemy.cc_sph[2].SetTgHit(&arrow);
 hit(enemy);assert(enemy.enemy.health==37&&!enemy.one_hit_kill&&powerCalls==2&&hitEffects==1);
 assert(enemy.action==5&&enemy.mode==2&&enemy.timer[1]==2&&enemy.enemy.speedF==8&&enemy.enemy.speed.y==0);
 assert(enemy.body.animation==&originalAnimation&&enemy.body.mFrameCtrl.frame==12&&enemy.bow.mFrameCtrl.frame==12);
 assert(enemy.damage_timer==10&&s_bulblinHits.empty());
 // Own bow damage is linear; preliminary and real checks must share one crit roll.
 auto& bow=s_attacks[0];bow.weapon=unsigned(Upgrade::Bow);bow.collider.power=3;bow.collider.owner=&player;
 enemy.cc_sph[2].ResetTgHit();enemy.cc_sph[0].SetTgHit(&bow.collider);enemy.damage_timer=0;
 s_run.ranks[unsigned(Upgrade::CritChance)]=30;s_run.ranks[unsigned(Upgrade::Luck)]=20;
 Run expected=s_run;const auto damage=expected.critical_power(3);
 clear_combat_feedback();hit(enemy);assert(enemy.enemy.health==37-int(damage)&&s_run.random==expected.random);
 assert(std::count_if(s_combatTexts.begin(),s_combatTexts.end(),[](auto& t){return t.remaining>0;})==(damage>3?1:0));
 // Burn injection reaches every Bulblin sphere, clears its retained collider and never one-shots.
 s_run.ranks.fill(0);for(auto& sphere:enemy.cc_sph)sphere.ResetTgHit();
 auto& effect=effect_for(s_enemies[0]);effect.pending=1;const auto hp=enemy.enemy.health;
 hit(enemy);assert(enemy.enemy.health==hp-1&&!effect.injected&&!enemy.at_info.mpCollider);
 clear_combat_feedback();effect.pending=2;effect.pendingCritical=true;
 hit(enemy);assert(s_combatTexts[0].remaining==30&&!effect.critical&&!effect.pendingCritical);
 // Ball hits use bounded movement instead of native launches, with one outbound and one fresh returning hit.
 auto& ball=s_attacks[1];ball.weapon=unsigned(Upgrade::Ball);ball.collider.power=2;
 enemy.cc_sph[1].SetTgHit(&ball.collider);enemy.damage_timer=10;hit(enemy);
 assert(enemy.action!=20&&enemy.damage_timer==10&&ball.ballVictimCount==1);
 const auto afterBall=enemy.enemy.health;enemy.cc_sph[1].SetTgHit(&ball.collider);hit(enemy);assert(enemy.enemy.health==afterBall);
 ball.ballVictimCount=0;enemy.cc_sph[1].SetTgHit(&ball.collider);hit(enemy);assert(enemy.enemy.health==afterBall-2);
 // Ordinary hits still damage a prone/airborne enemy without removing its recovery state.
 enemy.action=21;enemy.mode=2;enemy.jump_z=-4;enemy.enemy.speed.y=3;enemy.damage_timer=0;
 enemy.cc_sph[1].ResetTgHit();enemy.cc_sph[0].SetTgHit(&arrow);hit(enemy);
 assert(enemy.action==21&&enemy.mode==2&&enemy.jump_z==-4&&enemy.enemy.speed.y==3);
 // Shield attacks suppress native launches and keep linear upgrade damage.
 for(auto& sphere:enemy.cc_sph)sphere.ResetTgHit();
 enemy.enemy.health=40;enemy.damage_timer=0;
 s_run.ranks[unsigned(Upgrade::ShieldAttack)]=1;enemy.cc_sph[0].SetTgHit(&player.mGuardAtCps);
 const auto shieldDamage=s_run.attack_power(unsigned(Upgrade::ShieldAttack));hit(enemy);
 assert(enemy.action==21&&enemy.enemy.health==40-int(shieldDamage));
 enemy.cc_sph[0].SetTgHit(&arrow);
 // Native lethal state is retained; only tracked ordinary Bulblins lose the one-shot rule.
 enemy.enemy.health=2;enemy.damage_timer=0;hit(enemy);assert(enemy.enemy.health==0&&enemy.action==21);
 enemy.enemy.id=99;enemy.enemy.health=40;enemy.damage_timer=0;enemy.one_hit_kill=0;hit(enemy);
 assert(enemy.enemy.health==0&&enemy.action==ACTION_A_DAMAGE);
 // The preliminary power cache preserves the separate Dark Link bonus at cc_at_check.
 s_run.start(1);s_run.dark_link=29;s_run.dark_remaining=100;
 enemy.enemy.id=2;enemy.enemy.health=40;enemy.damage_timer=0;
 for(auto& sphere:enemy.cc_sph)sphere.ResetTgHit();
 bow.collider.power=10;enemy.cc_sph[0].SetTgHit(&bow.collider);hit(enemy);
 assert(enemy.enemy.health==20&&s_run.damage.total()==20);
 assert(s_run.damage.dealt[unsigned(DamageSource::Bow)]==10&&s_run.damage.dealt[unsigned(DamageSource::DarkLink)]==10);
 assert(s_bulblinHits.empty());
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+context['helpers']()+function('native_sword_collider')+(root/'src/survivors_status.inc').read_text()+
  (root/'src/survivors_defense.inc').read_text()+(root/'src/survivors_bulblin.inc').read_text()+function('after_power')+engine+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors Bulblins: native arrow one-shot guard, linear/critical/burn damage, movement/animation preservation, stagger, knockdown and isolation passed')
