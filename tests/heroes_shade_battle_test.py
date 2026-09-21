"""Exercise encounter progression and damaging animation windows without game assets."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r'''
#include "heroes_shade_battle.hpp"
#include <cassert>
using namespace dawnlight::shade;
int main() {
    Battle battle;
    // Every native success advances once; repeated events cannot drain HP.
    for (unsigned phase=0; phase<phases.size(); ++phase) {
        assert(battle.phase==phase);
        const int hp=battle.health;
        battle.event(1); // teacher timeout / lecture
        battle.event(4); // Link got hit
        assert(battle.health==hp && battle.recovery==0);
        battle.event(phases[phase].success);
        assert(battle.health==hp-1);
        battle.event(phases[phase].success);
        assert(battle.health==hp-1);
        for (int i=0;i<44;++i) assert(!battle.tick());
        assert(battle.tick());
    }
    assert(battle.dying && battle.health==0);
    for (int i=0;i<100;++i) {
        battle.event(24);
        assert(!battle.tick() && battle.health==0);
    }
    // A missed skill cannot soft-lock the fight or count as a successful hit.
    Battle timeout;
    for (unsigned phase=0;phase<16;++phase) {
        for (int i=0;i<599;++i) assert(!timeout.tick());
        assert(timeout.tick());
        assert(timeout.health==8 && !timeout.dying && timeout.phase==(phase+1)%8);
    }
    // Replay starts independently; no defeated flags or lesson save bits needed.
    battle={};
    assert(battle.health==8 && battle.phase==0 && !battle.dying);
    for (unsigned phase=0;phase<phases.size();++phase) {
        const int step=phase==3 ? 2 : 0;
        assert(!attack_window(phase,step,0,60));
        assert(!attack_window(phase,step,23,60));
        assert(attack_window(phase,step,24,60));
        assert(attack_window(phase,step,40,60));
        assert(!attack_window(phase,step,45,60));
        assert(!attack_window(phase,step,60,60));
        assert(!attack_window(phase,step+1,30,60));
        assert(!attack_window(phase,step,10,0));
    }
    assert(!attack_window(3,0,30,60)); // sidestep is not the cut
    assert(!attack_window(3,1,30,60)); // neither is the roll
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/"test.cpp"
    exe=Path(tmp)/"test"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root/"src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade progression, replay and attack windows: passed")
