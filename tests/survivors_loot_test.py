"""Exercise production chest purchases, placement and item trajectories with engine boundaries."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root/'src/survivors_loot.inc').read_text()


def extract(name):
    match = re.search(r'^(?:bool|void) ' + name + r'\(', source, re.M)
    assert match, name
    start = source.index('{', match.start())
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
using namespace dawnlight::survivors;
using ActorId=unsigned;constexpr ActorId kNone=~0u;
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
};
struct csXyz{csXyz(int,int,int){}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
float cM_ssin(s16 angle){return std::sin(angle*3.14159265f/32768);}
float cM_scos(s16 angle){return std::cos(angle*3.14159265f/32768);}
struct Actor {unsigned id=1;int health=10;struct{cXyz pos;}current;cXyz eyePos;};
struct daAlink_c:Actor {int tevStr=0;}player;
unsigned fopAcM_GetID(Actor* a){return a->id;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
std::unordered_map<unsigned,Actor*>actors;
Actor* actor_by_id(unsigned id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
struct Enemy {unsigned id=0;bool initialized=false,credited=false,retiring=false;};
std::array<Enemy,enemy_capacity>s_enemies;
struct dBgS_LinChk {cXyz from,to;void Set(const cXyz* f,const cXyz* t,daAlink_c*){from=*f;to=*t;}};
struct dBgS_ObjGndChk {cXyz pos;void SetPos(cXyz* p){pos=*p;}};
bool blocked=false,invalidFloor=false,blockPositiveX=false;
struct Background {
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
    void SetAtAtp(unsigned p){power=p;}void SetAtMtrl(int){}void OnAtNoConHit(){}
    void ResetAtHit(){hit=false;shield=false;++resets;}void OnAtSetBit(){active=true;}void OffAtSetBit(){active=false;}
    bool ChkAtHit(){return hit;}bool ChkAtShieldHit(){return shield;}
    void SetC(cXyz p){pos=p;}void SetR(float r){radius=r;}void SetAtVec(cXyz v){velocity=v;}
};
struct Attack {
    ActorId target=kNone;unsigned hops=0,visitedCount=0;std::array<ActorId,6>visited{};
    bool active=false;unsigned life=0,weapon=0,age=0;bool returning=false,exploding=false;
    float travel=0,limit=0;cXyz position,velocity;float radius=0;Status status;Collider collider;
    std::array<unsigned,2>emitters{};
};
constexpr int kAttackSource=0;constexpr unsigned kChestCount=2,kItemBegin=8,kSlotsPerItem=4;
std::array<Attack,projectile_capacity>s_attacks;
void stop_attack(Attack& a){a.active=false;a.collider.OffAtSetBit();a.emitters={};}
unsigned cleared=0,closed=0;
void clear_attacks(){++cleared;for(auto& a:s_attacks)stop_attack(a);}
void close_window(){++closed;}
struct Server {unsigned registrations=0;void Set(Collider* c){assert(c->active && c->status->owner==&player);++registrations;}}server;
Server* dComIfG_Ccsp(){return &server;}
std::array<unsigned,2>queued{};
bool queue_ranged(daAlink_c*,Upgrade w){++queued[unsigned(w)-8];return true;}
unsigned particles=0;
template<class... Args> unsigned dComIfGp_particle_set(Args...){return ++particles;}
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
bool arena=true,transition=false;
unsigned s_player=1,rupees=0;
bool in_arena(){return arena;}bool transitioning(){return transition;}
unsigned dComIfGs_getRupee(){return rupees;}void dComIfGs_setRupee(unsigned r){rupees=r;}
'''
main = r'''
void reset(){
    s_run.start(123);s_attacks={};s_cooldowns={};s_chests={};s_loot.ready=true;
    player.current.pos={};player.id=s_player=1;s_selectedChest=kChestCount;
    arena=true;transition=blocked=invalidFloor=blockPositiveX=false;server.registrations=0;
}
int main(){
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
    assert(queued[0]==1&&queued[1]==1);
    for(unsigned type=2;type<4;++type){
        assert(s_attacks[8+4*type].active && s_cooldowns[8+type]>0);
        assert(s_attacks[8+4*type].status.owner==&player);
    }
    assert(s_attacks[20].collider.type==AT_TYPE_NORMAL_SWORD &&
           s_attacks[16].collider.type==AT_TYPE_NORMAL_SWORD);
    fire_items(&player);assert(!s_attacks[9].active); // Cooldowns prevent every-frame firing.
    for(unsigned i=0;i<8;++i)assert(!s_attacks[i].active); // Separate base-weapon budget.
    reset();s_run.ranks[8]=5;
    assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    s_attacks[8].collider.hit=true;update_item_attack(s_attacks[8],&player);assert(!s_attacks[8].active);
    blocked=true;update_item_attack(s_attacks[9],&player);assert(!s_attacks[9].active);
    reset();s_run.ranks[10]=1;assert(launch_item(&player,Upgrade::Bombs,{0,70,600}));
    auto& bomb=s_attacks[16];
    for(unsigned i=0;i<29;++i){update_item_attack(bomb,&player);assert(!bomb.exploding && server.registrations==0);}
    update_item_attack(bomb,&player);
    assert(bomb.exploding && std::abs(bomb.position.z-600)<0.01f && std::abs(bomb.position.y-70)<0.01f);
    assert(bomb.collider.radius==200 && server.registrations==1 && particles==2);
    for(unsigned i=0;i<13;++i)update_item_attack(bomb,&player);
    assert(!bomb.active);
    reset();s_run.ranks[10]=5;assert(launch_item(&player,Upgrade::Bombs,{0,70,600}));blocked=true;
    update_item_attack(s_attacks[16],&player);assert(s_attacks[16].exploding && s_attacks[16].radius==320);
    for(auto weapon:{Upgrade::Boomerang,Upgrade::Ball}){
        reset();s_run.ranks[unsigned(weapon)]=1;assert(launch_item(&player,weapon,{0,70,400}));
        auto& a=s_attacks[8+4*(unsigned(weapon)-8)];
        while(!a.returning && a.active)update_item_attack(a,&player);
        assert(a.returning && a.active);
        player.current.pos.x=100;update_item_attack(a,&player);assert(a.velocity.x>0 && a.velocity.z<0);
        for(unsigned i=0;i<80 && a.active;++i)update_item_attack(a,&player);
        assert(!a.active);
    }
    // Rank 1 visits two targets, rank 5 visits six distinct targets, then returns.
    std::array<Actor,6> chain;
    for(unsigned rank:{1u,5u}){
        reset();actors.clear();s_enemies={};s_run.ranks[9]=rank;
        for(unsigned i=0;i<6;++i){chain[i].id=10+i;chain[i].health=10;chain[i].eyePos={0,70,150.0f+150*i};
            actors[10+i]=&chain[i];s_enemies[i]={10+i,true,false,false};}
        assert(launch_item(&player,Upgrade::Boomerang,chain[0].eyePos));
        auto& a=s_attacks[12];
        for(unsigned tick=0;tick<200&&!a.returning;++tick)update_item_attack(a,&player);
        assert(a.returning&&a.visitedCount==rank+1&&a.emitters[0]&&a.emitters[1]);
        for(unsigned i=0;i<a.visitedCount;++i)for(unsigned j=i+1;j<a.visitedCount;++j)assert(a.visited[i]!=a.visited[j]);
    }
    reset();s_run.ranks[8]=1;
    for(unsigned i=0;i<4;++i)assert(launch_item(&player,Upgrade::Bow,{0,70,300}));
    assert(!launch_item(&player,Upgrade::Bow,{0,70,300})); // Bounded objects, no extra allocation.
    reset();invalidFloor=true;assert(!spawn_chest(&player));invalidFloor=false;
    blocked=true;assert(!spawn_chest(&player));blocked=false;
    assert(spawn_chest(&player));assert(s_chests[0].active && s_chests[0].position.y==0);
    assert(spawn_chest(&player));assert(!spawn_chest(&player));
    assert(s_chests[0].position.abs(s_chests[1].position)>=250);
    // Proximity alone never pauses combat or opens a purchase window.
    player.current.pos=s_chests[0].position;rupees=100;update_chests(&player);
    assert(s_run.phase==Phase::Running && !s_chests[0].opening && rupees==100);
    std::cout<<"Survivors loot: targeting, four projectile types, caps, placement and proximity behavior passed\n";
}
'''
functions = ['nearest_item_target','next_boom_target','update_boom_wind','launch_item','explode_item','update_item_attack','fire_items',
             'spawn_chest','update_chests']
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'
    binary=Path(tmp)/'test'
    cpp.write_text(stubs+'\n'.join(extract(n) for n in functions)+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
