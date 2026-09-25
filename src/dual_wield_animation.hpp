#pragma once
#include "JSystem/J3DGraphAnimator/J3DAnimation.h"
#include "JSystem/J3DGraphBase/J3DTransform.h"
#include <array>

namespace dawnlight {
using DualGuardBodyPose=std::array<J3DTransformInfo,17>;
// Borrowed only during Link's body calculation. The native animation heaps,
// frame controllers and transition cache retain ownership of every BCK.
class DualWieldAnimation final : public J3DAnmTransformKey {
public:
    explicit DualWieldAnimation(const J3DAnmTransformKey& source,bool mirror=true,
                               const DualGuardBodyPose* guard=nullptr,float thrust=0)
        : J3DAnmTransformKey(source), original(&source), mirrored(mirror),guardPose(guard),thrust(thrust) {}
    const J3DAnmTransformKey* original;
    bool mirrored;
    const DualGuardBodyPose* guardPose;
    float thrust;
    void getTransform(u16 joint,J3DTransformInfo* out) const override {
        if(guardPose && joint<guardPose->size()) {
            // Hold the entering guard's root/pelvis and torso instead of the
            // shield bash's one-sided twist. Spine local Z leans straight ahead.
            *out=(*guardPose)[joint];
            if(joint==1) out->mRotation.z=static_cast<s16>(out->mRotation.z+static_cast<int>(910*thrust));
            return;
        }
        if(!mirrored) { original->calcTransform(getFrame(),joint,out);return; }
        const bool torso=joint>=1 && joint<=4;
        const bool left=joint>=6 && joint<=9;
        const bool right=joint>=11 && joint<=14;
        original->calcTransform(getFrame(),left ? joint+5 : right ? joint-5 : joint,out);
        if(torso || left || right) {
            // The paired arm bones use local Z as their mirror axis. Reflect
            // both sides of the rotation so the result remains right-handed.
            out->mRotation.x=static_cast<s16>(-static_cast<int>(out->mRotation.x));
            out->mRotation.y=static_cast<s16>(-static_cast<int>(out->mRotation.y));
            out->mTranslate.z=-out->mTranslate.z;
        }
    }
};
}
