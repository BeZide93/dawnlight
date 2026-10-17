"""Production ranged animation scheduler: release frames, aim, interrupts and fairness."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_ranged.inc').read_text().split('void draw_ranged()')[0]
fixture=r'''
#include <cassert>
#include <cmath>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u16=unsigned short;using s16=short;
constexpr int FALSE=0,dItemNo_NONE_e=255;
constexpr u16 dRes_ID_ALANM_BCK_ARELORD_e=1,dRes_ID_ALANM_BCK_ASHOOT_e=2,dRes_ID_ALANM_BCK_BOOMTHROW_e=3;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}float absXZ()const{return std::sqrt(x*x+z*z);}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int){return static_cast<T>(p);}}
struct Frame{float frame=0,rate=1;int end=12;float getFrame(){return frame;}int getEnd(){return end;}void setRate(float n){rate=n;}};
struct daAlink_c{
 enum{UPPER_2=2};int id=1,mEquipItem=0x103,proc=0;u16 animation=0;
 bool equipping=false;bool checkEquipAnime(){return equipping;}
 Frame mUpperFrameCtrl[3],mUnderFrameCtrl[1];float field_0x347c=5,field_0x3484=8;
 struct{cXyz pos;struct{s16 y=123;}angle;}current;
 struct{s16 x=0,y=0;}shape_angle,mBodyAngle;
 bool checkUpperAnime(u16 a){return animation==a;}
 void resetUpperAnime(int,float){animation=0;}
 void setUpperAnimeBaseSpeed(u16 a,float,float){animation=a;mUpperFrameCtrl[2]={0,1,a==1?12:18};}
 void deleteEquipItem(int,int){mEquipItem=dItemNo_NONE_e;}
 void procWaitInit(){proc=0;}
 bool checkAnmEnd(Frame* f){return f->frame>=f->end;}
}player;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
int fopAcM_GetID(daAlink_c* p){return p->id;}
bool playing=true,targetExists=true;
bool playable(){return playing;}bool in_arena(){return playing;}
bool normal_sword_proc(daAlink_c* p){return p->proc==1;}
bool sword_action_available(daAlink_c* p){return p->proc==0;}
struct{bool ready=true;}s_loot;
Run s_run;std::array<unsigned,12>s_cooldowns{};int s_player=1;
enum class SwordAction{None,Normal};SwordAction s_swordAction=SwordAction::Normal;
bool s_wavePending=false;unsigned s_comboStep=0,s_comboTimeout=0;
bool nearest_item_target(daAlink_c*,cXyz& target){target={300,100,0};return targetExists;}
std::vector<Upgrade>shots;std::vector<float>spread;
bool launch_item(daAlink_c*,Upgrade weapon,cXyz,float angle=0){shots.push_back(weapon);spread.push_back(angle);return true;}
'''
main=r'''
void fresh(){player={};s_run.start(1);s_cooldowns={};s_ranged={};s_rangedRest=0;s_lastRanged=Upgrade::Boomerang;
 shots.clear();spread.clear();playing=targetExists=true;s_loot.ready=true;s_run.ranks[8]=s_run.ranks[9]=1;}
void tick(){auto& f=player.mUpperFrameCtrl[2];f.frame=std::min(float(f.end),f.frame+f.rate);update_ranged(&player);}
int main(){
 fresh();assert(queue_ranged(&player,Upgrade::Bow));
 assert(player.animation==dRes_ID_ALANM_BCK_ARELORD_e&&shots.empty()&&player.mEquipItem==dItemNo_NONE_e);
 assert(!queue_ranged(&player,Upgrade::Boomerang));
 before_ranged_aim(nullptr,&player,nullptr,nullptr);
 assert(player.shape_angle.y==0x4000&&player.mBodyAngle.x<0&&player.current.angle.y==123);
 int result=1;assert(before_ranged_native(nullptr,&player,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==0);
 for(unsigned i=0;i<11;++i){tick();assert(shots.empty());}
 tick();assert(shots.size()==1&&s_ranged.released&&player.animation==dRes_ID_ALANM_BCK_ASHOOT_e);
 for(unsigned i=0;i<30;++i){tick();}assert(shots.size()==1&&!s_ranged.active&&player.animation==0);
 fresh();s_run.ranks[8]=5;s_run.ranks[4]=5;
 assert(queue_ranged(&player,Upgrade::Bow));const auto accelerated=player.mUpperFrameCtrl[2].rate;
 assert(accelerated>1);for(unsigned i=0;i<20;++i)tick();
 assert(shots.size()==3&&spread[1]>0&&spread[2]<0);
 fresh();s_lastRanged=Upgrade::Bow;assert(queue_ranged(&player,Upgrade::Boomerang));
 player.mUpperFrameCtrl[2].frame=5;update_ranged(&player);assert(shots.empty());
 player.mUpperFrameCtrl[2].frame=8;update_ranged(&player);assert(shots.size()==1&&s_ranged.released);
 update_ranged(&player);assert(shots.size()==1); // Fast frames never double-release.
 fresh();assert(queue_ranged(&player,Upgrade::Bow));player.proc=2;update_ranged(&player);
 assert(!s_ranged.active&&shots.empty());
 fresh();assert(queue_ranged(&player,Upgrade::Bow));player.animation=99;update_ranged(&player);
 assert(player.animation==99&&!s_ranged.active); // Preserve another system's animation.
 fresh();assert(queue_ranged(&player,Upgrade::Bow));cancel_ranged();
 for(unsigned i=0;i<30;++i){tick();}assert(shots.empty()); // Opening a chest cannot fire a delayed shot.
 fresh();assert(queue_ranged(&player,Upgrade::Bow));targetExists=false;
 for(unsigned i=0;i<30;++i){tick();}assert(shots.empty());
 fresh();player.equipping=true;assert(!queue_ranged(&player,Upgrade::Bow));
 fresh();s_comboStep=2;s_comboTimeout=60;assert(queue_ranged(&player,Upgrade::Bow));
 assert(s_comboStep==2&&s_comboTimeout==60); // Automatic item poses preserve sword combo progress.
 fresh();player.proc=1;player.mUnderFrameCtrl[0].frame=4;assert(!queue_ranged(&player,Upgrade::Bow));
 player.mUnderFrameCtrl[0].frame=10;assert(queue_ranged(&player,Upgrade::Bow)&&player.proc==0);
 cancel_ranged();s_rangedRest=0;s_cooldowns={};
 assert(!queue_ranged(&player,Upgrade::Bow));assert(queue_ranged(&player,Upgrade::Boomerang));
 // Equal due times cannot starve the boomerang behind rapid arrows.
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+source+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors ranged: native animation phases, release timing, haste, aim, cancellation and fairness passed')
