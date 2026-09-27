#pragma once
#include "JSystem/J3DGraphAnimator/J3DAnimation.h"
#include "JSystem/J3DGraphBase/J3DTransform.h"
#include <array>
#include "dual_wield_math.hpp"

namespace dawnlight {
using DualGuardHeadPose=std::array<J3DTransformInfo,2>;
using DualGuardBodyPose=std::array<J3DTransformInfo,17>;
// Borrowed only during Link's body calculation. The native animation heaps,
// frame controllers and transition cache retain ownership of every BCK.
class DualWieldAnimation final : public J3DAnmTransformKey {
public:
    explicit DualWieldAnimation(const J3DAnmTransformKey& source,bool mirror=true,
                               const DualGuardBodyPose* guard=nullptr,float thrust=0,
                               const DualGuardHeadPose* head=nullptr,float headWeight=0)
        : J3DAnmTransformKey(source), original(&source), mirrored(mirror),guardPose(guard),thrust(thrust),headPose(head),headWeight(headWeight) {}
    const J3DAnmTransformKey* original;
    bool mirrored;
    const DualGuardBodyPose* guardPose;
    float thrust;
    const DualGuardHeadPose* headPose;
    float headWeight;
    void getTransform(u16 joint,J3DTransformInfo* out) const override {
        if(headPose && (joint==3 || joint==4)) {
            // Override only the animated neck/head recoil. Native look-at is
            // applied later by the joint callback, and children (face/hat)
            // inherit the corrected transforms during the same model pass.
            if(guardPose) *out=(*guardPose)[joint];
            else original->calcTransform(getFrame(),joint,out);
            const auto& neutral=(*headPose)[joint-3];
            constexpr float radians=3.14159265358979323846f/32768;
            const auto rotation=[](const J3DTransformInfo& t) {
                return dual::from_euler({t.mRotation.x*radians,t.mRotation.y*radians,t.mRotation.z*radians});
            };
            const auto angles=dual::euler(dual::blend(rotation(*out),rotation(neutral),headWeight));
            out->mRotation.x=static_cast<s16>(std::lround(angles.x/radians));
            out->mRotation.y=static_cast<s16>(std::lround(angles.y/radians));
            out->mRotation.z=static_cast<s16>(std::lround(angles.z/radians));
            out->mTranslate.x+=(neutral.mTranslate.x-out->mTranslate.x)*headWeight;
            out->mTranslate.y+=(neutral.mTranslate.y-out->mTranslate.y)*headWeight;
            out->mTranslate.z+=(neutral.mTranslate.z-out->mTranslate.z)*headWeight;
            return;
        }
        if(guardPose && joint>0 && joint<16) {
            // Leave root, pelvis and legs together in the native stepping
            // animation. Only the upper body loses the one-sided shield twist.
            *out=(*guardPose)[joint];
            if(joint==1) {
                J3DTransformInfo root;
                original->calcTransform(getFrame(),0,&root);
                constexpr float radians=3.14159265358979323846f/32768;
                auto orientation=[&](const J3DTransformInfo& t) {
                    return dual::from_euler({t.mRotation.x*radians,t.mRotation.y*radians,t.mRotation.z*radians});
                };
                const auto parent=orientation(root);
                const auto reference=orientation((*guardPose)[0]);
                const auto local=dual::multiply(dual::conjugate(parent),reference);
                // Cancel the animated root's turn/lean for the chest only.
                // No extra pitch: just a small, straight 3-unit forward shift.
                const auto angles=dual::euler(dual::multiply(local,orientation(*out)));
                out->mRotation.x=static_cast<s16>(std::lround(angles.x/radians));
                out->mRotation.y=static_cast<s16>(std::lround(angles.y/radians));
                out->mRotation.z=static_cast<s16>(std::lround(angles.z/radians));
                const auto position=dual::rotate(local,{out->mTranslate.x,out->mTranslate.y,out->mTranslate.z})+
                    dual::rotate(dual::conjugate(parent),{0,0,3*thrust});
                out->mTranslate.x=position.x;out->mTranslate.y=position.y;out->mTranslate.z=position.z;
            }
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
