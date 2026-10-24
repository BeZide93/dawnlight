#pragma once
#include <algorithm>

namespace dawnlight::survivors {
struct TitleFlow {
    // Dusklight's prelaunch body fades for 1s after a 0.2s delay.
    static constexpr float menuFade = 1.2f, cameraDuration = 2.4f;
    bool launched = false, armed = false, logo = false, revealEdge = false;
    float elapsed = 0;
    void play() { *this = {}; launched = true; }
    void tick(float seconds, bool ready, bool obscured, bool held, bool pressed) {
        revealEdge = false;
        if (!launched || !ready || obscured) { armed = false; return; }
        elapsed = std::min(menuFade + cameraDuration, elapsed + std::clamp(seconds, 0.f, .1f));
        if (elapsed < menuFade + cameraDuration) { armed = false; return; }
        if (!held && !pressed) armed = true;
        if (armed && pressed && !logo) { logo = true; revealEdge = true; }
    }
    float camera() const {
        const float t = std::clamp((elapsed-menuFade)/cameraDuration, 0.f, 1.f);
        return t*t*(3.f-2.f*t);
    }
    bool show_logo() const { return logo && !revealEdge; }
};
}
