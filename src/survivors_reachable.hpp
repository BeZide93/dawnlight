#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace dawnlight::survivors {
enum class WalkResult { Pending, Blocked, Clear };
// Connect actual floor triangles, including narrow passages that a world grid
// can miss. Publish only the finished connected component: early discoveries
// must not consume all of the finite map-spawn slots near the entrance.
template<class Point> class ReachableFloor {
    static constexpr unsigned none=std::numeric_limits<unsigned>::max();
    struct Link {unsigned node=none;Point portal{};};
    struct Node {Point a,b,c,anchor;std::vector<Link> links{};bool reached=false;};
    // Match overlapping edge intervals on the same XZ line, not only whole
    // edges with identical endpoints. Native meshes can contain T-junctions.
    using Line=std::array<int64_t,4>; // XZ direction, line offset, interval bucket.
    struct Hash {size_t operator()(const Line& line)const {
        size_t h=0;for(auto n:line)h=(h*16777619u)^std::hash<int64_t>{}(n);return h;
    }};
    struct EdgeOwner {unsigned node=none;Point a{},b{};};
    std::unordered_map<Line,std::vector<EdgeOwner>,Hash> edges;
    Line edgeLine{};
    int64_t lastCell=0;
    EdgeOwner pendingEdge{};
    unsigned assembling=none,edgeCursor=0,ownerCursor=0;
    bool edgePrepared=false;
    std::vector<Node> nodes;
    std::vector<unsigned> queue;
    std::vector<float> areas;
    Point origin{};
    unsigned count=0,root=none,cursor=0,direction=0,leg=0;
    bool built=false,done=false;
    float area=0,rootHeight=25.f,stepHeight=35.f;
    static bool line_key(Point a,Point b,Line& key) {
        const int64_t ax=int64_t(std::round(a.x)),az=int64_t(std::round(a.z));
        int64_t dx=int64_t(std::round(b.x))-ax,dz=int64_t(std::round(b.z))-az;
        const auto divisor=std::gcd(dx,dz);if(!divisor)return false;
        dx/=divisor;dz/=divisor;
        if(dx<0 || (dx==0&&dz<0)){dx=-dx;dz=-dz;}
        key={dx,dz,dx*az-dz*ax,0};return true;
    }
    static int64_t cell(Point p,const Line& key) {
        return int64_t(std::floor((std::abs(key[0])>=std::abs(key[1])?p.x:p.z)/512.f));
    }
    static int64_t first_cell(const EdgeOwner& e,const Line& key) {
        return std::min(cell(e.a,key),cell(e.b,key));
    }
    bool overlap(const EdgeOwner& a,const EdgeOwner& b,Point& portal)const {
        const bool x=std::abs(a.b.x-a.a.x)>=std::abs(a.b.z-a.a.z);
        const auto coordinate=[&](Point p){return x?p.x:p.z;};
        const float a0=coordinate(a.a),a1=coordinate(a.b),b0=coordinate(b.a),b1=coordinate(b.b);
        const float lo=std::max(std::min(a0,a1),std::min(b0,b1));
        const float hi=std::min(std::max(a0,a1),std::max(b0,b1));
        if(hi-lo<1.f || std::abs(a1-a0)<.001f || std::abs(b1-b0)<.001f)return false;
        const auto point=[&](const EdgeOwner& e,float start,float end,float at){return e.a+(e.b-e.a)*((at-start)/(end-start));};
        const Point al=point(a,a0,a1,lo),ah=point(a,a0,a1,hi);
        const Point bl=point(b,b0,b1,lo),bh=point(b,b0,b1,hi);
        // Overlapping stacked floors and large risers remain disconnected.
        if(std::abs(al.y-bl.y)>stepHeight || std::abs(ah.y-bh.y)>stepHeight)return false;
        portal=(al+ah+bl+bh)*.25f;return true;
    }
    static float triangle_area(const Node& n) {
        return std::abs((n.b.x-n.a.x)*(n.c.z-n.a.z)-(n.b.z-n.a.z)*(n.c.x-n.a.x))*.5f;
    }
    static bool over(const Node& n,Point p,float& height) {
        const auto u=n.b-n.a,v=n.c-n.a;const float d=u.x*v.z-u.z*v.x;
        if(std::abs(d)<.001f)return false;
        const float a=((p.x-n.a.x)*v.z-(p.z-n.a.z)*v.x)/d;
        const float b=(u.x*(p.z-n.a.z)-u.z*(p.x-n.a.x))/d;
        if(a<-.0001f || b<-.0001f || a+b>1.0001f)return false;
        height=n.a.y+a*u.y+b*v.y;return std::isfinite(height);
    }
    void reach(unsigned index) {
        nodes[index].reached=true;queue.push_back(index);
        area+=triangle_area(nodes[index]);areas.push_back(area);
    }
public:
    void clear() {assembling=none;edgeCursor=ownerCursor=0;edgePrepared=false;edges.clear();nodes.clear();queue.clear();areas.clear();count=cursor=direction=leg=0;root=none;built=done=false;area=0;rootHeight=25.f;stepHeight=35.f;}
    bool empty()const {return count==0;}
    bool ready()const {return done;}
    float floor_area()const {return done?area:0;}
    void seed(Point start,unsigned triangles,float maxStep=35.f) {clear();origin=start;count=triangles;stepHeight=maxStep;}
    template<class Floors> void build(const Floors& floors,unsigned budget) {
        if(built || !count)return;
        // Budget edge comparisons too: a long boundary may have many shorter
        // owners. Resume that bucket next frame instead of a quadratic burst.
        while(budget) {
            if(assembling==none) {
                if(nodes.size()==count)break;
                --budget;assembling=unsigned(nodes.size());const auto& f=floors[assembling];
                nodes.push_back({f.a,f.b,f.c,(f.a+f.b+f.c)*(1.f/3.f)});
                float y;
                if(over(nodes.back(),origin,y) && std::abs(y-origin.y)<rootHeight) {root=assembling;rootHeight=std::abs(y-origin.y);}
                edgeCursor=0;continue;
            }
            if(edgeCursor==3){assembling=none;continue;}
            if(!edgePrepared) {
                --budget;const auto& n=nodes[assembling];const Point corners[3]{n.a,n.b,n.c};
                pendingEdge={assembling,corners[edgeCursor],corners[(edgeCursor+1)%3]};
                if(!line_key(pendingEdge.a,pendingEdge.b,edgeLine)){++edgeCursor;continue;}
                edgeLine[3]=first_cell(pendingEdge,edgeLine);
                lastCell=std::max(cell(pendingEdge.a,edgeLine),cell(pendingEdge.b,edgeLine));
                ownerCursor=0;edgePrepared=true;continue;
            }
            auto& owners=edges[edgeLine];--budget;
            if(ownerCursor<owners.size()) {
                const auto& other=owners[ownerCursor++];Point portal;
                // Long collinear edges share multiple buckets. Connect only in
                // their first common bucket to avoid duplicate graph links.
                if(other.node!=assembling &&
                   std::max(first_cell(pendingEdge,edgeLine),first_cell(other,edgeLine))==edgeLine[3] &&
                   overlap(pendingEdge,other,portal)) {
                    nodes[assembling].links.push_back({other.node,portal});
                    nodes[other.node].links.push_back({assembling,portal});
                }
            } else {
                owners.push_back(pendingEdge);ownerCursor=0;
                if(edgeLine[3]<lastCell)++edgeLine[3];
                else {edgePrepared=false;++edgeCursor;}
            }
        }
        if(nodes.size()==count && assembling==none) {
            built=true;edges.clear();
            if(root==none)done=true;
            else {nodes[root].anchor=origin;reach(root);}
        }
    }
    // walk resumes the same segment on Pending and consumes a shared per-frame
    // sample budget. The iteration bound also caps work on already visited edges.
    template<class Walk> void expand(Walk walk) {
        if(!built || done)return;
        unsigned iterations=256;
        while(iterations-- && cursor<queue.size()) {
            const auto& source=nodes[queue[cursor]];
            if(direction==source.links.size()){direction=leg=0;++cursor;continue;}
            const auto link=source.links[direction];
            if(link.node!=none && !nodes[link.node].reached) {
                const Point from=leg?link.portal:source.anchor,to=leg?nodes[link.node].anchor:link.portal;
                const auto result=walk(from,to);
                if(result==WalkResult::Pending)return;
                if(result==WalkResult::Clear && !leg) {leg=1;continue;}
                if(result==WalkResult::Clear)reach(link.node);
            }
            leg=0;++direction;
        }
        if(cursor==queue.size())done=true;
    }
    unsigned sample(float unit)const {
        if(!done || area<=0)return none;
        auto it=std::lower_bound(areas.begin(),areas.end(),unit*area);
        return queue[it==areas.end()?queue.size()-1:size_t(it-areas.begin())];
    }
    Point anchor(unsigned index)const {return nodes[index].anchor;}
    bool valid(unsigned index)const {return index<nodes.size();}
    unsigned containing(Point p)const {
        if(!done)return none;
        for(auto index:queue) {float y;if(over(nodes[index],p,y)&&std::abs(y-p.y)<25.f)return index;}
        return none;
    }
};
}
