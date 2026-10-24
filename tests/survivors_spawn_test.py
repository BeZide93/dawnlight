"""Arena entry/grounding regression using production hooks and SDK entry modes."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root/'dusklight'
source = (root/'src/survivors_maps.inc').read_text()
runtime = (root/'src/survivors.cpp').read_text()
native = (sdk/'src/d/actor/d_a_alink.cpp').read_text()

def extract(name):
    match = re.search(r'^(?:void|bool|HookAction) '+name+r'\(', source, re.M)
    assert match, name
    start = source.index('{', match.start())
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[match.start():end+1]
    raise AssertionError(name)

# This is the native condition that enters the 35-frame entrance demo. Mode 0
# was mistakenly used as "no demo". Read the SDK rather than duplicate the modes.
entry_condition = re.search(r'else if \((start_mode == 1 \|\| start_mode == 3[^\n]+)\) \{', native).group(1)
start = runtime.index('    if (s_player != fopAcM_GetID(link))')
end = runtime.index('    open_powerup_window();', start)
loading = 'void load_frame(daAlink_c* link) {\n'+runtime[start:end]+'}\n'
names = sorted(set(re.findall(r'fpcNm_\w+', source)))
fixture = r'''
namespace enemy_hud {void reset(){}}

bool s_victory=false,s_deathPending=false,s_resultsStored=false;void prepare_victory_wave(){}

#include <any>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include "survivors_rules.hpp"
#include "../include/survivors_maps.hpp"
#include "../include/survivors_barriers.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned; using s16=short;using BOOL=int;constexpr BOOL TRUE=1,FALSE=0;
struct cXyz {float x=0,y=0,z=0; cXyz()=default; cXyz(float a,float b,float c):x(a),y(b),z(c){} void set(float a,float b,float c){x=a;y=b;z=c;} cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};
struct csXyz {short x=0,y=0,z=0;};
struct fopAc_ac_c {unsigned id=7; int room=51; struct{cXyz pos;}current,old;cXyz speed;csXyz shape_angle;float speedF=0;};
float correctedHeight=0;
struct daAlink_c:fopAc_ac_c {struct{int m_gnd=0;bool hit=false;float floor=-1e9f;bool ChkGroundHit(){return hit;}float GetGroundH(){return floor;}void CrrPos(int){hit=true;floor=correctedHeight;}}mLinkAcch;
 int mGroundCode=0;float mNormalSpeed=0,mFallHeight=0,field_0x33c8=0;cXyz mLastJumpPos,field_0x3798,collisionPush;bool falling=false;
 unsigned teleports=0,waits=0;
 void setPlayerPosAndAngle(const cXyz* p,s16 yaw,BOOL force){assert(force);++teleports;current.pos=old.pos=field_0x3798=*p;collisionPush={};correctedHeight=p->y;shape_angle.y=yaw;}
 void procWaitInit(){++waits;falling=false;}
};
int life=12,damageWrites=0,finished=0,cleared=0,interrupted=0,cancelled=0;
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct dBgS_WtrChk{float bottom=0,top=0,height=0;void Set(cXyz p,float y){bottom=p.y;top=y;}float GetHeight(){return height;}};
float landingFloor=-1.e9f,waterY=0;int landingRoom=0,landingCode=0,groundProbes=0;bool wet=false,pit=false;
struct BG{
 operator int()const{return 0;}int GetGroundCode(int){return 0;}
 float GroundCross(dBgS_ObjGndChk* q){++groundProbes;if(pit&&q->pos.x>100)return -1.e9f;return !std::isfinite(landingFloor)||landingFloor<=q->pos.y?landingFloor:-1.e9f;}
 int GetRoomId(dBgS_ObjGndChk&){return landingRoom;}
 int GetGroundCode(dBgS_ObjGndChk&){return landingCode;}
 bool WaterChk(dBgS_WtrChk* q){q->height=waterY;return wet&&waterY>=q->bottom&&waterY<=q->top;}
}background;
BG& dComIfG_Bgsp(){return background;}int dComIfGs_getLife(){return life;}
void dComIfGs_setLife(int n){life=n;++damageWrites;}
void clear_attacks(){++cleared;}void interrupt_auto_actions(daAlink_c*){++interrupted;}
void cancel_jump_abilities_for(daAlink_c*){++cancelled;}
void begin_defeat(){++finished;}

struct fopAcM_prm_class {struct{unsigned parameters=0;cXyz position;csXyz angle;}base;int room_no=51;struct{int x=10,z=10;}scale;};
struct stage_actor_data_class {char name[8]{};};
struct ModContext{}; enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int i){return std::any_cast<T>(static_cast<std::any*>(args)[i]);}}
bool s_hub=false,s_hubDoorStarted=false,s_arrivingRun=false;void update_hub(daAlink_c*){} void begin_run(){}
unsigned s_map=0;const Arena& arena(){return arenas[s_map];}Run s_run;
ActorId s_player=7;unsigned s_warmup=0,recordsLoaded=0;
bool active=true,transition=false,event=false,creating=false,s_restart=false;
bool in_arena(){return active;}bool transitioning(){return transition;}bool playable(){return active&&!event&&!transition;}
bool fpcM_IsCreating(ActorId){return creating;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetRoomNo(fopAc_ac_c* a){return a->room;}
void fopAcM_SetRoomNo(fopAc_ac_c* a,int n){a->room=n;}
void load_records(){++recordsLoaded;}
void clear_elite_sites(){}
void clear_reachable_floor(){}
void retire_palace_ferries(){}
bool ferryCaptured=false;
bool capture_palace_ferry(int,const fopAcM_prm_class&){return ferryCaptured;}
void capture_elite_site(int,const fopAcM_prm_class&){}
void reset_midna_dialog(){}
void update_midna_dialog(){}
constexpr int fopAc_ENEMY_e=2,fopAc_NPC_e=4;
struct actor_process_profile_definition{int group=0;}profile;
const auto* fpcPf_Get(s16){return &profile;}
struct{int procname=0;}info;
const auto* dStage_searchName(const char*){return &info;}
unsigned freed=0;void JKRFree(void*){++freed;}
float cM_ssin(short a){return std::sin(a*3.14159265f/32768);}float cM_scos(short a){return std::cos(a*3.14159265f/32768);}
bool update_ending(daAlink_c*){return false;}
'''
main = r'''
int main(){
 stage_actor_data_class record;fopAcM_prm_class params;
 std::any spawnArgs[]={&record,&params};info.procname=fpcNm_ALINK_e;
 for(unsigned map=0;map<arenas.size();++map){
  s_map=map;params.base.parameters=0x2ab14a00u|unsigned(arena().room);
  params.base.position={12,34,56};params.base.angle.y=1234;
  const auto original=params.base.parameters;
  assert(before_arena_stage_actor(nullptr,spawnArgs,nullptr,nullptr)==HOOK_CONTINUE);
  assert(params.base.position.y==34&&params.base.angle.y==1234&&freed==0);
  if(map==0) assert(params.base.parameters==original);
  else {
   assert((params.base.parameters>>24)==255);
   assert(((params.base.parameters>>12)&31)==16);
   assert(!native_entrance_demo((params.base.parameters>>12)&31));
   assert((params.base.parameters&0x00fe0fff)==(original&0x00fe0fff));
  }
 }
 // A captured ferry transfers to explicit creation; release the append and
 // suppress its original creator so there cannot be two collision platforms.
 info.procname=fpcNm_BG_e;ferryCaptured=true;
 assert(before_arena_stage_actor(nullptr,spawnArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&freed==1);
 active=false;
 assert(before_arena_stage_actor(nullptr,spawnArgs,nullptr,nullptr)==HOOK_CONTINUE&&freed==1);
 active=true;ferryCaptured=false;freed=0;
 // Capture the native Darknut's arena-side location before filtering it.
 s_map=0;clear_arena_geometry();params.room_no=51;params.base.position={1500,0,100};
 info.procname=fpcNm_B_TN_e;profile.group=fopAc_ENEMY_e;
 assert(before_arena_stage_actor(nullptr,spawnArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(s_arenaCombat&&s_arenaCombatPosition.x==1500&&s_arenaCombatPosition.z==100);
 clear_arena_geometry();assert(!s_arenaCombat);profile.group=0;
 // Native story chests remain filtered; the hub chest uses its own actor.
 s_map=0;params.room_no=51;info.procname=fpcNm_TBOX_e;
 assert(before_arena_stage_actor(nullptr,spawnArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(native_entrance_demo(0)); // The old override entered this path.
 s_map=0;clear_arena_geometry();daAlink_c link;std::any moveArgs[]={&link};
 // Native entrance crosses an exit curtain from outside into the room. Applying
 // the running barrier sweep here traps Link on the wrong side of the floor.
 s_barriers.push_back({{0,0,-100},{0,0,100}});
 s_run.phase=Phase::Loading;s_arenaSafe=true;
 link.old.pos={-100,20,0};link.current.pos={100,0,0};
 after_arena_position(nullptr,moveArgs,nullptr,nullptr);
 assert(link.current.pos.x==100&&link.current.pos.y==0);
 // Once running, the same crossing is correctly blocked without changing Y.
 s_run.phase=Phase::Running;after_arena_position(nullptr,moveArgs,nullptr,nullptr);
 assert(link.current.pos.x<-39&&link.current.pos.y==0);
 active=false;link.current.pos={100,0,0};after_arena_position(nullptr,moveArgs,nullptr,nullptr);
 assert(link.current.pos.x==100);active=true;
 // Loading cannot time out into Ready while airborne, creating, in an event,
 // above stale collision, on an invalid floor, or in a neighbouring room.
 for(int reason=0;reason<6;++reason){
  clear_arena_geometry();s_run.phase=Phase::Loading;s_warmup=0;
  link.current.pos={0,reason==3?100.f:0.f,0};link.mLinkAcch.hit=reason!=0;
  link.mLinkAcch.floor=reason==4?-1e9f:0;link.room=reason==5?0:51;
  creating=reason==1;event=reason==2;
  for(int frame=0;frame<60;++frame)load_frame(&link);
  assert(s_run.phase==Phase::Loading&&s_warmup==0);
 }
 creating=event=false;link.room=51;link.current.pos={100,0,0};
 link.mLinkAcch.hit=true;link.mLinkAcch.floor=0;s_arenaSafe=false;landingFloor=0;landingRoom=51;
 for(int frame=0;frame<14;++frame){load_frame(&link);assert(s_run.phase==Phase::Loading);}
 // Capture native floor contact even if the derived barrier mesh is not ready.
 assert(s_arenaSafe&&!s_arenaMeshReady);
 load_frame(&link);assert(s_run.phase==Phase::Ready&&recordsLoaded==1);
 // Falling during loading must restore the recorded floor, not freeze below it.
 s_run.phase=Phase::Loading;link.current.pos.y=-600;link.mLinkAcch.hit=false;
 link.mLinkAcch.floor=-1e9f;landingFloor=-1e9f;link.speed.y=-50;load_frame(&link);
 assert(link.current.pos.y==0&&link.old.pos.y==0&&link.speed.y==0);
 assert(s_run.phase==Phase::Loading);
 // Replacement players cannot inherit the previous player's rescue position.
 link.id=8;link.current.pos.y=-100;creating=true;load_frame(&link);
 assert(!s_arenaSafe&&!s_arenaSpawn&&s_run.phase==Phase::Loading&&s_warmup==0);
 // Capture the Palace entrance once; walking never moves its respawn point.
 creating=false;link.id=s_player=7;s_map=3;clear_arena_geometry();link.room=arena().room;
 link.current.pos={10,0,20};link.shape_angle.y=1234;link.mLinkAcch.hit=true;link.mLinkAcch.floor=0;
 s_run.phase=Phase::Loading;landingRoom=arena().room;landingFloor=0;
 link.teleports=link.waits=0;cleared=interrupted=cancelled=0;
 update_arena_safety(&link);assert(s_arenaSpawn);
 link.current.pos={900,100,800};link.mLinkAcch.floor=100;landingFloor=100;update_arena_safety(&link);
 assert(s_arenaSpawnPosition.x==10&&s_arenaSafePosition.x==900);
 s_run.start(5);s_run.ticks=789;s_run.kills=50;
 // Native void handling must not apply a second cost or start a blocked event.
 BOOL restart=TRUE;assert(before_arena_restart(nullptr,moveArgs,&restart,nullptr)==HOOK_SKIP_ORIGINAL&&!restart);
 for(int gate=0;gate<5;++gate){
  active=gate!=0;transition=gate==1;s_arenaSafe=gate!=2;s_map=3;s_run.phase=gate==3?Phase::Ready:Phase::Running;
  link.id=gate==4?8:s_player;
  assert(before_arena_restart(nullptr,moveArgs,&restart,nullptr)==HOOK_CONTINUE);
 }
 active=s_arenaSafe=true;transition=false;s_map=3;s_run.phase=Phase::Running;link.id=s_player;
 // Ordinary jumps / air below the edge but above the cutoff are not falls.
 landingFloor=-1e9f;link.mLinkAcch.hit=false;link.current.pos.y=-390;update_arena_safety(&link);assert(!damageWrites);
 link.current.pos.y=-401;link.speed={5,-50,6};link.speedF=20;link.mNormalSpeed=20;
 link.mFallHeight=5000;update_arena_safety(&link);
 assert(life==8&&damageWrites==1&&link.teleports==1&&link.waits==1);
 assert(link.current.pos.x==10&&link.current.pos.y==0&&link.current.pos.z==20&&link.old.pos.x==10);
 assert(link.shape_angle.y==1234&&link.speed.y==0&&link.speedF==0&&link.mNormalSpeed==0&&link.mFallHeight==0);
 assert(link.mLastJumpPos.x==10&&link.field_0x33c8==0&&cleared==1&&interrupted==1&&cancelled==1);
 assert(s_run.ticks==789&&s_run.kills==50&&s_run.phase==Phase::Running);
 for(int frame=0;frame<60;++frame)update_arena_safety(&link);
 assert(damageWrites==1&&life==8); // No repeated charge at the destination.
 // Native void-floor codes cannot become the next safe position.
 link.mGroundCode=10;link.current.pos.y=-100;update_arena_safety(&link);
 assert(life==4&&damageWrites==2&&s_arenaSafePosition.y==0&&!link.mGroundCode);
 update_arena_safety(&link);assert(damageWrites==2);
 // The last heart is lethal; never underflow, refund or restart the run.
 life=3;link.current.pos.y=-501;link.mLinkAcch.hit=false;update_arena_safety(&link);
 assert(!life&&finished==1&&damageWrites==3);
 // Victory remains settled if Link falls while collecting the boss rewards.
 s_victory=true;life=1;link.current.pos.y=-501;link.mLinkAcch.hit=false;update_arena_safety(&link);
 assert(life==1&&finished==1&&damageWrites==4);s_victory=false;
 // Other arenas apply the same one-heart cost at their last safe floor.
 s_map=0;link.room=arena().room;life=12;link.current.pos.y=-501;update_arena_safety(&link);
 assert(life==8&&damageWrites==5);

 // High and low cliff jumps must keep falling toward valid lower floors on
 // every map, even with stale actor collision data. Preserve native velocity
 // and fall bookkeeping so landing/fall damage remain native.
 for(unsigned map=0;map<arenas.size();++map){
  s_map=map;clear_arena_geometry();link.room=landingRoom=arena().room;
  s_run.phase=Phase::Running;link.current.pos={10,2000,20};
  link.mLinkAcch.hit=true;link.mLinkAcch.floor=2000;link.mGroundCode=0;landingFloor=2000;
  update_arena_safety(&link);assert(s_arenaSafePosition.y==2000);
  const int writes=damageWrites;const auto teleports=link.teleports;
  landingFloor=-4000;link.mLinkAcch.hit=false;link.mLinkAcch.floor=-1.e9f;
  link.speed={3,-50,4};link.mFallHeight=2000;groundProbes=0;
  link.current.pos.y=1500;update_arena_safety(&link);
  assert(link.current.pos.y==1500&&!groundProbes); // No query for short drops.
  for(float y:{1499.f,0.f,-3990.f}){
   link.current.pos.y=y;update_arena_safety(&link);
   assert(link.current.pos.y==y&&link.speed.y==-50&&link.mFallHeight==2000);
   assert(s_arenaSafePosition.y==2000&&damageWrites==writes&&link.teleports==teleports);
  }
  link.current.pos.y=-4000;link.mLinkAcch.hit=true;link.mLinkAcch.floor=-4000;
  update_arena_safety(&link);
  assert(link.current.pos.y==-4000&&s_arenaSafePosition.y==-4000);
  assert(damageWrites==writes&&link.teleports==teleports);
  // Invalid landing surfaces must still recover. A valid floor seen on the
  // preceding frame cannot mask a later move out over the void.
  for(int bad=0;bad<8;++bad){
   link.current.pos={10,2000,20};link.mLinkAcch.hit=true;link.mLinkAcch.floor=2000;
   landingCode=0;landingRoom=arena().room;wet=false;landingFloor=2000;
   update_arena_safety(&link);landingFloor=0;
   link.mLinkAcch.hit=false;link.current.pos={100,1400,200};
   update_arena_safety(&link);assert(link.current.pos.y==1400);
   if(bad==0)landingFloor=-1.e9f;
   if(bad==1)landingFloor=NAN;
   if(bad>=2&&bad<=4)landingCode=bad==2?4:bad==3?5:10;
   if(bad==5)++landingRoom;
   if(bad==6){wet=true;waterY=50;}
   if(bad==7)landingFloor=1600; // A floor above Link cannot rescue a fall below it.
   life=12;const int before=damageWrites;
   update_arena_safety(&link);
   assert(link.current.pos.y==2000&&link.current.pos.x==10);
   assert(damageWrites==before+1&&life==8);
  }
  // Bridges are safe even when water exists below their floor.
  landingCode=0;landingRoom=arena().room;landingFloor=0;wet=true;waterY=-100;
  link.mLinkAcch.hit=false;link.current.pos={100,1400,200};
  update_arena_safety(&link);assert(link.current.pos.y==1400);
  wet=false;
 }
 // Gerudo-style pit: native contact lingers while Link steps over the lip.
 // Do not replace the inland checkpoint with an edge/unsupported position.
 for(unsigned map=0;map<arenas.size();++map){
  s_map=map;clear_arena_geometry();s_run.phase=Phase::Running;
  pit=true;landingFloor=0;landingCode=0;link.room=landingRoom=arena().room;
  link.current.pos={0,0,0};link.mLinkAcch.hit=true;link.mLinkAcch.floor=0;link.mGroundCode=0;
  update_arena_safety(&link);assert(s_arenaSafePosition.x==0);
  link.current.pos.x=80;update_arena_safety(&link);assert(s_arenaSafePosition.x==0);
  link.current.pos.x=110;update_arena_safety(&link);assert(s_arenaSafePosition.x==0);
  link.current.pos={200,-600,0};link.old.pos={190,-550,0};link.field_0x3798=link.current.pos;
  link.collisionPush={100,0,0};link.speed={20,-50,0};link.speedF=link.mNormalSpeed=30;
  link.mLastJumpPos={110,0,0};link.mFallHeight=link.field_0x33c8=900;link.falling=true;
  link.mLinkAcch.hit=false;link.mLinkAcch.floor=-1e9f;
  // Custom recovery owns void handling on every map; no blocked native event.
  restart=TRUE;
  assert(before_arena_restart(nullptr,moveArgs,&restart,nullptr)==HOOK_SKIP_ORIGINAL&&!restart);
  const auto teleports=link.teleports;const int writes=damageWrites;life=12;
  update_arena_safety(&link);
  assert(link.teleports==teleports+1&&link.current.pos.x==0&&link.current.pos.y==0);
  assert(link.old.pos.x==0&&link.field_0x3798.x==0&&link.field_0x3798.y==0);
  assert(link.collisionPush.x==0&&link.speed.y==0&&link.speed.x==0&&link.speedF==0&&link.mNormalSpeed==0);
  assert(!link.falling&&link.mLastJumpPos.x==0&&link.mFallHeight==0&&link.field_0x33c8==0);
  assert(link.mLinkAcch.ChkGroundHit()&&link.mLinkAcch.GetGroundH()==0&&link.mGroundCode==0);
  for(int frame=0;frame<60;++frame){
   // Residual motion would put Link back across the lip on the next frame.
   link.current.pos.x+=link.speed.x+link.speedF+link.collisionPush.x;
   link.current.pos.y+=link.speed.y;
   update_arena_safety(&link);
  }
  assert(link.teleports==teleports+1&&link.current.pos.x==0&&link.current.pos.y==0);
  assert(damageWrites==writes+1&&life==8);
  // A native void polygon also recovers before reaching the height threshold.
  link.current.pos={200,-100,0};link.mLinkAcch.hit=true;link.mGroundCode=10;
  update_arena_safety(&link);
  assert(link.teleports==teleports+2&&link.current.pos.x==0&&link.mGroundCode==0);
  assert(damageWrites==writes+2&&life==4);
  // Correcting a room boundary alone must not charge another heart.
  ++link.room;update_arena_safety(&link);
  assert(link.room==arena().room&&damageWrites==writes+2&&life==4);
  // Less than one heart is lethal on every map, without underflow.
  life=3;const int defeats=finished;
  link.current.pos={200,-600,0};link.mLinkAcch.hit=false;
  update_arena_safety(&link);
  assert(life==0&&finished==defeats+1&&damageWrites==writes+3);
 }
 pit=false;
 clear_arena_geometry();assert(!s_arenaSpawn&&!s_arenaSafe);
 std::cout<<"Survivors spawn: native entry, barrier timing, grounding, recovery and actor replacement passed\n";
}
'''
# Exercise the real floor policy together with the rescue hooks.
ground = (root/'src/survivors_ground.inc').read_text()
dry_floor = ground[:ground.index('struct ArenaWalk')]
functions = dry_floor+'\n'+'\n'.join(extract(n) for n in ['add_exit_box','arena_story_actor','before_arena_stage_actor',
    'confine_arena_actor','after_arena_position','before_arena_restart','arena_grounded','arena_fallen','arena_safe_footprint','restore_arena_player','update_arena_safety'])
globals = source[source.index('std::vector<barriers::Wall>'):source.index('void add_exit_box')]
with tempfile.TemporaryDirectory() as tmp:
    cpp,binary=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture+'enum{'+','.join(names)+'};\n'+globals+functions+loading+
        'bool native_entrance_demo(int start_mode){return '+entry_condition+';}\n'+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
