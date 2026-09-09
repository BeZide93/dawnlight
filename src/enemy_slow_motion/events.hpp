#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace dawnlight {
struct EnemyFrameEvents {
    const void* animation = nullptr;
    int frame = -1;
    std::array<std::uint32_t, 20> events{};

    void update(const void* currentAnimation, int currentFrame) {
        if (animation == currentAnimation && frame == currentFrame) return;
        animation = currentAnimation;
        frame = currentFrame;
        events = {};
    }

    bool claim(std::uint32_t id) {
        if (id == 0) return true;
        if (std::find(events.begin(), events.end(), id) != events.end()) return false;
        const auto empty = std::find(events.begin(), events.end(), 0);
        if (empty != events.end()) *empty = id;
        return true;
    }
};
}
