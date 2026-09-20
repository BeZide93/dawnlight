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
Matrix compose(const Matrix& a,const Matrix& b) {
    Matrix out{};
    for (unsigned r=0;r<3;++r) {
        for (unsigned c=0;c<3;++c) for (unsigned k=0;k<3;++k) out[r][c]+=a[r][k]*b[k][c];
        out[r][3]=a[r][3];
        for (unsigned k=0;k<3;++k) out[r][3]+=a[r][k]*b[k][3];
    }
    return out;
}
int main() {
    for (unsigned thin=0;thin<3;++thin) for (unsigned boss=0;boss<18;++boss) {
        Vec min={1200,4500,-21000},max={1400,4700,-20800};
        max[thin]=min[thin]+10;
        const Vec pivot={(min[0]+max[0])/2,(min[1]+max[1])/2,(min[2]+max[2])/2};
        // A vanilla attachment can rotate/offset a model's local disc axis.
        Matrix nativeMesh{};
        const unsigned horizontal=thin==0 ? 2 : 0, vertical=thin==1 ? 2 : 1;
        nativeMesh[0][horizontal]=-1;
        nativeMesh[1][vertical]=1;
        nativeMesh[2][thin]=thin==2 ? -1 : 1;
        const Vec nativeCenter={1800,4800,-21000};
        auto offset=vector_to_world(nativeMesh,pivot);
        for (unsigned i=0;i<3;++i) nativeMesh[i][3]=nativeCenter[i]-offset[i];
        Fit native;
        assert(describe_disc(min,max,nativeMesh,native));
        const float angle=6.28318530718f*boss/18;
        const Vec groundPosition={std::sin(angle)*1450,1100,std::cos(angle)*1450};
        Matrix placement;
        assert(place_assembly(native,groundPosition,angle+3.14159265359f,placement));
        // The stand is moved with a rigid yaw/translation, never enlarged.
        for (unsigned col=0;col<3;++col) {
            float length=0;
            for (unsigned row=0;row<3;++row) length+=placement[row][col]*placement[row][col];
            assert(close(length,1));
        }
        const Matrix mounted=compose(placement,nativeMesh);
        Fit fit;
        assert(describe_disc(min,max,mounted,fit));
        assert(close(fit.center[0],groundPosition[0]));
        assert(close(fit.center[2],groundPosition[2]));
        assert(close(fit.center[1],1284.3701f)); // native height above the buried platform
        // Regression: the platform TOP must be underground. Grounding its
        // bottom instead raises a wall beneath every mirror (the reported bug).
        assert(close(point_to_world(placement,{1800,4613.6299f,-21000})[1],1098));
        for (float masonryY : {4300.0f,4400.0f,4600.0f,4613.6299f}) {
            assert(point_to_world(placement,{1800,masonryY,-21000})[1]<groundPosition[1]);
        }
        assert(close(std::sqrt(dot(fit.right,fit.right)),164)); // artwork follows 200-unit asset
        assert(close(std::sqrt(dot(fit.up,fit.up)),164));
        assert(dot(fit.normal,{-fit.center[0],0,-fit.center[2]})>1449);
        const Vec a=point_to_world(nativeMesh,min),b=point_to_world(nativeMesh,max);
        const Vec x=point_to_world(mounted,min),y=point_to_world(mounted,max);
        float nativeDistance=0,placedDistance=0;
        for (unsigned i=0;i<3;++i) { nativeDistance+=(a[i]-b[i])*(a[i]-b[i]); placedDistance+=(x[i]-y[i])*(x[i]-y[i]); }
        assert(close(std::sqrt(nativeDistance),std::sqrt(placedDistance)));
    }
    // Authored disc with a baked-in tilt, a rear face, and a raised outer rim.
    // The label must follow the inner front plane, not the AABB or rim plane.
    for (float tilt : {-0.22f, 0.0f, 0.22f}) for (unsigned boss=0;boss<18;++boss) {
        std::vector<Vec> positions;
        std::vector<Vec> normals;
        const float sn=std::sin(tilt), cs=std::cos(tilt);
        auto local=[&](float x,float y,float z) -> Vec { return {x+1200, cs*y-sn*z+4800, sn*y+cs*z-21000}; };
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
        const Vec groundPosition={std::sin(angle)*1450,1100,std::cos(angle)*1450};
        const Matrix identity={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
        Fit native,fit;
        Matrix placement;
        assert(describe_disc(min,max,identity,native));
        assert(place_assembly(native,groundPosition,angle+3.14159265359f,placement));
        assert(describe_disc(min,max,placement,fit));
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
    const Matrix identity={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    assert(!describe_disc({0,0,0},{0,0,0},identity,fit));
    assert(!describe_disc({0,0,0},{-1,100,100},identity,fit));
    assert(!describe_disc({0,0,0},{100,100,std::numeric_limits<float>::infinity()},identity,fit));
    assert(!describe_disc({0,0,0},{100,10,1},identity,fit));
    Matrix placement;
    assert(!place_assembly(fit,{0,std::numeric_limits<float>::infinity(),0},0,placement));
}
