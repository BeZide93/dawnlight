from survivors_performance_fixture import helpers
"""Run production burn/multihit lifecycle and damage hooks with engine-boundary fakes."""
from pathlib import Path
import ast
import os
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
# Reuse the enemy/collision boundary, never a duplicate implementation of effects.
tree = ast.parse((root/'tests/survivors_defense_test.py').read_text())
fixture = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'fixture' for t in n.targets))
fixture = fixture.replace('#include <cassert>', '#include <cassert>\n#include <map>\n#include <cstdint>')
fixture = fixture.replace('using s16=short;', 'using ActorId=unsigned;using u32=unsigned;using u16=unsigned short;constexpr ActorId kNone=~0u;\nusing s16=short;')
fixture = fixture.replace('struct cXyz{float x=0,y=0,z=0;', '''struct cXyz{float x=0,y=0,z=0;
 cXyz operator*(float n)const{return{x*n,y*n,z*n};}
 float abs2()const{return x*x+y*y+z*z;}
 cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}
 float abs(cXyz b)const{auto d=*this-b;return std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);}''')
fixture = fixture.replace('struct fopAc_ac_c{int id=2,health=50;', 'struct fopAc_ac_c{int id=2,health=50;int field_0x560=100;cXyz eyePos{},speed{};')
fixture = fixture.replace('struct Collider{', '''struct dCcD_Stts{void Init(int,int,fopAc_ac_c*){}};
constexpr int kAttackSource=0,AT_TYPE_ARROW=1,HIT_TYPE_LINK_HEAVY_ATTACK=9;
struct Collider{unsigned power=0;fopAc_ac_c* owner=nullptr;fopAc_ac_c* GetAc(){return owner;}
 void Set(int){}void SetStts(dCcD_Stts*){}void SetAtType(int){}void SetAtAtp(unsigned p){power=p;}
 unsigned GetAtAtp(){return power;}void SetTgHit(Collider* c){attack=c;hit=true;}
 void SetTgHitApid(unsigned){}void SetTgHitPos(cXyz&){}''')
