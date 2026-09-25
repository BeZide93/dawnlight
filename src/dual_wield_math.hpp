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
// Local X/Y/Z angles in radians; J3D composes them as Rz * Ry * Rx.
inline Quat from_euler(Vec a) {
    const float sx=std::sin(a.x*.5f),cx=std::cos(a.x*.5f);
    const float sy=std::sin(a.y*.5f),cy=std::cos(a.y*.5f);
    const float sz=std::sin(a.z*.5f),cz=std::cos(a.z*.5f);
    return {sx*cy*cz-cx*sy*sz,cx*sy*cz+sx*cy*sz,
            cx*cy*sz-sx*sy*cz,cx*cy*cz+sx*sy*sz};
}
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
inline Pose cross_guard_blade(float side,bool right,float thrust) {
    // Keep the blades upright and the grips away from the torso. The right
    // shoulder leads this stance, so the Ordon blade occupies the front plane.
    return {between({1,0,0},unit({-.65f*side,.75f,.32f})),
            {18*side,112,44+16*thrust+(right ? 6.0f : -6.0f)}};
}
constexpr float stow_insert_end=.75f;
constexpr float draw_grip_start=.30f;
// Actor-local hand path (+Z forward). Two forward control points take the
// hand around the chest, rather than through it. Ease both ends for a clean
// handoff to the axial part of the motion and to the native idle pose.
inline Pose front_arc(Pose a,Pose b,float progress) {
    const float t=smooth(progress),u=1-t;
    const float front=std::max({44.0f,a.p.z,b.p.z});
    const Vec c1{a.p.x,a.p.y,front},c2{b.p.x,b.p.y,front};
    return {blend(a.q,b.q,t),a.p*(u*u*u)+c1*(3*u*u*t)+c2*(3*u*t*t)+b.p*(t*t*t)};
}
inline Pose sheath_hand_target(Pose start,Pose hip,Pose rest,Pose mount,float progress,bool drawing) {
    // Route the HAND, not the sword origin: rotating the grip mount must not
    // swing the wrist back through Link. All arguments are actor-local.
    const Pose unmount=inverse(mount);
    const Pose grip=compose(hip,unmount);
    const Pose raised=compose(compose(hip,Pose{{},{-42,0,0}}),unmount);
    const Pose first=compose(start,unmount),last=compose(rest,unmount);
    Pose hand;
    if(drawing) {
        if(progress<draw_grip_start) hand=front_arc(first,grip,progress/draw_grip_start);
        else if(progress<.65f) hand=blend(grip,raised,smooth((progress-draw_grip_start)/(.65f-draw_grip_start)));
        else hand=front_arc(raised,last,(progress-.65f)/.35f);
    } else {
        if(progress<.35f) hand=front_arc(first,raised,progress/.35f);
        else if(progress<stow_insert_end) hand=blend(raised,grip,smooth((progress-.35f)/(stow_insert_end-.35f)));
        else hand=front_arc(grip,last,(progress-stow_insert_end)/(1-stow_insert_end));
    }
    return compose(hand,mount);
}
struct StowMotion {
    bool active=false,special=false,drawing=false,nativeClock=true;
    int phase=0;
    float elapsed=0,lastFrame=0,duration=22,progress=0,release=0;
    Pose start;
    void advance(float frame,int nextPhase) {
        // Equip runs backwards; flourish changes clip and resets its controller.
        if(nextPhase==phase) elapsed+=std::max(0.0f,drawing ? lastFrame-frame : frame-lastFrame);
        else {elapsed+=std::max(0.0f,frame);phase=nextPhase;}
        lastFrame=frame;
        progress=std::clamp(elapsed/std::max(1.0f,duration),0.0f,1.0f);
    }
};
// Let the clavicle follow a cross-body reach, preserving its length. Without
// this small shoulder movement the forearm grazes the chest near the hilt.
inline Pose shoulder_follow(Pose clavicle,Pose upper,Vec forward,float progress,float weight=1) {
    const float amount=smooth(progress/.20f)*smooth((1-progress)/.20f)*weight;
    const Vec bone=upper.p-clavicle.p;
    const Quat turn=blend(Quat{},between(bone,bone+forward*12),amount);
    return {turn,clavicle.p-rotate(turn,clavicle.p)};
}
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
inline Arm reach(Arm source,Pose target,float weight,const Vec* elbowPole=nullptr) {
    const float l1=length(source.lower.p-source.upper.p),l2=length(source.hand.p-source.lower.p);
    if(l1<1e-3f || l2<1e-3f || weight<=0) return source;
    Vec delta=target.p-source.upper.p;Vec direction=unit(delta);
    float d=std::clamp(length(delta),std::abs(l1-l2)+.01f,l1+l2-.01f);
    Vec pole=(elbowPole ? *elbowPole : source.lower.p)-source.upper.p;
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
