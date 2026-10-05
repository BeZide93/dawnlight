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
enum { Vanilla, ThirdPerson, Cinema };
enum { Bow, BombArrow, Hawkeye, Boomerang, dItemNo_PACHINKO_e };
int mode = Cinema, zoom = 100, bowDraws = 0, otherDraws = 0;
u32 status = 0x1000;
bool aiming = true;
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
struct dCamera_c {
    u32 mPadID = 0;
    float mFovy = 60;
    static bool isAimActive() { return aiming; }
} camera;
struct ModContext {};
namespace mods {
template<class T> T arg(void* args, int) { return *static_cast<T*>(args); }
}
daAlink_c* daAlink_getAlinkActorClass() { return current; }
bool use_scope_suppress_camera() { return mode != Vanilla; }
bool use_cinema_camera_for(daAlink_c*) { return mode == Cinema; }
bool use_third_person_camera_for(daAlink_c*) { return mode == ThirdPerson; }
bool is_hawkeye_bow(daAlink_c* link) { return link && link->mEquipItem == Hawkeye; }
bool dComIfGp_checkPlayerStatus0(u32, u32 mask) { return status & mask; }
int cinema_zoom_percent() { return zoom; }
bool fixed_camera_sight_active(daAlink_c* link) { return link->mEquipItem == Boomerang; }
void draw_fixed_camera_sight(daAlink_c*) { ++otherDraws; }
void remember_custom_cinema_sight() { s_customCinemaSightActive = true; }
bool camera_bow_target(daAlink_c*, cXyz& target, cXyz&) {
    target.x = 10000; return true;
}
// PRODUCTION
void frame() {
    auto* cam = &camera;
    after_camera_run(nullptr, &cam, nullptr, nullptr);
}
void reset() {
    player = {}; current = &player; camera = {}; status = 0x1000;
    aiming = true; zoom = 100;
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
    for (int item : {Bow, dItemNo_PACHINKO_e}) {
        reset(); mode = Vanilla; player.mEquipItem = item;
        status = item == dItemNo_PACHINKO_e ? 0x40 : 0x1000;
        frame(); assert(bowDraws == 0 && !s_customCinemaSightActive);
    }
}
'''
production = "\n\n".join(function(name) for name in (
    "bool player_in_supported_aim_status",
    "bool fixed_bow_aim_active",
    "bool custom_slingshot_sight_active",
    "void draw_bow_trajectory_sight",
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
print("Mounted Bow and Slingshot sight regression checks passed")
