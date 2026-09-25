#pragma once
#include <algorithm>
#include <cmath>

namespace dawnlight::dual {
struct Vec {
    float x=0,y=0,z=0;
    Vec operator+(Vec b) const { return {x+b.x,y+b.y,z+b.z}; }
    Vec operator-(Vec b) const { return {x-b.x,y-b.y,z-b.z}; }
    Vec operator*(float s) const { return {x*s,y*s,z*s}; }
};
inline float dot(Vec a,Vec b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec cross(Vec a,Vec b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline float length(Vec a) { return std::sqrt(dot(a,a)); }
inline Vec unit(Vec a,Vec fallback={1,0,0}) { float n=length(a); return n>1e-6f ? a*(1/n) : fallback; }
struct Quat { float x=0,y=0,z=0,w=1; };
inline Quat normalized(Quat q) {
    float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    return n>1e-6f ? Quat{q.x/n,q.y/n,q.z/n,q.w/n} : Quat{};
}
inline Quat conjugate(Quat q) { return {-q.x,-q.y,-q.z,q.w}; }
inline Quat multiply(Quat a,Quat b) {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline Vec rotate(Quat q,Vec v) {
    Vec xyz{q.x,q.y,q.z};Vec t=cross(xyz,v)*2;
    return v+t*q.w+cross(xyz,t);
}
// J3D's Euler convention is Rz * Ry * Rx; return radians in that order.
inline Vec euler(Quat q) {
    q=normalized(q);
    return {std::atan2(2*(q.w*q.x+q.y*q.z),1-2*(q.x*q.x+q.y*q.y)),
            std::asin(std::clamp(2*(q.w*q.y-q.z*q.x),-1.0f,1.0f)),
            std::atan2(2*(q.w*q.z+q.x*q.y),1-2*(q.y*q.y+q.z*q.z))};
}
inline Quat blend(Quat a,Quat b,float t) {
    float sign=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0 ? -1.0f : 1.0f;
    return normalized({a.x*(1-t)+b.x*t*sign,a.y*(1-t)+b.y*t*sign,
        a.z*(1-t)+b.z*t*sign,a.w*(1-t)+b.w*t*sign});
}
inline Quat between(Vec a,Vec b) {
    a=unit(a);b=unit(b);float d=std::clamp(dot(a,b),-1.0f,1.0f);
    if(d<-.99999f) { Vec axis=unit(cross(a,std::abs(a.x)<.8f ? Vec{1,0,0} : Vec{0,1,0})); return {axis.x,axis.y,axis.z,0}; }
    Vec c=cross(a,b);return normalized({c.x,c.y,c.z,1+d});
}
struct Pose { Quat q;Vec p; };
inline Pose compose(Pose a,Pose b) { return {multiply(a.q,b.q),a.p+rotate(a.q,b.p)}; }
inline Pose inverse(Pose a) { Quat q=conjugate(a.q); return {q,rotate(q,a.p*-1)}; }
inline Pose blend(Pose a,Pose b,float t) { return {blend(a.q,b.q,t),a.p*(1-t)+b.p*t}; }
inline float smooth(float t) { t=std::clamp(t,0.0f,1.0f);return t*t*(3-2*t); }
inline float approach(float value,float target,float step) {
    return value<target ? std::min(target,value+step) : std::max(target,value-step);
}
constexpr float stow_insert_end=.75f;
inline Pose stow_hand_target(Pose start,Pose hip,Pose rest,float progress) {
    // Local +X points down the blade. Approach the sheath from above its
    // mouth, align first, then slide along its axis before releasing the grip.
    const Pose raised=compose(hip,Pose{{},{-42,0,0}});
    if(progress<.35f) return blend(start,raised,smooth(progress/.35f));
    if(progress<stow_insert_end)
        return blend(raised,hip,smooth((progress-.35f)/(stow_insert_end-.35f)));
    return blend(hip,rest,smooth((progress-stow_insert_end)/(1-stow_insert_end)));
}
struct StowMotion {
    bool active=false,special=false;
    int phase=0;
    float elapsed=0,lastFrame=0,duration=22,progress=0,release=0;
    Pose start;
    void advance(float frame,int nextPhase) {
        // Flourish changes from FINISH to FINISH_END and resets its controller.
        if(nextPhase==phase) elapsed+=std::max(0.0f,frame-lastFrame);
        else {elapsed+=std::max(0.0f,frame);phase=nextPhase;}
        lastFrame=frame;
        progress=std::clamp(elapsed/std::max(1.0f,duration),0.0f,1.0f);
    }
};
// Two-bone reach with a pole from the native elbow. Keeps the original lengths
// even while blending into the cross guard or reaching down to the hip sheath.
struct Arm { Pose upper,lower,hand; };
// Work in actor space (+Z forward). Limit the shared guard plane using both
// arms, rather than allowing IK to pull just the front blade behind its mate.
inline float max_sword_depth(Arm arm,Pose sword,Pose mount) {
    const Vec offset=compose(sword,inverse(mount)).p-sword.p;
    const Vec hand=sword.p+offset;
    const float radius=std::max(0.0f,length(arm.lower.p-arm.upper.p)+length(arm.hand.p-arm.lower.p)-.5f);
    const float dx=hand.x-arm.upper.p.x,dy=hand.y-arm.upper.p.y;
    return arm.upper.p.z+std::sqrt(std::max(0.0f,radius*radius-dx*dx-dy*dy))-offset.z;
}
inline Arm reach(Arm source,Pose target,float weight) {
    const float l1=length(source.lower.p-source.upper.p),l2=length(source.hand.p-source.lower.p);
    if(l1<1e-3f || l2<1e-3f || weight<=0) return source;
    Vec delta=target.p-source.upper.p;Vec direction=unit(delta);
    float d=std::clamp(length(delta),std::abs(l1-l2)+.01f,l1+l2-.01f);
    Vec pole=source.lower.p-source.upper.p;
    pole=pole-direction*dot(pole,direction);
    if(length(pole)<.01f) pole=cross(direction,{0,0,1});
    if(length(pole)<.01f) pole=cross(direction,{0,1,0});
    pole=unit(pole);
    float along=(l1*l1-l2*l2+d*d)/(2*d);
    Vec elbow=source.upper.p+direction*along+pole*std::sqrt(std::max(0.0f,l1*l1-along*along));
    Vec hand=source.upper.p+direction*d;
    Quat upper=multiply(between(source.lower.p-source.upper.p,elbow-source.upper.p),source.upper.q);
    Quat lower=multiply(between(source.hand.p-source.lower.p,hand-elbow),source.lower.q);
    Vec localLower=rotate(conjugate(source.upper.q),source.lower.p-source.upper.p);
    Vec localHand=rotate(conjugate(source.lower.q),source.hand.p-source.lower.p);
    Arm out=source;
    out.upper.q=blend(source.upper.q,upper,weight);
    out.lower.q=blend(source.lower.q,lower,weight);
    out.lower.p=out.upper.p+rotate(out.upper.q,localLower);
    out.hand.p=out.lower.p+rotate(out.lower.q,localHand);
    out.hand.q=blend(source.hand.q,target.q,weight);
    return out;
}
struct Alternation {
    bool nextRight=false,right=false;
    void begin() { right=nextRight;nextRight=!nextRight; }
    void reset() { nextRight=right=false; }
};
} // namespace dawnlight::dual
