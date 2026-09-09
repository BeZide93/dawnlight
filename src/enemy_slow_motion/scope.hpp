#pragma once

#include <array>
#include <cstddef>

namespace dawnlight {
template <typename Step, std::size_t N>
Step* active_enemy_scope(std::array<Step, N>& steps, std::size_t depth) {
    if (depth == 0 || depth > N) return nullptr;
    auto& step = steps[depth - 1];
    // An inactive nested execute masks its parent; never search farther down.
    return step.actor != nullptr && step.profile != nullptr ? &step : nullptr;
}
}
