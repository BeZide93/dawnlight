#pragma once
#include "generated/shade_stride.hpp"
#include <algorithm>
#include <cmath>

namespace dawnlight::shade::stride {
inline float angle_delta(float a, float b) { return std::remainder(a-b,65536.0f); }
inline std::int16_t angle(float value) {
    int rounded=static_cast<int>(std::lround(value));
    rounded=(rounded%65536+65536)%65536;
    return static_cast<std::int16_t>(rounded>=32768 ? rounded-65536 : rounded);
}
inline float cubic(float a,float b,float c,float d,float t) {
    return b+0.5f*t*(c-a+t*(2*a-5*b+4*c-d+t*(3*(b-c)+d-a)));
}
struct Frame {
    int index[4];
    float fraction;
    explicit Frame(float time) {
        time=std::fmod(std::max(0.0f,time),static_cast<float>(frames));
        const int frame=static_cast<int>(time);
        fraction=time-frame;
        for (int i=0;i<4;++i) index[i]=(frame+i-1+frames)%frames;
    }
    float rotation(int joint,int axis) const {
        const float b=rotations[index[1]][joint][axis];
        const float a=b+angle_delta(rotations[index[0]][joint][axis],b);
        const float c=b+angle_delta(rotations[index[2]][joint][axis],b);
        const float d=c+angle_delta(rotations[index[3]][joint][axis],c);
        return cubic(a,b,c,d,fraction);
    }
    float translation(int axis) const {
        return cubic(root[index[0]][axis],root[index[1]][axis],
            root[index[2]][axis],root[index[3]][axis],fraction);
    }
};
inline float approach_speed(float current) { return std::min(speed,current+0.8f); }
inline float blend_weight(float current,bool moving) {
    return std::clamp(current+(moving ? 0.125f : -0.125f),0.0f,1.0f);
}
} // namespace dawnlight::shade::stride
