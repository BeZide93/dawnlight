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

// The native forward/backward airborne reactions each have a matching
// landing sequence which advances into a flat lying pose.
inline int knockdown_landing_motion(int motion) {
    return motion==18 ? 19 : motion==14 ? 15 : -1;
}

inline bool jumping_attack(int attack) {
    return attack == helm_splitter || attack == jump_strike;
}
inline bool striking_step(int attack, int step) {
    if (attack == sword) return step == 0;
    // The Back Slice sequence has a sidestep and roll before the sword cut.
    if (step!=(attack==back_slice ? 2 : 0)) return false;
    for (const auto& phase : phases) if (phase.attack == attack && attack >= 0) return true;
    return false;
}

struct BladePoint { float x, y, z; };
inline float blade_distance_squared(const BladePoint& a, const BladePoint& b) {
    const float x=a.x-b.x, y=a.y-b.y, z=a.z-b.z;
    return x*x+y*y+z*z;
}
struct BladeMotion {
    std::array<BladePoint,2> previous{};
    int attack = -1;
    int step = -1;
    float frame = -1;
    bool resolved = false;
    unsigned contacts = 0;
    unsigned clear_samples = 0;
    bool sweep = false;

    void contact() {
        if (!resolved) { resolved=true; ++contacts; clear_samples=0; }
    }

    bool sample(int next_attack, int next_step, float next_frame,
                const std::array<BladePoint,2>& points, bool clear_of_target=false) {
        const bool continuous=attack==next_attack && step==next_step && next_frame>frame;
        const bool cut_entry=attack==back_slice && next_attack==back_slice && step==1 && next_step==2;
        const float travel=std::max(blade_distance_squared(points[0],previous[0]),
                                    blade_distance_squared(points[1],previous[1]));
        sweep=continuous && travel<=300.0f*300.0f;
        if (resolved && next_attack==jump_strike && contacts==1) {
            // Jump Strike has two blows in one native motion. Rearm only after
            // the blade has visibly withdrawn, never while it remains on Link.
            clear_samples=sweep && clear_of_target ? clear_samples+1 : 0;
            if (clear_samples>=2) resolved=false;
        }
        attack=next_attack; step=next_step; frame=next_frame; previous=points;
        // Register the first cut pose immediately. Do not sweep the preceding
        // non-damaging roll into this strike.
        if (cut_entry && !resolved) return true;
        if (resolved || !continuous || !striking_step(attack,step)) return false;
        if (attack==sword) return frame>=30 && frame<=40; // native ordinary swing
        // Special animation lengths/windups differ. Use the posed blade's
        // movement, not an invented common percentage of the animation. The
        // native collision volumes decide whether that blade actually hits.
        // Stationary poses and discontinuities (e.g. a warp) cannot hit.
        return travel>=1.0f && travel<=300.0f*300.0f;
    }
};

// Conservative separation from a hurt-cylinder's bounding box, with blade
// thickness and a margin. A false result keeps the strike consumed; it never
// grants another hit just because one of the two blade points left the body.
inline bool blade_clear_of_body(const std::array<BladePoint,2>& points,
                                BladePoint base, float radius, float height) {
    constexpr float margin=45;
    return std::max(points[0].x,points[1].x)+margin < base.x-radius ||
        std::min(points[0].x,points[1].x)-margin > base.x+radius ||
        std::max(points[0].z,points[1].z)+margin < base.z-radius ||
        std::min(points[0].z,points[1].z)-margin > base.z+radius ||
        std::max(points[0].y,points[1].y)+margin < base.y ||
        std::min(points[0].y,points[1].y)-margin > base.y+height;
}

struct GroundPoint { float x, z; };
inline GroundPoint jump_landing_target(int attack, GroundPoint origin, GroundPoint target) {
    const float dx=target.x-origin.x, dz=target.z-origin.z;
    const float distance=std::sqrt(dx*dx+dz*dz);
    if (distance<0.001f) return target;
    // Helm Splitter turns around during the somersault: land beyond Link so
    // its backwards-facing cut points at him. Jump Strike stays in front.
    const float offset=attack==helm_splitter ? 140.0f : -std::min(110.0f,distance);
    return {target.x+dx/distance*offset,target.z+dz/distance*offset};
}
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
