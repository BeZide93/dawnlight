#include "dual_wield_math.hpp"
#include <cassert>
#include <random>
using namespace dawnlight::dual;

void near(Vec a,Vec b,float tolerance=.003f) { assert(length(a-b)<tolerance); }
void lengths(const Arm& a) {
    assert(std::abs(length(a.lower.p-a.upper.p)-29)<.003f);
    assert(std::abs(length(a.hand.p-a.lower.p)-26.5f)<.003f);
}
int main() {
    // Antiparallel rotations and q/-q interpolation must not flip a blade.
    near(rotate(between({1,0,0},{-1,0,0}),{1,0,0}),{-1,0,0});
    Quat q=between({1,0,0},{0,1,0});
    near(rotate(blend(q,{-q.x,-q.y,-q.z,-q.w},.5f),{1,0,0}),{0,1,0});
    Pose mount{q,{9.66f,2.14f,-3.02f}};
    near(compose(mount,inverse(mount)).p,{});
    Vec angles=euler(q);
    assert(std::abs(angles.x)<.001f && std::abs(angles.y)<.001f);
    assert(std::abs(angles.z-1.57079632679f)<.001f);

    Arm original{{{},{}},{{},{29,0,0}},{{},{29,26.5f,0}}};
    std::mt19937 random(42);
    std::uniform_real_distribution<float> coordinate(-70,70);
    for(int i=0;i<1000;++i) {
        Pose target{between({1,0,0},{coordinate(random),coordinate(random),coordinate(random)}),
                    {coordinate(random),coordinate(random),coordinate(random)}};
        const float distance=length(target.p);
        for(int step=0;step<=10;++step) lengths(reach(original,target,smooth(step/10.0f)));
        auto solved=reach(original,target,1);
        if(distance>2.51f && distance<55.49f) near(solved.hand.p,target.p);
        // The same solve must work after Link turns or moves in the world.
        Pose world{between({0,0,1},unit({coordinate(random),0,coordinate(random)})),{50,120,-200}};
        Arm moved{compose(world,original.upper),compose(world,original.lower),compose(world,original.hand)};
        const auto rotated=reach(moved,compose(world,target),1);
        near(rotated.hand.p,compose(world,solved.hand).p,.01f);
    }
    // Degenerate and unreachable targets stay finite and preserve limb lengths.
    lengths(reach(original,{{},{}},1));
    lengths(reach(original,{{},{10000,0,0}},1));
    near(reach(original,{{},{1,1,1}},0).hand.p,original.hand.p);
    Arm zero{};near(reach(zero,{{},{100,100,100}},1).hand.p,{});

    // Use the native left/right grip mounts and the asymmetric guard stance.
    // The right shoulder leads, so Ordon must remain ahead after reach limits.
    Arm arms[2];Pose blades[2];
    const Pose guardMount[2]={
        {{-.579250f,-.405549f,.405549f,.579250f},{9.664279f,2.136274f,3.018636f}},
        {{.579250f,.405549f,.405549f,.579250f},{9.664279f,2.136274f,-3.018636f}}};
    for(int i=0;i<2;++i) {
        Vec shoulder{i ? -18.0f : 18.0f,130,i ? 16.0f : -10.0f};
        arms[i]={{{},shoulder},{{},shoulder+Vec{0,-29,0}},{{},shoulder+Vec{0,-55.5f,0}}};
        blades[i]=cross_guard_blade(i ? -1.0f : 1.0f,i!=0,0);
    }
    float restDepth=0;
    for(float thrust:{0.0f,.5f,1.0f}) {
        float plane=44+16*thrust;
        Arm reaching[2];
        for(int i=0;i<2;++i) {
            const Pose clavicle{{},{0,132,2}};
            const Pose follow=shoulder_follow(clavicle,arms[i].upper,{0,0,1},.5f,thrust);
            reaching[i]={compose(follow,arms[i].upper),compose(follow,arms[i].lower),compose(follow,arms[i].hand)};
            plane=std::min(plane,max_sword_depth(reaching[i],blades[i],guardMount[i])-(i ? 6 : -6));
        }
        Pose actual[2];
        for(int i=0;i<2;++i) {
            blades[i].p.z=plane+(i ? 6 : -6);
            auto solved=reach(reaching[i],compose(blades[i],inverse(guardMount[i])),1);
            actual[i]=compose(solved.hand,guardMount[i]);
            near(actual[i].p,blades[i].p);
            if(thrust==0) assert(solved.hand.p.z>36); // both wrists in front of the torso
            Vec direction=rotate(actual[i].q,{1,0,0});
            assert(direction.z/direction.y<.45f); // upright, not the previous 45-degree lean
            Vec crossing=actual[i].p+direction*(-actual[i].p.x/direction.x);
            assert(crossing.z>43); // rear blade also stays ahead of the face
        }
        assert(std::abs(actual[1].p.z-actual[0].p.z-12)<.003f);
        if(thrust==0) restDepth=actual[0].p.z;
        if(thrust==1) assert(actual[0].p.z-restDepth>8); // visible extension still available
    }

    // Eight ticks to draw/stow with no overshoot, including mid-transition reversal.
    float draw=0;
    for(int i=0;i<8;++i) draw=approach(draw,1,1.0f/8);
    assert(draw==1);
    for(int i=0;i<3;++i) draw=approach(draw,0,1.0f/8);
    assert(draw==.625f);
    for(int i=0;i<8;++i) draw=approach(draw,1,1.0f/8);
    assert(draw==1);
    assert(smooth(0)==0 && smooth(1)==1);
    assert(smooth(.001f)<.00001f && 1-smooth(.999f)<.00001f);

    Pose hip{between({1,0,0},unit({0,-1,-.4f})),{18,105,5}};
    Pose start{{},{-25,90,10}},rest{{},{-25,80,5}};
    near(sheath_hand_target(start,hip,rest,{},0,false).p,start.p);
    near(sheath_hand_target(start,hip,rest,{},.75f,false).p,hip.p);
    near(sheath_hand_target(start,hip,rest,{},1,false).p,rest.p);
    float previous=-43;
    for(int i=0;i<=40;++i) {
        const Pose p=sheath_hand_target(start,hip,rest,{},.35f+i*.01f,false);
        const Pose relative=compose(inverse(hip),p);
        // Insertion is axial: no lateral sweep through the scabbard wall.
        assert(std::abs(relative.p.y)<.001f && std::abs(relative.p.z)<.001f);
        assert(relative.p.x>=previous-.001f && relative.p.x<=.001f);
        previous=relative.p.x;
        near(rotate(relative.q,{1,0,0}),{1,0,0});
    }
    // Native neutral hand/hip geometry, including the actual grip offset.
    // Sample the solved wrist, not only the requested curve: IK clamping can
    // otherwise conceal a route that still cuts across the torso.
    const Pose sheath{{-.133265f,-.148526f,.694090f,-.691678f},{19.239038f,105.013702f,4.7635f}};
    const Pose gripMount{{.579250f,.405549f,.405549f,.579250f},{9.664279f,2.136274f,-3.018636f}};
    const Pose held{{.560311f,-.397551f,.279636f,.670678f},{-26.408121f,74.036564f,7.210265f}};
    const Arm native{{{-.407928f,.416675f,.701111f,-.410391f},{-16.337433f,134.640355f,2.338878f}},
                     {{.272577f,-.515485f,-.555439f,.592844f},{-25.917520f,108.093595f,-4.331315f}},
                     {{.210702f,-.437019f,-.567528f,.665229f},{-29.852111f,83.194294f,3.841432f}}};
    const Vec elbow{-16.337433f,134.640355f,62.338878f};
    const Pose clavicle{{},{-.749549f,136.704381f,2.512908f}};
    for(bool drawing:{false,true}) {
        Vec previousHand{};
        for(int i=0;i<=1000;++i) {
            const float t=i/1000.0f;
            const Pose blade=sheath_hand_target(held,sheath,held,gripMount,t,drawing);
            const Pose hand=compose(blade,inverse(gripMount));
            const Pose follow=shoulder_follow(clavicle,native.upper,{0,0,1},t);
            const Arm moved{compose(follow,native.upper),compose(follow,native.lower),compose(follow,native.hand)};
            const float poleWeight=smooth(t/.12f)*smooth((1-t)/.12f);
            const Vec pole=moved.lower.p*(1-poleWeight)+(moved.upper.p+Vec{0,0,60})*poleWeight;
            const Arm solved=reach(moved,hand,1,&pole);
            if(i==0 || i==1000) near(solved.lower.p,native.lower.p,.01f);
            if(std::abs(solved.hand.p.x)<16) assert(solved.hand.p.z>20);
            if(i) assert(length(solved.hand.p-previousHand)<1.5f);
            previousHand=solved.hand.p;
            // The forearm also stays in front of a conservative torso envelope.
            for(int j=1;j<=10;++j) {
                const Vec p=solved.lower.p+(solved.hand.p-solved.lower.p)*(j/10.0f);
                if(p.y>90 && p.y<130) assert(p.x*p.x/(20*20)+p.z*p.z/(14*14)>=1);
            }
        }
        const float contact=drawing ? draw_grip_start : stow_insert_end;
        const Pose blade=sheath_hand_target(held,sheath,held,gripMount,contact,drawing);
        const Arm solved=reach(native,compose(blade,inverse(gripMount)),1,&elbow);
        near(compose(solved.hand,gripMount).p,sheath.p,.01f); // no snap at grip/release
    }
    StowMotion drawMotion;drawMotion.drawing=true;drawMotion.lastFrame=22;
    drawMotion.advance(11,0);assert(drawMotion.progress==.5f);
    drawMotion.advance(11,0);assert(drawMotion.progress==.5f);
    drawMotion.advance(0,0);assert(drawMotion.progress==1);
    StowMotion stow;stow.active=true;stow.duration=18;
    stow.advance(9,0);assert(stow.progress==.5f);
    stow.advance(9,0);assert(stow.progress==.5f); // extra model calc cannot advance it
    stow.advance(2,1);assert(stow.elapsed==11); // flourish phase/controller reset
    stow.advance(20,1);assert(stow.progress==1);

    Alternation attacks;
    for(int i=0;i<12;++i) { attacks.begin();assert(attacks.right==bool(i%2)); }
    attacks.reset();attacks.begin();assert(!attacks.right);
}
