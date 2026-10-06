#pragma once
#include "JSystem/J3DGraphAnimator/J3DAnimation.h"
#include "JSystem/J3DGraphBase/J3DTransform.h"
#include <array>
#include "dual_wield_math.hpp"

namespace dawnlight {
using DualGuardBodyPose=std::array<J3DTransformInfo,17>;
using DualChargeArmPose=std::array<J3DTransformInfo,4>;
inline dual::Quat joint_rotation(const J3DTransformInfo& transform) {
    constexpr float radians=3.14159265358979323846f/32768;
    return dual::from_euler({transform.mRotation.x*radians,transform.mRotation.y*radians,
                             transform.mRotation.z*radians});
}
inline void set_joint_rotation(J3DTransformInfo& transform,dual::Quat rotation) {
    constexpr float units=32768/3.14159265358979323846f;
    rotation=dual::normalized(rotation);
    auto angles=dual::euler(rotation);
    const float pitch=2*(rotation.w*rotation.y-rotation.z*rotation.x);
    if(std::abs(pitch)>1-1e-6f) {
        // Link's right clavicle is at +90-degree pitch in bind pose. At that
        // singularity roll/yaw must be resolved together, not from two noisy
        // atan2(0,0) calls which can rotate the shoulder another 90 degrees.
        angles={2*std::atan2(rotation.x,rotation.w),
                std::copysign(3.14159265358979323846f/2,pitch),0};
    }
    transform.mRotation.x=static_cast<s16>(std::lround(angles.x*units));
    transform.mRotation.y=static_cast<s16>(std::lround(angles.y*units));
    transform.mRotation.z=static_cast<s16>(std::lround(angles.z*units));
}
inline void pose_jump_charge_arm(DualChargeArmPose& bind) {
    constexpr float radians=3.14159265358979323846f/180;
    // Native Link's arm points along local +X. Lower the upper arm 65 degrees,
    // bend the elbow 75 degrees in its actual hinge axis, and turn the wrist
    // only 35 degrees. Keep the clavicle in its model-authored neutral pose.
    const std::array<dual::Vec,4> angles{{{}, {0,65*radians,0}, {0,0,75*radians}, {35*radians,0,0}}};
    for(size_t i=1;i<bind.size();++i) {
        auto turn=dual::from_euler(angles[i]);
        // Swing the entire chain 35 degrees forward around the neutral upper
        // arm axis, BEFORE lowering it. Post-multiplying would only roll the
        // lowered arm and change its elbow plane instead of lifting the hand.
        if(i==1) turn=dual::multiply(dual::from_euler({35*radians,0,0}),turn);
        set_joint_rotation(bind[i],dual::multiply(joint_rotation(bind[i]),turn));
    }
}
// Borrowed only during Link's body calculation. The native animation heaps,
// frame controllers and transition cache retain ownership of every BCK.
class DualWieldAnimation final : public J3DAnmTransformKey {
public:
    explicit DualWieldAnimation(const J3DAnmTransformKey& source,bool mirror=true,
                               const DualGuardBodyPose* guard=nullptr,float thrust=0,
                               const DualChargeArmPose* charge=nullptr,float chargeWeight=0)
        : J3DAnmTransformKey(source), original(&source), mirrored(mirror),guardPose(guard),thrust(thrust),
          chargePose(charge),chargeWeight(chargeWeight) {}
    const J3DAnmTransformKey* original;
    bool mirrored;
    const DualGuardBodyPose* guardPose;
    float thrust;
    const DualChargeArmPose* chargePose;
    float chargeWeight;
    void getTransform(u16 joint,J3DTransformInfo* out) const override {
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
        if(chargePose && joint>=11 && joint<=14) {
            original->calcTransform(getFrame(),joint,out);
            if(chargeWeight>=1) out->mRotation=(*chargePose)[joint-11].mRotation;
            else set_joint_rotation(*out,dual::blend(joint_rotation(*out),
                joint_rotation((*chargePose)[joint-11]),chargeWeight));
            // Native translation, scale, morph cache and all other joints stay
            // in the normal animation pipeline. No post-animation arm IK.
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
