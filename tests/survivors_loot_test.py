"""Exercise production chest purchases, placement and item trajectories with engine boundaries."""
from pathlib import Path
import re
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root/'src/survivors_loot.inc').read_text()


def extract(name, source=source):
    match = re.search(r'^(?:bool|void) ' + name + r'\(', source, re.M)
    assert match, name
    start = match.start() + re.search(r'\)\s*\{', source[match.start():]).end() - 1
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[match.start():end + 1]
    raise AssertionError(name)


stubs = r'''
#include <cassert>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "survivors_rules.hpp"
#include "survivors_projectiles.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;constexpr ActorId kNone=~0u;
unsigned short bow_fire_effect(){return 0x1234;}
using s16=short;using u16=unsigned short;using u8=unsigned char;
struct ModContext {};
struct cXyz {
    float x=0,y=0,z=0;
    cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
    cXyz operator+(cXyz p)const{return {x+p.x,y+p.y,z+p.z};}
    cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}
    cXyz operator*(float n)const{return {x*n,y*n,z*n};}
    float abs2()const{return x*x+y*y+z*z;}float abs()const{return std::sqrt(abs2());}
    float abs(cXyz p)const{return (*this-p).abs();}
    float absXZ(cXyz p)const{return std::hypot(x-p.x,z-p.z);}
};
struct csXyz{csXyz(int,int,int){}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
float cM_ssin(s16 angle){return std::sin(angle*3.14159265f/32768);}
float cM_scos(s16 angle){return std::cos(angle*3.14159265f/32768);}
struct Actor {unsigned id=1;int health=10;struct{cXyz pos;}current;cXyz eyePos;};
void mDoMtx_multVecZero(const cXyz* matrix,cXyz* out){*out=*matrix;}
struct daAlink_c:Actor {cXyz hand{0,70,0};const cXyz* getLeftItemMatrix(){return &hand;}int tevStr=0;struct{short y=0;}shape_angle;}player;
unsigned fopAcM_GetID(Actor* a){return a->id;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
std::unordered_map<unsigned,Actor*>actors;
Actor* actor_by_id(unsigned id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
struct Enemy {unsigned id=0;bool initialized=false,credited=false,retiring=false;};
std::array<Enemy,enemy_capacity>s_enemies;
struct dBgS_LinChk {cXyz from,to;void Set(const cXyz* f,const cXyz* t,daAlink_c*){from=*f;to=*t;}};
struct dBgS_ObjGndChk {cXyz pos;void SetPos(cXyz* p){pos=*p;}};
bool blocked=false,invalidFloor=false,blockPositiveX=false,arenaBlocked=false;
int floorRoom=51;
struct Arena { int room=51; } selectedArena;
const Arena& arena(){return selectedArena;}
bool arena_path_clear(const cXyz&,const cXyz&,float){return !arenaBlocked;}
struct Background {
    int GetRoomId(const dBgS_ObjGndChk&){return floorRoom;}
    bool LineCross(dBgS_LinChk* wall){return blocked || (blockPositiveX && wall->to.x>0);}
    float GroundCross(dBgS_ObjGndChk*){return invalidFloor?-1.0e9f:0;}
}background;
Background& dComIfG_Bgsp(){return background;}
enum{AT_TYPE_NORMAL_SWORD,AT_TYPE_ARROW,AT_TYPE_BOMB,AT_TYPE_IRON_BALL,dCcD_MTRL_NONE};
struct Status {Actor* owner=nullptr;void Init(int,int,Actor* actor){owner=actor;}};
struct Collider {
    bool active=false,hit=false,shield=false;unsigned type=0,power=0,resets=0;float radius=0;
    cXyz pos,velocity;Status* status=nullptr;
    void Set(int){}void SetStts(Status* s){status=s;}void SetAtType(unsigned t){type=t;}
    unsigned GetAtAtp(){return power;}
    int material=0;void SetAtAtp(unsigned p){power=p;}void SetAtMtrl(int m){material=m;}void OnAtNoConHit(){}
    void ResetAtHit(){hit=false;shield=false;++resets;}void OnAtSetBit(){active=true;}void OffAtSetBit(){active=false;}
    bool ChkAtHit(){return hit;}bool ChkAtShieldHit(){return shield;}
    void SetC(cXyz p){pos=p;}void SetR(float r){radius=r;}void SetAtVec(cXyz v){velocity=v;}
};
struct Attack {bool chainSpin=false;unsigned chainSpinTick=0;float chainSpinRadius=0,chainSpinAngle=0;bool manualFlight=false,aimPending=false;float aimSpread=0;
    unsigned bowVolleySlot=20;uint64_t bowVolley=0;ActorId target=kNone;unsigned hops=0,visitedCount=0,ballVictimCount=0,arrowTargets=1;std::array<ActorId,enemy_capacity>ballVictims{};std::array<ActorId,13>visited{};
    bool bombArrow=false,hurricane=false,detonated=false;bool active=false;unsigned life=0,weapon=0,age=0;bool returning=false,exploding=false;
    float travel=0,limit=0;cXyz position,velocity;float radius=0;Status status;Collider collider;
    std::array<unsigned,9>emitters{};
};
constexpr int kAttackSource=0;constexpr unsigned kPaidChestCount=6,kSmallChestCount=6+map_chest_capacity,kChestCount=kSmallChestCount+elite_reward_capacity;
std::deque<Attack>s_attacks(initial_projectile_slots);
bool prepare_projectile_model(Attack&, unsigned){return true;}
bool prepare_bomb_arrow_model(Attack&){return true;}
void stop_attack(Attack& a){a.active=false;a.collider.OffAtSetBit();a.emitters={};}
unsigned cleared=0,closed=0;
void clear_attacks(){++cleared;for(auto& a:s_attacks)stop_attack(a);}
void close_window(){++closed;}
struct Server {unsigned registrations=0;void Set(Collider* c){assert(c->active && c->status->owner==&player);++registrations;}}server;
Server* dComIfG_Ccsp(){return &server;}
std::array<unsigned,4>queued{};
bool queue_ranged(daAlink_c*,Upgrade w){++queued[unsigned(w)-8];return true;}
unsigned particles=0;
std::vector<float>effectScales;
unsigned dComIfGp_particle_set(unsigned,u16,const cXyz*,int*,const csXyz*,const cXyz* scale,int,void*,int,void*,void*,void*){
 if(scale){effectScales.push_back(scale->x);}return ++particles;}
'''
# Use the production state definition; no duplicate fixture version of its flags.
stubs += source[source.index('struct Chest {'):source.index('struct LootModels {')]
stubs += r'''
Run s_run;
std::array<unsigned,12>s_cooldowns{};
struct {bool ready=true;}s_loot;
int potion_slot(){return -1;}
bool prepare_loot(daAlink_c*){return s_loot.ready;}
bool sword_action_available(daAlink_c*){return true;}
bool arenaPresent=true,transition=false;
unsigned s_player=1,rupees=0;
bool s_arenaMeshReady=true;unsigned s_map=0,placement=0;
float s_eliteFloorArea=400000000;
struct EliteFloor{cXyz a,b,c;float cumulative;};
std::vector<EliteFloor>s_eliteFloors{
 {{-10000,0,-10000},{10000,0,-10000},{-10000,0,10000},200000000},
 {{10000,0,10000},{-10000,0,10000},{10000,0,-10000},400000000}};
bool walkable_spawn_path(cXyz,cXyz&,float){return !blocked&&!invalidFloor&&!arenaBlocked;}
bool distributionReady=true;bool arena_distribution_ready(){return distributionReady;}
float arena_distribution_area(){return s_eliteFloorArea;}
bool needs_reachable_floor(){return false;}bool sample_reachable_arena_floor(cXyz&){return false;}
bool reachable=true;bool reachable_arena_floor(cXyz){return reachable;}
bool elite_floor_position(cXyz&){++placement;return !invalidFloor;}
bool in_arena(){return arenaPresent;}bool transitioning(){return transition;}
unsigned dComIfGs_getRupee(){return rupees;}void dComIfGs_setRupee(unsigned r){rupees=r;}
'''
main = r'''
void reset(){
    s_run.start(123);s_attacks=std::deque<Attack>(initial_projectile_slots);s_cooldowns={};s_chests={};s_chestsExhausted=false;s_chestSequence=0;s_mapChestPlaced={};s_loot.ready=true;s_map=placement=0;s_arenaMeshReady=true;
    player={};player.current.pos={};player.id=s_player=1;s_selectedChest=kChestCount;
    arenaPresent=true;transition=blocked=invalidFloor=blockPositiveX=false;server.registrations=0;
}
int main(){
    reset();s_run.ranks[unsigned(Upgrade::Bow)]=1;
    assert(launch_item(&player,Upgrade::Bow,{0,100,500},.1f,0,1,kNone,true));
    assert(s_attacks[8].aimPending&&s_attacks[8].aimSpread==.1f&&s_attacks[8].limit==s_run.item_range(Upgrade::Bow));

    // Spawn at the current native hand matrix, independent of world height/facing.
    for(auto hand:{cXyz(28,146,31),cXyz(-71,340,99),cXyz(15,-40,-200)}) {
        reset();player.current.pos={50,10,50};player.hand=hand;s_run.ranks[8]=5;
        const cXyz target=hand+cXyz(0,0,400);
        assert(launch_item(&player,Upgrade::Bow,target));
        assert(launch_item(&player,Upgrade::Bow,target,0.14f));
        assert(launch_item(&player,Upgrade::Bow,target,-0.14f));
        for(unsigned i=8;i<11;++i) {
            assert(s_attacks[i].active&&(s_attacks[i].position-hand).abs()<0.001f);
            assert(std::abs(s_attacks[i].velocity.y)<0.001f);
        }
    }
    reset();Actor near,far,occluded,dead;near.id=2;near.eyePos={0,70,150};far.id=3;far.eyePos={0,70,600};
    occluded.id=4;occluded.eyePos={20,70,0};dead.id=5;dead.eyePos={0,70,10};dead.health=0;
    actors={{2,&near},{3,&far},{4,&occluded},{5,&dead}};
    s_enemies[0]={2,true,false,false};s_enemies[1]={3,true,false,false};
    s_enemies[2]={4,true,false,false};s_enemies[3]={5,true,false,false};
    blockPositiveX=true;cXyz target;
    assert(nearest_item_target(&player,target) && target.z==150);
    s_enemies[0].credited=true;assert(nearest_item_target(&player,target) && target.z==600);
    s_enemies[1].retiring=true;assert(!nearest_item_target(&player,target));
    s_enemies[0].credited=false;s_enemies[1].retiring=false;
    fire_items(&player);for(auto& a:s_attacks)assert(!a.active); // Locked items cannot fire.
    for(auto weapon:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball})s_run.ranks[unsigned(weapon)]=1;
    fire_items(&player);
    assert(queued[0]==1&&queued[1]==1&&queued[2]==1&&queued[3]==1);
    assert(launch_item(&player,Upgrade::Bombs,{0,70,300}));
    assert(launch_item(&player,Upgrade::Ball,{0,70,300}));
    for(unsigned type=2;type<4;++type){
        assert(s_attacks[item_projectile_begin(type)].active);
        assert(s_attacks[item_projectile_begin(type)].status.owner==&player);
    }
    assert(s_attacks[36].collider.type==AT_TYPE_NORMAL_SWORD &&
           s_attacks[32].collider.type==AT_TYPE_NORMAL_SWORD);
    fire_items(&player);assert(!s_attacks[9].active); // Cooldowns prevent every-frame firing.
    for(unsigned i=0;i<8;++i)assert(!s_attacks[i].active); // Separate base-weapon budget.
    reset();s_run.ranks[8]=5;
    assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    s_attacks[8].collider.hit=true;update_item_attack(s_attacks[8],&player);assert(!s_attacks[8].active);
    blocked=true;update_item_attack(s_attacks[9],&player);assert(!s_attacks[9].active);
    reset();s_run.ranks[10]=1;assert(launch_item(&player,Upgrade::Bombs,{0,70,600}));
    auto& bomb=s_attacks[32];
    for(unsigned i=0;i<29;++i){update_item_attack(bomb,&player);assert(!bomb.exploding && server.registrations==0);}
    update_item_attack(bomb,&player);
    assert(bomb.exploding && std::abs(bomb.position.z-600)<0.01f && std::abs(bomb.position.y-70)<0.01f);
    assert(bomb.collider.radius==182 && server.registrations==1 && particles==9);
    for(unsigned i=0;i<61;++i)update_item_attack(bomb,&player);
    assert(!bomb.active);
    reset();s_run.ranks[10]=5;assert(launch_item(&player,Upgrade::Bombs,{0,70,600}));blocked=true;
    update_item_attack(s_attacks[32],&player);assert(s_attacks[32].exploding && s_attacks[32].radius==230);
    for(auto weapon:{Upgrade::Boomerang,Upgrade::Ball}){
        reset();s_run.ranks[unsigned(weapon)]=1;assert(launch_item(&player,weapon,{0,70,400}));
        auto& a=s_attacks[item_projectile_begin(unsigned(weapon)-unsigned(Upgrade::Bow))];
        while(!a.returning && a.active)update_item_attack(a,&player);
        assert(a.returning && a.active);
        player.current.pos.x=100;update_item_attack(a,&player);assert(a.velocity.x>0 && a.velocity.z<0);
        for(unsigned i=0;i<80 && a.active;++i)update_item_attack(a,&player);
        assert(!a.active);
    }

    // Chain Spin retracts throughout one complete moving-center revolution.
    // Snapshot the launch ability so gear swaps cannot cut it short.
    for(bool fromGear:{false,true})for(bool manual:{false,true}) {
        reset();s_run.ranks[11]=1;
        if(fromGear)s_run.gear_weapon={GearKind::IronBall,5,5,5,GearPerk::ChainSpin};else s_run.chain_spin=1;
        assert(launch_item(&player,Upgrade::Ball,{0,manual?270.f:70.f,400},0,20,0,kNone,manual));auto& a=s_attacks[36];assert(a.chainSpin);
        s_run.chain_spin=0;s_run.gear_weapon={};
        for(unsigned ticks=0;a.travel<a.limit&&ticks<200;++ticks)update_item_attack(a,&player);
        assert(a.active&&!a.returning&&a.chainSpinTick==0);
        const auto start=a.position-player.current.pos;const auto radius=std::sqrt(start.x*start.x+start.z*start.z);
        a.ballVictimCount=3;
        for(unsigned tick=1;tick<=60;++tick) {
            if(tick>1)player.current.pos.x+=1;
            update_item_attack(a,&player);
            assert(a.active&&a.returning&&a.chainSpinTick==tick);
            const float remainingRadius=radius*(1.f-float(tick)/60.f);
            assert(std::abs(a.position.absXZ(player.current.pos)-remainingRadius)<.01f);
            assert(a.collider.pos.abs(a.position)<.01f);
            const float expectedY=manual?70.f+(start.y-70.f)*(1.f-float(tick)/60.f):start.y;
            assert(std::abs(a.position.y-expectedY)<.01f);
            if(tick==1){assert(a.ballVictimCount==0);a.ballVictimCount=1;}
            else assert(a.ballVictimCount==1); // No repeated hits during the orbit.
            const auto relative=a.position-player.current.pos;
            if(tick==15)assert(relative.x>remainingRadius*.99f);
            if(tick==30)assert(relative.z<-remainingRadius*.99f);
            if(tick==45)assert(relative.x<-remainingRadius*.99f);
            if(tick==59)assert(relative.x<0&&relative.z>0); // Almost a full turn before reaching the center.
            if(tick==60)assert(std::abs(relative.x)<.01f&&std::abs(relative.z)<.01f);
        }
        update_item_attack(a,&player);assert(a.returning&&a.ballVictimCount==1);
        assert(!a.active&&a.chainSpinTick==60);
    }
    reset();s_run.ranks[11]=1;s_run.chain_spin=1;assert(launch_item(&player,Upgrade::Ball,{0,70,400}));
    auto& orbit=s_attacks[36];while(!orbit.chainSpinTick)update_item_attack(orbit,&player);
    blocked=true;update_item_attack(orbit,&player);assert(!orbit.active); // Orbit must respect walls too.
    // Hurricane ignores incidental contacts, retains its chosen hop target,
    // and doubles both geometry and native wind particle scale.
    reset();s_run.ranks[9]=1;s_run.ranks[unsigned(Upgrade::Hurricane)]=1;
    assert(launch_item(&player,Upgrade::Boomerang,far.eyePos,0,20,0,far.id));
    auto& hurricane=s_attacks[item_projectile_begin(1)];
    assert(hurricane.hurricane&&hurricane.radius==126);hurricane.collider.hit=true;
    const auto hops=hurricane.hops;effectScales.clear();update_item_attack(hurricane,&player);
    assert(!hurricane.returning&&hurricane.target==far.id&&hurricane.hops==hops);
    assert(effectScales.size()==2&&effectScales[0]==2&&effectScales[1]==2);
    // Each Multishot arrow owns exactly one explosion; piercing budget is
    // unchanged, and a level-5 bomb radius follows Reach independently of Bombs.
    for(unsigned reach:{0u,5u,30u}){
        reset();s_run.ranks[8]=18;s_run.ranks[unsigned(Upgrade::Reach)]=reach;
        s_run.ranks[unsigned(Upgrade::BombArrow)]=1;s_run.ranks[unsigned(Upgrade::Piercing)]=10;
        for(unsigned shot=0;shot<8;++shot){
            assert(launch_item(&player,Upgrade::Bow,{0,70,600}));auto& arrow=s_attacks[8+shot];
            assert(arrow.bombArrow&&!arrow.detonated&&arrow.arrowTargets==11);
            const auto count=s_attacks.size();detonate_bomb_arrow(arrow,{0,70,500});
            assert(arrow.active&&arrow.detonated&&arrow.arrowTargets==11&&s_attacks.size()==count+1);
            auto& blast=s_attacks.back();assert(blast.weapon==unsigned(Upgrade::BombArrow)&&blast.exploding);
            assert(blast.collider.power==10&&blast.radius==s_run.bomb_arrow_radius());
            update_item_attack(blast,&player);assert(blast.collider.active);
            detonate_bomb_arrow(arrow,{0,70,600});assert(s_attacks.size()==count+1);
        }
    }
    // Maximum Reach still completes both legs before expiry.
    reset();s_run.ranks[11]=1;s_run.ranks[unsigned(Upgrade::Reach)]=30;
    assert(launch_item(&player,Upgrade::Ball,{0,70,400}));
    auto& longBall=s_attacks[36];assert(longBall.life>100);
    bool returned=false;for(unsigned i=0;i<200&&longBall.active;++i){update_item_attack(longBall,&player);returned|=longBall.returning;}
    assert(returned&&!longBall.active&&longBall.position.absXZ(player.current.pos)<100);
    // High, low and vertically aligned targets never tilt or slow the ball.
    for(cXyz target : {cXyz(0,700,400),cXyz(0,-300,400),cXyz(0,900,0)}) {
        reset();s_run.ranks[11]=1;assert(launch_item(&player,Upgrade::Ball,target));
        auto& a=s_attacks[36];assert(a.velocity.y==0&&std::abs(a.velocity.abs()-40)<0.001f);
        const float height=a.position.y;
        while(a.active&&!a.returning){update_item_attack(a,&player);assert(a.position.y==height);}
        player.current.pos={100,300,0};
        for(unsigned i=0;i<80&&a.active;++i){update_item_attack(a,&player);assert(a.position.y==height&&a.velocity.y==0);}
        assert(!a.active);
    }
    // Rank 1 visits two targets, rank 4 visits three, rank 30 visits eleven, then returns.
    std::array<Actor,11> chain;
    for(unsigned rank:{1u,4u,30u}){
        reset();actors.clear();s_enemies={};s_run.ranks[9]=rank;
        for(unsigned i=0;i<11;++i){chain[i].id=10+i;chain[i].health=10;chain[i].eyePos={0,70,150.0f+150*i};
            actors[10+i]=&chain[i];s_enemies[i]={10+i,true,false,false};}
        assert(launch_item(&player,Upgrade::Boomerang,chain[0].eyePos));
        auto& a=s_attacks[28];
        for(unsigned tick=0;tick<500&&!a.returning;++tick)update_item_attack(a,&player);
        assert(a.returning&&a.visitedCount==s_run.boomerang_hops()+1&&a.emitters[0]&&a.emitters[1]);
        for(unsigned i=0;i<a.visitedCount;++i)for(unsigned j=i+1;j<a.visitedCount;++j)assert(a.visited[i]!=a.visited[j]);
    }
    // Reach extends bow acquisition without widening any arrow, including
    // volleys. The farthest supported shot must outlive the old 100-tick cap.
    for(unsigned reach:{0u,10u,30u}) {
        reset();actors.clear();s_enemies={};s_run.ranks[8]=30;
        s_run.ranks[unsigned(Upgrade::Reach)]=reach;
        const float range=s_run.item_range(Upgrade::Bow);
        near.id=2;near.health=10;near.eyePos={0,70,range-1};
        actors[2]=&near;s_enemies[0]={2,true,false,false};
        assert(nearest_item_target(&player,target,range));
        near.eyePos.z=range+1;assert(!nearest_item_target(&player,target,range));
        near.eyePos.z=range-1;
        player.hand={-30,150,-100}; // Flight starts at the hand, acquisition at Link.
        for(float spread:{0.f,0.14f,-0.14f}) {
            assert(launch_item(&player,Upgrade::Bow,near.eyePos,spread));
        }
        for(unsigned slot=8;slot<11;++slot)assert(s_attacks[slot].radius==35);
        auto& arrow=s_attacks[8];
        while(arrow.active&&arrow.position.z<near.eyePos.z)update_item_attack(arrow,&player);
        assert(arrow.active&&arrow.collider.radius==35);
        if(reach==30)assert(arrow.age>50);
        // The hit actor may already have been deleted, so the native query
        // returns false. The recorded impact still stops flight before moving,
        // emitting fire particles or registering another collision.
        arrow.ballVictimCount=1;arrow.collider.hit=false;
        const auto impact=arrow.position;const auto registrations=server.registrations;const auto effectCount=particles;
        update_item_attack(arrow,&player);
        assert(!arrow.active&&!arrow.collider.active&&arrow.position.abs(impact)==0);
        assert(server.registrations==registrations&&particles==effectCount);
        blocked=true;update_item_attack(s_attacks[9],&player);assert(!s_attacks[9].active);
        blocked=false;s_attacks[10].collider.shield=true;
        update_item_attack(s_attacks[10],&player);assert(!s_attacks[10].active);
        assert(launch_item(&player,Upgrade::Bow,near.eyePos));
        assert(s_attacks[8].ballVictimCount==0&&s_attacks[8].active); // Reused slot is a fresh arrow.
        s_attacks[8].collider.hit=true;update_item_attack(s_attacks[8],&player);assert(!s_attacks[8].active);
    }
    // Piercing is snapshotted on launch; hit metadata resets between victims,
    // while the remembered victim count stops at the exact penetration budget.
    for(unsigned rank=1;rank<=10;++rank){
        reset();s_run.ranks[8]=1;s_run.ranks[unsigned(Upgrade::Piercing)]=rank;
        assert(launch_item(&player,Upgrade::Bow,{0,70,600}));auto& arrow=s_attacks[8];
        s_run.ranks[unsigned(Upgrade::Piercing)]=0;assert(arrow.arrowTargets==rank+1);
        for(unsigned victims=1;victims<=rank;++victims){
            arrow.ballVictimCount=victims;arrow.collider.hit=true;
            update_item_attack(arrow,&player);assert(arrow.active&&!arrow.collider.hit);
        }
        arrow.ballVictimCount=rank+1;const auto end=arrow.position;
        update_item_attack(arrow,&player);assert(!arrow.active&&arrow.position.abs(end)==0);
    }
    // An explicitly assigned boomerang target must not be replaced by the nearest enemy.
    reset();s_run.ranks[9]=1;actors.clear();s_enemies={};
    near.id=2;near.health=10;near.eyePos={0,70,100};actors[2]=&near;s_enemies[0]={2,true,false,false};
    far.id=3;far.health=10;far.eyePos={0,70,600};actors[3]=&far;s_enemies[1]={3,true,false,false};
    assert(launch_item(&player,Upgrade::Boomerang,far.eyePos,0,20,0,3));
    assert(s_attacks[28].target==3&&s_attacks[28].visited[0]==3);
    // Item rank never grows the boomerang collider; Reach keeps its size bonus.
    for(unsigned rank:{1u,4u,30u}) {
        reset();s_run.ranks[9]=rank;actors.clear();s_enemies={};
        near.id=2;near.health=10;near.eyePos={0,70,300};actors[2]=&near;s_enemies[0]={2,true,false,false};
        assert(launch_item(&player,Upgrade::Boomerang,near.eyePos));
        assert(s_attacks[28].radius==63);
        s_attacks=std::deque<Attack>(initial_projectile_slots);s_run.ranks[unsigned(Upgrade::Reach)]=10;
        assert(launch_item(&player,Upgrade::Boomerang,near.eyePos));
        assert(s_attacks[28].radius==s_run.radius(63));
    }
    // An extended first target is acquired, every hop uses Reach, and the
    // projectile lives long enough to complete the maximum-length chain/return.
    reset();actors.clear();s_enemies={};s_run.ranks[9]=30;
    for(unsigned i=0;i<11;++i){chain[i].id=10+i;chain[i].health=10;chain[i].eyePos={0,70,4200.f+2300*i};
        actors[10+i]=&chain[i];s_enemies[i]={10+i,true,false,false};}
    assert(!nearest_item_target(&player,target));
    assert(!launch_item(&player,Upgrade::Boomerang,chain[0].eyePos));
    s_run.ranks[unsigned(Upgrade::Reach)]=30;
    assert(nearest_item_target(&player,target,s_run.item_range(Upgrade::Boomerang))&&target.z==4200);
    assert(!nearest_item_target(&player,target,s_run.item_range(Upgrade::Bombs)));
    assert(launch_item(&player,Upgrade::Boomerang,chain[0].eyePos));
    auto& longBoom=s_attacks[28];
    for(unsigned tick=0;tick<4000&&longBoom.active;++tick)update_item_attack(longBoom,&player);
    assert(longBoom.visitedCount==11&&longBoom.returning&&!longBoom.active&&longBoom.life>0);
    assert(longBoom.position.abs(player.current.pos+cXyz(0,70,0))<65); // Actual catch, not lifetime expiry.
    reset();s_run.ranks[11]=1;assert(launch_item(&player,Upgrade::Ball,{0,70,100}));
    auto& ball=s_attacks[36];assert(ball.limit==900);
    ball.collider.hit=true;ball.collider.shield=true;update_item_attack(ball,&player);
    assert(ball.active&&!ball.returning);ball.ballVictimCount=2;
    while(ball.active&&!ball.returning)update_item_attack(ball,&player);
    assert(ball.active&&ball.returning&&ball.ballVictimCount==0);
    reset();s_run.ranks[8]=1;s_run.ranks[12]=3;
    assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    assert(s_attacks[8].collider.material==dCcD_MTRL_NONE&&s_attacks[8].collider.power==3);
    update_item_attack(s_attacks[8],&player);assert(s_attacks[8].emitters[0]);
    for(unsigned rank:{1u,5u}){
        reset();s_run.ranks[10]=rank;s_run.ranks[5]=2;effectScales.clear();
        assert(launch_item(&player,Upgrade::Bombs,{0,70,300}));explode_item(s_attacks[32],&player);
        assert(effectScales.size()==9);for(auto scale:effectScales)assert(std::abs(scale-s_attacks[32].radius/200)<0.001f);
    }
    reset();s_run.ranks[8]=1;
    for(unsigned i=0;i<20;++i)assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    auto* first=&s_attacks[8];auto* collider=&first->collider;
    for(unsigned i=0;i<1000;++i)assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    assert(s_attacks.size()==initial_projectile_slots+1000&&first==&s_attacks[8]&&collider==&s_attacks[8].collider);
    auto* overflow=&s_attacks[initial_projectile_slots];stop_attack(*overflow);
    auto size=s_attacks.size();assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    assert(s_attacks.size()==size&&overflow->active); // Reuse without moving colliders.
    for(auto weapon:{Upgrade::Bombs,Upgrade::Ball}) {
        reset();s_run.ranks[unsigned(weapon)]=30;
        for(unsigned i=0;i<50;++i)assert(launch_item(&player,weapon,{0,70,300}));
        assert(s_attacks.size()==initial_projectile_slots+46);
    }
    reset();invalidFloor=true;assert(!spawn_chest(&player));invalidFloor=false;
    blocked=true;assert(!spawn_chest(&player));blocked=false;
    arenaBlocked=true;assert(!spawn_chest(&player));arenaBlocked=false;
    floorRoom=50;assert(!spawn_chest(&player));floorRoom=51;
    assert(spawn_chest(&player));assert(s_chests[0].active && s_chests[0].position.y==0);
    for(unsigned i=1;i<kPaidChestCount;++i)assert(spawn_chest(&player));
    const auto oldest=s_chests[0].spawnOrder,second=s_chests[1].spawnOrder;
    assert(spawn_chest(&player));
    assert(s_chests[0].spawnOrder>second&&s_chests[1].spawnOrder==second);
    assert(spawn_chest(&player));assert(s_chests[1].spawnOrder>s_chests[0].spawnOrder);
    assert(oldest<second&&s_chests[0].position.abs(s_chests[1].position)>=250);
    // Placement failure preserves every live chest and the spawn chronology.
    const auto sequence=s_chestSequence;
    const auto kept=s_chests[2];invalidFloor=true;assert(!spawn_chest(&player));invalidFloor=false;
    assert(s_chestSequence==sequence&&s_chests[2].spawnOrder==kept.spawnOrder&&s_chests[2].position.abs(kept.position)==0);
    // Opening and selection are protected; use the next oldest available chest.
    s_chests[2].opening=true;s_selectedChest=3;unsigned slot=kChestCount;
    assert(find_chest_spawn_slot(0,kPaidChestCount,slot)&&slot==4);
    s_chests[5].active=false;assert(find_chest_spawn_slot(0,kPaidChestCount,slot)&&slot==5);
    for(unsigned i=0;i<kPaidChestCount;++i)s_chests[i].opening=true;
    assert(!find_chest_spawn_slot(0,kPaidChestCount,slot));
    for(unsigned i=0;i<kPaidChestCount;++i)s_chests[i].opening=false;
    s_selectedChest=kChestCount;
    // Proximity alone never pauses combat or opens a purchase window.
    player.current.pos=s_chests[0].position;rupees=100;update_chests(&player);
    assert(s_run.phase==Phase::Running && !s_chests[0].opening && rupees==100);
    // All pools disappear together when the item pool is exhausted, even during
    // a spawn delay. Potion/slot availability later cannot restart this run's chests.
    reset();for(auto& chest:s_chests)chest.active=true;s_chestDelay=999;
    s_run.banned_items.fill(true);update_chests(&player);
    assert(s_chestsExhausted&&s_selectedChest==kChestCount);
    for(const auto& chest:s_chests)assert(!chest.active);
    s_run.banned_items.fill(false);seed_map_chests(&player);assert(!spawn_chest(&player));
    update_chests(&player);for(const auto& chest:s_chests)assert(!chest.active);
    reset();s_run.phase=Phase::Treasure;s_chests[0].active=true;s_chests[0].opening=true;
    s_run.banned_items.fill(true);assert(chest_spawning_allowed()&&s_chests[0].active);
    s_run.phase=Phase::Running;assert(!chest_spawning_allowed()&&!s_chests[0].active);
    // Seed every map budget independently; fixed chests do not replenish after purchase.
    for(unsigned map=0;map<5;++map) {
        reset();s_map=map;distributionReady=false;seed_map_chests(&player);
        for(const auto& c:s_chests)assert(!c.active);
        distributionReady=true;invalidFloor=true;seed_map_chests(&player);
        for(const auto& c:s_chests)assert(!c.active);
        // Safe destinations can be reached by going around obstacles. The old
        // Link-to-chest straight-path filter incorrectly rejected all of them.
        invalidFloor=false;reachable=false;seed_map_chests(&player);
        for(const auto& c:s_chests)assert(!c.active);
        reachable=true;blocked=true;
        for(unsigned frame=0;frame<200;++frame){
            const unsigned before=placement;seed_map_chests(&player);
            assert(placement-before<=24);
        }
        blocked=false;
        unsigned count=0;for(const auto& c:s_chests)if(c.active){++count;assert(c.fixed&&!c.free);}
        assert(count==map_chest_budget(map));
        for(unsigned i=0;i<kPaidChestCount;++i)assert(!s_chests[i].active);
        s_chests[kPaidChestCount]={};seed_map_chests(&player);
        assert(!s_chests[kPaidChestCount].active);
        assert(spawn_chest(&player)&&!s_chests[0].fixed);
        // Even the generic selector cannot evict the initial 40-rupee chests.
        for(unsigned i=kPaidChestCount+1;i<kSmallChestCount;++i)if(s_chests[i].active) {
            assert(s_chests[i].fixed&&s_chests[i].spawnOrder==0);
            s_chests[i].opening=false;
        }
        unsigned slot=kChestCount;
        // The purchased map slot is inactive but still reserved by its pool.
        s_chests[kPaidChestCount].active=true;s_chests[kPaidChestCount].fixed=true;
        for(unsigned i=kPaidChestCount;i<kSmallChestCount;++i)s_chests[i].fixed=true;
        assert(!find_chest_spawn_slot(kPaidChestCount,kSmallChestCount,slot));
    }
    // Exercise real area-weighted floor sampling across multiple field runs.
    for(unsigned seed=1;seed<=20;++seed){
        reset();s_map=1;s_run.start(seed);blocked=true;
        for(unsigned frame=0;frame<200;++frame)seed_map_chests(&player);
        std::array<bool,4> quadrants{};unsigned count=0;
        for(unsigned i=kPaidChestCount;i<kSmallChestCount;++i){
            const auto& chest=s_chests[i];assert(chest.active);++count;
            quadrants[(chest.position.x>0?1:0)+(chest.position.z>0?2:0)]=true;
            const float spacing=std::sqrt(s_eliteFloorArea/18.f)*0.5f;
            assert(chest.position.absXZ(player.current.pos)>=spacing);
            for(unsigned j=kPaidChestCount;j<i;++j)
                assert(chest.position.absXZ(s_chests[j].position)>=spacing);
        }
        assert(count==18&&std::count(quadrants.begin(),quadrants.end(),true)>=3);
    }
    // A manual boomerang keeps its reticle leg; Hurricane ignores incidental
    // contacts. Earned later hops skip enemies already crossed on that leg.
    for(bool hurricane:{false,true}) {
        reset();s_run.ranks[unsigned(Upgrade::Boomerang)]=4;s_run.ranks[unsigned(Upgrade::Hurricane)]=hurricane;
        Actor crossed,next;crossed.id=2;crossed.eyePos={50,70,450};next.id=3;next.eyePos={100,70,500};
        actors={{2,&crossed},{3,&next}};s_enemies={};s_enemies[0]={2,true,false,false};s_enemies[1]={3,true,false,false};
        assert(launch_item(&player,Upgrade::Boomerang,{0,70,420},0,20,0,kNone,true));
        auto& a=s_attacks[item_projectile_begin(1)];assert(a.aimPending&&a.manualFlight&&a.target==kNone);
        a.collider.hit=hurricane;update_item_attack(a,&player);
        assert(a.manualFlight&&!a.returning&&a.target==kNone&&a.velocity.x==0&&a.velocity.z==42);
        a.ballVictims[0]=2;a.ballVictimCount=1;a.travel=a.limit;
        update_item_attack(a,&player);assert(!a.manualFlight&&!a.returning&&a.target==3&&a.velocity.x>0);
    }
    // Manual ball flight and retrieval retain vertical aim; automatic balls
    // keep the established horizontal pass.
    reset();s_run.ranks[unsigned(Upgrade::Ball)]=1;
    assert(launch_item(&player,Upgrade::Ball,{0,350,400},0,20,0,kNone,true));
    auto& aimedBall=s_attacks[item_projectile_begin(3)];assert(aimedBall.manualFlight&&aimedBall.velocity.y>0);
    aimedBall.position={0,350,400};aimedBall.travel=aimedBall.limit;update_item_attack(aimedBall,&player);
    assert(aimedBall.returning&&aimedBall.velocity.y<0);
    // Equipment adds beyond the normal Boomerang cap: twelve hops, thirteen
    // distinct victims; faster outbound/return flights keep the whole path alive.
    reset();actors.clear();s_enemies={};s_run.ranks[9]=30;
    s_run.gear_weapon={GearKind::HurricaneBoomerang,5,2,5};
    std::array<Actor,13> equipmentChain;
    for(unsigned i=0;i<equipmentChain.size();++i) {
        auto& enemy=equipmentChain[i];enemy.id=10+i;enemy.eyePos={0,70,150.f+150*i};
        actors[enemy.id]=&enemy;s_enemies[i]={enemy.id,true,false,false};
    }
    assert(launch_item(&player,Upgrade::Boomerang,equipmentChain[0].eyePos));
    auto& equipmentBoom=s_attacks[28];assert(equipmentBoom.hops==12&&std::abs(equipmentBoom.velocity.abs()-52.5f)<.001f);
    for(unsigned tick=0;tick<500&&!equipmentBoom.returning;++tick)update_item_attack(equipmentBoom,&player);
    assert(equipmentBoom.returning&&equipmentBoom.visitedCount==13&&std::abs(equipmentBoom.velocity.abs()-68.75f)<.001f);
    for(unsigned i=0;i<13;++i)for(unsigned j=i+1;j<13;++j)assert(equipmentBoom.visited[i]!=equipmentBoom.visited[j]);
    for(unsigned tick=0;tick<500&&equipmentBoom.active;++tick)update_item_attack(equipmentBoom,&player);
    assert(!equipmentBoom.active&&equipmentBoom.life>0);
    reset();s_run.ranks[8]=1;s_run.ranks[unsigned(Upgrade::Piercing)]=10;
    s_run.gear_weapon={GearKind::SharpshooterBow,5,5,2};
    assert(launch_item(&player,Upgrade::Bow,{0,70,1700}));
    auto& equipmentArrow=s_attacks[8];assert(equipmentArrow.arrowTargets==13&&equipmentArrow.collider.power==8&&equipmentArrow.limit>1400);
    reset();s_run.ranks[11]=1;s_run.gear_weapon={GearKind::IronBall,5,5,5};
    assert(launch_item(&player,Upgrade::Ball,{0,70,400}));assert(std::abs(s_attacks[36].limit-1125)<.001f);
    reset();s_run.ranks[11]=1;s_run.gear_weapon={GearKind::BreakerBall,5,5,5};
    assert(launch_item(&player,Upgrade::Ball,{0,70,400}));
    auto& equipmentBall=s_attacks[36];assert(std::abs(equipmentBall.velocity.abs()-50)<.001f);
    equipmentBall.travel=equipmentBall.limit;equipmentBall.position={0,70,400};update_item_attack(equipmentBall,&player);
    assert(equipmentBall.returning&&std::abs(equipmentBall.velocity.abs()-68.75f)<.001f);
    reset();s_run.ranks[10]=1;s_run.gear_weapon={GearKind::BombBag,5,5,5};
    assert(launch_item(&player,Upgrade::Bombs,{0,70,400}));auto& equipmentBomb=s_attacks[32];
    assert(equipmentBomb.collider.power==8);explode_item(equipmentBomb,&player);assert(std::abs(equipmentBomb.radius-200.2f)<.001f);
    std::cout<<"Survivors loot: targeting, four projectile types, uncapped growth, placement and proximity behavior passed\n";
}
'''
functions = ['nearest_item_target','next_boom_target','update_boom_wind','launch_item','explode_item','detonate_bomb_arrow','update_item_attack','fire_items',
             'chest_spawning_allowed','find_chest_spawn_slot','spawn_chest','seed_map_chests','update_chests']
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'
    binary=Path(tmp)/'test'
    sampler=extract('random_arena_floor',(root/'src/survivors_elites.inc').read_text())
    cpp.write_text(stubs+sampler+'\n'.join(extract(n) for n in functions)+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
