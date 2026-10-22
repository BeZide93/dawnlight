"""Production roster adapters: typed targets, linear hits, status, lifecycle and AI gates."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
native=(root/'src/survivors_roster_native.inc').read_text().split('// Suppress only')[0]
profiles=sorted(set(re.findall(r'fpcNm_\w+',native))|{'fpcNm_E_MM_MT_e'})
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include "survivors_rules.hpp"
#include "survivors_roster.hpp"
using namespace dawnlight::survivors;
using ProfileName=int;using ActorId=unsigned;using u32=unsigned;using s16=short;using s8=signed char;
constexpr ActorId kNone=~0u;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* p,unsigned i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator-(cXyz v)const{return{x-v.x,y-v.y,z-v.z};}float absXZ()const{return std::sqrt(x*x+z*z);}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793);}
struct fopAc_ac_c {ActorId id=1,parentActorID=kNone;int profile=0,health=100,field_0x560=100,argument=-1;
 struct {cXyz pos;struct{s16 y=0;}angle;}current,home;struct{s16 y=0;}shape_angle;cXyz scale{1,1,1};};
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
std::map<ActorId,fopAc_ac_c*> actors;fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
struct dCcD_Stts {void Move(){}void Init(int,int,fopAc_ac_c*){}};
struct cCcD_Obj {fopAc_ac_c* owner=nullptr;bool tg=true,at=true;fopAc_ac_c* GetAc(){return owner;}
 void OffTgSetBit(){tg=false;}void OffAtSetBit(){at=false;}};
struct dCcD_GObjInf:cCcD_Obj {cCcD_Obj* hit=nullptr;bool shield=true;unsigned type=0,atp=0;float r=1,h=1;
 bool ChkTgHit(){return hit;}cCcD_Obj* GetTgHitObj(){return hit;}void ResetTgHit(){hit=nullptr;}
 void OnTgSetBit(){tg=true;}void SetTgType(unsigned t){type=t;}void OffTgShield(){shield=false;}
 void OffTgShieldFrontRange(){}float GetR(){return r;}void SetR(float n){r=n;}void SetH(float n){h=n;}
 void SetC(cXyz p){center=p;}cXyz center;
 void Set(int){}void SetStts(dCcD_Stts*){}void SetAtType(unsigned){}void SetAtAtp(unsigned n){atp=n;}
 void SetAtMtrl(int){} };
using dCcD_Sph=dCcD_GObjInf;using dCcD_Cyl=dCcD_GObjInf;
struct dCcU_AtInfo {cCcD_Obj* mpCollider=nullptr;int mPowerType=1;};
struct Model {float matrix[3][4]{};auto getAnmMtx(unsigned){return matrix;}cXyz scale;void setBaseScale(cXyz s){scale=s;}};
struct mDoExt_McaMorfSO {Model model;Model* getModel(){return &model;}};
// Some SDK actors use a scalar sphere, others an array; both share the same mock interface.
struct TargetSet:dCcD_GObjInf {dCcD_GObjInf extra[3];
 dCcD_GObjInf& operator[](unsigned i){return i?extra[i-1]:*this;}
 struct Iter {TargetSet* a;unsigned i;auto& operator*(){return (*a)[i];}Iter& operator++(){++i;return *this;}bool operator!=(Iter o){return i!=o.i;}};Iter begin(){return {this,0};}Iter end(){return {this,3};}};
struct NativeActor:fopAc_ac_c {
 enum {ACT_WAIT=1,ACT_MOVE=2,ACT_CHASEH=3,ACTION_ATTACK_e=4,ACTION_WAIT_e=2};
 int mAction=0,mMode=0,mActionMode=0,mMoveMode=0,mActionMode1=0,action=0,mode=0,mActionPhase=0;
 int field_0x5b4=0,mCurAction=0,field_0x672=0,mCryTimer=0,combine_off_timer=0,mPlayerStunTimer=0,mPushButtonCount=0,field_0x6a0=0,animation=0;float speedF=0;
 float field_0x6d4=0,pl_range=0,pl_dir=0,field_0x6b8=0,mRecognizeDist=0,pl_recognize_dist=0,pl_search_range=0;
 s16 angle_target=0,mTargetAngleY=0,mCurrentAngleYTarget=0,field_0x6c0=0;int timers[4]{};
 ActorId armorChildId=kNone;int field_0xb99=0,m_action=0,m_mode=0;
 dCcD_Sph mSphere,mSphere2,mSphere3,cc_sph[3],tate_sph,mCcSphs[3],mTateSph;
 dCcD_Sph field_0xa7c[3],field_0xf5c,mCcHookCyl,mCcShieldSph,mSphA[3],mSphB[3],mSphC,m_sphere,m_sph;
 TargetSet ccSph,mCcSph;
 dCcD_Sph field_0x944[3],mBodyCyls[3],mNeckSph,field_0x82c[2],field_0xa9c,mCylCollider,mWarpCylCollider,mSphCc[3],mGndSph[3];
 bool mBombArrowHit=false;
 dCcD_Stts mBodyCcStts,mColliderStts,field_0x958,mGndCcStts;
 dCcD_Sph field_0xa74[3],mSphArr[2],mSph2,mCcAtSph,mAtSph,mCollisionSphere,mHeadSph[5],field_0xa08;dCcD_Cyl field_0xb40;
 dCcU_AtInfo mAtColliderInfo;dCcD_Stts mCollisionStatus;
 mDoExt_McaMorfSO* mAnm_p=&morph;
 float field_0x6bc=0,mPlayerCheckDist=0,mSearchRange=0,mPlayerTrigger=0;
 int field_0x68c=0,field_0x676=0,field_0x678=0,mWeaponType=1,mGroupID=0;dCcD_Cyl mCcCyl;
 dCcU_AtInfo mAtInfo,at_info,atInfo;dCcD_Stts mStts,stts,ccStts,mCcStts,mStts1;
 mDoExt_McaMorfSO morph; mDoExt_McaMorfSO* mpMorfSO=&morph,*mpMorf=&morph,*anm_p=&morph,*modelMorf=&morph,*mpModelMorf=&morph,*mpModelMorf1=&morph;
 void setActionMode(int a,int m=0){mAction=mActionMode=mActionMode1=a;mMode=mMoveMode=m;}
 void setBck(int a,int,float,float){animation=a;}
 // Native damage_check's independent warp branch, before its HP-hit branch.
 // It may also be inlined in action; the charge must survive without its hook.
 void native_warp_reaction(){
  if(mWarpCylCollider.ChkTgHit()||mBombArrowHit){
   mWarpCylCollider.ResetTgHit();mBombArrowHit=false;animation=0x13;
   setActionMode(ACTION_WAIT_e,11);
  }
 }
};
using e_rdb_class=NativeActor;using daE_VA_c=NativeActor;using daE_YMB_c=NativeActor;using daE_PZ_c=NativeActor;
using e_th_class=NativeActor;using e_hzelda_class=NativeActor;using b_gnd_class=NativeActor;
using e_dd_class=NativeActor;using e_sh_class=NativeActor;using e_rdy_class=NativeActor;using e_s1_class=NativeActor;using e_yk_class=NativeActor;using e_yc_class=NativeActor;using daB_DS_c=NativeActor;using daE_ZH_c=NativeActor;
using daE_TT_c=NativeActor;using daE_GE_c=NativeActor;using e_kr_class=NativeActor;using e_dn_class=NativeActor;
using e_mm_class=NativeActor;using e_mm_mt_class=NativeActor;using e_cr_class=NativeActor;using e_bs_class=NativeActor;
using daE_GI_c=NativeActor;using e_sf_class=NativeActor;using e_fs_class=NativeActor;using e_bu_class=NativeActor;
using e_sm2_class=NativeActor;using e_mf_class=NativeActor;using daB_GG_c=NativeActor;using daB_TN_c=NativeActor;
constexpr int ACTION_COMBINE=2,ACTION_ATTACK=1,ACT_ATTACK=3;
unsigned s_map=0;
struct Enemy {int eliteSlot=-1;ActorId id=kNone;ProfileName profile=0;unsigned species=0,stagger=0,hitCooldown=0,age=1;
 cCcD_Obj* childHit=nullptr;bool elite=false,credited=false,retiring=false,initialized=true,dormant=false;};
std::array<Enemy,enemy_capacity> s_enemies;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id!=kNone&&e.id==id)return &e;return nullptr;}
bool stageChange=false;bool transitioning(){return stageChange;}
Run s_run;bool arena=true;bool in_arena(){return arena;}
bool arena_aggro(fopAc_ac_c* a,int p){return arena&&a&&a->profile==p;}
struct Player:fopAc_ac_c {dCcD_Sph mGuardAtCps;}player;
bool playerPresent=true;Player* daAlink_getAlinkActorClass(){return playerPresent?&player:nullptr;}ActorId s_player=100;
struct Effect {DamageQueue attribution;unsigned pending=0,damage=0,meleeSerial=0,fixedPending=0,fixedDamage=0;bool injected=false,critical=false,pendingCritical=false,pendingStagger=false,stagger=false;
 dCcD_Stts status;dCcD_Sph collider;};
std::array<Effect,enemy_capacity> effects;
Effect& effect_for(Enemy& e){return effects[&e-s_enemies.data()];}
struct Attack {dCcD_Sph collider;};std::array<Attack,4>s_attacks;
dCcD_Sph sword;bool native_sword_collider(const cCcD_Obj* a){return a==&sword;}
bool effect_collider(const cCcD_Obj* a){for(auto& e:effects)if(a==&e.collider)return true;return false;}
enum class SwordAction {Normal,None};SwordAction s_swordAction=SwordAction::Normal;unsigned s_meleeSerial=1;
constexpr int kAttackSource=0,AT_TYPE_ARROW=1,dCcD_MTRL_NONE=0;
int ballResult=-1;int ball_contact(fopAc_ac_c*,const cCcD_Obj*){return ballResult;}
bool causes_stagger(const cCcD_Obj* a){return a==&player.mGuardAtCps;}
unsigned calls=0,power=7,credits=0,disappears=0,deletes=0;bool injectedCritical=false;
void cc_at_check(fopAc_ac_c* a,dCcU_AtInfo* info){++calls;assert(info->mPowerType==0);
 unsigned damage=power;
 for(auto& e:effects)if(info->mpCollider==&e.collider){damage=e.damage+e.fixedDamage;injectedCritical=e.critical;}
 a->health=std::max(0,a->health-int(damage));}
void credit_death(Enemy& e,fopAc_ac_c*){if(!e.credited){++credits;e.credited=true;}}
void fopAcM_createDisappear(fopAc_ac_c*,cXyz*,int,int,int){++disappears;}
struct Service{void delete_actor(ModContext*,ActorId){++deletes;}}service;auto* svc_actor=&service;ModContext* mod_ctx=nullptr;
struct Collision {unsigned sets=0;void Set(cCcD_Obj*){++sets;}}collision;Collision* dComIfG_Ccsp(){return &collision;}
'''
fixture+=r'''
std::set<ActorId> creating;bool fpcM_IsCreating(ActorId id){return creating.contains(id);}
int fopAcM_GetRoomNo(fopAc_ac_c*){return 7;}
unsigned armorCreates=0;ActorId createdArmor=kNone;int armorArgument=0;ActorId armorParent=kNone;
template<class Angle>ActorId fopAcM_createChild(int,ActorId parent,unsigned params,cXyz*,int room,Angle*,cXyz*,int argument,void*){
 ++armorCreates;assert(params==0&&room==7);armorArgument=argument;armorParent=parent;return createdArmor;}
'''
fixture+=r'''
bool boss_accept_stagger(Enemy&){return true;}
void boss_received_damage(Enemy&,int){}
'''
main=r'''
int main(){
 for(int phase:{0,1,2,3}){
  NativeActor phantom;phantom.profile=fpcNm_E_PZ_e;phantom.mActionMode=NativeActor::ACTION_ATTACK_e;phantom.mMoveMode=phase;
  Enemy owner;owner.profile=fpcNm_E_PZ_e;
  assert(phantom_projectile_charge(&phantom,owner)==(phase<=2));
  phantom.health=0;assert(!phantom_projectile_charge(&phantom,owner));
 }

 player.id=s_player;player.current.pos={100,0,0};s_run.start(5);
 for(unsigned kind=5;kind<species_count;++kind) {
  NativeActor actor;actor.profile=native_roster[kind].profile;actors={{1,&actor}};
  auto& enemy=s_enemies[0];s_enemies={};enemy.id=1;enemy.species=kind;enemy.profile=actor.profile;
  auto body=extended_body(&actor,actor.profile);assert(body.targets[0]&&body.info&&body.status&&body.morph);
  initialize_extended_enemy(enemy,&actor);assert(actor.health==int(native_roster[kind].hp));
  s_run.ticks=duration;initialize_extended_enemy(enemy,&actor);
  assert(actor.health==int(s_run.enemy_health(native_roster[kind].hp,false,s_map))&&actor.field_0x560==actor.health);
  enemy.elite=true;initialize_extended_enemy(enemy,&actor);
  assert(actor.health==int(s_run.enemy_health(native_roster[kind].hp,true,s_map)));
  s_run.ticks=0;
  s_run.options.enemy_hp=3;enemy.elite=true;actor.scale={elite_scale(Species(kind)),elite_scale(Species(kind)),elite_scale(Species(kind))};
  initialize_extended_enemy(enemy,&actor);assert(actor.health==360);s_run.options.enemy_hp=1;
  actor.health=100;
  for(auto* target:body.targets)if(target){target->owner=&actor;target->hit=&sword;}
  const auto beforeCalls=calls;
  assert(update_extended_enemy(&actor,enemy));assert(calls==beforeCalls+1&&actor.health==93);
  for(auto* target:body.targets)if(target){assert(!target->hit);std::any args[]={0,static_cast<cCcD_Obj*>(target)};
   before_roster_targets(nullptr,args,nullptr,nullptr);const float radius=target->r;
   before_roster_targets(nullptr,args,nullptr,nullptr);assert(target->r==radius&&!target->shield&&target->tg);
  }
  body.targets[0]->shield=true;finish_extended_targets(&enemy,&actor);assert(!body.targets[0]->shield);
  body.targets[0]->hit=&sword;update_extended_enemy(&actor,enemy);assert(actor.health==86); // next-frame damage is accepted
  actor.health=93;
  auto& effect=effect_for(enemy);effect.pending=13;effect.pendingCritical=true;enemy.hitCooldown=0;
  update_extended_enemy(&actor,enemy);assert(actor.health==80&&injectedCritical&&!effect.pending&&!effect.injected&&!effect.critical);
  s_run.dual_wield=1;effect.meleeSerial=s_meleeSerial;body.targets[0]->hit=&sword;enemy.hitCooldown=0;
  update_extended_enemy(&actor,enemy);assert(actor.health==80);s_run.dual_wield=0;
  body.targets[0]->hit=&s_attacks[0].collider;ballResult=0;update_extended_enemy(&actor,enemy);assert(actor.health==80);
  ballResult=1;enemy.hitCooldown=10;body.targets[0]->hit=&s_attacks[0].collider;
  update_extended_enemy(&actor,enemy);assert(actor.health==73);ballResult=-1;
  enemy.hitCooldown=0;body.targets[0]->hit=&player.mGuardAtCps;
  assert(!update_extended_enemy(&actor,enemy)&&enemy.stagger==7&&actor.health==66);
  for(int n=0;n<7;++n)assert(!update_extended_enemy(&actor,enemy));
  assert(update_extended_enemy(&actor,enemy));
  // Foreign contacts never become run damage; only validated attacks count.
  dCcD_Sph foreign;body.targets[0]->hit=&foreign;enemy.hitCooldown=0;
  update_extended_enemy(&actor,enemy);assert(actor.health==66);
  std::any args[]={static_cast<fopAc_ac_c*>(&actor)};
  assert(before_roster_native_damage(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
  arena=false;assert(before_roster_native_damage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);arena=true;
  enemy.hitCooldown=0;body.targets[0]->hit=&sword;actor.health=7;unsigned beforeCredits=credits;
  assert(!update_extended_enemy(&actor,enemy)&&actor.health==0&&enemy.retiring&&enemy.credited&&credits==beforeCredits+1);
  // Normal runtime does not execute retiring actors again.
 }
 assert(credits==species_count-5&&disappears==credits&&deletes==credits);
 // Reproduce both native warp triggers without the standalone damage hook.
 for(bool bomb:{false,true}) {
  NativeActor phantom;phantom.mActionMode=NativeActor::ACTION_ATTACK_e;
  phantom.mBombArrowHit=bomb;if(!bomb)phantom.mWarpCylCollider.hit=&sword;
  phantom.native_warp_reaction();
  assert(phantom.mActionMode==NativeActor::ACTION_WAIT_e&&phantom.mMoveMode==11&&phantom.animation==0x13);
 }
 // Heavy direct and queued hits keep every charge phase running even with
 // simultaneous warp-cylinder contacts and the bomb-arrow callback latch.
 for(int phase:{0,1,2})for(bool queued:{false,true})for(unsigned triggers=0;triggers<4;++triggers) {
  NativeActor phantom;phantom.profile=fpcNm_E_PZ_e;phantom.health=100;
  phantom.mActionMode=NativeActor::ACTION_ATTACK_e;phantom.mMoveMode=phase;
  s_enemies={};effects={};auto& owner=s_enemies[0];owner.id=1;owner.profile=fpcNm_E_PZ_e;
  actors={{1,&phantom}};auto body=extended_body(&phantom,owner.profile);
  phantom.animation=0xC;phantom.mBombArrowHit=triggers&1;
  if(triggers&2)phantom.mWarpCylCollider.hit=&sword;
  owner.stagger=4; // An older stagger cannot freeze the newly entered charge.
  if(queued){auto& effect=effect_for(owner);effect.pending=7;effect.pendingStagger=true;}
  else body.targets[0]->hit=&player.mGuardAtCps;
  assert(update_extended_enemy(&phantom,owner)&&phantom.health==93&&owner.stagger==0);
  assert(!phantom.mWarpCylCollider.ChkTgHit()&&!phantom.mBombArrowHit);
  phantom.native_warp_reaction();assert(phantom.animation==0xC);
  assert(phantom.mActionMode==NativeActor::ACTION_ATTACK_e&&phantom.mMoveMode==phase);
  phantom.health=7;body.targets[0]->hit=&player.mGuardAtCps;const auto beforeCredits=credits;
  assert(!update_extended_enemy(&phantom,owner)&&phantom.health==0&&owner.retiring&&owner.credited);
  assert(credits==beforeCredits+1);
 }
 // No immunity and no suppressed reactions outside the protected charge.
 {
  NativeActor phantom;phantom.profile=fpcNm_E_PZ_e;phantom.mActionMode=NativeActor::ACTION_WAIT_e;
  s_enemies={};effects={};auto& owner=s_enemies[0];owner.id=1;owner.profile=fpcNm_E_PZ_e;
  phantom.mBombArrowHit=true;phantom.mWarpCylCollider.hit=&sword;
  assert(update_extended_enemy(&phantom,owner));
  assert(phantom.mBombArrowHit&&phantom.mWarpCylCollider.ChkTgHit());
  phantom.native_warp_reaction();assert(phantom.mMoveMode==11);
 }

 // Every map's final boss uses the full boss pool and selected HP multiplier.
 for(unsigned map=0;map<map_rosters.size();++map) {
  NativeActor finalBoss;Enemy finalEnemy;
  finalEnemy.species=unsigned(map_roster(map).elites.back());finalEnemy.elite=true;finalEnemy.eliteSlot=3;s_map=map;
  finalEnemy.profile=native_roster[finalEnemy.species].profile;
  for(unsigned minute:{0u,10u,20u})for(unsigned multiplier:{1u,3u,10u}) {
   s_run.ticks=minute*60*ticks_per_second;s_run.options.enemy_hp=multiplier;initialize_extended_enemy(finalEnemy,&finalBoss);
   assert(finalBoss.health==int(std::min(32767u,2*s_run.enemy_health(1200,true,map,true)))&&finalBoss.field_0x560==finalBoss.health);
  }
 }
 s_run.options.enemy_hp=1;
 // Endgame criticals must not erase Stallord's entire boss health pool.
 NativeActor head;head.profile=fpcNm_B_DS_e;s_enemies={};effects={};
 auto& boss=s_enemies[0];boss.id=head.id;boss.profile=head.profile;
 boss.species=unsigned(Species::StallordHead);boss.elite=true;boss.eliteSlot=3;s_map=2;
 s_run.start(7);s_run.ticks=duration;s_run.ranks.fill(30);s_run.master_sword=10;
 s_run.dark_link=29;s_run.dark_remaining=100; // maximal direct-hit upgrade damage
 initialize_extended_enemy(boss,&head);assert(head.health==9600&&head.field_0x560==9600);
 power=s_run.attack_power(unsigned(Upgrade::Spin))*s_run.crit_damage_percent()/100*2;
 assert(power>120&&power<1200);
 auto targets=extended_body(&head,boss.profile);
 for(auto* target:targets.targets)if(target)target->hit=&sword;
 const unsigned beforeHeadHits=calls;
 assert(update_extended_enemy(&head,boss));
 assert(head.health==9600-int(power)&&calls==beforeHeadHits+1&&!boss.credited);
 for(auto* target:targets.targets)if(target)target->hit=&sword;
 update_extended_enemy(&head,boss);assert(head.health==9600-2*int(power)); // Fresh next-frame contact; no shared lockout.
 s_run.options.enemy_hp=10;initialize_extended_enemy(boss,&head);assert(head.health==32767);
 s_run.start(5);power=7;
 // Owned screams have a short native stun and a shared cooldown across Gibdos.
 NativeActor gibdo;gibdo.profile=fpcNm_E_GI_e;s_enemies={};effects={};
 auto& gi=s_enemies[0];gi.id=gibdo.id;gi.profile=gibdo.profile;
 std::any cryArgs[]={static_cast<daE_GI_c*>(&gibdo)};bool cry=false;
 arena=false;assert(before_roster_cry(nullptr,cryArgs,&cry,nullptr)==HOOK_CONTINUE);arena=true;
 for(int gate=0;gate<8;++gate) {
  gi.initialized=gate!=0;gi.retiring=gate==1;gi.credited=gate==2;gi.dormant=gate==3;gi.stagger=gate==4;
  gibdo.health=gate==5?0:100;stageChange=gate==6;s_run.phase=gate==7?Phase::Upgrade:Phase::Running;
  before_roster_cry(nullptr,cryArgs,&cry,nullptr);assert(!cry&&!gibdo.mPlayerStunTimer);
 }
 gi={};gi.id=gibdo.id;gi.profile=gibdo.profile;gibdo.health=100;stageChange=false;s_run.phase=Phase::Running;
 before_roster_cry(nullptr,cryArgs,&cry,nullptr);
 assert(cry&&gibdo.mPlayerStunTimer==30&&gibdo.mCryTimer==80&&gibdo.animation==10&&gibdo.field_0x6a0==3);
 assert(s_run.gibdo_scream_cooldown==180);
 NativeActor other;other.id=2;other.profile=fpcNm_E_GI_e;s_enemies[1]=gi;s_enemies[1].id=2;
 cryArgs[0]=static_cast<daE_GI_c*>(&other);before_roster_cry(nullptr,cryArgs,&cry,nullptr);
 assert(!cry&&!other.mPlayerStunTimer);
 s_run.phase=Phase::Upgrade;s_run.tick(true);assert(s_run.gibdo_scream_cooldown==180);
 s_run.phase=Phase::Running;s_run.tick(false);assert(s_run.gibdo_scream_cooldown==180);
 for(int i=0;i<180;++i)s_run.tick(true);
 before_roster_cry(nullptr,cryArgs,&cry,nullptr);assert(cry&&other.mPlayerStunTimer==30);
 auto gibdoBody=extended_body(&gibdo,gi.profile);gibdoBody.targets[0]->hit=&sword;
 update_extended_enemy(&gibdo,gi);assert(!gibdo.mPlayerStunTimer&&gibdo.mCryTimer==20);
 s_run.start(5);assert(!s_run.gibdo_scream_cooldown);
 // Shell targets use parent health and lose shield immunity, while preserving model ownership.
 NativeActor helmet,shell;helmet.profile=fpcNm_E_MM_e;helmet.id=1;helmet.armorChildId=2;
 shell.id=2;shell.profile=fpcNm_E_MM_MT_e;shell.parentActorID=1;
 actors={{1,&helmet},{2,&shell}};s_enemies={};auto& e=s_enemies[0];e.id=1;e.profile=fpcNm_E_MM_e;
 auto body=extended_body(&helmet,e.profile);assert(body.targets[1]==&shell.m_sphere&&body.targets[2]==&shell.m_sph);
 shell.m_sphere.owner=&shell;std::any targetArgs[]={0,static_cast<cCcD_Obj*>(&shell.m_sphere)};
 before_roster_targets(nullptr,targetArgs,nullptr,nullptr);assert(!shell.m_sphere.shield);
 shell.m_sphere.hit=&sword;effects={};update_extended_enemy(&helmet,e);assert(helmet.health==93&&shell.health==100);
 // Child-first execution queues one hit without triggering native armor damage.
 e.hitCooldown=0;shell.m_sphere.hit=&sword;std::any shellArgs[]={static_cast<e_mm_mt_class*>(&shell)};
 before_roster_shell(nullptr,shellArgs,nullptr,nullptr);assert(e.childHit==&sword&&!shell.m_sphere.hit);
 update_extended_enemy(&helmet,e);assert(helmet.health==86&&!e.childHit);
 update_extended_enemy(&helmet,e);assert(helmet.health==86);
 // A missing/pending shell cannot leave the armored body without a target.
 helmet.field_0x672=1;helmet.ccSph.owner=&helmet;helmet.scale={2,2,2};
 const auto registered=collision.sets;finish_extended_targets(&e,&helmet);assert(collision.sets==registered);
 actors.erase(2);finish_extended_targets(&e,&helmet);
 assert(collision.sets==registered+1&&helmet.ccSph.r==80&&helmet.ccSph.center.y==80&&!helmet.ccSph.at);
 // Restore the attached state before either parent-first or child-first execution.
 actors[2]=&shell;helmet.field_0x672=0;helmet.field_0xb99=4;shell.m_action=2;shell.m_mode=5;
 before_roster_shell(nullptr,shellArgs,nullptr,nullptr);
 assert(helmet.field_0x672==1&&helmet.field_0xb99==0&&shell.m_action==0&&shell.m_mode==0);
 actors.erase(2);helmet.armorChildId=kNone;e.age=30;createdArmor=77;
 maintain_helmasaur_armor(e,&helmet);assert(armorCreates==1&&helmet.armorChildId==77&&armorParent==1&&armorArgument==-1);
 creating.insert(77);e.age=60;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==1);
 creating.clear();e.age=61;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==1);
 createdArmor=kNone;e.age=90;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==2&&helmet.armorChildId==kNone);
 e.age=91;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==2);
 helmet.argument=1;createdArmor=88;e.age=120;maintain_helmasaur_armor(e,&helmet);
 assert(armorCreates==3&&armorArgument==1&&helmet.armorChildId==88);
 actors[88]=&shell;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==3);
 actors.erase(88);e.retiring=true;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==3);
 e.retiring=false;e.credited=true;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==3);
 e.credited=false;helmet.health=0;maintain_helmasaur_armor(e,&helmet);assert(armorCreates==3);
 helmet.health=86;helmet.armorChildId=kNone;e.hitCooldown=0;helmet.ccSph.hit=&sword;
 update_extended_enemy(&helmet,e);assert(helmet.health==79); // Body damage during failed armor creation.
 NativeActor tektite;tektite.profile=fpcNm_E_TT_e;std::any searchArgs[]={static_cast<fopAc_ac_c*>(&tektite)};bool found=false;
 assert(before_roster_search(nullptr,searchArgs,&found,nullptr)==HOOK_SKIP_ORIGINAL&&found);
 NativeActor bubble;bubble.profile=fpcNm_E_BU_e;bubble.pl_search_range=1e9;
 std::any bubbleArgs[]={static_cast<e_bu_class*>(&bubble),2000.f};int seen=0;
 assert(before_roster_bubble_search(nullptr,bubbleArgs,&seen,nullptr)==HOOK_SKIP_ORIGINAL&&seen);
 bubbleArgs[1]=200.f;assert(before_roster_bubble_search(nullptr,bubbleArgs,&seen,nullptr)==HOOK_CONTINUE);
 arena=false;assert(before_roster_search(nullptr,searchArgs,&found,nullptr)==HOOK_CONTINUE);arena=true;
 // Helmasaur and Helmasaurus commit to native dash headings. Test before
 // and after crossing Link: retargeting at either movement boundary would
 // reverse the charge and trap its attack collider in Link's model.
 for(auto species:{Species::Helmasaur,Species::Helmasaurus})for(bool elite:{false,true}) {
  NativeActor runner;runner.profile=fpcNm_E_MM_e;runner.argument=species==Species::Helmasaurus?1:0;
  e.profile=runner.profile;e.species=unsigned(species);e.stagger=0;e.elite=elite;
  const float size=(runner.argument==1?2.5f:1.f)*(elite?2.f:1.f);runner.scale={size,size,size};
  player.current.pos={0,0,0};runner.action=1;
  for(int mode=0;mode<=5;++mode) {
   runner.mode=mode;runner.current.angle.y=123;runner.shape_angle.y=77;
   runner.current.pos={100,0,100};face_extended_enemy(&runner,e);
   assert(runner.current.angle.y==123&&runner.shape_angle.y==77);
  }
  runner.mode=3;runner.current.angle.y=runner.shape_angle.y=0;
  runner.current.pos={0,0,-150*size};const float step=(runner.argument==1?8.f:15.f)*size;
  for(int frame=0;frame<70;++frame) {
   face_extended_enemy(&runner,e);assert(runner.current.angle.y==0);
   runner.current.pos.z+=step; // Native forward charge, including scale.
   face_extended_enemy(&runner,e);assert(runner.current.angle.y==0&&runner.shape_angle.y==0);
  }
  assert(runner.current.pos.z>150*size); // Runs fully clear, never turns back at contact.
  runner.action=0;runner.current.pos={100,0,0};face_extended_enemy(&runner,e);
  assert(runner.current.angle.y==-0x4000); // Normal acquisition still turns toward Link.
 }
 player.current.pos={100,0,0};e.species=0;e.elite=false;
 // Every additional committed-motion exclusion preserves travel and visual
 // headings, plus native target fields, before/after crossing the player.
 struct MotionCase{int profile;unsigned species;int NativeActor::*action;int active;int NativeActor::*phase;int first,last;};
 const MotionCase motions[]={
  {fpcNm_E_TT_e,unsigned(Species::Tektite),&NativeActor::mAction,1,&NativeActor::mMode,6,8},
  {fpcNm_E_TT_e,unsigned(Species::Tektite),&NativeActor::mAction,2,&NativeActor::mMode,2,4},
  {fpcNm_E_TT_e,unsigned(Species::Tektite),&NativeActor::mAction,6,&NativeActor::mMode,12,14},
  {fpcNm_E_GE_e,unsigned(Species::Guay),&NativeActor::mActionMode,2,&NativeActor::mMode,2,3},
  {fpcNm_E_KR_e,unsigned(Species::Kargarok),&NativeActor::mCurAction,3,&NativeActor::field_0x672,0,11},
  {fpcNm_E_YK_e,unsigned(Species::ShadowKeese),&NativeActor::mAction,ACT_ATTACK,&NativeActor::mActionPhase,2,3},
  {fpcNm_E_SH_e,unsigned(Species::Stalhound),&NativeActor::field_0x676,3,&NativeActor::field_0x678,2,4},
  {fpcNm_E_SM2_e,unsigned(Species::BlackChu),&NativeActor::action,ACTION_ATTACK,&NativeActor::mode,0,2},
  {fpcNm_E_DN_e,unsigned(Species::Lizalfos),&NativeActor::action,5,&NativeActor::mode,0,1},
  {fpcNm_E_DN_e,unsigned(Species::Lizalfos),&NativeActor::action,6,&NativeActor::mode,0,1},
  {fpcNm_E_DN_e,unsigned(Species::Lizalfos),&NativeActor::action,8,&NativeActor::mode,0,4},
  {fpcNm_E_DN_e,unsigned(Species::Lizalfos),&NativeActor::action,25,&NativeActor::mode,0,4},
  {fpcNm_E_MF_e,unsigned(Species::Dynalfos),&NativeActor::mAction,5,&NativeActor::field_0x5b4,0,1},
  {fpcNm_E_MF_e,unsigned(Species::Dynalfos),&NativeActor::mAction,6,&NativeActor::field_0x5b4,0,1},
  {fpcNm_E_MF_e,unsigned(Species::Dynalfos),&NativeActor::mAction,8,&NativeActor::field_0x5b4,0,4},
  {fpcNm_E_MF_e,unsigned(Species::Dynalfos),&NativeActor::mAction,25,&NativeActor::field_0x5b4,0,4},
  {fpcNm_B_TN_e,unsigned(Species::Darknut),&NativeActor::mActionMode1,9,&NativeActor::mMode,0,17},
  {fpcNm_B_TN_e,unsigned(Species::Darknut),&NativeActor::mActionMode1,4,&NativeActor::mMode,0,3},
  {fpcNm_B_TN_e,unsigned(Species::Darknut),&NativeActor::mActionMode1,10,&NativeActor::mMode,0,3}
 };
 for(const auto& test:motions)for(bool elite:{false,true})for(int phase=test.first;phase<=test.last;++phase) {
  NativeActor runner;runner.profile=test.profile;runner.*test.action=test.active;runner.*test.phase=phase;
  e.profile=test.profile;e.species=test.species;e.stagger=0;e.elite=elite;
  const float size=elite?2.f:1.f;runner.scale={size,size,size};
  runner.current.angle.y=0;runner.shape_angle.y=77;runner.current.pos={0,0,-100*size};
  runner.angle_target=111;runner.mCurrentAngleYTarget=222;runner.field_0x6c0=333;
  player.current.pos={0,0,0};
  for(int frame=0;frame<30;++frame) {
   face_extended_enemy(&runner,e);assert(runner.current.angle.y==0&&runner.shape_angle.y==77);
   runner.current.pos.z+=20*size;face_extended_enemy(&runner,e);
   assert(runner.current.angle.y==0&&runner.shape_angle.y==77);
   assert(runner.angle_target==111&&runner.mCurrentAngleYTarget==222&&runner.field_0x6c0==333);
  }
  assert(runner.current.pos.z>100*size);
  runner.*test.action=0;runner.*test.phase=0;if(test.profile==fpcNm_B_TN_e)runner.mActionMode1=3;
  runner.current.pos={100,0,0};face_extended_enemy(&runner,e);assert(runner.current.angle.y==-0x4000);
 }
 player.current.pos={100,0,0};e.species=0;e.elite=false;
 // Acquisition changes no real player distances and excludes heavy-hit stagger.
 NativeActor liz;e.profile=fpcNm_E_DN_e;e.stagger=0;engage_extended_enemy(&liz,e);assert(liz.action==3&&liz.pl_range>100000);
 assert(extended_locomotion(&liz,e.profile));liz.action=5;assert(!extended_locomotion(&liz,e.profile));
 liz.action=3;face_extended_enemy(&liz,e);assert(liz.shape_angle.y==0x4000);e.stagger=1;liz.shape_angle.y=7;face_extended_enemy(&liz,e);assert(liz.shape_angle.y==7);
 NativeActor chu;e.stagger=0;e.profile=fpcNm_E_SM2_e;chu.action=ACTION_COMBINE;engage_extended_enemy(&chu,e);
 assert(chu.combine_off_timer==30000&&chu.action==ACTION_ATTACK);
 // The doubled Lizalfos must enter native battle spacing before body contact
 // can keep it beyond the ordinary 300/350-unit battle/attack thresholds.
 e.profile=fpcNm_E_DN_e;e.elite=true;e.initialized=true;e.retiring=e.credited=e.dormant=false;e.stagger=0;
 liz.id=e.id;liz.profile=e.profile;liz.health=100;liz.scale={2,2,2};liz.action=3;liz.current.pos={0,0,0};
 player.current.pos={0,0,590};s_run.phase=Phase::Running;arena=true;stageChange=false;
 std::any lizArgs[]={static_cast<fopAc_ac_c*>(&liz)};
 for(unsigned frame=0;frame<5;++frame){
  liz.pl_dir=590;assert(before_roster_native_damage(nullptr,lizArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
  assert(liz.pl_dir==295&&liz.pl_dir<300&&liz.pl_dir<350);
  before_roster_native_damage(nullptr,lizArgs,nullptr,nullptr);assert(liz.pl_dir==295); // Idempotent.
 }
 player.current.pos.z=650;before_roster_native_damage(nullptr,lizArgs,nullptr,nullptr);
 assert(liz.pl_dir==325&&liz.pl_dir>=300&&liz.pl_dir<350);
 player.current.pos.z=710;before_roster_native_damage(nullptr,lizArgs,nullptr,nullptr);assert(liz.pl_dir>350);
 player.current.pos.z=590;
 for(int reason=0;reason<13;++reason){
  if(reason==0)e.elite=false;
  if(reason==1)e.initialized=false;
  if(reason==2)e.retiring=true;
  if(reason==3)e.credited=true;
  if(reason==4)e.dormant=true;
  if(reason==5)e.stagger=1;
  if(reason==6)liz.health=0;
  if(reason==7)s_run.phase=Phase::Ready;
  if(reason==8)stageChange=true;
  if(reason==9)liz.action=5;
  if(reason==10)liz.scale={1,1,1};
  if(reason==11)arena=false;
  if(reason==12)playerPresent=false;
  liz.pl_dir=590;before_roster_native_damage(nullptr,lizArgs,nullptr,nullptr);assert(liz.pl_dir==590);
  e.elite=e.initialized=true;e.retiring=e.credited=e.dormant=false;e.stagger=0;liz.health=100;
  s_run.phase=Phase::Running;stageChange=false;liz.action=3;liz.scale={2,2,2};arena=true;playerPresent=true;
 }
 liz.id=kNone;liz.pl_dir=590;assert(before_roster_native_damage(nullptr,lizArgs,nullptr,nullptr)==HOOK_CONTINUE&&liz.pl_dir==590);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    constants='constexpr int '+','.join(f'{p}={i+1}' for i,p in enumerate(profiles))+';\n'
    cpp.write_text(fixture+constants+native+(root/'src/survivors_extended_ai.inc').read_text()+
                   (root/'src/survivors_extended_damage.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors roster: all typed targets, HP, one-hit damage/status, cooldowns, stagger, shell routing and defeat ownership passed')
