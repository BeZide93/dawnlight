#pragma once
#include <algorithm>

namespace dawnlight::survivors {
// Active game ticks only. A failed relocation is retried once per second,
// without resetting the damage clock or repeatedly probing geometry each frame.
struct BossWatchdog {
    unsigned quiet=0, retry=0;
    int health=-1;
    bool tick(int hp) {
        if(health<0 || hp<health)quiet=0;
        else if(quiet<300)++quiet;
        health=hp;
        if(retry)--retry;
        if(hp<=0 || quiet<300 || retry)return false;
        retry=30;return true;
    }
    void damaged(int hp) {health=hp;quiet=0;}
    void relocated() {quiet=retry=0;}
};
struct BossStagger {
    unsigned immunity=0;
    void tick() {if(immunity)--immunity;}
    bool accept() {if(immunity)return false;immunity=45;return true;}
};
inline constexpr unsigned boss_recovery_ticks=24;
inline constexpr unsigned boss_relocation_grace=30;
inline constexpr unsigned boss_telegraph_ticks=18;
inline constexpr float boss_lead_ticks=12.f;
inline constexpr float boss_lead_limit=240.f;
}
