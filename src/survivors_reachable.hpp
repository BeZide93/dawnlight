#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
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
    using Vertex=std::array<int32_t,3>;
    using Edge=std::array<Vertex,2>;
    struct Hash {size_t operator()(const Edge& edge)const {
        size_t h=0;for(const auto& v:edge)for(auto n:v)h=(h*16777619u)^uint32_t(n);return h;
    }};
    struct EdgeOwner {unsigned node;Point a,b;};
    std::unordered_map<Edge,std::vector<EdgeOwner>,Hash> edges;
    std::vector<Node> nodes;
    std::vector<unsigned> queue;
    std::vector<float> areas;
    Point origin{};
    unsigned count=0,root=none,cursor=0,direction=0,leg=0;
    bool built=false,done=false;
    float area=0,rootHeight=25.f;
    static Vertex vertex(Point p) {return {int32_t(std::round(p.x)),0,int32_t(std::round(p.z))};}
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
    void clear() {edges.clear();nodes.clear();queue.clear();areas.clear();count=cursor=direction=leg=0;root=none;built=done=false;area=0;rootHeight=25.f;}
    bool empty()const {return count==0;}
    bool ready()const {return done;}
    float floor_area()const {return done?area:0;}
    void seed(Point start,unsigned triangles) {clear();origin=start;count=triangles;}
    template<class Floors> void build(const Floors& floors,unsigned budget) {
        if(built || !count)return;
        while(budget-- && nodes.size()<count) {
            const unsigned index=unsigned(nodes.size());const auto& f=floors[index];
            nodes.push_back({f.a,f.b,f.c,(f.a+f.b+f.c)*(1.f/3.f)});
            auto& n=nodes.back();float y;
            if(over(n,origin,y) && std::abs(y-origin.y)<rootHeight) {root=index;rootHeight=std::abs(y-origin.y);}
            const Point corners[3]{f.a,f.b,f.c};
            for(unsigned e=0;e<3;++e) {
                Point a=corners[e],b=corners[(e+1)%3];
                Edge key{vertex(a),vertex(b)};if(key[1]<key[0]){std::swap(key[0],key[1]);std::swap(a,b);}
                auto& owners=edges[key];
                for(const auto& other:owners) {
                    // Stair treads share an XZ edge across a short vertical riser.
                    // Upper/lower overlapping floors must remain separate.
                    if(std::abs(a.y-other.a.y)>35.f || std::abs(b.y-other.b.y)>35.f)continue;
                    const Point portal=(a+b+other.a+other.b)*.25f;
                    n.links.push_back({other.node,portal});nodes[other.node].links.push_back({index,portal});
                }
                owners.push_back({index,a,b});
            }
        }
        if(nodes.size()==count) {
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
