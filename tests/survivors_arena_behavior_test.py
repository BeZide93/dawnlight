"""Production collision filtering, directional boss guard and music ownership."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <any>
#include <array>
#include <cassert>
#include <cmath>
using u32=unsigned;using s16=short;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void*p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct cXyz{float x=0,y=0,z=0;cXyz operator-(cXyz p)const{return{x-p.x,y-p.y,z-p.z};}float absXZ()const{return std::hypot(x,z);}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
struct fopAc_ac_c{int id=1,health=100;struct{cXyz pos;}current;struct{short y=0;}shape_angle;};
int fopAcM_GetID(fopAc_ac_c* a){return a->id;}
struct cCcD_Obj{fopAc_ac_c* owner=nullptr;bool shield=false,hit=false;int target=0;
 auto* GetAc(){return owner;}auto* GetGObjInf(){return this;}void SetAtHit(cCcD_Obj*){hit=true;}
 void SetAtHitApid(int id){target=id;}void OnAtShieldHit(){shield=true;}};
using dCcD_GObjInf=cCcD_Obj;
struct Player:fopAc_ac_c{cCcD_Obj mGuardAtCps,sword;bool dead=false;bool checkDeadHP(){return dead;}}player;
Player* daAlink_getAlinkActorClass(){return &player;}
bool arena=true,s_hub=false,s_active=true,s_gameStarted=true,s_restart=false,s_deathPending=false,moving=false;
bool in_arena(){return arena;}bool transitioning(){return moving;}
int s_player=1;unsigned s_map=4;
enum class Phase{Ready,Loading,Running,Upgrade,Victory,Defeat};
struct{struct{bool enemy_collision=false;}options;Phase phase=Phase::Running;}s_run;
struct Enemy{int profile=0;bool elite=true,dormant=false,stagger=false;};
bool enemy_child_root(int id){return id==2||id==3;}
constexpr int fpcNm_B_TN_e=10,fpcNm_B_GND_e=11,fpcNm_E_SF_e=12,fpcNm_E_MF_e=13;
struct daB_TN_c:fopAc_ac_c{enum{ACT_GUARDH=6,ACT_GUARDL=12};int mActionMode1=0;};
struct b_gnd_class:fopAc_ac_c{int mActionMode=0;};
struct e_sf_class:fopAc_ac_c{int arg3=0,mAction=7,mActionPhase=1;
 struct Morph{float frame=2;float getFrame(){return frame;}}morph;Morph* mpModelMorf=&morph;};
struct e_mf_class:e_sf_class{int field_0x5b4=6;};
struct Attack{bool active=false;cCcD_Obj collider;cXyz position,velocity;};std::array<Attack,2>s_attacks;
bool native_sword_collider(cCcD_Obj*c){return c==&player.sword;}
constexpr u32 Z2BGM_VS_GANON_04=10;
struct Audio{u32 id=0;int starts=0,stops=0;u32 getMainBgmID(){return id;}
 void bgmStart(u32 v,int,int){id=v;++starts;}void bgmStop(int,int){id=0;++stops;}}audio;
Audio* Z2GetAudioMgr(){return &audio;}
'''
main=r'''
int main(){
 fopAc_ac_c enemy,child,pot;enemy.id=2;child.id=3;pot.id=4;
 cCcD_Obj linkCo{&player},enemyCo{&enemy},childCo{&child},potCo{&pot};
 auto contact=[](cCcD_Obj*a,cCcD_Obj*b){std::any args[]={nullptr,a,nullptr,b};return before_enemy_contact(nullptr,args,nullptr,nullptr);};
 for(auto* enemyShape:{&enemyCo,&childCo}) {
  assert(contact(&linkCo,enemyShape)==HOOK_SKIP_ORIGINAL);
  assert(contact(enemyShape,&linkCo)==HOOK_SKIP_ORIGINAL);
 }
 assert(contact(&linkCo,&potCo)==HOOK_CONTINUE&&contact(&enemyCo,&childCo)==HOOK_CONTINUE);
 assert(contact(nullptr,&linkCo)==HOOK_CONTINUE);
 s_run.options.enemy_collision=true;assert(contact(&linkCo,&enemyCo)==HOOK_CONTINUE);
 s_run.options.enemy_collision=false;s_hub=true;assert(contact(&linkCo,&enemyCo)==HOOK_CONTINUE);
 s_hub=false;arena=false;assert(contact(&linkCo,&enemyCo)==HOOK_CONTINUE);arena=true;
 // Facing guard blocks sword, shield and incoming projectiles, with no damage path.
 daB_TN_c darknut;darknut.id=7;Enemy owner{fpcNm_B_TN_e};cCcD_Obj target{&darknut};
 player.current.pos={0,0,100};
 for(int action:{daB_TN_c::ACT_GUARDH,daB_TN_c::ACT_GUARDL}) {
  darknut.mActionMode1=action;
  assert(block_boss_contact(owner,&darknut,&player.sword,&target));
  assert(player.sword.hit&&player.sword.shield&&player.sword.target==7);
  assert(block_boss_contact(owner,&darknut,&player.mGuardAtCps,&target));
  assert(!boss_blocks_from(owner,&darknut,{0,0,-100}));
 }
 auto& arrow=s_attacks[0];arrow.active=true;arrow.position={0,0,-5};arrow.velocity={0,0,-30};
 assert(block_boss_contact(owner,&darknut,&arrow.collider,&target)); // overshot center, approaching from front
 cCcD_Obj foreign;assert(!block_boss_contact(owner,&darknut,&foreign,&target));
 darknut.mActionMode1=1;assert(!block_boss_contact(owner,&darknut,&player.sword,&target));
 b_gnd_class ganon;owner.profile=fpcNm_B_GND_e;ganon.mActionMode=12;
 assert(boss_blocks_from(owner,&ganon,{0,0,100}));ganon.mActionMode=10;assert(!boss_blocks_from(owner,&ganon,{0,0,100}));
 // Stalfos blocks in its visible guard window, with a real native shield
 // contact response. Rear hits, startup/release and ordinary attacks penetrate.
 e_sf_class sf;sf.id=8;owner.profile=fpcNm_E_SF_e;target.owner=&sf;
 for(int phase=0;phase<4;++phase){sf.mActionPhase=phase;
  assert(block_boss_contact(owner,&sf,&player.sword,&target)==(phase==1||phase==2));
  assert(!boss_blocks_from(owner,&sf,{0,0,-100}));
 }
 sf.mActionPhase=1;sf.morph.frame=1;assert(!boss_blocks_from(owner,&sf,{0,0,100}));
 sf.morph.frame=2;assert(block_boss_contact(owner,&sf,&arrow.collider,&target));
 assert(arrow.collider.shield&&arrow.collider.hit&&arrow.collider.target==8);
 for(int action:{3,4,5}){sf.mAction=action;assert(!boss_blocks_from(owner,&sf,{0,0,100}));}
 sf.mAction=7;sf.arg3=0xff;assert(!boss_blocks_from(owner,&sf,{0,0,100}));
 sf.arg3=0;owner.dormant=true;assert(!boss_blocks_from(owner,&sf,{0,0,100}));
 owner.dormant=false;owner.stagger=true;assert(!boss_blocks_from(owner,&sf,{0,0,100}));
 // Dynalfos: raised guard blocks front contacts, but not rear/startup/release.
 e_mf_class dyn;dyn.id=9;owner={fpcNm_E_MF_e};target.owner=&dyn;dyn.mAction=7;
 for(int mode:{0,1,5,6,7,8}){dyn.field_0x5b4=mode;dyn.morph.frame=2;
  assert(block_boss_contact(owner,&dyn,&player.sword,&target)==(mode==6||mode==7));
  assert(!boss_blocks_from(owner,&dyn,{0,0,-100}));}
 dyn.field_0x5b4=6;dyn.morph.frame=1;assert(!boss_blocks_from(owner,&dyn,{0,0,100}));
 dyn.morph.frame=2;assert(block_boss_contact(owner,&dyn,&arrow.collider,&target));
 assert(arrow.collider.shield&&arrow.collider.hit&&arrow.collider.target==9);
 for(int action:{3,5,6}){dyn.mAction=action;assert(!boss_blocks_from(owner,&dyn,{0,0,100}));}
 dyn.mAction=7;owner.dormant=true;assert(!boss_blocks_from(owner,&dyn,{0,0,100}));
 owner.dormant=false;owner.stagger=true;assert(!boss_blocks_from(owner,&dyn,{0,0,100}));
 // One music start, pause continuity, departure cleanup, destination ownership.
 update_survivors_music();update_survivors_music();assert(audio.starts==1&&audio.stops==0);
 s_run.phase=Phase::Upgrade;update_survivors_music();assert(audio.starts==1);
 moving=true;update_survivors_music();assert(audio.stops==1&&!s_throneMusic);
 audio.id=42;update_survivors_music();assert(audio.id==42&&audio.stops==1);
 moving=false;s_run.phase=Phase::Running;update_survivors_music();assert(audio.starts==2);
 audio.id=43;s_map=0;update_survivors_music();assert(audio.id==43&&audio.stops==1&&!s_throneMusic);
 s_map=4;update_survivors_music();player.dead=true;update_survivors_music();assert(audio.stops==2);
 player.dead=false;s_hub=true;update_survivors_music();assert(audio.starts==3);
 s_hub=false;s_active=false;update_survivors_music();assert(audio.starts==3);
}
'''
source=''.join((root/'src'/f'survivors_{name}.inc').read_text() for name in ['collision','boss_guard','music'])
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(fixture+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Arena behavior: contact pairs, directional guard and scoped music lifecycle passed')
