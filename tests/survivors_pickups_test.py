"""Native pickup magnet, scheduled-elite EXP crystal and XP snapshot/lifecycle regression."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
start=core.index('void update_orbs(')
orbs=core[start:core.index('\nbool create_enemy_at',start)]
# Validate the actual SDK resource mapping, not a stubbed item ID: the old
# SMALL_KEY2 has no field archive, while the chosen alias shares the normal key.
sdk=root/'dusklight'
data=(sdk/'src/d/d_item_data.cpp').read_text()
data=re.sub(r'#if TARGET_PC\n(.*?)#else\n.*?#endif',lambda m:m[1],data,flags=re.S)
table=re.search(r'field_item_res\[\]\s*=\s*\{(.*?)\n\};',data,re.S)[1]
rows=re.findall(r'\{([^{}]+)\}',table)
assert rows[0xee].lstrip().startswith('NULL')
assert rows[0xfe]==rows[0x20] and '"T_g_key"' in rows[0xfe]
native=(sdk/'src/d/actor/d_a_obj_smallkey.cpp').read_text()
start=native.index('void daKey_c::setBaseMtx()')
native=native[start:native.index('\n}',start)+2].replace('daKey_c::setBaseMtx','daItem_c::nativeKeyMatrix')
fixture=r'''
void draw_victory_wave(){}

#include <any>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include "survivors_rules.hpp"
#include "actor_slot_index.hpp"
using dawnlight::ActorSlotIndex;
using namespace dawnlight::survivors;
using s16=short;using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}void operator+=(cXyz b){x+=b.x;y+=b.y;z+=b.z;}
 void operator*=(float f){x*=f;y*=f;z*=f;}
 float abs()const{return std::sqrt(x*x+y*y+z*z);}
 float abs2(cXyz p)const{auto d=*this-p;return d.x*d.x+d.y*d.y+d.z*d.z;}};
struct fopAc_ac_c{ActorId id=0;struct{cXyz pos;}current,old;cXyz speed;float speedF=0;};
using leafdraw_class=fopAc_ac_c;
constexpr int fpcNm_ITEM_e=9;int profile=fpcNm_ITEM_e;int fopAcM_GetName(fopAc_ac_c*){return profile;}
struct Angles{int x=0,y=0,z=0;};
namespace mDoMtx_stack_c{
 std::vector<float> matrix;
 void transS(cXyz p){matrix={p.x,p.y,p.z};}
 void transM(float x,float y,float z){matrix.insert(matrix.end(),{x,y,z});}
 void ZXYrotM(Angles a){matrix.insert(matrix.end(),{float(a.z),float(a.x),float(a.y)});}
 void XrotM(int x){matrix.push_back(float(x));}
 const auto& get(){return matrix;}
}
struct Model{std::vector<float> matrix;void setBaseTRMtx(const std::vector<float>& value){matrix=value;}} keyModel;
struct daAlink_c:fopAc_ac_c{}player;ActorId s_player=1;
auto* daAlink_getAlinkActorClass(){return &player;}ActorId fopAcM_GetID(fopAc_ac_c* p){return p->id;}
struct daItem_c:fopAc_ac_c{
 Model* mpModel=nullptr;Angles shape_angle{};
 void nativeKeyMatrix();bool chkDraw(){return !hidden;}
 enum{TYPE_FIXED_PLACE_e=1,TYPE_LAUNCH_e=0,STATUS_NORMAL_e=0,FLAG_NO_TIMER_e=8};
 cXyz mItemScale{1,1,1},scale{1,1,1};int status=0,mWaitTimer=0;bool hidden=false;int flags=0;int getStatus(){return status;}
 struct Cylinder{float r=0,h=0;void SetR(float n){r=n;}void SetH(float n){h=n;}}mCcCyl;
 void setFlag(int f){flags|=f;}void hide(){hidden=true;}};
struct daObjLife_c:daItem_c{enum{STATUS_WAIT_e=0};bool chkStatus(int s){return status==s;}};
struct Enemy{bool elite=false;int eliteSlot=-1;cXyz lootPosition;};
struct Orb{cXyz position;unsigned value=0;bool summoned=false,merged=false;};std::array<Orb,orb_capacity>s_orbs;
Run s_run;bool s_allowDrop=false,playing=true,failCreate=false,dry=true,path=true;
bool playable(){return playing;}
struct Arena{int room=6;}data;const Arena& arena(){return data;}
enum{dItemNo_RECOVERY_FAILY_e=40,dItemNo_HORSE_FLUTE_e=132,dItemNo_KAKERA_HEART_e=33,dItemNo_HEART_e=0,dItemNo_SILVER_RUPEE_e=7,dItemNo_KEY_OF_FILONE_e=0xfe,dItemNo_SMALL_KEY_e=0x20};
struct dItem_data{static float getR(int item){assert(item==dItemNo_SMALL_KEY_e);return 30;}static float getH(int item){assert(item==dItemNo_SMALL_KEY_e);return 40;}};
struct ModContext{};ModContext* mod_ctx=nullptr;enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int i){return std::any_cast<T>(static_cast<std::any*>(args)[i]);}}
std::map<ActorId,daObjLife_c>actors;std::set<ActorId>pending;unsigned nextId=2,creates=0;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:&it->second;}
bool fpcM_IsCreating(ActorId id){return pending.count(id);}
ActorId fopAcM_createItem(const cXyz* pos,int item,int bit,int room,void*,void*,int type){
 assert(s_allowDrop&&bit==-1&&room==6);assert(type==0);(void)item;++creates;
 if(failCreate)return kNone;
 const auto id=nextId++;actors[id].id=id;actors[id].current.pos=*pos;actors[id].mpModel=&keyModel;return id;
}
struct Service{void delete_actor(ModContext*,ActorId id){actors.erase(id);}}service;auto* svc_actor=&service;
bool loot_floor_position(cXyz& p){if(!dry)return false;p.y=0;return true;}
bool dry_arena_floor(cXyz& pos,float,float){pos.y=0;return dry;}
bool walkable_spawn_path(cXyz,cXyz& pos,float){pos.y=0;return path;}
float cM_ssin(s16 a){return std::sin(float(a)*3.14159265f/32768);}float cM_scos(s16 a){return std::cos(float(a)*3.14159265f/32768);}
struct dBgS_ObjGndChk{void SetPos(cXyz*){}};
struct Bg{float GroundCross(dBgS_ObjGndChk*){return 0;}}bg;Bg& dComIfG_Bgsp(){return bg;}
bool activeArena=true,changingStage=false;bool in_arena(){return activeArena;}bool transitioning(){return changingStage;}
struct csXyz{csXyz(int,int,int){}};struct GXColor{unsigned char r,g,b,a;};
std::vector<GXColor>colors;std::vector<cXyz>sizes;
void dDbVw_drawCubeOpa(cXyz&,cXyz size,csXyz,GXColor color){colors.push_back(color);sizes.push_back(size);}
void draw_lightning(){}
void draw_loot(){}void draw_ranged(){}void draw_arena_barriers(){}
struct Chest{bool free=false,fixed=false;};

using u8=unsigned char;
int nativeKeys=0,pendingKeys=0;
int dComIfGs_getKeyNum(){return nativeKeys;}
int dComIfGp_getItemKeyNumCount(){return pendingKeys;}
void dComIfGp_clearItemKeyNumCount(){pendingKeys=0;}
void dComIfGs_setKeyNum(u8 n){nativeKeys=n;}
#include "survivors_keys.inc"
unsigned keySounds=0;constexpr int Z2SE_CONSUMP_ITEM_GET=1;
void mDoAud_seStart(int sound,void*,int,int){assert(sound==Z2SE_CONSUMP_ITEM_GET);++keySounds;}
'''
main=r'''
void fresh(){nativeKeys=pendingKeys=0;keySounds=0;activeArena=true;changingStage=false;profile=fpcNm_ITEM_e;keyModel.matrix.clear();s_run.start(123);s_orbs={};s_pickups.clear();s_expMagnets.assign(timed_elite_count,{});actors.clear();pending.clear();
 player={};player.id=1;playing=dry=path=true;failCreate=false;s_allowDrop=false;creates=0;}
int main(){
 fresh();
 const auto keyId=create_run_pickup({100,0,0},dItemNo_KEY_OF_FILONE_e);
 auto* keyActor=static_cast<daItem_c*>(actor_by_id(keyId));auto* key=tracked_pickup(keyId);
 move_pickup(*key,keyActor);assert(keyActor->flags&daItem_c::FLAG_NO_TIMER_e);
 assert(keyActor->current.pos.x<100&&keyActor->mWaitTimer>0&&keyActor->mCcCyl.r==30&&keyActor->mCcCyl.h==40);
 // Match the real native small-key transform at arbitrary positions/rotations.
 keyActor->shape_angle={1200,2000,3000};keyActor->nativeKeyMatrix();const auto expectedMatrix=keyModel.matrix;
 const auto beforePos=keyActor->current.pos;const auto beforeScale=keyActor->scale;
 std::any drawArgs[]={static_cast<leafdraw_class*>(keyActor)};keyModel.matrix.clear();
 assert(before_key_draw(nullptr,drawArgs,nullptr,nullptr)==HOOK_CONTINUE&&keyModel.matrix==expectedMatrix);
 assert(keyActor->current.pos.abs2(beforePos)==0&&keyActor->scale.abs2(beforeScale)==0);
 for(unsigned reason=0;reason<7;++reason){
  keyModel.matrix.clear();key->retiring=reason==0;keyActor->hidden=reason==1;
  keyActor->mpModel=reason==2?nullptr:&keyModel;activeArena=reason!=3;changingStage=reason==4;
  key->item=reason==5?1:dItemNo_KEY_OF_FILONE_e;profile=reason==6?0:fpcNm_ITEM_e;
  before_key_draw(nullptr,drawArgs,nullptr,nullptr);assert(keyModel.matrix.empty());
 }
 key->retiring=keyActor->hidden=false;keyActor->mpModel=&keyModel;activeArena=true;changingStage=false;
 key->item=dItemNo_KEY_OF_FILONE_e;profile=fpcNm_ITEM_e;
 std::any keyArgs[]={keyActor};playing=false;
 assert(before_key_pickup(nullptr,keyArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&chest_keys()==0&&!key->retiring);
 playing=true;pendingKeys=1;
 assert(before_key_pickup(nullptr,keyArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&chest_keys()==2&&pendingKeys==0);
 assert(key->retiring&&keyActor->hidden&&keySounds==1);
 before_key_pickup(nullptr,keyArgs,nullptr,nullptr);assert(chest_keys()==2&&keySounds==1);
 cleanup_pickups();cleanup_pickups();assert(!actor_by_id(keyId)&&!tracked_pickup(keyId));
 const auto unrelatedId=create_run_pickup({0,0,0},1);
 std::any otherArgs[]={static_cast<daItem_c*>(actor_by_id(unrelatedId))};
 assert(before_key_pickup(nullptr,otherArgs,nullptr,nullptr)==HOOK_CONTINUE);
 fresh();
 for(unsigned i=0;i<orb_capacity;++i)add_orb({float(i*1000),0,0},i==0?20:1);
 assert(!s_orbs[0].merged); // A naturally larger reward is not a merged crystal.
 add_orb({2,0,0},7);assert(s_orbs[0].value==27&&s_orbs[0].merged);
 after_draw(nullptr,nullptr,nullptr,nullptr);assert(colors.size()==orb_capacity);
 assert(colors[0].r==255&&colors[0].g==70&&colors[1].r==90&&colors[1].g==235);
 player.current.pos=s_orbs[0].position;update_orbs(&player);assert(s_run.xp==27&&!s_orbs[0].merged);
 add_orb({0,0,0},1);assert(!s_orbs[0].merged&&s_orbs[0].value==1);
 for(int item:{0,1,2,3,4,5,6,7,33}){
  fresh();auto id=create_run_pickup({300,20,0},item);auto* p=tracked_pickup(id);auto* actor=actor_by_id(id);
  assert(p&&!s_allowDrop);move_pickup(*p,actor);assert(actor->current.pos.x==300);
  s_run.ranks[unsigned(Upgrade::Magnet)]=5;move_pickup(*p,actor);assert(actor->current.pos.x==275);
  playing=false;move_pickup(*p,actor);assert(actor->current.pos.x==275);playing=true;
  actors[id].status=2;move_pickup(*p,actor);assert(actor->current.pos.x==275); // Native Get Item must finish undisturbed.
  assert(!pickup_waiting(*p,actor));actors[id].status=0;assert(pickup_waiting(*p,actor));
 }
 fresh();s_allowDrop=true;create_run_pickup({},0);assert(s_allowDrop);s_allowDrop=false;
 fresh();Enemy guard{true,int(timed_elite_count),{}};queue_exp_magnet(guard,&player);update_exp_magnets(&player);
 for(const auto& d:s_expMagnets)assert(!d.active&&!d.pending);
 Enemy timed{true,0,{1000,0,0}};player.current.pos={1000,0,0};queue_exp_magnet(timed,&player);queue_exp_magnet(timed,&player);
 path=false;update_exp_magnets(&player);assert(s_expMagnets[0].pending&&!s_expMagnets[0].active);
 path=true;update_exp_magnets(&player);assert(!s_expMagnets[0].pending&&s_expMagnets[0].active&&creates==0);
 const auto pos=s_expMagnets[0].position;update_exp_magnets(&player);
 colors.clear();sizes.clear();after_draw(nullptr,nullptr,nullptr,nullptr);
 assert(colors.size()==1&&colors[0].r==255&&colors[0].g==210&&colors[0].b==45&&sizes[0].x==25&&sizes[0].y==40&&sizes[0].z==25);
 assert(std::abs((pos-player.current.pos).abs()-167.6f)<2);
 s_orbs[0]={{50000,30,0},8,false};s_orbs[1]={{-50000,30,0},9,false};
 player.current.pos=pos;playing=false;update_exp_magnets(&player);assert(s_expMagnets[0].active&&!s_orbs[0].summoned);
 playing=true;update_exp_magnets(&player);assert(!s_expMagnets[0].active&&s_expMagnets[0].collected&&s_orbs[0].summoned&&s_orbs[1].summoned);
 s_orbs[2]={{40000,30,0},10,false}; // Future XP is not part of the one-time sweep.
 for(int i=0;i<200;++i)update_orbs(&player);
 assert(s_run.xp==17&&s_orbs[2].value==10&&!s_orbs[2].summoned);
 queue_exp_magnet(timed,&player);update_exp_magnets(&player);assert(!s_expMagnets[0].active&&creates==0);
 // Normal drops grow beyond the four reserved boss slots, then reuse collected slots.
 fresh();Enemy normal{};
 for(unsigned i=0;i<12;++i)queue_exp_magnet(normal,&player);
 assert(s_expMagnets.size()==timed_elite_count+12);
 for(unsigned i=0;i<timed_elite_count;++i)assert(!s_expMagnets[i].pending);
 s_expMagnets[timed_elite_count].collected=true;
 queue_exp_magnet(normal,&player);assert(s_expMagnets.size()==timed_elite_count+12);
 assert(s_expMagnets[timed_elite_count].pending&&!s_expMagnets[timed_elite_count].collected);
 // All rupee colours and hearts persist and join the global sweep, including pending creates.
 fresh();
 for(int item=0;item<=7;++item) {
  const auto id=create_run_pickup({50000,20,0},item);auto& actor=actors[id];auto* pickup=tracked_pickup(id);
  for(unsigned tick=0;tick<60*60*30;++tick) {
   move_pickup(*pickup,&actor);
   if(!(actor.flags&daItem_c::FLAG_NO_TIMER_e))--actor.mWaitTimer;
   assert(actor.mWaitTimer>0); // Native expiry/flashing cannot begin.
  }
 }
 const auto piece=create_run_pickup({50000,20,0},33);
 const auto pendingDrop=create_run_pickup({50000,20,0},0);actors.erase(pendingDrop);pending.insert(pendingDrop);
 const auto retiring=create_run_pickup({50000,20,0},1);tracked_pickup(retiring)->retiring=true;
 queue_exp_magnet(normal,&player);update_exp_magnets(&player);
 player.current.pos=s_expMagnets[timed_elite_count].position;
 update_exp_magnets(&player);
 for(const auto& p:s_pickups)assert(p.summoned==(!p.retiring&&p.item<=7));
 assert(!(actors[piece].flags&daItem_c::FLAG_NO_TIMER_e));
 auto late=create_run_pickup({50000,20,0},0);assert(!tracked_pickup(late)->summoned);
 auto* pickup=tracked_pickup(s_pickups.front().id);auto* actor=actor_by_id(pickup->id);
 const auto start=actor->current.pos.x;s_run.phase=Phase::Upgrade;move_pickup(*pickup,actor);assert(actor->current.pos.x==start);
 s_run.phase=Phase::Running;actors[pickup->id].status=2;move_pickup(*pickup,actor);assert(actor->current.pos.x==start);
 actors[pickup->id].status=0;
 for(unsigned tick=0;tick<300;++tick)move_pickup(*pickup,actor);
 assert((actor->current.pos-(player.current.pos+cXyz(0,20,0))).abs()<1);
 pending.erase(pendingDrop);actors[pendingDrop].id=pendingDrop;actors[pendingDrop].current.pos={50000,20,0};
 move_pickup(*tracked_pickup(pendingDrop),&actors[pendingDrop]);assert(actors[pendingDrop].current.pos.x<50000);
 retire_pickups();assert(s_expMagnets.size()==timed_elite_count);
 for(const auto& drop:s_expMagnets)assert(!drop.pending&&!drop.active);
 // Pending native actor creation is retained until it can be deleted, never rewarded.
 fresh();auto pendingId=create_run_pickup({},132);actors.erase(pendingId);pending.insert(pendingId);
 retire_pickups();cleanup_pickups();assert(tracked_pickup(pendingId));
 pending.erase(pendingId);actors[pendingId].id=pendingId;cleanup_pickups();cleanup_pickups();assert(s_pickups.empty());
 // Actual one-XP orb collection retains Greed's half-unit bonus across pickups.
 fresh();s_run.apply_fate(FateEffect::Greed);s_run.xp=0;
 for(unsigned n=0;n<2;++n){s_orbs[0].position=player.current.pos+cXyz(0,30,0);s_orbs[0].value=1;update_orbs(&player);}
 assert(s_run.xp==3&&!s_orbs[0].value);
 // Fairy is a separate native actor: never cast/magnet-pull it as daItem_c.
 fresh();const auto fairyId=create_run_pickup({100,20,0},dItemNo_RECOVERY_FAILY_e);
 auto* fairy=tracked_pickup(fairyId);fopAc_ac_c fairyActor;fairyActor.current.pos={100,20,0};
 assert(pickup_waiting(*fairy,&fairyActor));move_pickup(*fairy,&fairyActor);
 assert(fairyActor.current.pos.x==100&&fairyActor.current.pos.y==20);
 retire_pickups();cleanup_pickups();cleanup_pickups();assert(!tracked_pickup(fairyId));
 // At Endless 40:00, no new hearts/pieces and no leftover healing pickups.
 fresh();const auto heart=create_run_pickup({},dItemNo_HEART_e);
 const auto latePiece=create_run_pickup({},dItemNo_KAKERA_HEART_e);
 s_run.endless=true;s_run.ticks=40*60*ticks_per_second;
 assert(create_run_pickup({},dItemNo_HEART_e)==kNone&&create_run_pickup({},dItemNo_KAKERA_HEART_e)==kNone);
 move_pickup(*tracked_pickup(heart),&actors[heart]);move_pickup(*tracked_pickup(latePiece),&actors[latePiece]);
 assert(tracked_pickup(heart)->retiring&&tracked_pickup(latePiece)->retiring);
 assert(create_run_pickup({},1)!=kNone);
 // Retiring the run removes pending/active drops without granting XP.
 fresh();queue_exp_magnet(timed,&player);retire_pickups();update_exp_magnets(&player);
 assert(!s_expMagnets[0].active&&!s_expMagnets[0].pending&&s_run.xp==0);
 std::cout<<"Survivors pickups: native magnet, EXP crystal scoping, XP snapshot, pause and deferred cleanup passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+native+(root/'src/survivors_pickups.inc').read_text()+(root/'src/survivors_key_visual.inc').read_text()+core[core.index('void add_orb('):core.index('void drop_reward(')]+orbs+core[core.index('void after_draw('):core.index('void label(')]+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