fixture = fixture.replace('using cCcD_Obj=Collider;', 'using dCcD_Sph=Collider;\nstruct dCcU_AtInfo{Collider* mpCollider=nullptr;unsigned mAttackPower=0;int mHitType=0;};\nusing cCcD_Obj=Collider;')
fixture = fixture.replace('int fopAcM_GetID', 'unsigned fopAcM_GetID')
fixture = fixture.replace('struct daE_OC_c:fopAc_ac_c{', 'struct daE_OC_c:fopAc_ac_c{dCcU_AtInfo mAtInfo;')
fixture = fixture.replace('struct e_ba_class:fopAc_ac_c{', 'struct e_ba_class:fopAc_ac_c{dCcU_AtInfo mAtInfo;')
fixture = fixture.replace('struct e_ms_class:fopAc_ac_c{', 'struct e_ms_class:fopAc_ac_c{dCcU_AtInfo mAtInfo;')
fixture = fixture.replace('struct Enemy{int profile=1;bool initialized=true,retiring=false,credited=false;}enemy;', '''struct Enemy{ActorId id=kNone;int profile=1;bool initialized=false,retiring=false,credited=false,dormant=false,swordKill=false;};
std::array<Enemy,enemy_capacity>s_enemies;
std::map<ActorId,fopAc_ac_c*> actors;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}''')
fixture = fixture.replace('Enemy* tracked(int id){return present&&id==2?&enemy:nullptr;}', 'Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id==id)return &e;return nullptr;}')
fixture = fixture.replace('SwordAction{None,Normal}', 'SwordAction{None,Normal,Spin,Jump}')
fixture = fixture.replace('Run s_run;int s_player=1;', 'Run s_run;ActorId s_player=1;bool s_active=true;')
fixture = fixture.replace('struct dCcU_AtInfo{Collider* mpCollider;};', '')
fixture = re.sub(r'void (?:prepare_effect_hit|finish_effect_hit|record_status_hit)\([^\n]+\n', '', fixture)
fixture=fixture.replace('bool effect_staggers(const cCcD_Obj*){return false;}','')
fixture=fixture.replace('struct daAlink_c:fopAc_ac_c{','''struct SwordModel{cXyz origin{};const cXyz* getBaseTRMtx(){return &origin;}} swordModel;
void mDoMtx_multVec(const cXyz* matrix,const cXyz* local,cXyz* result){*result=*matrix+*local;}
struct daAlink_c:fopAc_ac_c{SwordModel* mSwordModel=&swordModel;bool master=false,drawn=true;
bool checkMasterSwordEquip(){return master;}bool checkItemSwordEquip(){return drawn;}''')
fixture=fixture.replace('struct Attack{','struct Attack{bool bombArrow=false,detonated=false;')
fixture+='void detonate_bomb_arrow(Attack& a,const cXyz&){a.detonated=true;}\n'
fixture += r'''
bool sword_kill_hit(const cCcD_Obj*);
void trigger_lightning(fopAc_ac_c*){}
unsigned knockRequests=0;
void queue_knockback(Enemy&,fopAc_ac_c*,bool,DamageSource){++knockRequests;}
void update_knockback(Enemy&,fopAc_ac_c*){}
bool jump_strike_pose(const daAlink_c*){return false;}
struct dBgS_LinChk{cXyz target;void Set(const cXyz*,cXyz* to,daAlink_c*){target=*to;}};
struct Background{bool LineCross(dBgS_LinChk* line){return line->target.x==999;}} bg;
Background& dComIfG_Bgsp(){return bg;}
u16 bow_fire_effect(){return 99;}
struct Emitter{u16 effect=0;bool stopped=false,deleted=false;uintptr_t user=0;cXyz scale,position;
 void setGlobalPrmColor(unsigned r,unsigned,unsigned){black=r<20;}void setGlobalEnvColor(unsigned,unsigned,unsigned){}bool black=false;
 void stopCreateParticle(){stopped=true;}void quitImmortalEmitter(){}void becomeInvalidEmitter(){deleted=true;}
 bool isEnableDeleteEmitter(){return deleted;}void setParticleCallBackPtr(void*){}
 void setUserWork(uintptr_t p){user=p;}void setGlobalScale(cXyz s){scale=s;}};
struct BlackFlameCallback{void bind(Emitter*){}void unbind(Emitter*){}};
std::map<u32,Emitter> emitters;u32 nextKey=1;
Emitter* dComIfGp_particle_getEmitter(u32 key){auto it=emitters.find(key);return it==emitters.end()?nullptr:&it->second;}
u32 dComIfGp_particle_set(u32 key,u16 effect,cXyz* pos,void*,void*){if(!key)key=nextKey++;emitters.try_emplace(key);emitters[key].effect=effect;emitters[key].position=*pos;return key;}
struct dPa_control_c{static void* getParticleTracePCB(){return nullptr;}};
constexpr u16 ID_ZI_J_LK_BURNS_A=1,ID_ZI_J_LK_BURNS_B=2;
'''
fixture += r'''
constexpr int fpcNm_E_RD_e=4;
struct e_rd_class{fopAc_ac_c enemy;Collider cc_sph[3];dCcU_AtInfo at_info;s16 damage_timer=0;};
'''
# Exercise actual critical feedback at the native damage boundary.
fixture=re.sub(r'enum class CombatText[^\n]+\n|void (?:combat_feedback|begin_damage_feedback|end_damage_feedback|mark_damage_critical)\([^\n]+\n', '', fixture)
fixture+='DamageAmounts hit_damage_sources(const cCcD_Obj*);\n'
fixture+=(root/'src/survivors_feedback.inc').read_text()
fixture += r'''
bool boss_blocks_from(const Enemy&,fopAc_ac_c*,const cXyz&){return false;}
'''
source = (root/'src/survivors.cpp').read_text()
def function(name):
    match = re.search(r'^(?:bool|void) '+name+r'\([^;{}]*\)\s*\{', source, re.M)
    start = source.index('{', match.start()); depth=0
    for i in range(start,len(source)):
        depth += (source[i]=='{')-(source[i]=='}')
        if not depth:return source[match.start():i+1]
