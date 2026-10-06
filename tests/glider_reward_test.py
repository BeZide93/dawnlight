"""Run production reward lifecycle callbacks against a fake native event/actor host."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/glider_reward.cpp').read_text()

def function(signature):
    start = source.index(signature + '(')
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

fixture = r'''
#include <cassert>
#include <cstdint>
using u8=uint8_t;
using ActorId=int;
constexpr int fpcM_ERROR_PROCESS_ID_e=-1, fpcNm_Demo_Item_e=20;
constexpr int dItemNo_BOMB_5_e=10, dEvtCnd_CANGETITEM_e=8;
struct ModContext{};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct Vec {float x=0,y=0,z=0;};
struct leafdraw_class {int id=0;};
struct layer_class {bool deleting=false;};
layer_class rootLayer, sceneLayer, callbackLayer;
layer_class* currentLayer=&rootLayer;
layer_class* fpcLy_RootLayer(){return &rootLayer;}
layer_class* fpcLy_CurrentLayer(){return currentLayer;}
void fpcLy_SetCurrentLayer(layer_class* layer){currentLayer=layer;}
bool fpcLy_IsDeletingMesg(layer_class* layer){return layer->deleting;}
struct Actor:leafdraw_class {int name=0;struct {layer_class* layer=&sceneLayer;} layer_tag;};
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
    const process_method_class* sub_method=&methods;int mAction=ACTION_END_e;
    void setAction(int a){mAction=a;}
    EventInfo eventInfo; bool visible=true,dead=false;
    bool chkDraw(){return visible;} bool chkDead(){return dead;}
} prop;
struct daAlink_c:Actor {
    enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_TIRED_WAIT,PROC_GET_ITEM,OTHER,ANM_GET_A};
    int mProcID=PROC_WAIT,animation=-1;bool wolf=false,dead=false,inEvent=false;
    struct {Vec pos;} current;
    struct {int p0=90,p1=90;void setParam0(int n){p0=n;} void setParam1(int n){p1=n;}} mDemo;
    struct {int field_0x300a=-7;} mProcVar1;
    unsigned field_0x32cc=0;
    bool checkWolf(){return wolf;}bool checkDeadHP(){return dead;}bool checkEventRun(){return inEvent;}
    void setSingleAnimeBase(int a){animation=a;}
} player;
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
void dComIfGp_event_setGtItm(int item){assert(item==dItemNo_BOMB_5_e);}
bool dComIfGp_evmng_endCheck(const char*){return eventEnded;}
void dComIfGp_event_reset(){++reset;running=false;event.mPt2=-1;}
int fopAcM_GetRoomNo(Actor*){return 0;}
int fopAcM_delete(Actor*){if(!deleteAllowed)return 0;alive=false;++deleted;return 1;}
int fopAcM_createDemoItem(Vec*,int item,int bit,void*,int,void*,u8 flags){
    assert(item==dItemNo_BOMB_5_e&&bit==-1&&flags==1); // Presentation-only, no inventory grant.
    assert(currentLayer==player.layer_tag.layer&&currentLayer!=&rootLayer);
    ++created;if(createFails)return -1;
    alive=true;prop.id=100;prop.name=fpcNm_Demo_Item_e;return prop.id;
}
int fopAcM_orderItemEvent(daDitem_c* p,int,int){assert(p==&prop);++ordered;return 1;}
bool progression_system_enabled(){return mode;}
void clear_glider_reward_visual(){++cleared;}
void queue_glider_reward_visual(daAlink_c* l,int id){assert(l==&player&&id==prop.id);++drawn;}
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
namespace flow {struct RegisteredMessage {unsigned id(){return 9000;}void reset(){}};}
}
void cancel_glider_reward();
// PRODUCTION
void fresh(){
    s_reward={};prop={};player={};player.id=1;event.mPt2=-1;
    currentLayer=&rootLayer;sceneLayer.deleting=false;
    alive=creating=doing=running=paused=talking=stageChange=eventEnded=createFails=false;
    mode=playerPresent=deleteAllowed=true;window=pauseStatus=0;
    created=ordered=reset=deleted=drawn=cleared=0;itemPartner=-1;
    g_fpcCtTg_Queue.mpHead=nullptr;
}
void accept(){event.mPt2=prop.id;prop.eventInfo.command=true;running=true;}
int main(){
    fresh();queue_glider_reward();queue_glider_reward();
    // Wait until the original dialogue and event both finish.
    talking=true;update_glider_reward();assert(created==0&&s_reward.pending);
    talking=false;running=true;update_glider_reward();assert(created==0);
    running=false;window=1;update_glider_reward();assert(created==0);window=0;
    player.wolf=true;update_glider_reward();assert(created==0);player.wolf=false;
    player.mProcID=daAlink_c::OTHER;update_glider_reward();assert(created==0);
    player.mProcID=daAlink_c::PROC_WAIT;
    update_glider_reward();assert(created==1&&ordered==0&&currentLayer==&rootLayer);
    update_glider_reward();assert(ordered==1&&prop.eventInfo.condition==dEvtCnd_CANGETITEM_e);
    // Another actor's reward cannot alter Link's parameters or message.
    before_get_init(nullptr,&player,nullptr,nullptr);assert(player.mDemo.p0==90);
    accept();before_get_init(nullptr,&player,nullptr,nullptr);
    assert(s_reward.started&&itemPartner==prop.id&&player.mDemo.p0==0&&player.mDemo.p1==0);
    after_get_init(nullptr,&player,nullptr,nullptr);assert(!s_reward.poseStarted); // Unequip first.
    player.mProcID=daAlink_c::PROC_GET_ITEM;after_get_init(nullptr,&player,nullptr,nullptr);
    assert(s_reward.poseStarted&&player.animation==daAlink_c::ANM_GET_A);
    assert(player.mProcVar1.field_0x300a==0&&player.field_0x32cc==9000);
    player.animation=99;after_get_init(nullptr,&player,nullptr,nullptr);assert(player.animation==99);
    dispatchArg=&prop;
#if defined(__APPLE__)
    void* methodArg=reinterpret_cast<void*>(execute);
#else
    void* methodArg=&methods;
#endif
    before_item_execute(nullptr,methodArg,nullptr,nullptr);
    assert(prop.mAction==daDitem_c::ACTION_WAIT_LIGHT_END_e); // Keep event owner until camera ends.
    prop.mAction=daDitem_c::ACTION_END_e;dispatchArg=&player;
    before_item_execute(nullptr,methodArg,nullptr,nullptr);assert(prop.mAction==daDitem_c::ACTION_END_e);
    dispatchArg=&prop;s_reward.retiring=true;before_item_execute(nullptr,methodArg,nullptr,nullptr);
    assert(prop.mAction==daDitem_c::ACTION_END_e);s_reward.retiring=false;
    int result=0;assert(before_item_draw(nullptr,&prop,&result,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(result==1&&drawn==1);
    daDitem_c realBomb;realBomb.id=101;
    assert(before_item_draw(nullptr,&realBomb,&result,nullptr)==HOOK_CONTINUE&&drawn==1);
    prop.dead=true;before_item_draw(nullptr,&prop,&result,nullptr);assert(drawn==1);prop.dead=false;
    mode=false;update_glider_reward();assert(alive&&reset==0); // Active sequence finishes even if disabled.
    eventEnded=true;update_glider_reward();assert(reset==1&&!alive&&s_reward.item==-1);
    update_glider_reward();assert(created==1); // No repeated reward after completion.

    fresh();queue_glider_reward();mode=false;update_glider_reward();assert(created==0&&!s_reward.pending);
    fresh();queue_glider_reward();update_glider_reward();accept();mode=false;
    update_glider_reward();assert(s_reward.started&&alive); // Accepted before Init callback: don't cancel.
    fresh();queue_glider_reward();update_glider_reward();event.mPt2=999;running=true;
    cancel_glider_reward();assert(reset==0&&!alive); // Save callback never resets another save's event.
    fresh();queue_glider_reward();update_glider_reward();player.id=2;update_glider_reward();
    assert(!alive&&reset==0); // Actor/scene change.
    fresh();queue_glider_reward();update_glider_reward();
    for(int i=0;i<kOrderTimeout+2;++i)update_glider_reward();
    assert(!alive&&s_reward.item==-1&&created==1); // Event never accepted.
    // A global mod callback must create the prop in Link's scene, then restore
    // the caller's layer, including when native actor creation fails.
    fresh();queue_glider_reward();currentLayer=&callbackLayer;
    update_glider_reward();assert(created==1&&currentLayer==&callbackLayer);
    fresh();queue_glider_reward();player.layer_tag.layer=nullptr;
    update_glider_reward();assert(created==0&&s_reward.pending&&currentLayer==&rootLayer);
    player.layer_tag.layer=&rootLayer;
    update_glider_reward();assert(created==0&&s_reward.pending);
    player.layer_tag.layer=&sceneLayer;sceneLayer.deleting=true;
    update_glider_reward();assert(created==0&&s_reward.pending);
    sceneLayer.deleting=false;update_glider_reward();assert(created==1&&!s_reward.pending);
    fresh();createFails=true;queue_glider_reward();update_glider_reward();update_glider_reward();assert(created==1&&currentLayer==&rootLayer);
    fresh();queue_glider_reward();update_glider_reward();deleteAllowed=false;cancel_glider_reward();
    assert(s_reward.retiring&&alive);deleteAllowed=true;update_glider_reward();assert(!alive&&s_reward.item==-1);
    fresh();queue_glider_reward();update_glider_reward();creating=true;doing=true;
    tag.base.mpTagData=&request;g_fpcCtTg_Queue.mpHead=&tag.base.node;
    cancel_glider_reward();assert(s_reward.retiring&&creating);
    doing=false;update_glider_reward();assert(!creating&&!alive&&s_reward.item==-1);
}
'''
state = source[source.index('constexpr ActorId kNoActor'):source.index('// Never identify')]
names = ['daDitem_c* reward_actor','bool owns_event','bool safe_to_start','void retire_prop',
         'HookAction before_item_execute','HookAction before_get_init','void after_get_init','HookAction before_item_draw',
         'void queue_glider_reward','void update_glider_reward','void cancel_glider_reward']
fixture=fixture.replace('// PRODUCTION',state+'\n'+'\n'.join(function(n) for n in names))
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'reward.cpp',Path(tmp)/'reward'
    cpp.write_text(fixture)
    for extra in ([], ['-D__APPLE__']):
        subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',*extra,str(cpp),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print('Glider reward: scene ownership/layer restoration, dialogue safety, native pose/message, prop isolation and cleanup passed')
