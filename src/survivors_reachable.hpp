#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace dawnlight::survivors {
// A conservative connected subset of the native floor, built over multiple
// frames. Every node has a verified walking route back to the arena entrance.
// Unknown cells never count as reachable; no search occurs inside a spawn retry.
template<class Point> class ReachableFloor {
    struct Node { int x,z; Point floor; unsigned direction=0; };
    std::vector<Node> nodes;
    std::unordered_map<uint64_t,unsigned> cells;
    unsigned cursor=0;
    Point origin{};
    static uint64_t key(int x,int z) {return (uint64_t(uint32_t(x))<<32)|uint32_t(z);}
public:
    static constexpr float step=512.f;
    static constexpr unsigned capacity=32768;
    void clear() {nodes.clear();cells.clear();cursor=0;}
    bool empty() const {return nodes.empty();}
    void seed(Point floor) {clear();origin=floor;nodes.push_back({0,0,floor});cells.emplace(key(0,0),0);}
    template<class Walk> void expand(unsigned budget,Walk walk) {
        constexpr int directions[8][2]{{1,0},{0,1},{-1,0},{0,-1},{1,1},{-1,1},{-1,-1},{1,-1}};
        while(budget-- && cursor<nodes.size() && nodes.size()<capacity) {
            // Copy before push_back: the vector may move while adding a node.
            const Node source=nodes[cursor];
            if(++nodes[cursor].direction==8)++cursor;
            const int x=source.x+directions[source.direction][0],z=source.z+directions[source.direction][1];
            if(cells.contains(key(x,z)))continue;
            Point target=source.floor;target.x=origin.x+x*step;target.z=origin.z+z*step;
            if(!walk(source.floor,target))continue;
            cells.emplace(key(x,z),unsigned(nodes.size()));nodes.push_back({x,z,target});
        }
    }
    template<class Walk> bool contains(Point target,Walk walk) const {
        if(empty() || !std::isfinite(target.x) || !std::isfinite(target.y) || !std::isfinite(target.z))return false;
        const float dx=(target.x-origin.x)/step,dz=(target.z-origin.z)/step;
        if(std::abs(dx)>capacity || std::abs(dz)>capacity)return false;
        const int x=int(std::round(dx)),z=int(std::round(dz));
        // Nearby cells allow detours around obstacles without any long ray to Link.
        for(const auto& offset:std::array<std::array<int,2>,9>{{{0,0},{1,0},{0,1},{-1,0},{0,-1},{1,1},{-1,1},{-1,-1},{1,-1}}}) {
            const auto it=cells.find(key(x+offset[0],z+offset[1]));if(it==cells.end())continue;
            Point landing=target;
            if(walk(nodes[it->second].floor,landing) && std::abs(landing.y-target.y)<25.f)return true;
        }
        return false;
    }
};
}
