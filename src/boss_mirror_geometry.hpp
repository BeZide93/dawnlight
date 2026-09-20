#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace dawnlight::mirror_geometry {
using Vec = std::array<float, 3>;
using Matrix = std::array<std::array<float, 4>, 3>;
struct Fit {
    Matrix meshToWorld{};
    Vec center{}, right{}, up{}, normal{};
};


struct Face { Vec center{}, right{}, up{}, normal{}; };
inline float dot(const Vec& a, const Vec& b) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline Vec cross(const Vec& a, const Vec& b) {
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
inline bool normalize(Vec& v) {
    const float length = std::sqrt(dot(v,v));
    if (!std::isfinite(length) || length < 0.00001f) return false;
    for (float& x : v) x /= length;
    return true;
}
inline Vec vector_to_world(const Matrix& m, const Vec& p) {
    Vec out{};
    for (unsigned row=0; row<3; ++row)
        out[row] = m[row][0]*p[0]+m[row][1]*p[1]+m[row][2]*p[2];
    return out;
}

inline Vec point_to_world(const Matrix& m, const Vec& p) {
    Vec out=vector_to_world(m,p);
    for (unsigned i=0;i<3;++i) out[i]+=m[i][3];
    return out;
}

// Describe the vanilla disc under its actual attachment/root matrix. This
// only measures it: no target diameter, scaling or pivot cancellation.
inline bool describe_disc(const Vec& min, const Vec& max, const Matrix& meshToWorld, Fit& result) {
    Vec span{}, midpoint{};
    for (unsigned i=0;i<3;++i) {
        if (!std::isfinite(min[i]) || !std::isfinite(max[i]) || max[i]<min[i]) return false;
        span[i]=max[i]-min[i]; midpoint[i]=(min[i]+max[i])*0.5f;
    }
    const unsigned thin=static_cast<unsigned>(std::min_element(span.begin(),span.end())-span.begin());
    const unsigned horizontal=thin==0 ? 2 : 0, vertical=thin==1 ? 2 : 1;
    const float extent=std::max(span[horizontal],span[vertical]);
    if (extent<1 || span[horizontal]<extent*0.5f || span[vertical]<extent*0.5f) return false;
    Vec normal{}, localRight{}, localUp{};
    // The complete mirror's authored front is opposite the old bounds normal.
    normal[thin]=thin==2 ? -1.0f : 1.0f;
    localRight[horizontal]=1; localUp[vertical]=1;
    normal=vector_to_world(meshToWorld,normal);
    const Vec r=vector_to_world(meshToWorld,localRight), u=vector_to_world(meshToWorld,localUp);
    const float width=span[horizontal]*std::sqrt(dot(r,r));
    const float height=span[vertical]*std::sqrt(dot(u,u));
    if (!normalize(normal) || !std::isfinite(width) || !std::isfinite(height) || width<1 || height<1) return false;
    Vec up={0,1,0};
    for (unsigned i=0;i<3;++i) up[i]-=normal[i]*normal[1];
    if (!normalize(up)) return false;
    const Vec right=cross(up,normal);
    result={};
    result.meshToWorld=meshToWorld;
    result.center=point_to_world(meshToWorld,midpoint);
    result.normal=normal;
    // Scale the artwork to the asset, never the asset to the artwork.
    const float faceSize=std::min(width,height)*0.82f;
    for (unsigned i=0;i<3;++i) {
        result.right[i]=right[i]*faceSize;
        result.up[i]=up[i]*faceSize;
    }
    return true;
}

// MR-Table is authored in chamber coordinates and includes the stone platform.
// The standing panel's native Y is recorded by the SDK mirror-table actor in
// l_minPos/l_maxPos (src/d/actor/d_a_obj_mirror_table.cpp). Align that surface,
// not the bottom of the masonry, to the hub floor. A small burial avoids a
// coplanar platform top showing through the existing floor.
inline constexpr float kChamberPlatformY = 4613.6299f;
inline constexpr float kPlatformBurial = 2.0f;

// Move the entire posed frame + attached mirror with only yaw and translation.
// Preserve their native relative transform, tilt and scale.
inline bool place_assembly(const Fit& native, const Vec& position,
                           float inwardYaw, Matrix& placement) {
    for (float value : position) if (!std::isfinite(value)) return false;
    if (native.normal[0]*native.normal[0]+native.normal[2]*native.normal[2]<0.01f) return false;
    const float yaw=inwardYaw-std::atan2(native.normal[0],native.normal[2]);
    const float sine=std::sin(yaw), cosine=std::cos(yaw);
    placement={{{cosine,0,sine,0},{0,1,0,0},{-sine,0,cosine,0}}};
    const Vec center=vector_to_world(placement,native.center);
    placement[0][3]=position[0]-center[0];
    placement[1][3]=position[1]-kChamberPlatformY-kPlatformBurial;
    placement[2][3]=position[2]-center[2];
    return true;
}

// Find the broad inner front plane, rather than placing a label on the outer
// bounding box/rim. Use authored normals so the mesh's baked-in tilt is exact.
// All inputs are read-only; the model's shared vertices/materials stay untouched.
inline bool mount_face(std::span<const Vec> positions, std::span<const Vec> normals,
                       const Fit& fit, Face& face) {
    if (positions.size()<3) return false;
    Vec horizontal=fit.right, vertical=fit.up;
    const float width=std::sqrt(dot(horizontal,horizontal));
    const float height=std::sqrt(dot(vertical,vertical));
    if (!normalize(horizontal) || !normalize(vertical)) return false;
    struct Point { Vec relative; float x,y,weight; };
    std::vector<Point> points;
    for (const auto& p : positions) {
        Vec relative=vector_to_world(fit.meshToWorld,p);
        for (unsigned i=0;i<3;++i) relative[i]+=fit.meshToWorld[i][3]-fit.center[i];
        const float x=dot(relative,horizontal), y=dot(relative,vertical);
        // The original rim lies outside this disc; downweight its bevel vertices.
        const float r2=(x*x/(width*width)+y*y/(height*height))*4.0f;
        if (!std::isfinite(r2) || r2>1.8f) continue;
        points.push_back({relative,x,y,std::max(0.01f,1.0f-r2)});
    }
    std::vector<Vec> candidates;
    auto add_candidate=[&](Vec normal) {
        if (!normalize(normal) || dot(normal,fit.normal)<0.85f) return;
        for (const auto& n : candidates) if (dot(n,normal)>0.999999f) return;
        candidates.push_back(normal);
    };
    for (const auto& original : normals) add_candidate(vector_to_world(fit.meshToWorld,original));
    // Smooth shading can average a face's edge normals with its bevel. Add a
    // bounded, deterministic set of geometric plane normals from inner vertices.
    std::vector<unsigned> inner;
    for (unsigned i=0;i<points.size();++i) if (points[i].weight>0.05f) inner.push_back(i);
    if (inner.size()>=3) {
        uint32_t random=0x4d495252;
        auto pick=[&]() -> const Vec& {
            random=random*1664525u+1013904223u;
            return points[inner[(random>>8)%inner.size()]].relative;
        };
        for (unsigned attempt=0;attempt<128;++attempt) {
            const Vec a=pick(), b=pick(), c=pick();
            Vec ab{},ac{};
            for (unsigned i=0;i<3;++i) { ab[i]=b[i]-a[i]; ac[i]=c[i]-a[i]; }
            Vec normal=cross(ab,ac);
            if (dot(normal,fit.normal)<0) for (float& x:normal) x=-x;
            add_candidate(normal);
        }
    }
    struct Depth { float depth; unsigned point; };
    std::vector<Depth> depths;
    float bestScore=0, bestDepth=0;
    Vec bestNormal{};
    constexpr float tolerance=0.35f;
    for (const auto& normal : candidates) {
        depths.clear();
        for (unsigned i=0;i<points.size();++i) {
            const float depth=dot(points[i].relative,normal);
            // The back plane must never become a candidate for the symbol.
            if (std::isfinite(depth) && depth>=-tolerance) depths.push_back({depth,i});
        }
        std::sort(depths.begin(),depths.end(),[](const Depth& a,const Depth& b) { return a.depth<b.depth; });
        for (size_t begin=0;begin<depths.size();) {
            size_t end=begin;
            float score=0, sumDepth=0, minX=width, maxX=-width, minY=height, maxY=-height;
            while (end<depths.size() && depths[end].depth-depths[begin].depth<=tolerance) {
                const auto& p=points[depths[end].point];
                score+=p.weight;
                sumDepth+=depths[end].depth*p.weight;
                minX=std::min(minX,p.x); maxX=std::max(maxX,p.x);
                minY=std::min(minY,p.y); maxY=std::max(maxY,p.y);
                ++end;
            }
            // Reject small details and edges. A mirror face spans both axes.
            if (end-begin>=3 && maxX-minX>width*0.55f && maxY-minY>height*0.55f && score>bestScore) {
                bestScore=score;
                bestDepth=sumDepth/score;
                bestNormal=normal;
            }
            begin=end;
        }
    }
    if (bestScore<=0) return false;
    Vec up={0,1,0};
    for (unsigned i=0;i<3;++i) up[i]-=bestNormal[i]*bestNormal[1];
    if (!normalize(up)) return false;
    const Vec right=cross(up,bestNormal);
    for (unsigned i=0;i<3;++i) {
        // Tiny fixed normal offset avoids z-fighting, not the old half-depth gap.
        face.center[i]=fit.center[i]+bestNormal[i]*(bestDepth+0.4f);
        face.right[i]=right[i]*width;
        face.up[i]=up[i]*height;
    }
    face.normal=bestNormal;
    return true;
}

}
