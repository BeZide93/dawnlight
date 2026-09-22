"""Run the actual trial collision/timing methods against instrumented native APIs.

The fixture does not load game archives or duplicate the trial implementation.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/heroes_shade_trials.inc").read_text()
runtime = source[source.index("struct ShadeTrials {"):]

def method(signature):
    start = runtime.index("    " + signature)
    end = runtime.index("\n    }", start) + len("\n    }")
    return runtime[start:end]

fixture = r'''
#include "heroes_shade_battle.hpp"
#include <cassert>
#include <vector>
using namespace dawnlight;
struct cXyz {
    float x=0,y=0,z=0;
    cXyz()=default;
    cXyz(float a,float b,float c):x(a),y(b),z(c) {}
    cXyz operator+(cXyz b) const {return {x+b.x,y+b.y,z+b.z};}
    cXyz operator-(cXyz b) const {return {x-b.x,y-b.y,z-b.z};}
    cXyz operator*(float f) const {return {x*f,y*f,z*f};}
    cXyz& operator+=(cXyz b) {x+=b.x;y+=b.y;z+=b.z;return *this;}
    float abs() const {return std::sqrt(x*x+y*y+z*z);}
    float absXZ() const {return std::sqrt(x*x+z*z);}
};
struct fopAc_ac_c { struct {cXyz pos;} current; };
struct daNpc_Kn_c : fopAc_ac_c {};
struct Player : fopAc_ac_c {
    bool boots=false; int forces=0; float power=0; int yaw=0;
    bool checkEquipHeavyBoots() {return boots;}
    void setOutPower(float p,int y,int) {++forces;power=p;yaw=y;}
} player;
auto daAlink_getAlinkActorClass() {return &player;}
int cLib_targetAngleY(const cXyz* from,const cXyz* to) {
    return static_cast<int>(std::atan2(to->x-from->x,to->z-from->z)*32768/3.1415926536);
}
constexpr unsigned AT_TYPE_BOMB=0x20,AT_TYPE_IRON_BALL=0x400000,AT_TYPE_ARROW=0x2000;
constexpr unsigned AT_TYPE_CSTATUE_SWING=0x8000,AT_TYPE_100=0x100;
constexpr int dCcD_SE_HARD_BODY=1,dCcD_SE_SWORD=2,dCcD_MTRL_FIRE=3;
enum dCcG_At_Spl {dummy};
struct Attack {unsigned type;bool ChkAtType(unsigned mask) {return type&mask;}};
struct dCcD_Stts {void Init(int,int,fopAc_ac_c*) {} void Move() {}};
struct Collider {
    bool at=false,tg=false;unsigned atType=0,tgType=0;int power=0,special=0,material=0;
    float radius=0,height=0;cXyz center;Attack* hit=nullptr;
    void SetStts(dCcD_Stts*) {} void SetTgType(unsigned t){tgType=t;}
    void SetAtType(unsigned t){atType=t;}void SetAtAtp(int p){power=p;}
    void SetTgSPrm(unsigned p){tg=p&1;}void SetAtSPrm(unsigned p){at=p&1;}
    void SetAtSpl(dCcG_At_Spl s){special=s;}void SetAtMtrl(int m){material=m;}
    void SetR(float r){radius=r;}void SetH(float h){height=h;}void SetC(cXyz p){center=p;}
    void SetTgSe(int){}void SetAtSe(int){}void SetAtVec(cXyz&){}
    void OffTgSetBit(){tg=false;}void OnTgSetBit(){tg=true;}
    void OffAtSetBit(){at=false;}void OnAtSetBit(){at=true;}
    void ClrAtHit(){}void ClrTgHit(){hit=nullptr;}
    bool ChkTgHit(){return hit;}Attack* GetTgHitObj(){return hit;}
};
struct dCcD_Sph:Collider{};
struct dCcD_Cyl:Collider{};
struct cM3dGCps {cXyz start,end;void Set(cXyz a,cXyz b,float){start=a;end=b;}};
struct dCcD_Cps:Collider,cM3dGCps{};
struct CollisionWorld {
    std::vector<Collider*> entries;
    void Set(Collider* c) {entries.push_back(c);}
    bool contains(Collider* c) {return std::find(entries.begin(),entries.end(),c)!=entries.end();}
} world;
auto dComIfG_Ccsp(){return &world;}
struct dBgS_LinChk {cXyz end;void Set(cXyz*,cXyz* b,fopAc_ac_c*){end=*b;}cXyz GetCross(){return end;}};
struct Background {bool LineCross(dBgS_LinChk*){return false;}} background;
auto& dComIfG_Bgsp(){return background;}
struct Message {int msg_idx=0;int getStatusLocal(){return 1;}};
Message* dMsgObject_getMsgObjectClass(){return nullptr;}
constexpr int Z2SE_EN_BM_EYE_BREAK=1,Z2SE_EN_FM_BLAST=2,Z2SE_EN_BM_BEAM2=3;
void mDoAud_seStart(int,cXyz*,int,int){}void mDoAud_seStartLevel(int,cXyz*,int,int){}
int dComIfGp_getReverb(int){return 0;}void cinema_pose(daNpc_Kn_c*,int){}
struct Animation {float frame=0;float getFrame(){return frame;}void play(){}};
struct Fire {Animation bck;void play(float rate){bck.frame+=rate;}};
struct Packet {
    shade::TrialClock clock;cXyz center,boss;std::array<cXyz,2> eyes,ends;float radius=0;
};
struct ShadeTrials {
    shade::TrialClock clock;Packet packet;dCcD_Stts status;dCcD_Sph shield;
    dCcD_Cyl flame;std::array<dCcD_Sph,2> eyeTargets;std::array<dCcD_Cps,2> beams;
    std::array<cXyz,2> eyePos{},aim{};std::array<Fire,2> fire;Animation eyeColor;
    fopAc_ac_c* owner=nullptr;cXyz center,origin;float radius=2000,fireScale=2.5;
    bool visible=false,hintOpened=false;int hint=0;
    void close_hint(){hint=0;hintOpened=false;}
'''
# The shared archive may also be used by a real Beamos with actor callbacks.
head_start = source.index("    void calc() override {")
head_end = source.index("\n    }", head_start) + len("\n    }")
head_fixture = r'''
struct J3DJoint;
using J3DJointCallBack=int (*)(J3DJoint*,int);
struct J3DJoint {
    J3DJointCallBack callback=nullptr;
    auto getCallBack(){return callback;}void setCallBack(J3DJointCallBack cb){callback=cb;}
};
struct ModelData {
    std::array<J3DJoint,7> joints;
    J3DJoint* getJointNodePointer(unsigned i){return &joints.at(i);}
};
struct J3DModel {
    ModelData data;
    auto getModelData(){return &data;}
    virtual void calc(){for(auto& joint:data.joints) assert(!joint.callback);}
};
struct TrialEyeModel:J3DModel {
    unsigned jointCount=7;
    std::array<J3DJointCallBack,7> callbacks{};
''' + source[head_start:head_end] + r'''
};
int native_callback(J3DJoint*,int){assert(false);return 1;}
'''
checks = r'''
};
int main() {
    TrialEyeModel head;
    for(auto& joint:head.data.joints) joint.callback=native_callback;
    head.calc();
    for(auto& joint:head.data.joints) assert(joint.callback==native_callback);
    fopAc_ac_c owner; daNpc_Kn_c boss; ShadeTrials t;t.init(&owner);
    assert(t.shield.tgType==(AT_TYPE_BOMB|AT_TYPE_IRON_BALL));
    assert(t.eyeTargets[0].tgType==AT_TYPE_ARROW && t.eyeTargets[1].tgType==AT_TYPE_ARROW);
    assert(t.flame.special>=12 && t.flame.material==dCcD_MTRL_FIRE);
    assert(t.flame.height==300 && t.flame.power==8);
    auto tick=[&]{world.entries.clear();return t.tick(&boss);};
    Attack sword{1},arrow{AT_TYPE_ARROW},bomb{AT_TYPE_BOMB},ball{AT_TYPE_IRON_BALL};
    for (auto* breaker:{&bomb,&ball}) {
        t.cancel();t.clock.begin(shade::Trial::Shield);
        for (auto* invalid:{&sword,&arrow}) {
            t.shield.hit=invalid;assert(!tick() && !t.clock.broken && t.shield.tg);
        }
        t.shield.hit=breaker;assert(!tick() && t.clock.broken && !t.shield.tg);
        for (int i=0;i<10;++i) assert(!tick());
        assert(tick()); // twelve-tick burst, no extra boss HP damage
    }
    t.cancel();t.clock.begin(shade::Trial::Fire);
    for (int i=0;i<150;++i) {assert(!tick());assert(!world.contains(&t.flame));}
    float radius=0;
    for (int i=0;i<43;++i) {
        assert(!tick() && world.contains(&t.flame) && t.flame.at);
        assert(t.flame.radius>radius);radius=t.flame.radius;
    }
    assert(std::abs(radius-1496.25f)<0.1f);
    for (int i=193;i<219;++i) {assert(!tick());assert(!t.flame.at);}
    assert(tick());
    t.cancel();t.clock.begin(shade::Trial::Eyes);
    player.current.pos={400,0,200};
    for(int i=0;i<60;++i) {assert(!tick());assert(!world.contains(&t.beams[0]));}
    assert(!tick() && world.contains(&t.beams[0]) && world.contains(&t.beams[1]));
    assert(t.beams[0].end.x==t.packet.ends[0].x); // drawing/collision agree
    for (auto* invalid:{&sword,&bomb,&ball}) {
        t.eyeTargets[0].hit=invalid;assert(!tick() && t.clock.eyes==3);
    }
    t.eyeTargets[0].hit=&arrow;assert(!tick() && t.clock.eyes==2 && !t.beams[0].at);
    assert(!world.contains(&t.beams[0]) && world.contains(&t.beams[1]));
    int saved=t.clock.ticks;t.suspend();
    assert(t.clock.ticks==saved && t.clock.eyes==2 && !t.visible && !t.beams[1].at);
    assert(!tick() && t.clock.ticks==saved+1 && t.beams[1].at);
    t.eyeTargets[1].hit=&arrow;assert(tick() && t.clock.eyes==0);
    t.cancel();t.clock.begin(shade::Trial::Wind);
    player.current.pos={100,0,0}; player.forces=0;
    for(int i=0;i<150;++i) assert(!tick());
    assert(player.forces==150 && player.power==18 && player.yaw==0x4000);
    player.boots=true;
    for(int i=150;i<299;++i) assert(!tick());
    assert(tick() && player.forces==150 && t.clock.ticks==300);
    t.cancel();assert(!t.clock.active() && !t.visible);
    assert(!t.shield.tg && !t.flame.at);
    for (int i=0;i<2;++i) assert(!t.eyeTargets[i].tg && !t.beams[i].at);
}
'''
code = fixture + '\n'.join(method(sig) for sig in (
    'void init(', 'void suspend()', 'void cancel()', 'bool tick(')) + checks.replace("};\nint main()", "};\n" + head_fixture + "\nint main()", 1)
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'trials.cpp'
    binary = Path(tmp) / 'trials'
    cpp.write_text(code)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print("Hero's Shade trial weapon filters, native collision registration, telegraph, wind and cancellation: passed")
