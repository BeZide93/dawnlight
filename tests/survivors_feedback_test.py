"""Production floating feedback, critical-hit confirmation and scoped no-flinch."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
bool s_victory=false;bool fateTips=false;bool show_fate_tooltips(){return fateTips;}

#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;using s16=short;using u8=unsigned char;
struct ModContext{};
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};
struct fopAc_ac_c{unsigned id=1;int health=20;cXyz eyePos{320,120,1000};struct{cXyz pos{320,0,1000};}current;};
struct daAlink_c:fopAc_ac_c{unsigned flags=0;short mDamageTimer=0;
 bool checkModeFlg(unsigned f){return flags&f;}void onModeFlg(unsigned f){flags|=f;}void offModeFlg(unsigned f){flags&=~f;}};
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}unsigned s_player=1;
bool leech=true;bool sword_kill_hit(const void*){return leech;}
struct Enemy{bool swordKill=false;} trackedEnemy;Enemy* tracked(unsigned){return &trackedEnemy;}
DamageAmounts hit_damage_sources(const void*){DamageAmounts weights{};weights[0]=1;return weights;}
struct dCcU_AtInfo{void* mpCollider=nullptr;unsigned mAttackPower=3;};
Run s_run;bool arena=true,event=false,view=true,font=true;
bool in_arena(){return arena;}bool playable(){return arena&&!event;}
bool dComIfGp_event_runCheck(){return event;}bool dComIfGd_getView(){return view;}
void* mDoExt_getMesgFont(){return font?&s_run:nullptr;}
float matrix[4][4]={{},{},{},{0,0,1,0}};auto* dComIfGd_getProjViewMtx(){return &matrix;}
void mDoLib_project(cXyz* w,cXyz* s){*s=*w;}
namespace mDoGph_gInf_c{float getMinXF(){return 0;}float getMaxXF(){return 640;}float getHeightF(){return 480;}}
namespace JUtility{struct TColor{TColor(unsigned,unsigned,unsigned,unsigned){}};}
enum{HBIND_CENTER,VBIND_TOP};
struct Printed{std::string text;float y;unsigned alpha;float size;};std::vector<Printed>printed;
struct J2DPrint{float size=0;J2DPrint(void*,JUtility::TColor,JUtility::TColor){}void setFontSize(float x,float){size=x;}
 void printReturn(const char* t,float,float,int,int,float,float y,unsigned a){printed.push_back({t,y,a,size});}};
'''
main=r'''
int main(){
 s_run.start(1);fopAc_ac_c enemy;daAlink_c link;dCcU_AtInfo info;int args=1,other=2;
 for(auto kind:{CombatText::Block,CombatText::Dodge,CombatText::Crit}){
  clear_combat_feedback();printed.clear();combat_feedback(&enemy,kind);draw_combat_feedback();
  assert(printed.size()==2&&printed.back().alpha==255);
  assert(printed.back().text==(kind==CombatText::Block?"Block":kind==CombatText::Dodge?"Dodge":"Crit"));
  const auto y=printed.back().y;for(unsigned i=0;i<15;++i)update_combat_feedback();
  printed.clear();draw_combat_feedback();assert(printed.back().alpha<255&&printed.back().alpha>0&&printed.back().y>y);
  for(unsigned i=0;i<15;++i)update_combat_feedback();printed.clear();draw_combat_feedback();assert(printed.empty());
 }
 // Every Fate name rises for two seconds and fades, then disappears.
 for(unsigned i=0;i<unsigned(FateEffect::Count);++i){
  clear_combat_feedback();printed.clear();fate_feedback({320,20,1000},FateEffect(i));draw_combat_feedback();
  assert(printed.size()==2&&printed.back().text==fate_names[i]&&printed.back().alpha==255);
  const auto y=printed.back().y;for(unsigned n=0;n<30;++n)update_combat_feedback();
  printed.clear();draw_combat_feedback();assert(printed.back().y>y&&printed.back().alpha==127);
  for(unsigned n=0;n<30;++n)update_combat_feedback();printed.clear();draw_combat_feedback();assert(printed.empty());
 }
 fateTips=true;
 for(unsigned i=0;i<unsigned(FateEffect::Count);++i){
  clear_combat_feedback();printed.clear();fate_feedback({320,20,1000},FateEffect(i));draw_combat_feedback();
  assert(printed.size()==4&&printed[1].text==fate_names[i]&&printed[3].text==fate_tooltips[i]);
  assert(printed[3].size<printed[1].size&&printed[3].y==printed[1].y+18&&printed[3].alpha==printed[1].alpha);
  for(unsigned n=0;n<30;++n)update_combat_feedback();printed.clear();draw_combat_feedback();
  assert(printed[3].alpha==127&&printed[3].alpha==printed[1].alpha);
  fateTips=false;printed.clear();draw_combat_feedback();assert(printed.size()==2&&printed[1].text==fate_names[i]);fateTips=true;
 }
 fateTips=false;
 clear_combat_feedback();begin_damage_feedback(&args,&enemy,&info);mark_damage_critical(&info,true);
 end_damage_feedback(&other);assert(s_damageFeedback.size()==1);
 end_damage_feedback(&args);assert(s_combatTexts[0].remaining==0); // Blocked/invincible: no damage, no Crit.
 begin_damage_feedback(&args,&enemy,&info);mark_damage_critical(&info,true);enemy.health=0;end_damage_feedback(&args);
 assert(s_combatTexts[0].remaining==30&&trackedEnemy.swordKill); // Includes lethal critical damage.
 leech=false;enemy.health=20;begin_damage_feedback(&args,&enemy,&info);enemy.health=0;end_damage_feedback(&args);
 assert(!trackedEnemy.swordKill);leech=true; // A later burn/ranged kill must overwrite the previous sword provenance.
 clear_combat_feedback();enemy.health=20;begin_damage_feedback(&args,&enemy,&info);enemy.health-=3;end_damage_feedback(&args);
 assert(!s_combatTexts[0].remaining); // Ordinary damage/burn does not claim a crit.
 for(unsigned i=0;i<1000;++i)combat_feedback(&enemy,CombatText::Crit);
 assert(s_combatTexts.size()==64);clear_combat_feedback();assert(s_damageFeedback.empty());
 combat_feedback(&enemy,CombatText::Dodge);
 for(unsigned reason=0;reason<5;++reason){printed.clear();event=reason==0;view=reason!=1;font=reason!=2;
  s_run.phase=reason==3?Phase::Upgrade:Phase::Running;s_combatTexts[0].position.z=reason==4?-1:1000;
  draw_combat_feedback();assert(printed.empty());}
 event=false;s_run.phase=Phase::Running;
 // Bonus attribution uses actual removed HP, including critical/base damage and overkill.
 s_run.damage={};enemy.health=100;begin_damage_feedback(&args,&enemy,&info);
 mark_damage_split(&info,20,26);enemy.health=74;end_damage_feedback(&args);
 assert(s_run.damage.dealt[0]==20&&s_run.damage.dealt[unsigned(DamageSource::DarkLink)]==6);
 begin_damage_feedback(&args,&enemy,&info);mark_damage_split(&info,20,26);enemy.health=-500;end_damage_feedback(&args);
 assert(s_run.damage.total()==100);
 s_run.phase=Phase::Victory;enemy.health=100;begin_damage_feedback(&args,&enemy,&info);enemy.health=0;end_damage_feedback(&args);
 assert(s_run.damage.total()==100);s_run.phase=Phase::Running;
 // Late callbacks during the free-movement victory grace cannot add damage.
 s_victory=true;enemy.health=100;begin_damage_feedback(&args,&enemy,&info);enemy.health=0;end_damage_feedback(&args);
 assert(s_run.damage.total()==100&&s_damageFeedback.empty());s_victory=false;
 // Scope the native no-reaction flag to checkDamageAction; damage still occurs.
 for(bool original:{false,true}){link.flags=original?0x4000000:0;link.mDamageTimer=0;
  begin_player_damage(&args,&link);suppress_player_reaction(&link);
  assert(link.checkModeFlg(0x4000000));link.health-=3;link.mDamageTimer=20;
  after_player_damage(nullptr,&args,nullptr,nullptr);
  assert(link.checkModeFlg(0x4000000)==original&&link.mDamageTimer==0&&s_playerDamageFrames.empty());}
 link.flags=0;suppress_player_reaction(&link);assert(link.flags==0); // Unscoped damage is untouched.
 arena=false;begin_player_damage(&args,&link);suppress_player_reaction(&link);assert(link.flags==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+''.join((root/'src'/name).read_text() for name in
        ['survivors_feedback.inc','survivors_feedback_draw.inc','survivors_player_damage.inc'])+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors feedback: critical confirmation, rising/fading labels, caps, cleanup and no-flinch scope passed')
