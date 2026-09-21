#pragma once
#include <array>
namespace dawnlight::shade {
// Native teaching action numbers, animation sequences and success events.
// No teaching cutscene is allowed to run in the arena.
struct Phase { int type, action, success, attack; };
inline constexpr std::array<Phase, 8> phases{{
    {0, 2, 7, 21},   // Ending Blow / downward thrust
    {1, 5, 10, 23},  // Shield Attack / shield strike
    {1, 7, 11, -1},  // reflected magic ball (native projectile)
    {2, 8, 14, 17},  // Back Slice / sidestep, roll, slash
    {3, 9, 16, 13},  // Helm Splitter
    {4, 12, 19, 12}, // Mortal Draw
    {5, 13, 21, 10}, // Jump Strike and two doubles
    {6, 19, 24, 26}, // Great Spin and two doubles
}};
inline bool attack_window(unsigned phase, int step, float frame, float end) {
    if (phase>=phases.size() || end<=0 || frame<0) return false;
    // The Back Slice sequence has a sidestep and roll before the sword cut.
    if (step!=(phase==3 ? 2 : 0)) return false;
    const float progress=frame/end;
    return progress>=0.4f && progress<0.75f;
}
struct Battle {
    unsigned phase = 0;
    int health = 8;
    int remaining = 600;
    int recovery = 0;
    bool advance = false;
    bool dying = false;

    void event(int event) {
        if (recovery || dying) return;
        if (event == phases[phase].success) {
            --health;
            recovery = 45;
            advance = true;
        }
    }
    bool tick() {
        if (dying) return false;
        if (recovery && --recovery) return false;
        if (health == 0) { dying = true; return true; }
        if (advance || --remaining == 0) {
            phase = (phase + 1) % phases.size();
            remaining = 600;
            advance = false;
            return true;
        }
        return false;
    }
};
}
