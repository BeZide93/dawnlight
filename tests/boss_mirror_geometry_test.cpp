#include "../src/boss_mirror_geometry.hpp"

#include <cassert>
#include <cmath>
#include <limits>

using namespace dawnlight::mirror_geometry;
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
    // Authored disc with a baked-in tilt, a rear face, and a raised outer rim.
    // The label must follow the inner front plane, not the AABB or rim plane.
    for (float tilt : {-0.22f, 0.0f, 0.22f}) for (unsigned boss=0;boss<18;++boss) {
        std::vector<Vec> positions;
        std::vector<Vec> normals;
        const float sn=std::sin(tilt), cs=std::cos(tilt);
        auto local=[&](float x,float y,float z) -> Vec { return {x+1200, cs*y-sn*z+4500, sn*y+cs*z-21000}; };
        for (unsigned i=0;i<32;++i) {
            const float a=6.28318530718f*i/32;
            for (float z : {-5.0f,5.0f}) positions.push_back(local(75*std::cos(a),75*std::sin(a),z));
            positions.push_back(local(100*std::cos(a),100*std::sin(a),-8));
        }
        positions.push_back(local(0,0,-5));
        positions.push_back(local(0,0,5));
        // Complete mirror's authored front is negative local Z.
        normals.push_back({0,sn,-cs});
        normals.push_back({0,-sn,cs});
        normals.push_back({1,0,0});
        Vec min=positions[0],max=positions[0];
        for (const auto& p:positions) for(unsigned i=0;i<3;++i) {
            min[i]=std::min(min[i],p[i]); max[i]=std::max(max[i],p[i]);
        }
        const float angle=6.28318530718f*boss/18;
        const Vec center={std::sin(angle)*1450,1340,std::cos(angle)*1450};
        Fit fit;
        assert(dawnlight::mirror_geometry::fit(min,max,center,angle+3.14159265359f,440,fit));
        Face face;
        assert(mount_face(positions,normals,fit,face));
        Face smoothed;
        const std::vector<Vec> bevelNormals={{1,0,0}};
        assert(mount_face(positions,bevelNormals,fit,smoothed));
        assert(dot(smoothed.normal,face.normal)>0.99999f);
        Vec expected=vector_to_world(fit.meshToWorld,normals[0]);
        assert(normalize(expected));
        assert(dot(face.normal,expected)>0.99999f);
        assert(dot(face.normal,fit.normal)>0.97f);
        assert(close(dot(face.right,face.normal),0));
        assert(close(dot(face.up,face.normal),0));
        const Vec front=transform(fit.meshToWorld,local(0,0,-5));
        for (float x : {-0.5f,0.5f}) for (float y : {-0.5f,0.5f}) {
            Vec corner;
            for(unsigned i=0;i<3;++i) corner[i]=face.center[i]+x*face.right[i]+y*face.up[i]-front[i];
            assert(close(dot(corner,expected),0.4f));
        }
        // Camera side affects visibility only, never changes this fixed frame.
        Vec toHub={-face.center[0],0,-face.center[2]};
        assert(dot(face.normal,toHub)>0);
        for(float& x:toHub) x=-x;
        assert(dot(face.normal,toHub)<0);
    }
    Fit fit;
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{0,0,0},{0,0,0},0,440,fit));
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{-1,100,100},{0,0,0},0,440,fit));
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{100,100,std::numeric_limits<float>::infinity()},{0,0,0},0,440,fit));
    assert(!dawnlight::mirror_geometry::fit({0,0,0},{100,10,1},{0,0,0},0,440,fit));
}
