#pragma once
#include "heroes_shade_stride.hpp"
#include "generated/shade_combat.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.h"
#include "JSystem/J3DGraphBase/J3DTransform.h"

namespace dawnlight {
// Each arena actor owns its sampling and transition state. Shared archive
// tracks remain read-only, including when two Shades play different abilities.
class ShadeStrideAnimation final : public J3DAnmTransformKey {
public:
    struct Quat { float x=0,y=0,z=0,w=1; };
    explicit ShadeStrideAnimation(const J3DAnmTransformKey& source,int motion=1)
        : J3DAnmTransformKey(source), original(&source), animation(motion) {}
    const J3DAnmTransformKey* original;
    int animation;
    float weight=0;
    std::array<Quat,6> previous{};
    float transition=1;

    static bool accepts(int motion,const J3DAnmTransform& source) {
        return motion>=0 && motion<35 && source.getKind()==8 && source.field_0x1e==37 &&
            source.getFrameMax()==shade::combat_pose::clips[motion].frames;
    }
    void advance() { transition=std::min(1.0f,transition+1.0f/6.0f); }
    template<class Q> void capture(const Q* quaternions) {
        if (!quaternions) return;
        for (unsigned i=0;i<previous.size();++i) {
            const auto& q=quaternions[shade::stride::joints[i]];
            previous[i]={q.x,q.y,q.z,q.w};
        }
        transition=0;
    }
    static Quat quaternion(s16 x,s16 y,s16 z) {
        constexpr float unit=3.14159265358979323846f/65536;
        float sx=std::sin(x*unit),cx=std::cos(x*unit),sy=std::sin(y*unit),cy=std::cos(y*unit);
        float sz=std::sin(z*unit),cz=std::cos(z*unit);
        return {sx*cy*cz-cx*sy*sz,cx*sy*cz+sx*cy*sz,cx*cy*sz-sx*sy*cz,cx*cy*cz+sx*sy*sz};
    }
    static void blend_rotation(J3DTransformInfo* out,Quat a,float mix) {
        auto b=quaternion(out->mRotation.x,out->mRotation.y,out->mRotation.z);
        const float sign=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0 ? -1.0f : 1.0f;
        Quat q{a.x*(1-mix)+sign*b.x*mix,a.y*(1-mix)+sign*b.y*mix,
               a.z*(1-mix)+sign*b.z*mix,a.w*(1-mix)+sign*b.w*mix};
        set_rotation(out,q);
    }
    static void set_rotation(J3DTransformInfo* out,Quat q) {
        const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
        if (n<0.00001f) return;
        q.x/=n;q.y/=n;q.z/=n;q.w/=n;
        constexpr float unit=32768/3.14159265358979323846f;
        const float sinY=std::clamp(2*(q.w*q.y-q.z*q.x),-1.0f,1.0f);
        // At the XYZ singularity choose an equivalent pose with Z=0 instead
        // of independently evaluating two ill-conditioned atan2 expressions.
        if (std::abs(sinY)>0.999999f) {
            out->mRotation.x=shade::stride::angle(unit*std::atan2(2*(q.w*q.x-q.y*q.z),1-2*(q.x*q.x+q.z*q.z)));
            out->mRotation.y=sinY>0 ? 16384 : -16384;
            out->mRotation.z=0;
            return;
        }
        out->mRotation.x=shade::stride::angle(unit*std::atan2(2*(q.w*q.x+q.y*q.z),1-2*(q.x*q.x+q.y*q.y)));
        out->mRotation.y=shade::stride::angle(unit*std::asin(sinY));
        out->mRotation.z=shade::stride::angle(unit*std::atan2(2*(q.w*q.z+q.x*q.y),1-2*(q.y*q.y+q.z*q.z)));
    }

    float correction(unsigned joint,int axis) const {
        const auto& clip=shade::combat_pose::clips[animation];
        if (!clip.delta) return axis==3 ? 1.0f : 0.0f;
        float time=std::max(0.0f,getFrame());
        if (clip.loop) time=std::fmod(time,static_cast<float>(clip.frames));
        else time=std::min(time,static_cast<float>(clip.frames));
        time*=shade::combat_pose::samples_per_frame;
        const int last=clip.frames*shade::combat_pose::samples_per_frame;
        const int frame=static_cast<int>(time);
        const float fraction=time-frame;
        auto sample=[&](int index) {
            if (clip.loop) index=(index%last+last)%last;
            else index=std::clamp(index,0,last);
            return clip.delta[index][joint][axis]/32767.0f;
        };
        return shade::stride::cubic(sample(frame-1),sample(frame),sample(frame+1),sample(frame+2),fraction);
    }

    void getTransform(u16 joint,J3DTransformInfo* out) const override {
        original->calcTransform(getFrame(),joint,out);
        const shade::stride::Frame frame(getFrame());
        // Ease into/out of the authored gait while keeping the original idle,
        // upper body, bone lengths and native animation-to-attack blending.
        const float mix=animation==1 ? weight*weight*(3-2*weight) : 0;
        if (joint==0 && mix>0) {
            out->mTranslate.x+=(frame.translation(0)-out->mTranslate.x)*mix;
            out->mTranslate.y+=(frame.translation(1)-out->mTranslate.y)*mix;
            out->mTranslate.z+=(frame.translation(2)-out->mTranslate.z)*mix;
        }
        for (unsigned i=0;i<shade::stride::joints.size();++i) {
            if (joint!=shade::stride::joints[i]) continue;
            if (animation==1) {
                auto blend=[&](s16 old,int axis) {
                    return shade::stride::angle(old+mix*shade::stride::angle_delta(frame.rotation(i,axis),old));
                };
                out->mRotation.x=blend(out->mRotation.x,0);
                out->mRotation.y=blend(out->mRotation.y,1);
                out->mRotation.z=blend(out->mRotation.z,2);
            } else if (shade::combat_pose::clips[animation].delta) {
                const Quat a{correction(i,0),correction(i,1),correction(i,2),correction(i,3)};
                const auto b=quaternion(out->mRotation.x,out->mRotation.y,out->mRotation.z);
                set_rotation(out,{a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
                    a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
                    a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
                    a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z});
            }
            if (transition<1) blend_rotation(out,previous[i],transition*transition*(3-2*transition));
            break;
        }
    }
};
} // namespace dawnlight
