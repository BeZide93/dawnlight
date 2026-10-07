"""Exercise production camera rays, boomerang sight state and subject routing."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/aim_hooks.cpp").read_text()

def function(signature):
    start = source.index(signature + "(")
    return source[start:source.index("\n}", start) + 2]

fixture = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
using f32=float;using s16=int16_t;
struct cXyz {
    float x=0,y=0,z=0;
    cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
    cXyz operator+(cXyz b)const{return {x+b.x,y+b.y,z+b.z};}
    cXyz operator-(cXyz b)const{return {x-b.x,y-b.y,z-b.z};}
    cXyz operator*(float s)const{return {x*s,y*s,z*s};}
    float abs()const{return std::sqrt(x*x+y*y+z*z);}
    float absXZ()const{return std::sqrt(x*x+z*z);}
    void normalize(){float n=abs();x/=n;y/=n;z/=n;}
};
bool near(cXyz a,cXyz b){return (a-b).abs()<.005f;}
struct csXyz {s16 x=0,y=0,z=0;};
s16 cM_atan2s(float y,float x){return std::atan2(y,x)*32768/M_PI;}
struct fopAc_ac_c {};
struct daBoomerang_c:fopAc_ac_c {
    bool reserved=false;int locks=0;
    bool getLockReserve(){return reserved;}
    bool getLockCntMax(){return locks>=5;}
};
struct Keep {fopAc_ac_c* actor=nullptr;auto getActor(){return actor;}
    void setData(fopAc_ac_c* a){actor=a;}void clearData(){actor=nullptr;}};
struct Sight {cXyz position;bool draw=false,locked=true;
    void setPos(cXyz* p){position=*p;}void onDrawFlg(){draw=true;}
    void offDrawFlg(){draw=false;}void offLockFlg(){locked=false;}};
struct BoomerangLine {cXyz start,end,cross;void* owner=nullptr;
    void Set(cXyz* a,cXyz* b,void* o){start=*a;end=*b;owner=o;}
    cXyz GetCross(){return cross;}};
namespace daPy_py_c {constexpr unsigned RFLG0_ITEM_SIGHT_BG_HIT=0x2000000;}
constexpr int BUTTON_STATUS_LOCK=7,BUTTON_STATUS_BACK=8;
constexpr int dItemNo_BOOMERANG_e=1,dItemNo_HAWK_ARROW_e=2;
int mode=0,offset=0,remembered=0,genericQueries=0;
bool movement=true,cameraAvailable=true;
float s_thirdPersonFovy=45;
struct daAlink_c {
    int field_0x317c=0,mEquipItem=dItemNo_BOOMERANG_e;
    Keep mItemAcKeep,mHookTargetAcKeep;Sight mSight;BoomerangLine mBoomerangLinChk;
    cXyz mHeldItemRootPos{1,2,3};csXyz mBodyAngle,shape_angle;
    bool throwing=false,next=false,ready=true,bodyReady=true;
    unsigned flags=0;float range=1200;int prompt=0,triggers=0,nativeCalls=0;
    bool checkBoomerangThrowAnime(){return throwing;}float getBoomLockMax(){return range;}
    void offResetFlg0(unsigned f){flags&=~f;}void onResetFlg0(unsigned f){flags|=f;}
    void setItemActionButtonStatus(int s){prompt=s;}bool itemActionTrigger(){++triggers;return true;}
    bool checkHookshotItem(int){return false;}bool checkHookshotWait(){return true;}
    void setHookshotSight(){}void setBowSight(){}void setCopyRodSight(){}
    void setBoomerangSight(){++nativeCalls;}
    bool checkItemActorPointer(){return mItemAcKeep.actor!=nullptr;}
    bool checkBoomerangReadyAnime(){return ready;}void setDoStatus(int){}
    void setShapeAngleToAtnActor(int){}bool checkNextAction(int){return next;}
    bool setBodyAngleToCamera(){return bodyReady;}
};
struct Camera {struct {cXyz mEye{10,20,30},mCenter{10,20,130};
    cXyz Up(){return {0,1,0};}} mCamera;} camera;
Camera* dComIfGp_getCamera(int){return cameraAvailable?&camera:nullptr;}
bool use_third_person_camera_for(daAlink_c*){return mode==1;}
int third_person_reticle_offset_y(){return offset;}
bool use_scope_suppress_camera(){return mode!=0;}
bool fixed_camera_sight_active(daAlink_c*){return mode!=0;}
bool fixed_clawshot_aim_active(daAlink_c*){return false;}
void remember_custom_cinema_sight(){++remembered;}
struct World {
    bool hit=true;int queries=0;float hitDistance=250;
    bool LineCross(BoomerangLine* line){
        ++queries;auto delta=line->end-line->start;auto length=delta.abs();
        if(!hit||hitDistance>length)return false;
        line->cross=line->start+delta*(hitDistance/length);return true;
    }
} world;
World& dComIfG_Bgsp(){return world;}
bool camera_bow_target(daAlink_c*,cXyz&,cXyz&,fopAc_ac_c**){++genericQueries;return false;}
bool bow_target_direction(const cXyz&,const cXyz&,const cXyz&,cXyz&){return false;}
bool is_custom_hookshot_target(fopAc_ac_c*){return false;}
void draw_iron_ball_sight(daAlink_c*){}
enum class AimItem {Bow,Boomerang,Hookshot,IronBall,CopyRod};
struct ModContext {};using HookAction=int;constexpr int HOOK_SKIP_ORIGINAL=1;
namespace mods {template<class T>T arg(void* args,int){return *static_cast<T*>(args);}}
// RAY
// BOOMERANG
// FIXED
// SUBJECT
bool update_subject_aim(daAlink_c* link,AimItem item){
    if(!movement)return false;draw_subject_sight(link,item);return true;
}
// REPLACE
int main(){
    daBoomerang_c boomerang;
    auto fresh=[&]{daAlink_c link;link.mItemAcKeep.actor=&boomerang;
        boomerang={};world={};camera={};offset=0;cameraAvailable=true;return link;};
    // Both subject routes must provide the same gameplay state as the cursor.
    for(int cameraMode:{1,2})for(bool aimMovement:{false,true}){
        auto link=fresh();mode=cameraMode;movement=aimMovement;
        daAlink_c* pointer=&link;int result=0;
        assert(replace_boomerang_subject(nullptr,&pointer,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(result==1&&link.nativeCalls==0&&world.queries==1);
        assert(link.mBoomerangLinChk.owner==&link);
        assert(std::fabs((link.mBoomerangLinChk.end-camera.mCamera.mEye).abs()-1200)<.005f);
        assert(near(link.mHeldItemRootPos,{10,20,280}));
        assert(near(link.mSight.position,link.mHeldItemRootPos)&&link.mSight.draw);
        assert(link.flags&daPy_py_c::RFLG0_ITEM_SIGHT_BG_HIT);
        assert(link.prompt==BUTTON_STATUS_LOCK&&link.triggers==1);
        assert(link.mSight.locked&&genericQueries==0); // No generic arrow ray or lock reset.
    }
    // Repeated camera refreshes do not add targets themselves; native procWait owns that.
    auto link=fresh();mode=1;draw_fixed_camera_sight(&link);draw_fixed_camera_sight(&link);
    assert(boomerang.locks==0);
    // Misses clear stale terrain state; sky and out-of-range points cannot be locked.
    for(bool hit:{false,true}){
        world.hit=hit;world.hitDistance=link.range+1;link.prompt=0;link.triggers=0;
        draw_fixed_camera_sight(&link);
        assert(!(link.flags&daPy_py_c::RFLG0_ITEM_SIGHT_BG_HIT));
        assert(link.prompt==0&&link.triggers==0);
        assert(near(link.mHeldItemRootPos,link.mBoomerangLinChk.end));
    }
    // Preserve the five-target limit and reserved actor-target exception.
    link=fresh();boomerang.locks=5;draw_fixed_camera_sight(&link);assert(link.prompt==0);
    world.hit=false;boomerang.reserved=true;draw_fixed_camera_sight(&link);
    assert(link.prompt==BUTTON_STATUS_LOCK&&!(link.flags&daPy_py_c::RFLG0_ITEM_SIGHT_BG_HIT));
    // Native range variations (including special rooms) and reticle offset apply to the ray.
    for(float range:{800.f,2600.f}){
        link=fresh();link.range=range;mode=1;offset=56;draw_fixed_camera_sight(&link);
        auto direction=link.mBoomerangLinChk.end-camera.mCamera.mEye;
        assert(std::fabs(direction.abs()-range)<.005f&&direction.y>0);
        float expectedSlope=(2.f*offset/448)*std::tan(45.f*M_PI/360);
        assert(std::fabs(direction.y/direction.z-expectedSlope)<.00001f);
        mode=2;draw_fixed_camera_sight(&link);assert(link.mBoomerangLinChk.end.y==20);
    }
    // No item / throwing / missing or degenerate camera must not invent a new target.
    for(int unavailable=0;unavailable<4;++unavailable){
        link=fresh();if(unavailable==0)link.mItemAcKeep.actor=nullptr;
        if(unavailable==1)link.throwing=true;if(unavailable==2)cameraAvailable=false;
        if(unavailable==3)camera.mCamera.mCenter=camera.mCamera.mEye;
        draw_fixed_camera_sight(&link);assert(world.queries==0&&link.prompt==0);
        assert(near(link.mHeldItemRootPos,{1,2,3}));
    }
    draw_fixed_camera_sight(nullptr);
    // Vanilla and exit-to-next-action paths keep their original behavior.
    for(bool aimMovement:{false,true}){
        link=fresh();mode=0;movement=aimMovement;daAlink_c* pointer=&link;int result=0;
        replace_boomerang_subject(nullptr,&pointer,&result,nullptr);
        assert(link.nativeCalls==1&&world.queries==0);
    }
    link=fresh();mode=1;link.next=true;link.mSight.draw=true;
    daAlink_c* pointer=&link;int result=0;
    replace_boomerang_subject(nullptr,&pointer,&result,nullptr);
    assert(!link.mSight.draw&&world.queries==0);
}
'''
for marker, signature in [
    ("RAY", "bool camera_aim_ray"),
    ("BOOMERANG", "void draw_boomerang_camera_sight"),
    ("FIXED", "void draw_fixed_camera_sight"),
    ("SUBJECT", "void draw_subject_sight"),
    ("REPLACE", "HookAction replace_boomerang_subject"),
]:
    fixture = fixture.replace("// " + marker + "\n", function(signature) + "\n")
with tempfile.TemporaryDirectory() as directory:
    cpp = Path(directory) / "test.cpp"
    exe = Path(directory) / "test"
    cpp.write_text(fixture)
    subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-Wno-misleading-indentation", str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Boomerang terrain lock passed: both cameras, movement on/off, range, reticle offset, terrain state, actor reserve, target limit and Vanilla fallback")
