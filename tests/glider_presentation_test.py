"""Exercise production glider presentation with simulated host matrices/shapes.
Run: python3 tests/glider_presentation_test.py. No game assets required.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
visual = (ROOT / 'src/glider_visual.cpp').read_text()
presentation = (ROOT / 'src/glide_presentation.inc').read_text()

def function(source, signature):
    start = re.search(re.escape(signature) + r'\([^;{}]*\)\s*\{', source).start()
    return source[start:source.index('\n}', start) + 2]

fixture = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
using u8=unsigned char;
using Mtx=float[3][4];
using MtxP=float (*)[4];
void MTXCopy(MtxP a,MtxP b){std::memcpy(b,a,sizeof(Mtx));}
struct Vec {float x=0,y=0,z=0;void set(float a,float b,float c){x=a;y=b;z=c;}};
struct Shape {bool visible=false;void hide(){visible=false;}void show(){visible=true;}};
struct Material {Shape shape;Shape* getShape(){return &shape;}};
struct Data {
    int count=11;Material materials[11];
    int getMaterialNum(){return count;}
    Material* getMaterialNodePointer(int i){assert(i<count);return &materials[i];}
};
struct Model {
    Mtx joints[5]{};Data data;
    MtxP getAnmMtx(int i){return joints[i];}
    Data* getModelData(){return &data;}
};
struct Actor {} owned,ordinary;
struct daAlink_c: Actor {
    enum {PROC_GET_ITEM=5}; int mProcID=0;
    Model body,hands;Model* mpLinkModel=&body;Model* mpLinkHandModel=&hands;
    int mLeftHandJntNo=1,mRightHandJntNo=2;
    int mLeftItemJntNo=3,mRightItemJntNo=4;
    Shape* field_0x06d0=&hands.data.materials[4].shape;
    Shape* field_0x06d4=&hands.data.materials[10].shape;
    bool owner=true,airborne=true,hidden=false,status=false;
    struct {Actor* actor=&owned;Actor* getActor(){return actor;}} mGrabItemAcKeep;
    struct {short y=0;} shape_angle;
    struct {struct {int r=10,g=20,b=30;} AmbCol;} tevStr;
    bool checkPlayerNoDraw(){return hidden;}
    bool checkStatusWindowDraw(){return status;}
    void nativeOpenHands(){
        field_0x06d0->hide();field_0x06d4->hide();
        field_0x06d0=&hands.data.materials[4].shape;
        field_0x06d4=&hands.data.materials[10].shape;
        field_0x06d0->show();field_0x06d4->show();
    }
};
using ActorId=int;
constexpr int fpcNm_ALINK_e=1,fpcNm_Demo_Item_e=2,fpcM_ERROR_PROCESS_ID_e=-1;
struct daDitem_c:Actor {
    bool visible=true,dead=false;Vec scale{1,1,1};
    bool chkDraw(){return visible;}bool chkDead(){return dead;}
} rewardItem;
bool rewardAlive=true;
daAlink_c* liveLink=nullptr;
int fopAcM_GetID(daAlink_c*){return 42;}
Actor* fopAcM_SearchByID(int id){if(id==43&&rewardAlive)return &rewardItem;return id==42?liveLink:nullptr;}
daAlink_c* daAlink_getAlinkActorClass(){return liveLink;}
int fopAcM_GetName(Actor* actor){return actor==liveLink?fpcNm_ALINK_e:actor==&rewardItem?fpcNm_Demo_Item_e:0;}
struct ModContext {};
namespace mods {template<class T>T arg(void* p,int){return static_cast<T>(p);}}
struct {bool attached=true,retiring=false;} s_jumpAbilities;
enum class GlideItem {Cucco,Glider};
GlideItem item=GlideItem::Glider;
GlideItem glide_item(){return item;}
bool enabled=true,live=true;
bool glide_enabled(){return enabled;}
bool owns_jump_abilities(daAlink_c* l){return l&&l->owner;}
bool aerial_glide_context(daAlink_c* l){return l->airborne;}
Actor* glide_actor(){return live?&owned:nullptr;}
// HANDS

struct J3DPacket {
    J3DPacket* next=nullptr;
    J3DPacket* getNextPacket(){return next;}
    void setNextPacket(J3DPacket* p){next=p;}
};
struct GliderPacket:J3DPacket {int ownerId=-1;Vec origin;float sine=0,cosine=1;struct {u8 r,g,b,a;} color;bool active=false,reward=false;int rewardItem=-1;float modelScale=1;} s_glider,s_rewardGlider;
struct List {
    int entries=0;J3DPacket* heads[1]{};J3DPacket** mpBuffer=heads;
    unsigned getEntryTableSize(){return 1;}
    // Match native J3DDrawBuffer: inserting the same packet twice makes a cycle.
    void entryImm(J3DPacket* packet,int i){
        ++entries;packet->setNextPacket(mpBuffer[i]);mpBuffer[i]=packet;
    }
    void frameInit(){mpBuffer[0]=nullptr;}
    int drawCount(){
        int count=0;
        for(auto* p=mpBuffer[0];p;p=p->getNextPacket())assert(++count<=4);
        return count;
    }
} list;
List* dComIfGd_getOpaList(){return &list;}
using LookupPresentationMatrix=bool (*)(const void*,Mtx);
LookupPresentationMatrix s_lookupPresentationMatrix=nullptr;
// QUEUE_ONCE
// PRESENTED_MATRIX
// PREPARE
// CLEAR
// QUEUE
// REWARD

Model* sampledModel=nullptr;
Mtx replacements[5]{};
bool interpolation=true;
bool lookup(const void* key,Mtx out){
    if(!interpolation)return false;
    for(int i=0;i<5;++i)if(key==sampledModel->joints[i]){MTXCopy(replacements[i],out);return true;}
    assert(false);return false;
}
void yawMatrix(Mtx m,float angle){
    std::memset(m,0,sizeof(Mtx));
    m[0][0]=m[2][2]=std::cos(angle);m[0][2]=std::sin(angle);m[2][0]=-std::sin(angle);m[1][1]=1;
}
bool near(float a,float b){return std::abs(a-b)<0.001f;}
int main(){
    daAlink_c link;liveLink=&link;sampledModel=&link.body;s_lookupPresentationMatrix=lookup;
    // Link's actor draw runs ONCE, outside presentation. The retained packet
    // must resample each later render without queueing/re-running Link::draw.
    interpolation=false;queue_glider_visual(&link);assert(list.entries==1);
    interpolation=true;
    yawMatrix(link.body.joints[0],0);
    for(int i=1;i<3;++i){yawMatrix(link.body.joints[i],0);link.body.joints[i][0][3]=100+(i==1?-22:22);link.body.joints[i][1][3]=80;}
    // Distinct wrist and grip points: the canopy must follow the latter.
    for(int i=3;i<5;++i){MTXCopy(link.body.joints[i-2],link.body.joints[i]);link.body.joints[i][1][3]=89;link.body.joints[i][2][3]=-3;}
    for(float t:{0.f,.25f,.5f,.75f,1.f}){
        for(int i=0;i<5;++i)MTXCopy(link.body.joints[i],replacements[i]);
        for(int i=1;i<5;++i)replacements[i][0][3]-=20*(1-t);
        assert(prepare_glider_draw());
        assert(list.entries==1);
        assert(near(s_glider.origin.x,80+20*t)&&near(s_glider.origin.y,89)&&near(s_glider.origin.z,-3));
    }
    // No interpolation/history (including first frame): use current joints.
    interpolation=false;assert(prepare_glider_draw());assert(near(s_glider.origin.x,100)&&near(s_glider.origin.y,89));
    // Ambient tint must preserve readable texture midtones, even in shadow.
    for(int ambient:{0,64,128,255}){
        link.tevStr.AmbCol={ambient,ambient,ambient};assert(prepare_glider_draw());
        assert(s_glider.color.r>=160&&s_glider.color.g>=160&&s_glider.color.b>=160);
    }
    assert(s_glider.color.r==255);
    s_lookupPresentationMatrix=nullptr;assert(prepare_glider_draw());assert(near(s_glider.origin.x,100)&&near(s_glider.origin.y,89));
    s_lookupPresentationMatrix=lookup;interpolation=true;
    // Turn through the signed-angle boundary: presented facing is still -Z,
    // not an unrelated half-turn or the latest simulation yaw.
    const float pi=3.14159265358979323846f;
    link.shape_angle.y=-32000;
    yawMatrix(link.body.joints[0],link.shape_angle.y*pi/32768);
    yawMatrix(replacements[0],pi);
    assert(prepare_glider_draw());assert(near(s_glider.sine,0)&&near(s_glider.cosine,-1));
    // Root animation may point a basis vector vertically; use the full rotation.
    yawMatrix(link.body.joints[0],0);
    std::swap(link.body.joints[0][1][1],link.body.joints[0][1][2]);
    std::swap(link.body.joints[0][2][1],link.body.joints[0][2][2]);
    link.body.joints[0][2][1]=-1;
    MTXCopy(link.body.joints[0],replacements[0]);link.shape_angle.y=0;
    assert(prepare_glider_draw());assert(near(s_glider.sine,0)&&near(s_glider.cosine,1));
    int entries=list.entries;link.hidden=true;assert(!prepare_glider_draw());assert(entries==list.entries);link.hidden=false;
    liveLink=nullptr;assert(!prepare_glider_draw());liveLink=&link;
    clear_glider_visual();assert(!prepare_glider_draw());assert(s_glider.ownerId==-1);

    // Reward uses a separate retained packet; clearing ordinary Glide must
    // not clear it. Re-evaluate visibility/actor lifetime on every render.
    link.mProcID=daAlink_c::PROC_GET_ITEM;
    queue_glider_reward_visual(&link,43);
    assert(s_rewardGlider.active&&s_rewardGlider.reward);
    clear_glider_visual();assert(prepare_glider_reward_draw());
    assert(near(s_rewardGlider.modelScale,.45f)&&near(s_rewardGlider.origin.y,89));
    rewardItem.scale.x=.5f;assert(prepare_glider_reward_draw());assert(near(s_rewardGlider.modelScale,.225f));
    rewardItem.visible=false;assert(!prepare_glider_reward_draw());rewardItem.visible=true;
    rewardItem.dead=true;assert(!prepare_glider_reward_draw());rewardItem.dead=false;
    rewardAlive=false;assert(!prepare_glider_reward_draw());rewardAlive=true;
    link.mProcID=0;assert(!prepare_glider_reward_draw());link.mProcID=daAlink_c::PROC_GET_ITEM;
    clear_glider_reward_visual();assert(!prepare_glider_reward_draw());
    assert(s_rewardGlider.ownerId==-1&&s_rewardGlider.rewardItem==-1);

    // Repeated native actor callbacks must not turn the intrusive packet
    // chain into an endless draw loop, even with another packet in between.
    list.frameInit();int before=list.entries;
    queue_glider_reward_visual(&link,43);
    queue_glider_reward_visual(&link,43);
    assert(list.entries==before+1&&list.drawCount()==1);
    J3DPacket other;list.entryImm(&other,0);
    queue_glider_reward_visual(&link,43);
    assert(list.entries==before+2&&list.drawCount()==2);
    clear_glider_reward_visual();queue_glider_reward_visual(&link,43);
    assert(list.entries==before+2&&list.drawCount()==2);
    queue_glider_visual(&link);queue_glider_visual(&link);
    assert(list.entries==before+3&&list.drawCount()==3);
    // Presentation-only draws keep the list; a new simulation frame resets it.
    assert(list.drawCount()==3&&list.drawCount()==3);
    list.frameInit();queue_glider_reward_visual(&link,43);
    queue_glider_visual(&link);assert(list.entries==before+5&&list.drawCount()==2);

    link.nativeOpenHands();after_glider_draw_hand(nullptr,&link,nullptr,nullptr);
    assert(link.field_0x06d0==&link.hands.data.materials[0].shape);
    assert(link.field_0x06d4==&link.hands.data.materials[6].shape);
    assert(link.field_0x06d0->visible&&link.field_0x06d4->visible);
    assert(!link.hands.data.materials[4].shape.visible&&!link.hands.data.materials[10].shape.visible);
    auto unchanged=[&]{
        link.nativeOpenHands();after_glider_draw_hand(nullptr,&link,nullptr,nullptr);
        assert(link.field_0x06d0==&link.hands.data.materials[4].shape&&link.field_0x06d0->visible);
        assert(link.field_0x06d4==&link.hands.data.materials[10].shape&&link.field_0x06d4->visible);
        assert(!link.hands.data.materials[0].shape.visible&&!link.hands.data.materials[6].shape.visible);
    };
    link.airborne=false;unchanged();link.airborne=true;
    item=GlideItem::Cucco;unchanged();item=GlideItem::Glider;
    link.mGrabItemAcKeep.actor=&ordinary;unchanged();link.mGrabItemAcKeep.actor=&owned;
    s_jumpAbilities.retiring=true;unchanged();s_jumpAbilities.retiring=false;
    s_jumpAbilities.attached=false;unchanged();s_jumpAbilities.attached=true;
    enabled=false;unchanged();enabled=true;
    link.status=true;unchanged();link.status=false;
    link.hidden=true;unchanged();link.hidden=false;
    link.owner=false;unchanged();link.owner=true;
    live=false;unchanged();live=true;
    link.hands.data.count=6;unchanged();
    assert(!custom_glider_active(nullptr));
}
'''
fixture = fixture.replace('// HANDS', presentation[:presentation.index('HookAction before_glide_carrier_draw')])
fixture = fixture.replace('// QUEUE_ONCE', function(visual, 'void queue_glider_packet'))
fixture = fixture.replace('// PRESENTED_MATRIX', function(visual, 'void presented_matrix'))
fixture = fixture.replace('// PREPARE', function(visual, 'bool prepare_glider_pose') + '\n' + function(visual, 'bool prepare_glider_draw'))
fixture = fixture.replace('// CLEAR', function(visual, 'void clear_glider_visual'))
fixture = fixture.replace('// QUEUE', function(visual, 'void queue_glider_visual'))
fixture = fixture.replace('// REWARD', '\n'.join(function(visual, name) for name in ['bool prepare_glider_reward_draw', 'void clear_glider_reward_visual', 'void queue_glider_reward_visual']))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Glider presentation passed: duplicate-safe queue, frame reset/replay, grip anchors, turning, fallback and cleanup')
