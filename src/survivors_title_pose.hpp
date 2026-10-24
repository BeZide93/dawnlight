#pragma once
#include "dual_wield_math.hpp"
#include <array>

namespace dawnlight::survivors {
// Actor-local poses, with the model's own hierarchy and bone lengths. All
// descendants (hands, item sockets, hat and clothes) follow through normal FK.
struct TitleRestPose {
    static constexpr unsigned count = 35;
    std::array<dual::Pose,count> local{}, world{};
    std::array<int,count> parent{};
    bool evaluate() {
        for (unsigned i=0;i<count;++i) {
            if (parent[i]>=int(i) || parent[i]<-1) return false;
            world[i]=parent[i]<0 ? local[i] : dual::compose(world[parent[i]],local[i]);
        }
        return true;
    }
    void orient(unsigned joint, dual::Quat rotation) {
        local[joint].q=parent[joint]<0 ? rotation :
            dual::multiply(dual::conjugate(world[parent[joint]].q),rotation);
        evaluate();
    }
    void aim(unsigned joint,unsigned child,dual::Vec direction) {
        orient(joint,dual::multiply(dual::between(world[child].p-world[joint].p,direction),world[joint].q));
    }
    void arm(unsigned first,dual::Vec target,dual::Vec pole) {
        const dual::Arm source{world[first],world[first+1],world[first+2]};
        const auto solved=dual::reach(source,{source.hand.q,target},1,&pole);
        orient(first,solved.upper.q);
        orient(first+1,solved.lower.q);
        orient(first+2,solved.hand.q);
    }
    bool kneel() {
        if (!evaluate()) return false;
        // Native human rig: left thigh/shin/ankle 18/19/20, right 23/24/25.
        const float thigh=dual::length(world[24].p-world[23].p);
        const float shin=dual::length(world[20].p-world[19].p);
        if (thigh<1 || shin<1) return false;
        const auto leftFoot=world[20].q, rightFoot=world[25].q;
        const float ankleHeight=world[20].p.y;
        const float kneeHeight=thigh*.2f;
        local[0].p.y+=thigh+kneeHeight-world[23].p.y;
        evaluate();
        // Right knee rests on the floor; the shin extends backwards.
        aim(23,24,{0,-1,0});
        const float rearShin=dual::length(world[25].p-world[24].p);
        const float rise=std::clamp(ankleHeight-world[24].p.y,-rearShin*.4f,rearShin*.4f);
        aim(24,25,{0,rise,-std::sqrt(rearShin*rearShin-rise*rise)});
        // Turn the trailing sole upward, then pitch its backward-pointing
        // toes down by 20 degrees so the instep approaches the floor. A full
        // half turn leaves the sole horizontal and the front of the boot raised.
        // Rotate in actor space; keep the ankle, planted foot and leg lengths.
        constexpr float rearFootPitch=2.792526803f; // 180 - 20 degrees.
        orient(25,dual::multiply(dual::from_euler({rearFootPitch,0,0}),rightFoot));
        // Left foot is planted in front, its knee raised above the hip.
        const float frontThigh=dual::length(world[19].p-world[18].p);
        const float lift=std::clamp(shin+ankleHeight-world[18].p.y,-frontThigh*.8f,frontThigh*.8f);
        aim(18,19,{0,lift,std::sqrt(frontThigh*frontThigh-lift*lift)});
        aim(19,20,{0,-1,0});
        orient(20,leftFoot);
        // A quiet forward lean and lowered gaze, with the left hand resting
        // above the raised knee and the other hanging beside the hip.
        orient(1,dual::multiply(dual::from_euler({.12f,0,0}),world[1].q));
        orient(4,dual::multiply(dual::from_euler({.18f,0,0}),world[4].q));
        const auto knee=world[19].p, hip=world[23].p;
        arm(7,knee+dual::Vec{0,8,3},world[7].p+dual::Vec{22,-28,8});
        arm(12,hip+dual::Vec{-8,2,8},world[12].p+dual::Vec{-18,-25,0});
        return true;
    }
};
}
