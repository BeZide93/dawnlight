#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace dawnlight::survivor_perf {
struct IndexedRoomBatch {
    std::vector<std::array<int16_t,3>> positions;
    std::vector<std::array<uint8_t,4>> colors;
    std::vector<std::array<uint16_t,2>> uv,shadow;
    std::vector<std::array<uint16_t,4>> indices;
    size_t storage_bytes() const {return positions.size()*6+colors.size()*4+(uv.size()+shadow.size())*4;}
};
struct ArrayHash {
    template<class T,size_t N> size_t operator()(const std::array<T,N>& a)const {
        size_t h=0;for(auto v:a)h=(h*16777619u)^static_cast<uint16_t>(v);return h;
    }
};
template<class T> uint16_t intern(const T& value,std::vector<T>& values,std::unordered_map<T,uint16_t,ArrayHash>& map) {
    const auto [it,added]=map.emplace(value,static_cast<uint16_t>(values.size()));
    if(added)values.push_back(value);
    return it->second;
}
template<class Vertex> std::vector<IndexedRoomBatch> index_room(const std::vector<Vertex>& vertices) {
    std::vector<IndexedRoomBatch> batches;
    // Triangle-aligned chunks keep every attribute index representable by u16.
    for(size_t start=0;start<vertices.size();start+=60000) {
        auto& b=batches.emplace_back();
        std::unordered_map<std::array<int16_t,3>,uint16_t,ArrayHash> pos;
        std::unordered_map<std::array<uint8_t,4>,uint16_t,ArrayHash> color;
        std::unordered_map<std::array<uint16_t,2>,uint16_t,ArrayHash> uv,shadow;
        const size_t end=std::min(vertices.size(),start+60000);b.indices.reserve(end-start);
        for(size_t i=start;i<end;++i) {
            const auto& v=vertices[i];
            b.indices.push_back({intern(std::array<int16_t,3>{v.x,v.y,v.z},b.positions,pos),
                intern(std::array<uint8_t,4>{v.r,v.g,v.b,v.a},b.colors,color),
                intern(std::array<uint16_t,2>{v.u,v.v},b.uv,uv),
                intern(std::array<uint16_t,2>{v.lu,v.lv},b.shadow,shadow)});
        }
    }
    return batches;
}
}
