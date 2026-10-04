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
enum { Vanilla, ThirdPerson, Cinema };
enum { Bow, BombArrow, Hawkeye, Boomerang };
int mode = Cinema, zoom = 100, bowDraws = 0, otherDraws = 0;
u32 status = 0x1000;
bool aiming = true, bulletTime = false;
bool s_customCinemaSightActive = false, s_thirdPersonAimActive = true;
struct Sight {
    bool draw = false, locked = false;
    void offDrawFlg() { draw = false; }
    void offLockFlg() { locked = false; }
};
struct daAlink_c {
    int mEquipItem = Bow;
    bool wolf = false, lock = false, event = false;
    Sight mSight;
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
bool bullet_time_active_for(daAlink_c*) { return bulletTime; }
bool dComIfGp_checkPlayerStatus0(u32, u32 mask) { return status & mask; }
int cinema_zoom_percent() { return zoom; }
bool fixed_camera_sight_active(daAlink_c* link) { return link->mEquipItem == Boomerang; }
void draw_fixed_camera_sight(daAlink_c*) { ++otherDraws; }
void draw_bow_trajectory_sight(daAlink_c* link) {
    ++bowDraws; link->mSight.draw = true; s_customCinemaSightActive = true;
}
// PRODUCTION
void frame() {
    auto* cam = &camera;
    after_camera_run(nullptr, &cam, nullptr, nullptr);
}
void reset() {
    player = {}; current = &player; camera = {}; status = 0x1000;
    aiming = true; bulletTime = false; zoom = 100;
    bowDraws = otherDraws = 0; s_customCinemaSightActive = false;
}
int main() {
    for (int cameraMode : {ThirdPerson, Cinema}) {
        mode = cameraMode;
        for (int item : {Bow, BombArrow}) {
            reset(); player.mEquipItem = item;
            // Epona's native routine bypasses replace_bow_subject entirely.
            // No earlier hook has set the custom sight flag, even on frame 1.
            assert(!s_customCinemaSightActive);
            frame();
            assert(bowDraws == 1 && player.mSight.draw && s_customCinemaSightActive);
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
        for (int excluded = 0; excluded < 7; ++excluded) {
            reset();
            if (excluded == 0) player.mEquipItem = Hawkeye;
            if (excluded == 1) aiming = false;
            if (excluded == 2) status = 0;
            if (excluded == 3) player.lock = true;
            if (excluded == 4) player.event = true;
            if (excluded == 5) player.wolf = true;
            if (excluded == 6) current = nullptr;
            frame(); assert(bowDraws == 0);
        }
        reset(); player.mEquipItem = Boomerang; status = 0x80000;
        frame(); assert(otherDraws == 1 && bowDraws == 0);
    }
    reset(); mode = Vanilla; frame();
    assert(bowDraws == 0 && !s_customCinemaSightActive);
}
'''
production = "\n\n".join(function(name) for name in (
    "bool player_in_supported_aim_status",
    "bool fixed_bow_aim_active",
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
print("Mounted bow sight regression checks passed")
