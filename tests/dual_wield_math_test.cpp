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

    Alternation attacks;
    for(int i=0;i<12;++i) { attacks.begin();assert(attacks.right==bool(i%2)); }
    attacks.reset();attacks.begin();assert(!attacks.right);
}
