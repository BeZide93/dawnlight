#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
namespace dawnlight::enemy_hud {
// Keep provider ownership independent for each of the three controls.
struct Options {
    bool bars=false,numbers=false,damage=false;
    bool operator==(const Options&) const = default;
};
inline Options local_effects(Options saved,Options te) {
    return {saved.bars&&!te.bars,saved.numbers&&!te.numbers&&(saved.bars||te.bars),saved.damage&&!te.damage};
}
inline bool disabled(unsigned index,Options saved,Options te) {
    return index==0?te.bars:index==1?te.numbers||!(saved.bars||te.bars):te.damage;
}
struct Health {
    uint32_t id=~0u;
    int hp=0,maximum=0;
    // Initial observation, healing and an Endless max-HP rescale aren't damage.
    unsigned observe(uint32_t actor,int current,int max) {
        const unsigned loss=id==actor&&maximum==max&&hp>0&&current<hp?unsigned(hp-std::max(0,current)):0;
        id=actor;hp=std::max(0,current);maximum=std::max(1,max);return loss;
    }
};
inline std::array<uint8_t,3> health_color(float ratio) {
    ratio=std::clamp(ratio,0.f,1.f);
    if(ratio>.5f){const float f=(ratio-.5f)*2;return {uint8_t((1-f)*230+f*30),uint8_t(f*190+(1-f)*170),uint8_t(f*70+(1-f)*30)};}
    const float f=ratio*2;return {uint8_t((1-f)*220+f*230),uint8_t(f*170),uint8_t((1-f)*40+f*30)};
}
}
