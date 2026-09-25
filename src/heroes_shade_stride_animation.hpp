#pragma once
#include "heroes_shade_stride.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.h"
#include "JSystem/J3DGraphBase/J3DTransform.h"

namespace dawnlight {
// Each arena actor owns its sampling state. The game's shared KN_STEP and its
// tracks are never edited; the source remains alive in that actor's archive.
class ShadeStrideAnimation final : public J3DAnmTransformKey {
public:
    explicit ShadeStrideAnimation(const J3DAnmTransformKey& source)
        : J3DAnmTransformKey(source), original(&source) {}
    const J3DAnmTransformKey* original;
    float weight=0;

    void getTransform(u16 joint,J3DTransformInfo* out) const override {
        original->calcTransform(getFrame(),joint,out);
        if (weight<=0) return;
        const shade::stride::Frame frame(getFrame());
        // Ease into/out of the authored gait while keeping the original idle,
        // upper body, bone lengths and native animation-to-attack blending.
        const float mix=weight*weight*(3-2*weight);
        if (joint==0) {
            out->mTranslate.x+=(frame.translation(0)-out->mTranslate.x)*mix;
            out->mTranslate.y+=(frame.translation(1)-out->mTranslate.y)*mix;
            out->mTranslate.z+=(frame.translation(2)-out->mTranslate.z)*mix;
        }
        for (unsigned i=0;i<shade::stride::joints.size();++i) {
            if (joint!=shade::stride::joints[i]) continue;
            auto blend=[&](s16 old,int axis) {
                return shade::stride::angle(old+mix*shade::stride::angle_delta(frame.rotation(i,axis),old));
            };
            out->mRotation.x=blend(out->mRotation.x,0);
            out->mRotation.y=blend(out->mRotation.y,1);
            out->mRotation.z=blend(out->mRotation.z,2);
            break;
        }
    }
};
} // namespace dawnlight
