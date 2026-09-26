#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace dawnlight::touch {
constexpr size_t Midna = 5;
constexpr size_t Count = 6;
constexpr std::array<const char*, Count> Names = {
    "LB", "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right", "Midna"};
// Keep the old first-slot key so existing enabled state and layout survive the LB correction.
constexpr std::array<const char*, Count> Keys = {"zl", "up", "down", "left", "right", "midna"};
constexpr std::array<const char*, Count> Labels = {"LB", "&#8593;", "&#8595;", "&#8592;", "&#8594;", "Midna"};
struct Layout { int x, y, size; bool operator==(const Layout&) const = default; };
// Positions are percentages of the available travel inside the safe screen area.
constexpr std::array<Layout, Count> Defaults = {{{3, 22, 56}, {19, 55, 44},
    {19, 81, 44}, {13, 68, 44}, {25, 68, 44}, {88, 22, 56}}};
inline Layout clamp(Layout value) {
    return {std::clamp(value.x, 0, 100), std::clamp(value.y, 0, 100),
        std::clamp(value.size, 28, 120)};
}
struct Rect { float x, y, size; };
inline Rect rect(Layout layout, float width, float height, float left = 0, float top = 0) {
    layout = clamp(layout);
    const float size = std::max(0.f, std::min({float(layout.size), width, height}));
    return {left + std::max(0.f, width - size) * layout.x / 100.f,
        top + std::max(0.f, height - size) * layout.y / 100.f, size};
}

// Finger ownership keeps simultaneous buttons (and two fingers on one button)
// independent. UI transitions clear ownership; a fresh press is then required.
class Presses {
    struct Pointer { int64_t id = 0; size_t button = Count; };
    std::array<Pointer, 16> pointers{};
public:
    void clear() { pointers = {}; }
    bool owns(int64_t id) const {
        for (const auto& p : pointers) if (p.button < Count && p.id == id) return true;
        return false;
    }
    bool press(int64_t id, size_t button) {
        if (button >= Count) return false;
        if (owns(id)) return true;
        for (auto& p : pointers) if (p.button == Count) { p = {id, button}; return true; }
        return false;
    }
    bool release(int64_t id) {
        bool found = false;
        for (auto& p : pointers) if (p.button < Count && p.id == id) { p = {}; found = true; }
        return found;
    }
    void disable(size_t button) {
        for (auto& p : pointers) if (p.button == button) p = {};
    }
    bool held(size_t button) const {
        for (const auto& p : pointers) if (p.button == button) return true;
        return false;
    }
    template<class Pad> bool merge(Pad& pad) const {
        // LB is supplied through native button queries, not a GameCube trigger.
        // Keep the virtual port active for LB/Midna-only touch with no physical pad.
        // Midna uses a direct press edge, without borrowing START, Z or D-pad.
        constexpr uint16_t masks[Count] = {0, 0x0008, 0x0004, 0x0001, 0x0002, 0};
        bool active = held(0) || held(Midna);
        for (size_t i = 1; i < Count; ++i) if (held(i)) { pad.button |= masks[i]; active = true; }
        return active;
    }
};
}  // namespace dawnlight::touch
