#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace dawnlight::touch {
constexpr size_t Midna = 5;
constexpr size_t L3 = 6, R3 = 7, Jump = 8, DarkLink = 9;
constexpr size_t Count = 10;
constexpr std::array<const char*, Count> Names = {
    "LB", "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right", "Midna", "L3", "R3", "Jump", "Dark Link"};
// Keep the old first-slot key so existing enabled state and layout survive the LB correction.
constexpr std::array<const char*, Count> Keys = {"zl", "up", "down", "left", "right", "midna", "l3", "r3", "jump", "dark-link"};
constexpr std::array<const char*, Count> Labels = {"LB", "&#8593;", "&#8595;", "&#8592;", "&#8594;", "Midna", "L3", "R3", "Jump", "Dark Link"};
struct Layout { int x, y, size; bool operator==(const Layout&) const = default; };
// Positions are percentages of the available travel inside the safe screen area.
constexpr std::array<Layout, Count> Defaults = {{{3, 22, 56}, {19, 55, 44},
    {19, 81, 44}, {13, 68, 44}, {25, 68, 44}, {88, 22, 56},
    {9, 85, 46}, {69, 85, 46}, {76, 61, 56}, {67, 22, 56}}};
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
    bool owns(int64_t id, size_t button) const {
        for (const auto& p : pointers) if (p.button == button && p.id == id) return true;
        return false;
    }
    bool press(int64_t id, size_t button) {
        if (button >= Count) return false;
        for (const auto& p : pointers)
            if (p.button < Count && p.id == id) return p.button == button;
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
        // Extended stick bits are kept separate from the GameCube button mask.
        if (held(L3)) pad.extButton |= 0x4000000;
        if (held(R3)) pad.extButton |= 0x2000000;
        return active;
    }
};

// Direct actions are sampled once per pad frame, independently of any binding.
struct ActionInput {
    bool pending = false, pressed = false;
    void press() { pending = true; }
    void clear() { pending = pressed = false; }
    void sample() { pressed = pending; pending = false; }
    bool consume() { const bool value = pressed; pressed = false; return value; }
};
}  // namespace dawnlight::touch
