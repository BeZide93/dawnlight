#pragma once
#include <algorithm>

namespace dawnlight::shade {
enum class Trial { None, Shield, Fire, Eyes, Wind };
inline constexpr int starting_health = 10;
inline constexpr int fire_warning_ticks = 150; // five seconds to reach a wall target
inline constexpr int fire_wave_ticks = 70;
inline constexpr int wind_ticks = 300; // simulation runs at 30 Hz

// Simulation time only: drawing, pause menus and unrelated events never tick it.
struct TrialClock {
    Trial kind = Trial::None;
    int ticks = 0;
    unsigned eyes = 3;
    bool broken = false;
    int burst = 0;
    void begin(Trial trial) { kind=trial; ticks=0; eyes=3; broken=false; burst=0; }
    bool active() const { return kind!=Trial::None; }
    bool fire_live() const {
        return kind==Trial::Fire && ticks>=fire_warning_ticks &&
            ticks<fire_warning_ticks+43;
    }
    bool beam_live() const { return kind==Trial::Eyes && ticks>=60; }
    bool tick() {
        ++ticks;
        return (kind==Trial::Shield && broken && ++burst>=12) || (kind==Trial::Eyes && !eyes) ||
            (kind==Trial::Fire && ticks>=fire_warning_ticks+fire_wave_ticks) ||
            (kind==Trial::Wind && ticks>=wind_ticks);
    }
    void hit_eye(unsigned index) { if (kind==Trial::Eyes && index<2) eyes &= ~(1u<<index); }
};
}
