"""Exercise the production camera callback with no previous on-foot sight."""
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
#include <initializer_list>
using u32 = unsigned;
using f32 = float;
constexpr bool TRUE = true;
struct cXyz { float x = 0; };
struct view_class {} presentationView;
const view_class* s_twilitPresentationView = nullptr;
view_class* dComIfGd_getView() { return &presentationView; }
enum { Vanilla, ThirdPerson, Cinema };
enum { Bow, BombArrow, Hawkeye, Boomerang, dItemNo_PACHINKO_e };
int mode = Cinema, zoom = 100, bowDraws = 0, otherDraws = 0;
u32 status = 0x1000;
bool aiming = true, bulletTime = false, teAimEnabled = false;
unsigned s_cameraRunDepth = 0;
int paused = 0, captures = 0, cinemaAligns = 0;
bool s_customCinemaSightActive = false, s_thirdPersonAimActive = true;
struct Sight {
    bool draw = false, locked = false;
    cXyz position;
    void setPos(cXyz* p) { position = *p; }
    void onDrawFlg() { draw = true; ++bowDraws; }
    void offDrawFlg() { draw = false; }
    void offLockFlg() { locked = false; }
};
struct daAlink_c {
    enum { PROC_AUTO_JUMP, PROC_FALL, PROC_WAIT, MODE_SWIMMING = 8 };
    int mProcID = PROC_WAIT;
    bool swimming = false, bowReady = true;
    struct { bool ground = true; bool ChkGroundHit() { return ground; } } mLinkAcch;
    bool checkModeFlg(int) { return swimming; }
    bool checkBowReloadAnime() { return false; }
    bool checkBowChargeWaitAnime() { return bowReady; }
    bool checkBowWaitAnime() { return false; }
    bool checkBowShootAnime() { return false; }
    int mEquipItem = Bow;
    bool wolf = false, lock = false, event = false;
    Sight mSight;
    int nativeSightQueries = 0;
    void getArrowFlyData(float* distance, float* speed, bool) {
        *distance = 900; *speed = 100;
    }
    void checkSightLine(float distance, cXyz* position) {
        ++nativeSightQueries; position->x = distance;
    }
    static bool checkBowItem(int item) { return item <= Hawkeye; }
    bool checkWolf() { return wolf; }
    bool checkAttentionLock() { return lock; }
    bool checkEventRun() { return event; }
} player;
daAlink_c* current = &player;
daAlink_c* s_twilitAimOwner = nullptr;
struct dCamera_c {
    u32 mPadID = 0;
    float mFovy = 60;
    static bool isAimActive() { return aiming; }
} camera;
struct ModContext {};
using HookAction = int; constexpr int HOOK_CONTINUE = 0;
namespace mods {
template<class T> T arg(void* args, int) { return *static_cast<T*>(args); }
}
daAlink_c* daAlink_getAlinkActorClass() { return current; }
bool use_scope_suppress_camera() { return mode != Vanilla; }
bool use_cinema_camera_for(daAlink_c*) { return mode == Cinema; }
bool use_third_person_camera_for(daAlink_c*) { return mode == ThirdPerson; }
bool is_hawkeye_bow(daAlink_c* link) { return link && link->mEquipItem == Hawkeye; }
bool bullet_time_active_for(daAlink_c*) { return bulletTime; }
bool dComIfGp_checkPlayerStatus0(u32, u32 mask) { return status & mask; }
int cinema_zoom_percent() { return zoom; }
bool fixed_camera_sight_active(daAlink_c* link) { return link->mEquipItem == Boomerang; }
void draw_fixed_camera_sight(daAlink_c*) { ++otherDraws; }
void remember_custom_cinema_sight() { s_customCinemaSightActive = true; }
bool camera_bow_target(daAlink_c*, cXyz& target, cXyz&) {
    target.x = s_twilitPresentationView ? 12000 : 10000; return true;
}
bool twilit_bullet_time_aim_enabled() { return teAimEnabled; }
int dComIfGp_isPauseFlag() { return paused; }
void prepare_third_person_aim(daAlink_c*) { if (mode == ThirdPerson) ++captures; }
void face_camera_view_yaw(daAlink_c*) { ++cinemaAligns; }
// PRODUCTION
void frame() {
    auto* cam = &camera;
    before_camera_run(nullptr, &cam, nullptr, nullptr);
    after_camera_run(nullptr, &cam, nullptr, nullptr);
}
void reset() {
    player = {}; current = &player; camera = {}; status = 0x1000;
    aiming = true; bulletTime = false; zoom = 100;
    teAimEnabled = false; s_twilitAimOwner = nullptr; paused = 0;
    captures = cinemaAligns = 0; s_cameraRunDepth = 0;
    bowDraws = otherDraws = 0; s_customCinemaSightActive = false;
}
int main() {
    for (int cameraMode : {ThirdPerson, Cinema}) {
        mode = cameraMode;
        for (int item : {Bow, BombArrow, dItemNo_PACHINKO_e}) {
            reset(); player.mEquipItem = item;
            status = item == dItemNo_PACHINKO_e ? 0x40 : 0x1000;
            // Epona's native routine bypasses replace_bow_subject entirely.
            // No earlier hook has set the custom sight flag, even on frame 1.
            assert(!s_customCinemaSightActive);
            frame();
            assert(bowDraws == 1 && player.mSight.draw && s_customCinemaSightActive);
            if (item == dItemNo_PACHINKO_e) {
                assert(!fixed_bow_aim_active(&player));
                assert(player.nativeSightQueries == 1 && player.mSight.position.x == 900);
            } else {
                assert(player.nativeSightQueries == 0 && player.mSight.position.x == 10000);
            }
            player.mSight.offDrawFlg(); frame();
            assert(bowDraws == 2 && player.mSight.draw);
            // Leaving the subject state must clear the custom sight.
            aiming = false;
            auto* link = &player;
            after_player_execute(nullptr, &link, nullptr, nullptr);
            assert(!player.mSight.draw && !s_customCinemaSightActive);
            frame(); assert(bowDraws == 2);
        }
        // These states must not bootstrap a custom bow sight.
        for (int item : {Bow, dItemNo_PACHINKO_e})
        for (int excluded = 0; excluded < 8; ++excluded) {
            reset(); player.mEquipItem = item;
            status = item == dItemNo_PACHINKO_e ? 0x40 : 0x1000;
            if (excluded == 0) player.mEquipItem = Hawkeye;
            if (excluded == 1) aiming = false;
            if (excluded == 2) status = 0;
            if (excluded == 3) player.lock = true;
            if (excluded == 4) player.event = true;
            if (excluded == 5) player.wolf = true;
            if (excluded == 6) current = nullptr;
            if (excluded == 7) status = item == dItemNo_PACHINKO_e ? 0x1000 : 0x40;
            frame(); assert(bowDraws == 0);
        }
        reset(); player.mEquipItem = Boomerang; status = 0x80000;
        frame(); assert(otherDraws == 1 && bowDraws == 0);
    }
    // TE invokes the same native body-angle entry repeatedly during its extra
    // simulation steps. Capture framing once and let TE consume input itself.
    for (int cameraMode : {ThirdPerson, Cinema}) {
        reset(); mode = cameraMode; teAimEnabled = true;
        player.mProcID = daAlink_c::PROC_FALL; player.mLinkAcch.ground = false;
        auto* link = &player;
        for (int i = 0; i < 5; ++i)
            assert(before_body_angle_to_camera(nullptr, &link, nullptr, nullptr) == HOOK_CONTINUE);
        assert(s_twilitAimOwner == link);
        assert(captures == (mode == ThirdPerson ? 1 : 0));
        assert(cinemaAligns == (mode == Cinema ? 1 : 0));
        frame(); assert(player.mSight.draw);
        assert(before_aim_presentation(nullptr,nullptr,nullptr,nullptr) == HOOK_CONTINUE);
        assert(player.mSight.position.x == 12000 && !s_twilitPresentationView);
        player.mLinkAcch.ground = true; aiming = false; status = 0;
        after_player_execute(nullptr, &link, nullptr, nullptr);
        assert(!s_twilitAimOwner && !player.mSight.draw);
        // Configured but not active/eligible must not seize the camera.
        for (int excluded = 0; excluded < 10; ++excluded) {
            reset(); mode = cameraMode; teAimEnabled = true;
            player.mProcID = daAlink_c::PROC_AUTO_JUMP; player.mLinkAcch.ground = false;
            if (excluded == 0) status = 0;
            if (excluded == 1) teAimEnabled = false;
            if (excluded == 2) player.mEquipItem = Hawkeye;
            if (excluded == 3) player.mLinkAcch.ground = true;
            if (excluded == 4) player.mProcID = daAlink_c::PROC_WAIT;
            if (excluded == 5) player.bowReady = false;
            if (excluded == 6) player.swimming = true;
            if (excluded == 7) player.event = true;
            if (excluded == 8) player.lock = true;
            if (excluded == 9) paused = 1;
            before_body_angle_to_camera(nullptr, &link, nullptr, nullptr);
            assert(!s_twilitAimOwner && captures == 0 && cinemaAligns == 0);
        }
    }
    // TE recursively calls Camera::Run from its post-hook. Cinema zoom must
    // be applied once to the final camera, not compounded on all five passes.
    reset(); mode = Cinema; zoom = 200;
    auto* cam = &camera;
    before_camera_run(nullptr, &cam, nullptr, nullptr);
    for (int i = 0; i < 4; ++i) {
        frame(); assert(camera.mFovy == 60 && bowDraws == 0);
    }
    after_camera_run(nullptr, &cam, nullptr, nullptr);
    assert(camera.mFovy == 30 && bowDraws == 1 && s_cameraRunDepth == 0);
    for (int item : {Bow, dItemNo_PACHINKO_e}) {
        reset(); mode = Vanilla; player.mEquipItem = item;
        status = item == dItemNo_PACHINKO_e ? 0x40 : 0x1000;
        frame(); assert(bowDraws == 0 && !s_customCinemaSightActive);
    }
}
'''
production = "\n\n".join(function(name) for name in (
    "bool player_in_supported_aim_status",
    "bool twilit_air_bow_aim_active",
    "HookAction before_body_angle_to_camera",
    "HookAction before_camera_run",
    "bool fixed_bow_aim_active",
    "bool custom_slingshot_sight_active",
    "void draw_bow_trajectory_sight",
    "HookAction before_aim_presentation",
    "bool should_keep_cinema_bow_sight",
    "bool player_in_supported_aim_state",
    "void after_camera_run",
    "void after_player_execute",
))
with tempfile.TemporaryDirectory(prefix="dawnlight-mounted-sight-") as directory:
    test = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    test.write_text(fixture.replace("// PRODUCTION", production))
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(test), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Mounted sights and TE airborne aim/camera nesting regression checks passed")