fixture += r'''
struct BulblinHit{bool critical=false;unsigned basePower=0,scaledPower=0;};BulblinHit* bulblin_power_hit(dCcU_AtInfo*){return nullptr;}
bool reuse_bulblin_power(dCcU_AtInfo*){return false;}
struct BulblinPowerCache{dCcU_AtInfo* info;~BulblinPowerCache(){}};
'''
main = r'''
void hit(e_ms_class& rat){
 Args args{&rat,nullptr};before_enemy_hit(nullptr,&args,nullptr,nullptr);
 if(rat.mSph1.ChkTgHit()){
  rat.mAtInfo.mpCollider=rat.mSph1.GetTgHitObj();rat.mAtInfo.mAttackPower=200;
  Args feedbackArgs{&rat,&rat.mAtInfo};begin_damage_feedback(&feedbackArgs,&rat,&rat.mAtInfo);
  Args power{&rat.mAtInfo,nullptr};after_power(nullptr,&power,nullptr,nullptr);
  rat.health=std::max(0,rat.health-int(rat.mAtInfo.mAttackPower));rat.mAction=rat.health?4:9;
  end_damage_feedback(&feedbackArgs);record_status_hit(&rat,&rat.mAtInfo);
 }
 after_enemy_hit(nullptr,&args,nullptr,nullptr);
}
int main(){
 s_run.start(1);s_run.ranks[unsigned(Upgrade::FireArrows)]=1;player.id=1;
 e_ms_class rat;rat.id=2;rat.health=10;actors[2]=&rat;s_enemies[0]={2,fpcNm_E_MS_e,true};
 s_attacks[0].weapon=unsigned(Upgrade::Bow);dCcU_AtInfo arrow{&s_attacks[0].collider,3};
 s_enemies[0].dormant=true;record_status_hit(&rat,&arrow);assert(s_enemies[0].dormant);
 s_attacks[0].collider.owner=&player;
 record_status_hit(&rat,&arrow);assert(!s_enemies[0].dormant);auto& burn=s_effects[0];assert(burn.burn==90);
 for(unsigned t=1;t<=90;++t){update_enemy_effects();hit(rat);assert(rat.health==10-int(t/30));assert(rat.mAction==1);}
 assert(!burn.burn&&burn.pending==0&&!burn.injected&&!rat.mAtInfo.mpCollider);
 for(auto& [key,e]:emitters){(void)key;assert(e.stopped&&e.deleted&&e.scale.x==3&&e.user==reinterpret_cast<uintptr_t>(&burn.velocity));}
 // Queued independent hits bypass the original roster's native cooldown.
 rat.health=100;rat.mCooldown1=10;burn.pending=7;hit(rat);assert(rat.health==93&&rat.mCooldown1==0);
 rat.mCooldown1=10;burn.pending=11;hit(rat);assert(rat.health==82&&rat.mCooldown1==0);
 // Rapid refresh keeps the tick clock; it must not prevent all periodic damage.
 rat.health=10;record_status_hit(&rat,&arrow);
 for(unsigned t=1;t<=60;++t){if(t%10==0)record_status_hit(&rat,&arrow);update_enemy_effects();hit(rat);}
 assert(rat.health==8&&burn.burn==89);clear_enemy_effects();
 // Upgrade damage, last tick, native lethal reaction and no stale repeats.
 s_run.ranks[unsigned(Upgrade::FireArrows)]=4;rat.health=16;
 record_status_hit(&rat,&arrow);assert(burn.burn==120);
 for(unsigned t=0;t<120;++t){update_enemy_effects();hit(rat);}
 assert(rat.health==0&&rat.mAction==9);update_enemy_effects();assert(!burn.burn&&!burn.pending);
 rat.health=10;rat.mAction=1;s_run.dark_link=29;s_run.dark_remaining=150;
 record_status_hit(&rat,&arrow);for(int t=0;t<30;++t){update_enemy_effects();hit(rat);}assert(rat.health==2);
 // Deletion, slot reuse and shutdown stop particles and queued damage.
 auto old=burn.emitters;actors.erase(2);update_enemy_effects();assert(!burn.burn&&!burn.pending);
 for(auto key:old)assert(emitters[key].deleted);
 actors[2]=&rat;record_status_hit(&rat,&arrow);s_enemies[0].id=3;effect_for(s_enemies[0]);assert(!burn.burn&&!burn.pending);
 clear_enemy_effects();s_enemies.fill({});actors.clear();s_run.dark_remaining=0;
 // Rank 1: primary plus the nearest reachable enemy. Never a far/walled/elevated one.
 std::array<e_ms_class,7> group;
 for(unsigned i=0;i<group.size();++i){group[i].id=i+2;group[i].current.pos={float(30*i),0,50};group[i].eyePos=group[i].current.pos;
  actors[i+2]=&group[i];s_enemies[i]={i+2,fpcNm_E_MS_e,true};}
 group[4].eyePos.x=999;group[5].current.pos.y=200;group[6].current.pos.x=450;
 s_run.dual_wield=1;s_swordAction=SwordAction::Normal;dCcU_AtInfo sword{&player.mAtCps[0],3};
 record_status_hit(&group[0],&sword);assert(s_effects[1].pending==3&&s_effects[2].pending==0);
 record_status_hit(&group[0],&sword);assert(s_effects[1].pending==3); // once per swing
 group[1].mSph1.hit=true;group[1].mSph1.attack=&player.mAtCps[1];hit(group[1]);
 assert(group[1].health==47&&!s_effects[1].pending); // natural capsule cannot duplicate multihit
 clear_enemy_effects();++s_meleeSerial;s_run.dual_wield=30;
 record_status_hit(&group[0],&sword);
 for(unsigned i=1;i<4;++i){assert(s_effects[i].pending==3);hit(group[i]);}
 for(unsigned i=4;i<7;++i)assert(!s_effects[i].pending);
 // Reach admits farther additional targets while retaining LOS/height gates
 // and the same per-swing target budget, with no duplicate queued damage.
 clear_enemy_effects();++s_meleeSerial;s_run.ranks[unsigned(Upgrade::Reach)]=10;
 record_status_hit(&group[0],&sword);assert(s_effects[6].pending==3);
 assert(!s_effects[4].pending&&!s_effects[5].pending);
 record_status_hit(&group[0],&sword);assert(s_effects[6].pending==3);
 s_run.ranks[unsigned(Upgrade::Reach)]=0;clear_enemy_effects();
 // Natural projectile collisions are preserved until they have been processed.
 ++s_meleeSerial;record_status_hit(&group[0],&sword);auto& target=group[2];
 target.mSph1.hit=true;target.mSph1.attack=&s_attacks[0].collider;prepare_effect_hit(&target,s_enemies[2]);
 assert(s_effects[2].pending==3&&!s_effects[2].injected&&target.mSph1.attack==&s_attacks[0].collider);
 target.mSph1.ResetTgHit();hit(target);assert(!s_effects[2].pending);
 clear_enemy_effects();for(auto& e:s_effects)assert(!e.burn&&!e.pending&&!e.injected);
 // Secondary melee rolls independently; queued burn never rolls or doubles again.
 s_run.ranks[unsigned(Upgrade::CritChance)]=30;s_run.ranks[unsigned(Upgrade::CritDamage)]=30;
 s_run.ranks[unsigned(Upgrade::Luck)]=20;s_run.dark_remaining=150;s_run.dual_wield=1;
 ++s_meleeSerial;s_run.random=1;record_status_hit(&group[0],&sword);
 assert(s_effects[1].pending==11&&s_run.random==270369&&s_effects[1].pendingCritical);
 clear_combat_feedback();group[1].health=50;group[1].mSph1.ResetTgHit();hit(group[1]);
 assert(s_combatTexts[0].remaining==30);
 assert(group[1].health==28&&s_run.random==270369); // x3.5 then Dark x2, exactly once
 clear_enemy_effects();clear_combat_feedback();record_status_hit(&group[1],&arrow);
 const auto burnRandom=s_run.random;
 for(int t=0;t<30;++t){update_enemy_effects();hit(group[1]);}
 assert(!s_combatTexts[0].remaining);
 assert(group[1].health==20&&s_run.random==burnRandom); // 4 damage/s, only Dark x2
 clear_enemy_effects();record_status_hit(&group[1],&arrow);
 for(int t=0;t<30;++t)update_enemy_effects();
 assert(s_effects[1].pending==4);
 ++s_meleeSerial;s_run.random=1;record_status_hit(&group[0],&sword);
 assert(s_effects[1].pending==15);group[1].health=50;s_run.damage={};hit(group[1]);
 assert(s_run.damage.total()==30&&s_run.damage.dealt[unsigned(DamageSource::DarkLink)]==15);
 assert(s_run.damage.dealt[unsigned(DamageSource::DualWield)]==11&&s_run.damage.dealt[unsigned(DamageSource::FireArrows)]==4);
 assert(group[1].health==20&&s_run.random==270369); // (11 melee + 4 burn) x2
 // Dropping flames releases only graphics: all three burn pulses still land.
 clear_enemy_effects();s_run.dark_remaining=0;s_run.ranks[unsigned(Upgrade::FireArrows)]=1;
 group[1].health=50;group[1].mSph1.ResetTgHit();survivor_perf::testEnabled=true;s_burnVisuals.fill(false);
 record_status_hit(&group[1],&arrow);const unsigned duration=s_effects[1].burn;
 assert(duration==90);
 for(unsigned tick=0;tick<duration;++tick){update_enemy_effects();hit(group[1]);assert(!s_effects[1].emitters[0]&&!s_effects[1].emitters[1]);}
 assert(group[1].health==47);survivor_perf::testEnabled=false;

 clear_enemy_effects();
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+helpers()+function('native_sword_collider')+(root/'src/survivors_status.inc').read_text()+function('after_power')+(root/'src/survivors_defense.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors status: burn timing/refresh/deaths, Dark damage, nearest multihits, duplicate suppression and cleanup passed')
