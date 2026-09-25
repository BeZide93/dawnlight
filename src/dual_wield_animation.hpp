#pragma once
#include "JSystem/J3DGraphAnimator/J3DAnimation.h"
#include "JSystem/J3DGraphBase/J3DTransform.h"

namespace dawnlight {
// Borrowed only during Link's body calculation. The native animation heaps,
// frame controllers and transition cache retain ownership of every BCK.
class DualWieldAnimation final : public J3DAnmTransformKey {
public:
    explicit DualWieldAnimation(const J3DAnmTransformKey& source)
        : J3DAnmTransformKey(source), original(&source) {}
    const J3DAnmTransformKey* original;
    void getTransform(u16 joint,J3DTransformInfo* out) const override {
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
