"""Run production aim transforms/callbacks against floor, ceiling and wall cameras."""
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
#include <vector>
#include <cstdio>
#include <source_location>
using f32 = float;
using s16 = short;
using BOOL = bool;
constexpr bool TRUE = true, FALSE = false;
#define SQUARE(x) ((x) * (x))
float JMAFastSqrt(float x) { return std::sqrt(x); }
float cM_ssin(s16 a) { return std::sin(a * M_PI / 32768.0); }
float cM_scos(s16 a) { return std::cos(a * M_PI / 32768.0); }
s16 cM_atan2s(float y, float x) { return std::atan2(y, x) * 32768.0 / M_PI; }
struct cXyz {
    float x = 0, y = 0, z = 0;
    cXyz() = default;
    cXyz(float a, float b, float c) : x(a), y(b), z(c) {}
    cXyz operator+(cXyz b) const { return {x+b.x,y+b.y,z+b.z}; }
    cXyz operator-(cXyz b) const { return {x-b.x,y-b.y,z-b.z}; }
    cXyz operator*(float s) const { return {x*s,y*s,z*s}; }
    float abs() const { return std::sqrt(x*x+y*y+z*z); }
    float absXZ() const { return std::sqrt(x*x+z*z); }
    void normalize() { *this = *this * (1.0f / abs()); }
    cXyz cross(cXyz b) const { return {y*b.z-z*b.y,z*b.x-x*b.z,x*b.y-y*b.x}; }
};
// Quarter turns around Z: floor (0), ceiling (2), and magnetic wall (1).
cXyz rotate(int turns, cXyz v) {
    for (int i=0; i<(turns+4)%4; ++i) v = {-v.y,v.x,v.z};
    return v;
}
void mDoMtx_multVecSR(int turns, cXyz* in, cXyz* out) { *out = rotate(turns,*in); }
struct daAlink_c {
    bool magnetic = false, hawkeye = false;
    int rotation = 0, field_0x317c = 0;
    struct { s16 x = 0, y = 0; } shape_angle, mBodyAngle;
    s16 field_0x310a = 0, field_0x310c = 0;
    struct { cXyz pos; } current;
    cXyz up;
    bool checkMagneBootsOn() { return magnetic; }
    int getMagneBootsMtx() { return rotation; }
    int getMagneBootsInvMtx() { return -rotation; }
    cXyz* getMagneBootsTopVec() { return &up; }
    s16 getCameraAngleY() { return field_0x310c; }
    s16 getCameraAngleX() { return field_0x310a; }
    void checkBodyAngleX(s16&) {}
} player;
struct dBgS_CamLinChk { cXyz cross; cXyz GetCross() { return cross; } };
struct Globe { cXyz v; void Val(cXyz p) { v = p; } };
struct dCamera_c {
    cXyz mEye, mCenter, up;
    float mFovy = 50;
    int mCurMode = 8;
    struct { cXyz mCenter, mEye; Globe mDirection; float mFovy = 0; } mViewCache;
    struct { cXyz field_0x0, field_0xc, field_0x18; Globe field_0x24; } mUpOverride;
    bool collide = false;
    std::vector<cXyz> starts, ends;
    cXyz Up() { return up; }
    bool lineBGCheck(cXyz* start, cXyz* end, dBgS_CamLinChk* hit, int) {
        starts.push_back(*start); ends.push_back(*end);
        hit->cross = *start + (*end-*start)*0.5f;
        return collide;
    }
};
struct Actor { dCamera_c mCamera; } actor;
bool cameraAvailable = true, supported = true;
enum { Vanilla, Cinema, ThirdPerson };
int mode = ThirdPerson, reticleOffset = 0;
bool s_thirdPersonAimActive = false;
float s_thirdPersonDistance = 300, s_thirdPersonFovy = 45, s_thirdPersonHeight = 130;
Actor* dComIfGp_getCamera(int) { return cameraAvailable ? &actor : nullptr; }
cXyz* fopCamM_GetCenter_p(Actor* a) { return &a->mCamera.mCenter; }
cXyz* fopCamM_GetEye_p(Actor* a) { return &a->mCamera.mEye; }
daAlink_c* daAlink_getAlinkActorClass() { return &player; }
bool use_third_person_camera_for(daAlink_c*) { return mode == ThirdPerson; }
bool is_hawkeye_bow(daAlink_c* p) { return p->hawkeye; }
bool player_in_supported_aim_state(dCamera_c*) { return supported; }
int third_person_reticle_offset_y() { return reticleOffset; }
struct ModContext {};
namespace mods {
template<class T> T arg(void* args, int) { return *static_cast<T*>(args); }
}
// PRODUCTION
void near(cXyz a, cXyz b, std::source_location at = std::source_location::current()) {
    if ((a-b).abs() >= 0.02f) std::fprintf(stderr,"line %u: (%f,%f,%f) != (%f,%f,%f)\n",at.line(),a.x,a.y,a.z,b.x,b.y,b.z);
    assert((a-b).abs() < 0.02f);
}
void close(float a, float b) { assert(std::fabs(a-b) < 0.02f); }
void setup(int rotation, s16 yaw, s16 pitch) {
    player = {}; actor = {}; cameraAvailable = supported = true;
    mode = ThirdPerson; reticleOffset = 0; s_thirdPersonAimActive = false;
    player.rotation = rotation; player.magnetic = rotation != 0;
    player.current.pos = {700,900,-200}; player.up = rotate(rotation,{0,1,0});
    auto& cam = actor.mCamera; cam.up = player.up;
    cam.mCenter = player.current.pos + rotate(rotation,{0,145,0});
    cXyz backward(-320*cM_scos(pitch)*cM_ssin(yaw),320*cM_ssin(pitch),
                  -320*cM_scos(pitch)*cM_scos(yaw));
    cam.mEye = cam.mCenter + rotate(rotation,backward);
}
void frame() {
    auto* cam = &actor.mCamera;
    after_subject_camera(nullptr,&cam,nullptr,nullptr);
}
int main() {
    // Initial yaw/pitch and height must be identical in the surface's local frame.
    for (int rotation : {0,2,1}) for (s16 yaw : {s16(0),s16(6000),s16(-12000)})
    for (s16 pitch : {s16(0),s16(3500),s16(-4500)}) {
        setup(rotation,yaw,pitch); prepare_third_person_aim(&player);
        assert(s_thirdPersonAimActive);
        assert(std::abs(player.field_0x310c-yaw)<=1);
        assert(std::abs(player.field_0x310a-pitch)<=1);
        close(s_thirdPersonHeight,145); close(s_thirdPersonDistance,320);
        close(s_thirdPersonFovy,50);
    }
    // Rotate the complete ground result, including both world-space collision rays.
    for (bool collide : {false,true}) for (s16 pitch : {s16(0),s16(4000)}) {
        setup(0,6000,pitch); prepare_third_person_aim(&player);
        // Pin angles after the separately tested, quantized initialization.
        player.field_0x310c=6000; player.field_0x310a=pitch;
        actor.mCamera.collide=collide; frame(); auto ground=actor.mCamera;
        for (int rotation : {2,1}) {
            setup(rotation,6000,pitch); prepare_third_person_aim(&player);
            player.field_0x310c=6000; player.field_0x310a=pitch;
            auto& cam=actor.mCamera; cam.collide=collide; frame();
            auto transformed=[&](cXyz p) {
                return player.current.pos+rotate(rotation,p-player.current.pos);
            };
            near(cam.mViewCache.mCenter,transformed(ground.mViewCache.mCenter));
            near(cam.mViewCache.mEye,transformed(ground.mViewCache.mEye));
            assert(cam.starts.size()==2);
            for (int i=0;i<2;++i) {
                near(cam.starts[i],transformed(ground.starts[i]));
                near(cam.ends[i],transformed(ground.ends[i]));
            }
            near(cam.mUpOverride.field_0x0,cam.mViewCache.mCenter);
            near(cam.mUpOverride.field_0xc,cam.mViewCache.mEye);
            near(cam.mUpOverride.field_0x24.v,cam.mViewCache.mEye-cam.mViewCache.mCenter);
            near(cam.mUpOverride.field_0x18,player.up);
        }
    }
    setup(0,0,0); prepare_third_person_aim(&player); frame();
    near(actor.mCamera.mViewCache.mCenter,player.current.pos+cXyz(60,185,0));
    near(actor.mCamera.mViewCache.mEye,player.current.pos+cXyz(60,185,-320));
    // Reticle displacement follows screen up, including inverted/sideways views.
    for (int rotation : {0,2,1}) for (int offset : {-56,0,56}) {
        setup(rotation,0,0); prepare_third_person_aim(&player); frame();
        auto& cam=actor.mCamera;
        cam.mEye=cam.mViewCache.mEye; cam.mCenter=cam.mViewCache.mCenter;
        reticleOffset=offset; cXyz eye,ray; assert(camera_aim_ray(&player,eye,ray));
        float angle=std::atan((2.0f*offset/448.0f)*std::tan(50*M_PI/360));
        near(ray,rotate(rotation,{0,std::sin(angle),std::cos(angle)}));
        near(eye,cam.mEye);
        mode=Cinema; assert(camera_aim_ray(&player,eye,ray));
        near(ray,{0,0,1}); // Cinema has no third-person reticle offset.
    }
    // Cinema/Vanilla/Hawkeye and other camera states keep their native framing.
    for (int excluded=0;excluded<6;++excluded) {
        setup(2,0,0);
        if (excluded==0) mode=Cinema;
        if (excluded==1) mode=Vanilla;
        if (excluded==2) player.hawkeye=true;
        if (excluded==3) supported=false;
        if (excluded==4) actor.mCamera.mCurMode=7;
        if (excluded!=5) prepare_third_person_aim(&player);
        frame(); assert(actor.mCamera.starts.empty());
    }
    setup(2,6000,0); mode=Cinema;
    assert(face_camera_view_yaw(&player)); assert(std::abs(player.shape_angle.y-6000)<=1);
    cameraAvailable=false; assert(!face_camera_view_yaw(&player));
    prepare_third_person_aim(nullptr);
}
'''
production = "\n\n".join(function(name) for name in (
    "cXyz aim_vector_to_local", "cXyz aim_vector_to_world",
    "BOOL face_camera_view_yaw", "bool camera_aim_ray",
    "void prepare_third_person_aim", "void after_subject_camera",
))
with tempfile.TemporaryDirectory(prefix="dawnlight-magnetic-aim-") as directory:
    test = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    test.write_text(fixture.replace("// PRODUCTION", production))
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(test), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Magnetic surface aim camera regression checks passed")
