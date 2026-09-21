#pragma once
#include <algorithm>
#include <array>
#include <cmath>
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
inline constexpr int sword = 25;
inline constexpr int back_slice = 17;
inline constexpr int helm_splitter = 13;
inline constexpr int jump_strike = 10;

// Native sequences keep their sequence number when they return to the ready
// stance. Checking only sequence 9 would strand the offensive controller.
inline bool ready_motion(int motion, int step) {
    // Ending Blow's ordinary hit reaction (31) returns to the walking stance;
    // the arena must resume attacking once that reaction/sliding has finished.
    return motion == 9 || ((motion == sword || motion == 27 || motion == 24 || motion == 31) && step >= 1);
}

struct AttackChain {
    unsigned blocks = 0;
    unsigned counters = 0;
    unsigned cycle = 0;
    int remaining = 0;
    int finisher = helm_splitter;

    void blocked_sword() {
        if (++blocks == 2) { blocks = 0; ++counters; }
    }
    void cancel() { remaining = 0; }
    void begin(unsigned phase) {
        if (counters) {
            --counters;
            remaining = 2;
            finisher = back_slice;
        } else {
            // Jumping moves are available from the first combo, independent of
            // which lesson counter currently exposes the boss to damage.
            const int skill = phases[phase].attack;
            const std::array<int, 4> finishers{
                helm_splitter, back_slice, jump_strike, skill >= 0 ? skill : back_slice};
            finisher = finishers[cycle++ % finishers.size()];
            remaining = 3;
        }
    }
    int next() {
        if (!remaining) return -1;
        return --remaining ? sword : finisher;
    }
};

inline bool jumping_attack(int attack) {
    return attack == helm_splitter || attack == jump_strike;
}
inline bool attack_window(int attack, int step, float frame, float end) {
    if (end<=0 || frame<0) return false;
    if (attack == sword) return step == 0 && frame >= 30 && frame <= 40;
    // The Back Slice sequence has a sidestep and roll before the sword cut.
    if (step!=(attack==back_slice ? 2 : 0)) return false;
    bool known = false;
    for (const auto& phase : phases) if (phase.attack == attack && attack >= 0) known = true;
    if (!known) return false;
    const float progress=frame/end;
    return progress>=0.4f && progress<0.75f;
}

struct GroundPoint { float x, z; };
inline GroundPoint back_slice_offset(float start_angle, float start_radius, int step, float progress) {
    constexpr float pi = 3.14159265358979323846f;
    progress = std::clamp(progress, 0.0f, 1.0f);
    const float fraction = step == 0 ? progress / 3.0f : (1.0f + 2.0f * progress) / 3.0f;
    const float radius = step == 0 ? start_radius + (150.0f-start_radius)*progress : 150.0f;
    const float angle = start_angle + pi * fraction;
    return {std::sin(angle)*radius, std::cos(angle)*radius};
}
// Drive normal actor movement rather than changing positions directly; native
// wall/floor and actor collision still constrain the resulting displacement.
inline GroundPoint approach_velocity(float dx, float dz, float max_speed, float stop_distance) {
    const float distance = std::sqrt(dx*dx + dz*dz);
    if (distance <= stop_distance || distance < 0.001f) return {0,0};
    const float speed = std::min(max_speed, distance-stop_distance);
    return {dx/distance*speed, dz/distance*speed};
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
