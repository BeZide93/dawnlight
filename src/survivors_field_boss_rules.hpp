#pragma once
namespace dawnlight::survivors {
inline constexpr float boss_ring_radius=1500.f;
struct AeralfosCycle {
    unsigned ticks=0;
    bool ground=false;
    bool advance() {
        if(++ticks<300)return false;
        ticks=0;ground=!ground;return true;
    }
};
struct KingWaveClock {
    bool normalWindow=false;
    unsigned spinTicks=0;
    unsigned advance(bool normal,bool spin) {
        const bool strike=normal&&!normalWindow;
        normalWindow=normal;
        if(!spin){spinTicks=0;return strike?1:0;}
        const bool volley=spinTicks==0;
        spinTicks=(spinTicks+1)%30;
        return volley?5:0;
    }
};
}
