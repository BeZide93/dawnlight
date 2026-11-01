"""Exercise production chest interaction and native item-event lifecycle."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1]) if len(sys.argv)>1 else root/'dusklight'
source=(root/'src/survivors_treasure.inc').read_text().split('HookAction before_treasure_draw')[0]
core=(root/'src/survivors.cpp').read_text()
start=core.index('bool pause_run_actor(')
source+=core[start:core.index('\nHookAction before_process', start)]
fixture=r'''
bool s_victory=false,s_deathPending=false;

constexpr int dItemNo_POU_SPIRIT_e=0xe0;

#include <cassert>
#include <cstdint>
#include <cmath>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using s16=short;
constexpr int kNone=-1;
using u8=uint8_t;
using ActorId=int;
bool messageAvailable=true;
bool prepare_treasure_message(ChestReward, const Run&, const Run&, Rarity){return messageAvailable;}
unsigned treasure_message_id(){return 0x8123;}
unsigned inventorySyncs=0;
void sync_survivors_inventory(){++inventorySyncs;}
constexpr int fpcNm_Demo_Item_e=20;
constexpr int dItemNo_NONE_e=255,dItemNo_EMPTY_BOTTLE_e=20,dItemNo_RED_BOTTLE_e=21,dItemNo_MASTER_SWORD_e=22,
    SLOT_11=11,SLOT_14=14,COLLECT_MASTER_SWORD=1,COLLECT_LIGHT_SWORD=3,
 dItemNo_LIGHT_SWORD_e=23,dItemNo_SWORD_e=24,dItemNo_WOOD_SHIELD_e=25,dItemNo_SHIELD_e=26,dItemNo_HYLIA_SHIELD_e=27;
constexpr int dItemNo_BOMB_5_e=10, dItemNo_BOW_e=11,dItemNo_BOOMERANG_e=12,dItemNo_IRONBALL_e=13, dEvtCnd_CANGETITEM_e=8, BUTTON_STATUS_OPEN=6;
struct ModContext{};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct cXyz {float x=0,y=0,z=0;
    cXyz operator+(cXyz p)const{return {x+p.x,y+p.y,z+p.z};}
    cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}
    float absXZ()const{return std::sqrt(x*x+z*z);}};
using Vec=cXyz;
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}

struct leafdraw_class {int id=0;};
struct layer_class {bool deleting=false;};
layer_class rootLayer, sceneLayer, callbackLayer;
layer_class* currentLayer=&rootLayer;
layer_class* fpcLy_RootLayer(){return &rootLayer;}
layer_class* fpcLy_CurrentLayer(){return currentLayer;}
void fpcLy_SetCurrentLayer(layer_class* layer){currentLayer=layer;}
bool fpcLy_IsDeletingMesg(layer_class* layer){return layer->deleting;}
struct Actor:leafdraw_class {void* sub_method=nullptr;int name=0;struct {layer_class* layer=&sceneLayer;} layer_tag;};
using fopAc_ac_c=Actor;
struct EventInfo {bool command=false; int condition=0;
    bool checkCommandItem(){return command;}
    void onCondition(int c){condition|=c;}};
using process_method_func=int(*)(void*);
struct process_method_class {process_method_func execute_method;};
int execute(void*){return 1;}
process_method_class methods{execute};
void* dispatchArg=nullptr;
struct daDitem_c:Actor {
    enum {ACTION_END_e,ACTION_WAIT_LIGHT_END_e};
    daDitem_c(){sub_method=&methods;}
    int itemNo=0;int getItemNo(){return itemNo;}int mAction=ACTION_END_e;
    void setAction(int a){mAction=a;}
    EventInfo eventInfo; bool visible=true,dead=false;
    bool chkDraw(){return visible;} bool chkDead(){return dead;}
} prop;
int equippedShield=255;
unsigned shieldRequests=0;
struct daAlink_c:Actor {
    enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_TIRED_WAIT,PROC_GET_ITEM,OTHER,ANM_GET_A,UPPER_2};
    int mProcID=PROC_WAIT,animation=-1;bool wolf=false,dead=false,inEvent=false;
    struct {Vec pos;} current;
    struct {int p0=90,p1=90;void setParam0(int n){p0=n;} void setParam1(int n){p1=n;}} mDemo;
    struct {int field_0x300a=-7;} mProcVar1;
    unsigned field_0x32cc=0;
    struct Collider{bool active=true;void OffAtSetBit(){active=false;}void ResetAtHit(){}}mAtCps[3],mAtSph;
    void resetUpperAnime(int,int){}void procWaitInit(){mProcID=PROC_WAIT;}
    struct {s16 y=0;}shape_angle;int doStatus=0;bool trigger=false;
    void setDoStatus(int s){doStatus=s;}bool doTrigger(){return trigger;}
    bool checkWolf(){return wolf;}bool checkDeadHP(){return dead;}bool checkEventRun(){return inEvent;}
    void onLightSwordGetEffect(){}
    void setSingleAnimeBase(int a){animation=a;}
} player;
void request_shield_reload(daAlink_c* link,u8 item){
    assert(link==&player&&item==equippedShield);++shieldRequests;
}
constexpr int PAD_1=0,PAD_BUTTON_A=1;
struct Pad {unsigned mPressedButtonFlags=0;};
struct mDoCPd_c {static inline Pad pad{};
    static bool getTrigA(int){return player.trigger;}static Pad& getCpadInfo(int){return pad;}};
void dComIfGp_setDoStatusForce(int status,int){player.doStatus=status;}
int dComIfGs_getLife(){return 12;}
enum class SwordAction{None,Normal};SwordAction s_swordAction= SwordAction::None;
bool s_wavePending=false;unsigned s_comboStep=0,s_comboTimeout=0;
struct {int mPt2=-1;} event;
bool alive=false,creating=false,doing=false,deleteAllowed=true,mode=true,stageChange=false;
bool running=false,paused=false,talking=false,eventEnded=false,playerPresent=true,createFails=false;
int window=0,pauseStatus=0,created=0,ordered=0,reset=0,deleted=0,drawn=0,cleared=0,itemPartner=-1;
Actor* fopAcM_SearchByID(int id){return alive&&id==prop.id?&prop:nullptr;}
int fopAcM_GetName(Actor* a){return a->name;}
int fopAcM_GetID(leafdraw_class* a){return a->id;}
bool fpcM_IsCreating(int){return creating;}
daAlink_c* daAlink_getAlinkActorClass(){return playerPresent?&player:nullptr;}
auto* dComIfGp_getEvent(){return &event;}
bool dComIfGp_event_runCheck(){return running;}
bool dComIfGp_isEnableNextStage(){return stageChange;}
bool dComIfGp_isPauseFlag(){return paused;}
int dMeter2Info_getWindowStatus(){return window;}
int dMeter2Info_getPauseStatus(){return pauseStatus;}
bool dMsgObject_isTalkNowCheck(){return talking;}
void dComIfGp_event_setItemPartnerId(int id){itemPartner=id;}
void dComIfGp_event_setGtItm(int item){assert(item==prop.itemNo);}
bool dComIfGp_evmng_endCheck(const char*){return eventEnded;}
void dComIfGp_event_reset(){++reset;running=false;event.mPt2=-1;}
int fopAcM_GetRoomNo(Actor*){return 0;}
int fopAcM_delete(Actor*){if(!deleteAllowed)return 0;alive=false;++deleted;return 1;}
int fopAcM_createDemoItem(Vec*,int item,int bit,void*,int,void*,u8 flags){
    assert(((item>=10&&item<=27)||item==dItemNo_POU_SPIRIT_e)&&bit==-1&&flags==1);prop.itemNo=item; // Presentation-only, no inventory grant.
    assert(currentLayer==player.layer_tag.layer&&currentLayer!=&rootLayer);
    ++created;if(createFails)return -1;
    alive=true;prop.id=100;prop.name=fpcNm_Demo_Item_e;return prop.id;
}
int fopAcM_orderItemEvent(daDitem_c* p,int,int){assert(p==&prop);++ordered;return 1;}
struct Node{Node* next=nullptr;};
struct create_tag {struct {Node node;void* mpTagData=nullptr;} base;};
struct create_request {int id=100;};
create_request request;
create_tag tag;
struct {Node* mpHead=nullptr;} g_fpcCtTg_Queue;
#define NODE_GET_NEXT(n) ((n)->next)
bool fpcCtRq_IsDoing(create_request*){return doing;}
bool fpcCtRq_Cancel(create_request*){creating=false;alive=false;g_fpcCtTg_Queue.mpHead=nullptr;return true;}
namespace mods {
template<class T> T arg(void* args,int n){
    if(n==1)return reinterpret_cast<T>(dispatchArg);
    return reinterpret_cast<T>(args);
}
}


std::array<int,4> bottles;
int equippedSword=0,swordGrants=0,potionGrants=0;
int dComIfGs_getItem(int slot,bool){return bottles.at(slot-SLOT_11);}
void dComIfGs_setItem(int slot,int item){bottles.at(slot-SLOT_11)=item;++potionGrants;}
void dComIfGs_onItemFirstBit(int){}
void dComIfGs_setCollectSword(int){++swordGrants;}
enum{COLLECT_WOODEN_SHIELD=0,COLLECT_HYLIAN_SHIELD=2};void dComIfGs_offCollectShield(int){}
u8 dComIfGs_getSelectEquipShield(){return equippedShield;}
void dComIfGs_setCollectShield(int){}
void dMeter2Info_setShield(int item,bool){equippedShield=item;}
void dMeter2Info_setSword(int item,bool){equippedSword=item;}
void dComIfGs_setBButtonItemKey(int item){assert(item==equippedSword);}
bool s_hub=false,s_hubDeparture=false,s_hubDoorStarted=false;Run s_run;unsigned rupees=0;int s_player=1;
struct Chest{bool active=false,opening=false,free=false,fixed=false;cXyz position{};unsigned age=0;ChestReward reward=ChestReward::Count;std::array<ChestReward,3> offers{ChestReward::Count,ChestReward::Count,ChestReward::Count};std::array<Rarity,3> rarities{};};
constexpr unsigned kChestCount=2;unsigned s_selectedChest=kChestCount;
std::array<Chest,2>s_chests;
struct {bool ready=true;}s_loot;
bool blocked=false;
struct dBgS_LinChk {void Set(const cXyz*,const cXyz*,daAlink_c*){}};
struct Background {bool LineCross(dBgS_LinChk*){return blocked;}}background;
Background& dComIfG_Bgsp(){return background;}
Actor* actor_by_id(int id){return !creating?fopAcM_SearchByID(id):nullptr;}
bool save_modal_active(){return false;}
bool in_arena(){return mode;}bool transitioning(){return stageChange;}
bool ui_visible(){return window!=0;}
bool midna_dialog_busy(){return false;}
bool playable(){return mode&&!stageChange&&!running&&!paused&&!ui_visible();}
void close_window(){window=0;}
bool sword_action_available(daAlink_c* link){return link->mProcID==daAlink_c::PROC_WAIT;}
unsigned dComIfGs_getRupeeMax(){return 10000;}
unsigned dComIfGs_getRupee(){return rupees;}void dComIfGs_setRupee(unsigned n){rupees=n;}
void clear_attacks(){++cleared;}

using u8=unsigned char;
int nativeKeys=0,pendingKeys=0;
int dComIfGs_getKeyNum(){return nativeKeys;}
int dComIfGp_getItemKeyNumCount(){return pendingKeys;}
void dComIfGp_clearItemKeyNumCount(){pendingKeys=0;}
void dComIfGs_setKeyNum(u8 n){nativeKeys=n;}
#include "survivors_keys.inc"
'''
main=r'''

void fresh(){nativeKeys=pendingKeys=0;
    bottles.fill(dItemNo_NONE_e);equippedSword=swordGrants=potionGrants=0;
    equippedShield=dItemNo_NONE_e;shieldRequests=0;
    s_run.start(123);s_treasure={};s_chests={};s_chests[0].active=true;s_chests[0].position={0,0,100};
    s_chosenTreasure=ChestReward::Count;s_selectedChest=kChestCount;rupees=100;s_loot.ready=true;
    prop={};player={};player.id=1;event.mPt2=-1;
    currentLayer=&rootLayer;sceneLayer.deleting=false;
    alive=creating=doing=running=paused=talking=stageChange=eventEnded=createFails=blocked=false;
    mode=playerPresent=deleteAllowed=true;window=pauseStatus=0;
    created=ordered=reset=deleted=drawn=cleared=0;itemPartner=-1;
    g_fpcCtTg_Queue.mpHead=nullptr;
}
bool open_first(Rarity rarity=Rarity::Common){
    if(!begin_treasure_choice(0,&player))return false;
    s_chests[0].rarities.fill(rarity);
    unsigned choice=0;select_treasure(nullptr,&choice);update_treasure_choice(&player);
    return s_run.phase==Phase::Treasure;
}
void accept(){event.mPt2=prop.id;prop.eventInfo.command=true;running=true;}
int main(){
    fresh();int result=0;
    begin_starting_item_choice();assert(s_run.phase==Phase::Running&&s_selectedChest==kChestCount);
    Talents starting;starting.ranks[unsigned(Talent::StartingItem)]=1;
    for(unsigned reward=0;reward<unsigned(ChestReward::Count);++reward){
        fresh();s_run.start(25,{},starting);s_loot.ready=false;rupees=0;begin_starting_item_choice();
        assert(s_run.phase==Phase::TreasureChoice&&s_run.ticks==0&&s_chests[0].free&&!s_chests[0].active);
        const auto offers=s_chests[0].offers;const auto rng=s_run.random;
        begin_starting_item_choice();dismiss_treasure_choice();
        assert(s_run.phase==Phase::TreasureChoice&&s_run.random==rng&&s_chests[0].offers==offers);
        assert(!s_run.tick(true)&&s_run.ticks==0&&pause_run_actor(true));
        s_chests[0].offers={ChestReward(reward),ChestReward::Count,ChestReward::Count};
        s_chests[0].rarities.fill(Rarity::Common);unsigned choice=0;select_treasure(nullptr,&choice);
        window=1;update_treasure_choice(&player);assert(s_run.starting_item_pending);
        window=0;update_treasure_choice(&player);
        assert(!s_run.starting_item_pending&&s_run.phase==Phase::Running&&s_run.owns(ChestReward(reward)));
        assert(created==0&&ordered==0&&rupees==0&&s_run.chests_opened==0&&s_run.chest_cost()==20&&s_run.ticks==0);
        assert(s_selectedChest==kChestCount&&!s_chests[0].active);
        const auto ranks=s_run.ranks;select_treasure(nullptr,&choice);update_treasure_choice(&player);begin_starting_item_choice();
        assert(s_run.ranks==ranks&&s_run.phase==Phase::Running&&!grant_starting_item(&player));
        if(ChestReward(reward)==ChestReward::MasterSword)assert(equippedSword==dItemNo_MASTER_SWORD_e&&swordGrants==1);
        if(ChestReward(reward)==ChestReward::Shield)assert(equippedShield==dItemNo_WOOD_SHIELD_e);
        if(ChestReward(reward)==ChestReward::RedPotion)assert(potionGrants==1&&bottles[0]==dItemNo_RED_BOTTLE_e);
        s_run.start(26,{},starting);begin_starting_item_choice();assert(s_run.starting_item_pending&&s_run.phase==Phase::TreasureChoice);
        cancel_treasure_demo();assert(!s_run.starting_item_pending&&s_selectedChest==kChestCount);
    }
    fresh();
    fresh();s_run.start(25,{},starting);rupees=0;begin_starting_item_choice();
    s_chests[0].offers={ChestReward::RedPotion,ChestReward::Count,ChestReward::Count};
    s_chests[0].rarities.fill(Rarity::Legendary);unsigned startingIndex=0;
    select_treasure(nullptr,&startingIndex);update_treasure_choice(&player);
    assert(potionGrants==4&&rupees==20&&s_run.chests_opened==0&&!s_run.starting_item_pending);
    fresh();
    for(auto phase:{Phase::Ready,Phase::Running,Phase::Treasure,Phase::TreasureChoice,Phase::Upgrade,Phase::Defeat}){
        s_run.phase=phase;
        assert(pause_run_actor(true)==(phase!=Phase::Running));
        assert(pause_run_actor(false)==(phase!=Phase::Running&&phase!=Phase::Treasure));
    }
    s_run.phase=Phase::Treasure;window=1;assert(pause_run_actor(false));
    fresh();
    rupees=19;player.trigger=true;
    assert(before_chest_action(nullptr,&player,&result,nullptr)==HOOK_CONTINUE);
    assert(player.doStatus==0 && created==0 && s_run.phase==Phase::Running);
    rupees=20;player.trigger=false;
    assert(before_chest_action(nullptr,&player,&result,nullptr)==HOOK_CONTINUE);
    assert(player.doStatus==BUTTON_STATUS_OPEN && created==0);
    for(unsigned reason=1;reason<8;++reason){
        fresh();
        if(reason==1)player.current.pos.z=-100;
        if(reason==2)player.current.pos.y=50;
        if(reason==3)blocked=true;
        if(reason==4)s_loot.ready=false;
        if(reason==5)paused=true;
        if(reason==6)stageChange=true;
        if(reason==7){for(unsigned i=8;i<12;++i)s_run.ranks[i]=30;s_run.master_sword=30;s_run.dual_wield=30;s_run.dark_link=30;s_run.shield=10;bottles.fill(dItemNo_RED_BOTTLE_e);}
        assert(!open_first()&&created==0&&rupees==100);
    }
    // Cancel and reopen retain the same offers and never charge or reroll.
    fresh();assert(begin_treasure_choice(0,&player));
    const auto savedOffers=s_chests[0].offers;const auto savedRarities=s_chests[0].rarities;const auto savedRng=s_run.random;
    dismiss_treasure_choice();assert(s_run.phase==Phase::Running&&rupees==100&&created==0);
    assert(begin_treasure_choice(0,&player));assert(s_chests[0].offers==savedOffers&&s_chests[0].rarities==savedRarities&&s_run.random==savedRng);
    window=1;cancel_treasure_demo();assert(window==0&&s_selectedChest==kChestCount&&s_run.phase==Phase::Running);
    // Banishing the first card must not prevent reopening the remaining offers.
    fresh();s_chests[0].offers={ChestReward::Count,ChestReward::Shield,ChestReward::Bow};
    const auto sparse=s_chests[0].offers;const auto sparseRng=s_run.random;
    assert(begin_treasure_choice(0,&player)&&s_chests[0].offers==sparse&&s_run.random==sparseRng);
    dismiss_treasure_choice();assert(begin_treasure_choice(0,&player)&&s_chests[0].offers==sparse);
    // Fully exhausted cached offers refresh, while partially usable ones stay fixed.
    fresh();s_chests[0].offers.fill(ChestReward::OrdonSword);s_run.dual_wield=30;s_run.dark_link=30;
    assert(begin_treasure_choice(0,&player));
    for(auto offer:s_chests[0].offers)assert(offer!=ChestReward::OrdonSword);
    // Revalidate affordability and chosen eligibility after the UI closes.
    fresh();assert(begin_treasure_choice(0,&player));rupees=0;
    unsigned zero=0;select_treasure(nullptr,&zero);update_treasure_choice(&player);
    assert(created==0&&s_run.phase==Phase::Running&&s_run.chests_opened==0);
    // Paid and free chests use the same Luck distribution, including high rarities.
    bool orange=false,purple=false;
    for(unsigned seed=1;seed<=100;++seed){
        fresh();s_run.start(seed);s_run.ranks[unsigned(Upgrade::Luck)]=20;
        assert(begin_treasure_choice(0,&player));
        const auto paid=s_chests[0].rarities;
        fresh();s_run.start(seed);s_run.ranks[unsigned(Upgrade::Luck)]=20;s_chests[0].free=true;rupees=0;
        assert(begin_treasure_choice(0,&player)&&s_chests[0].rarities==paid);
        for(auto rarity:paid){orange|=rarity==Rarity::Legendary;purple|=rarity==Rarity::Epic;}
    }
    assert(orange&&purple);
    fresh();s_chests[0].free=true;rupees=0;
    assert(chest_interactable(s_chests[0],&player));
    assert(begin_treasure_choice(0,&player));
    assert(s_chosenTreasure==ChestReward::Count);
    assert(s_chests[0].offers[1]!=ChestReward::Count && s_chests[0].offers[2]!=ChestReward::Count);
    Run expectedFree;expectedFree.start(123);expectedFree.roll_chest_offers(true);
    for(auto rarity:s_chests[0].rarities)assert(rarity==expectedFree.roll_rarity());
    update_treasure_choice(&player);assert(s_run.phase==Phase::TreasureChoice && created==0);
    const auto freeOffers=s_chests[0].offers;const auto freeRarities=s_chests[0].rarities;const auto freeRng=s_run.random;
    dismiss_treasure_choice();assert(rupees==0 && s_run.chests_opened==0);
    assert(begin_treasure_choice(0,&player) && s_chests[0].offers==freeOffers && s_chests[0].rarities==freeRarities && s_run.random==freeRng);
    unsigned freeChoice=2;select_treasure(nullptr,&freeChoice);
    update_treasure_choice(&player);assert(s_run.phase==Phase::Treasure && s_treasure.price==0);
    assert(s_chests[0].reward==freeOffers[2]);
    accept();
    update_treasure_demo(&player);eventEnded=true;update_treasure_demo(&player);
    assert(!s_chests[0].active && rupees==0 && s_run.chests_opened==0);
    fresh();player.shape_angle.y=0x4000;assert(chest_interactable(s_chests[0],&player));
    fresh();createFails=true;assert(!open_first());
    assert(currentLayer==&rootLayer&&s_run.phase==Phase::Running&&rupees==100);
    fresh();messageAvailable=false;assert(!open_first());
    assert(created==0&&rupees==100&&s_run.phase==Phase::Running);messageAvailable=true;
    fresh();player.trigger=true;
    assert(before_chest_action(nullptr,&player,&result,nullptr)==HOOK_CONTINUE);
    assert(created==0&&cleared==1&&s_run.phase==Phase::TreasureChoice&&rupees==100);
    unsigned chosenIndex=2;const auto picked=s_chests[0].offers[chosenIndex];
    window=1;select_treasure(nullptr,&chosenIndex);
    unsigned another=0;select_treasure(nullptr,&another);assert(s_chosenTreasure==picked);
    // Fade/another overlay must finish before the native event starts.
    window=1;update_treasure_choice(&player);assert(created==0);
    window=0;update_treasure_choice(&player);
    assert(created==1&&s_run.phase==Phase::Treasure&&s_chests[0].reward==picked);
    assert(!open_first());
    const auto reward=s_chests[0].reward;
    assert(!player.mAtCps[0].active && !player.mAtSph.active && player.mProcID==daAlink_c::PROC_WAIT);
    assert(!s_run.tick(true));
    update_treasure_demo(&player);assert(ordered==1&&prop.eventInfo.condition==dEvtCnd_CANGETITEM_e);
    before_treasure_get(nullptr,&player,nullptr,nullptr);assert(player.mDemo.p0==90);
    accept();before_treasure_get(nullptr,&player,nullptr,nullptr);
    assert(s_treasure.started&&itemPartner==prop.id&&player.mDemo.p0==0&&player.mDemo.p1==0);
    after_treasure_get(nullptr,&player,nullptr,nullptr);assert(!s_treasure.poseStarted);
    player.mProcID=daAlink_c::PROC_GET_ITEM;
    after_treasure_get(nullptr,&player,nullptr,nullptr);
    assert(s_treasure.poseStarted&&player.animation==daAlink_c::ANM_GET_A&&player.mProcVar1.field_0x300a==0);
    assert(player.field_0x32cc==treasure_message_id());
    player.animation=99;after_treasure_get(nullptr,&player,nullptr,nullptr);assert(player.animation==99);
    retain_treasure_actor(&prop,reinterpret_cast<void*>(execute));
    assert(prop.mAction==daDitem_c::ACTION_WAIT_LIGHT_END_e);
    update_treasure_demo(&player);assert(s_chests[0].opening&&s_chests[0].age==1&&rupees==100);
    const auto syncedBefore=inventorySyncs;
    eventEnded=true;update_treasure_demo(&player);assert(inventorySyncs==syncedBefore+1);
    assert(s_run.phase==Phase::Running&&rupees==80&&s_run.chest_cost()==25);
    assert(!s_chests[0].active&&s_run.chests_opened==1&&reset==1&&!alive);
    if (reward<=ChestReward::Ball) assert(s_run.rank(chest_weapon(reward))==1);
    update_treasure_demo(&player);assert(rupees==80&&s_run.chests_opened==1);
    // Keys pay first, even at zero rupees; only a successful native event
    // consumes one. Fixed map prices and free elite rewards keep their rules.
    for(bool fixed:{false,true})for(unsigned coins:{0u,100u}){
        fresh();s_chests[0].fixed=fixed;rupees=coins;nativeKeys=1;pendingKeys=1;
        assert(open_first()&&s_treasure.key&&s_treasure.price==0&&chest_keys()==2&&rupees==coins);
        accept();eventEnded=true;update_treasure_demo(&player);
        assert(chest_keys()==1&&pendingKeys==0&&rupees==coins&&!s_chests[0].active);
        assert(s_run.chests_opened==(fixed?0u:1u));
        update_treasure_demo(&player);assert(chest_keys()==1&&rupees==coins);
    }
    fresh();nativeKeys=1;s_chests[0].free=true;rupees=0;
    assert(open_first()&&!s_treasure.key);accept();eventEnded=true;update_treasure_demo(&player);
    assert(chest_keys()==1&&rupees==0&&s_run.chests_opened==0);
    fresh();s_run.start(25,{},starting);nativeKeys=1;rupees=0;begin_starting_item_choice();
    unsigned startKeyChoice=0;select_treasure(nullptr,&startKeyChoice);update_treasure_choice(&player);
    assert(chest_keys()==1&&!s_run.starting_item_pending&&s_run.chests_opened==0);
    for(unsigned failure=0;failure<5;++failure){
        fresh();nativeKeys=1;rupees=0;
        if(failure==0){createFails=true;assert(!open_first());}
        else {
            if(failure==4)s_chests[0].offers={ChestReward::RedPotion,ChestReward::Count,ChestReward::Count};
            assert(open_first());
            if(failure==1)cancel_treasure_demo();
            if(failure==2){s_treasure.waitFrames=300;update_treasure_demo(&player);}
            if(failure==3){nativeKeys=0;accept();eventEnded=true;update_treasure_demo(&player);}
            if(failure==4){bottles.fill(dItemNo_RED_BOTTLE_e);accept();eventEnded=true;update_treasure_demo(&player);}
        }
        assert(chest_keys()==(failure==3?0u:1u)&&rupees==0&&s_run.chests_opened==0&&s_chests[0].active);
    }
    fresh();nativeKeys=1;rupees=0;s_chests[0].offers={ChestReward::RedPotion,ChestReward::Count,ChestReward::Count};
    assert(open_first(Rarity::Legendary));accept();eventEnded=true;update_treasure_demo(&player);
    assert(chest_keys()==0&&rupees==20&&potionGrants==4&&s_run.chests_opened==1);
    // Pending counts clamp safely; a fresh run clears keys and pending pickups.
    nativeKeys=99;pendingKeys=2;assert(chest_keys()==99);set_chest_keys(0);assert(!chest_keys()&&!pendingKeys);
    nativeKeys=0;pendingKeys=-1;assert(chest_keys()==0);set_chest_keys(0);
    // A fixed-price chest checks and commits exactly 40 through the native event.
    fresh();s_chests[0].fixed=true;s_run.chests_opened=10;rupees=39;
    assert(!open_first()&&rupees==39&&s_run.chests_opened==10);
    rupees=40;assert(open_first());accept();eventEnded=true;update_treasure_demo(&player);
    assert(s_run.phase==Phase::Running&&rupees==0&&s_run.chests_opened==10&&!s_chests[0].active);
    update_treasure_demo(&player);assert(rupees==0&&s_run.chests_opened==10);
    // Pre-execute Open has priority even in damage/roll/sword/item procedures.
    fresh();player.mProcID=daAlink_c::OTHER;player.trigger=true;mDoCPd_c::pad.mPressedButtonFlags=PAD_BUTTON_A;
    s_swordAction=SwordAction::Normal;s_wavePending=true;s_comboStep=3;
    before_chest_action(nullptr,&player,&result,nullptr);
    assert(s_run.phase==Phase::TreasureChoice&&player.mProcID==daAlink_c::PROC_WAIT);
    assert(!mDoCPd_c::pad.mPressedButtonFlags&&!s_wavePending&&s_comboStep==0);
    // Sword ranks are chest-only and equip the native sword after presentation.
    fresh();s_run.banned_items.fill(true);s_run.banned_items[unsigned(ChestReward::MasterSword)]=false;
    assert(open_first()&&s_chests[0].reward==ChestReward::MasterSword);
    assert(prop.itemNo==dItemNo_MASTER_SWORD_e && swordGrants==0);
    accept();eventEnded=true;update_treasure_demo(&player);
    assert(s_run.master_sword&&equippedSword==dItemNo_MASTER_SWORD_e&&swordGrants==1&&!s_chests[0].active);
    update_treasure_demo(&player);assert(swordGrants==1&&rupees==80);
    // Light Sword unlocks at five and remains equipped through level 30.
    for(unsigned n=2;n<=30;++n){
        s_chests[0].active=true;s_chests[0].position={0,0,100};rupees=1000;eventEnded=false;
        assert(open_first());
        const auto expected=n>=5?dItemNo_LIGHT_SWORD_e:dItemNo_MASTER_SWORD_e;
        assert(prop.itemNo==expected);accept();eventEnded=true;update_treasure_demo(&player);
        assert(s_run.master_sword==n&&equippedSword==expected);
    }
    assert(!s_run.chest_rewards_left());
    fresh();s_run.banned_items.fill(true);s_run.banned_items[unsigned(ChestReward::Shield)]=false;
    for(unsigned n=1;n<=10;++n){
        s_chests[0].active=true;s_chests[0].position={0,0,100};rupees=1000;eventEnded=false;
        assert(open_first());
        const auto expected=n>=7?dItemNo_HYLIA_SHIELD_e:n>=4?dItemNo_SHIELD_e:dItemNo_WOOD_SHIELD_e;
        assert(prop.itemNo==expected);accept();eventEnded=true;update_treasure_demo(&player);
        assert(s_run.block_chance()==5*n&&equippedShield==expected);
    }
    assert(!s_run.chest_rewards_left());
    fresh();s_run.banned_items.fill(true);s_run.banned_items[unsigned(ChestReward::OrdonSword)]=false;s_run.dual_wield=29;
    assert(open_first()&&prop.itemNo==dItemNo_SWORD_e);
    accept();eventEnded=true;update_treasure_demo(&player);
    assert(s_run.dual_wield&&!s_run.chest_rewards_left());
    // Acquiring the other offhand item preserves the first, including native shield equipment.
    for(bool toShield:{true,false}){
        fresh();if(toShield)s_run.dual_wield=8;else {s_run.shield=8;equippedShield=dItemNo_HYLIA_SHIELD_e;}
        const auto reward=toShield?ChestReward::Shield:ChestReward::OrdonSword;
        s_chests[0].offers={reward,ChestReward::Count,ChestReward::Count};
        assert(open_first());assert(toShield?s_run.dual_wield==8:s_run.shield==8);
        accept();eventEnded=true;update_treasure_demo(&player);
        assert(s_run.shield==(toShield?1u:8u)&&s_run.dual_wield==(toShield?8u:1u)&&s_run.equipment_count()==2);
        assert(equippedShield==(toShield?dItemNo_WOOD_SHIELD_e:dItemNo_HYLIA_SHIELD_e));
    }
    // Dark Link is a deferred Poe Soul presentation, not a story Poe grant.
    fresh();assert(begin_treasure_choice(0,&player));s_chests[0].offers[0]=ChestReward::DarkLink;
    s_chests[0].rarities[0]=Rarity::Common;unsigned darkChoice=0;
    select_treasure(nullptr,&darkChoice);update_treasure_choice(&player);
    assert(prop.itemNo==dItemNo_POU_SPIRIT_e&&!s_run.dark_link&&rupees==100);
    accept();eventEnded=true;update_treasure_demo(&player);
    assert(s_run.dark_link==1&&rupees==80&&!s_chests[0].active&&swordGrants==0&&potionGrants==0);
    // Potion rewards fill a new slot first, even with an existing empty bottle.
    fresh();s_run.banned_items.fill(true);s_run.banned_items[unsigned(ChestReward::RedPotion)]=false;rupees=1000;
    for(unsigned n=0;n<4;++n){
        s_chests[0]={};s_chests[0].active=true;s_chests[0].position={0,0,100};
        if(n>0)bottles[0]=dItemNo_EMPTY_BOTTLE_e;
        assert(potion_slot()==int(SLOT_11+n));
        eventEnded=false;
        assert(open_first()&&s_chests[0].reward==ChestReward::RedPotion);
        assert(prop.itemNo==dItemNo_RED_BOTTLE_e);
        accept();eventEnded=true;update_treasure_demo(&player);
        assert(bottles[n]==dItemNo_RED_BOTTLE_e && potionGrants==int(n+1)&&!s_chests[0].active);
        assert(s_run.red_potion&&s_run.equipment_count()==1&&s_run.potion_heal_quarters()==4);
        if(n>0)assert(bottles[0]==dItemNo_EMPTY_BOTTLE_e);
    }
    assert(rupees==882&&s_run.chest_cost()==52&&potion_slot()==SLOT_11);
    s_chests[0].active=true;s_chests[0].position={0,0,100};
    assert(open_first());accept();update_treasure_demo(&player);
    assert(potionGrants==5&&potion_slot()==-1&&rupees==830);
    s_chests[0].active=true;s_chests[0].position={0,0,100};
    assert(!open_first()); // Four full bottles: no wasted paid reward.
    bottles[2]=dItemNo_EMPTY_BOTTLE_e;
    assert(open_first());accept();update_treasure_demo(&player);
    assert(bottles[2]==dItemNo_RED_BOTTLE_e&&potionGrants==6&&rupees==765);
    // Another system filling the reserved bottle during loading must not be overwritten.
    s_chests[0].active=true;s_chests[0].position={0,0,100};bottles[1]=dItemNo_EMPTY_BOTTLE_e;
    assert(open_first());bottles[1]=99;
    accept();update_treasure_demo(&player);
    assert(bottles[1]==99&&rupees==765&&s_run.chests_opened==6&&s_chests[0].active);
    // Orange potion grants four bottles, then pays the fifth unit as rupees.
    fresh();s_chests[0].offers={ChestReward::RedPotion,ChestReward::Count,ChestReward::Count};
    assert(open_first(Rarity::Legendary)&&s_treasure.potions==4&&s_treasure.bonusRupees==20);
    assert(potionGrants==0&&rupees==100);accept();eventEnded=true;update_treasure_demo(&player);
    assert(potionGrants==4&&potion_capacity()==0&&rupees==100&&s_run.chests_opened==1);
    for(auto item:bottles)assert(item==dItemNo_RED_BOTTLE_e);
    update_treasure_demo(&player);assert(potionGrants==4&&rupees==100);
    // A full bottle is never overwritten; cancelled multi-potion events pay nothing.
    fresh();bottles[0]=99;s_chests[0].offers={ChestReward::RedPotion,ChestReward::Count,ChestReward::Count};
    assert(open_first(Rarity::Rare)&&s_treasure.potions==3);
    bottles[1]=98;accept();eventEnded=true;update_treasure_demo(&player);
    assert(bottles[0]==99&&bottles[1]==98&&potionGrants==0&&rupees==100&&s_run.chests_opened==0);
    // Multi-rank shield evolution is displayed and committed once; price rises once.
    fresh();s_run.shield=5;s_chests[0].offers={ChestReward::Shield,ChestReward::Count,ChestReward::Count};
    assert(open_first(Rarity::Epic)&&prop.itemNo==dItemNo_HYLIA_SHIELD_e);
    accept();eventEnded=true;update_treasure_demo(&player);
    assert(s_run.shield==9&&equippedShield==dItemNo_HYLIA_SHIELD_e&&rupees==80&&s_run.chest_cost()==25);
    // Only committed item changes queue a reload; the dedicated reload fixture
    // exercises native resource phases and collision/draw safety after each tick.
    for(unsigned rank:{0u,1u,3u,4u,6u,7u}) {
        fresh();s_run.shield=rank;
        equippedShield=rank?shield_item(s_run):u8(dItemNo_NONE_e);
        const int oldShield=equippedShield;
        s_chests[0].offers={ChestReward::Shield,ChestReward::Count,ChestReward::Count};
        assert(open_first());accept();update_treasure_demo(&player);
        assert(equippedShield==oldShield&&shieldRequests==0);
        eventEnded=true;update_treasure_demo(&player);
        assert(s_run.shield==rank+1&&equippedShield==shield_item(s_run));
        assert(shieldRequests==unsigned(equippedShield!=oldShield));
        update_treasure_demo(&player);
        assert(shieldRequests==unsigned(equippedShield!=oldShield));
    }
    fresh();s_run.shield=6;equippedShield=dItemNo_SHIELD_e;
    s_chests[0].offers={ChestReward::Shield,ChestReward::Count,ChestReward::Count};
    assert(open_first());cancel_treasure_demo();
    assert(s_run.shield==6&&equippedShield==dItemNo_SHIELD_e&&shieldRequests==0);
    // Async failure, order timeout, interrupted event: no charge or reward.
    for(unsigned reason=0;reason<3;++reason){
        fresh();assert(open_first());
        if(reason==0)alive=false;
        if(reason==1)s_treasure.waitFrames=300;
        if(reason==2){accept();before_treasure_get(nullptr,&player,nullptr,nullptr);event.mPt2=999;}
        update_treasure_demo(&player);
        assert(s_run.phase==Phase::Running&&rupees==100&&s_run.chests_opened==0&&s_chests[0].active);
        if(reason==2)assert(reset==0&&event.mPt2==999);
    }
    fresh();assert(open_first());creating=true;doing=true;
    tag.base.mpTagData=&request;g_fpcCtTg_Queue.mpHead=&tag.base.node;
    cancel_treasure_demo();assert(s_treasure.retiring&&creating);
    doing=false;retire_treasure_actor();assert(!creating&&s_treasure.item==kNone);
    fresh();assert(open_first());deleteAllowed=false;
    cancel_treasure_demo();assert(s_treasure.retiring&&alive);
    deleteAllowed=true;retire_treasure_actor();assert(!alive&&s_treasure.item==kNone);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+"\nbool hub_gear_interaction(daAlink_c*){return false;}\n"+source+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors treasure: affordability, native interaction, item demo, single commit and cleanup passed')
