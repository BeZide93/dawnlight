#include "../src/boss_mirror_geometry.hpp"

#include <cassert>
#include <cmath>
#include <limits>

using namespace dawnlight::mirror_geometry;
float dot(const Vec& a, const Vec& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Vec transform(const Matrix& m, const Vec& v) {
    Vec result{};
    for (unsigned r=0;r<3;++r) result[r]=dot({m[r][0],m[r][1],m[r][2]},v)+m[r][3];
    return result;
}
bool close(float a,float b) { return std::abs(a-b)<0.02f; }
int main() {
    for (unsigned thin=0;thin<3;++thin) for (unsigned boss=0;boss<18;++boss) {
        // Deliberately off-center bind geometry, as used by chamber props.
        Vec min={1200,4500,-21000}, max={1400,4700,-20800};
        max[thin]=min[thin]+10;
        const Vec pivot={(min[0]+max[0])/2,(min[1]+max[1])/2,(min[2]+max[2])/2};
        const float angle=6.28318530718f*boss/18;
        const Vec center={std::sin(angle)*1450,1340,std::cos(angle)*1450};
        Fit fit;
        assert(dawnlight::mirror_geometry::fit(min,max,center,angle+3.14159265359f,440,fit));
        const auto actualCenter=transform(fit.meshToWorld,pivot);
        for (unsigned i=0;i<3;++i) assert(close(actualCenter[i],center[i]));
        assert(close(dot(fit.normal,fit.normal),1));
        assert(close(dot(fit.right,fit.normal),0));
        assert(close(dot(fit.up,fit.normal),0));
        assert(close(dot(fit.right,fit.up),0));
        assert(close(std::sqrt(dot(fit.right,fit.right)),360.8f));
        assert(close(std::sqrt(dot(fit.up,fit.up)),360.8f));
        assert(close(fit.halfDepth,11));
        // Every mirror faces the center, without reflected/negative scale geometry.
        assert(dot(fit.normal,{-center[0],0,-center[2]})>1449);
        const auto& m=fit.meshToWorld;
        const float det=m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])-
                        m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])+
                        m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
        assert(det>0);
        for (unsigned corner=0;corner<8;++corner) {
            Vec v;
            for (unsigned i=0;i<3;++i) v[i]=corner&(1u<<i)?max[i]:min[i];
            auto world=transform(m,v);
            for (unsigned i=0;i<3;++i) world[i]-=center[i];
            assert(std::abs(dot(world,fit.normal))<=fit.halfDepth+0.02f);
        }
    }
    Fit fit;
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{0,0,0},{0,0,0},0,440,fit));
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{-1,100,100},{0,0,0},0,440,fit));
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{100,100,std::numeric_limits<float>::infinity()},{0,0,0},0,440,fit));
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{100,10,1},{0,0,0},0,440,fit));
}
