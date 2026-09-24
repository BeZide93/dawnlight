"""Exercise production glider presentation with simulated host matrices/shapes.
Run: python3 tests/glider_presentation_test.py. No game assets required.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
visual = (ROOT / 'src/glider_visual.cpp').read_text()
presentation = (ROOT / 'src/glide_presentation.inc').read_text()

def function(source, signature):
    start = source.index(signature + '(')
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
    Mtx joints[3]{};Data data;
    MtxP getAnmMtx(int i){return joints[i];}
    Data* getModelData(){return &data;}
};
struct Actor {} owned,ordinary;
struct daAlink_c {
    Model body,hands;Model* mpLinkModel=&body;Model* mpLinkHandModel=&hands;
    int mLeftHandJntNo=1,mRightHandJntNo=2;
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

struct Packet {Vec origin;float sine=0,cosine=1;struct {u8 r,g,b,a;} color;bool active=false;} s_glider;
struct List {int entries=0;void entryImm(Packet*,int){++entries;}} list;
List* dComIfGd_getOpaList(){return &list;}
using LookupPresentationMatrix=bool (*)(const void*,Mtx);
LookupPresentationMatrix s_lookupPresentationMatrix=nullptr;
// PRESENTED_MATRIX
// QUEUE

Model* sampledModel=nullptr;
Mtx replacements[3]{};
bool interpolation=true;
bool lookup(const void* key,Mtx out){
    if(!interpolation)return false;
    for(int i=0;i<3;++i)if(key==sampledModel->joints[i]){MTXCopy(replacements[i],out);return true;}
    assert(false);return false;
}
void yawMatrix(Mtx m,float angle){
    std::memset(m,0,sizeof(Mtx));
    m[0][0]=m[2][2]=std::cos(angle);m[0][2]=std::sin(angle);m[2][0]=-std::sin(angle);m[1][1]=1;
}
bool near(float a,float b){return std::abs(a-b)<0.001f;}
int main(){
    daAlink_c link;sampledModel=&link.body;s_lookupPresentationMatrix=lookup;
    yawMatrix(link.body.joints[0],0);
    for(int i=1;i<3;++i){yawMatrix(link.body.joints[i],0);link.body.joints[i][0][3]=100+(i==1?-22:22);link.body.joints[i][1][3]=80;}
    for(float t:{0.f,.25f,.5f,.75f,1.f}){
        for(int i=0;i<3;++i)MTXCopy(link.body.joints[i],replacements[i]);
        for(int i=1;i<3;++i)replacements[i][0][3]-=20*(1-t);
        queue_glider_visual(&link);
        assert(near(s_glider.origin.x,80+20*t)&&near(s_glider.origin.y,80));
    }
    // No interpolation/history (including first frame): use current joints.
    interpolation=false;queue_glider_visual(&link);assert(near(s_glider.origin.x,100));
    s_lookupPresentationMatrix=nullptr;queue_glider_visual(&link);assert(near(s_glider.origin.x,100));
    s_lookupPresentationMatrix=lookup;interpolation=true;
    // Turn through the signed-angle boundary: presented facing is still -Z,
    // not an unrelated half-turn or the latest simulation yaw.
    const float pi=3.14159265358979323846f;
    link.shape_angle.y=-32000;
    yawMatrix(link.body.joints[0],link.shape_angle.y*pi/32768);
    yawMatrix(replacements[0],pi);
    queue_glider_visual(&link);assert(near(s_glider.sine,0)&&near(s_glider.cosine,-1));
    // Root animation may point a basis vector vertically; use the full rotation.
    yawMatrix(link.body.joints[0],0);
    std::swap(link.body.joints[0][1][1],link.body.joints[0][1][2]);
    std::swap(link.body.joints[0][2][1],link.body.joints[0][2][2]);
    link.body.joints[0][2][1]=-1;
    MTXCopy(link.body.joints[0],replacements[0]);link.shape_angle.y=0;
    queue_glider_visual(&link);assert(near(s_glider.sine,0)&&near(s_glider.cosine,1));
    int entries=list.entries;link.hidden=true;queue_glider_visual(&link);assert(entries==list.entries);link.hidden=false;

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
fixture = fixture.replace('// PRESENTED_MATRIX', function(visual, 'void presented_matrix'))
fixture = fixture.replace('// QUEUE', function(visual, 'void queue_glider_visual'))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Glider presentation passed: intermediate hand poses, turning, fallback, grip shapes and cleanup')
