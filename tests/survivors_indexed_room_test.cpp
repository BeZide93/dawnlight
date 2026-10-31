#include "survivors_indexed_room.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
struct Vertex{int16_t x,y,z;uint16_t u,v,lu,lv;uint8_t r,g,b,a;};
int main(int argc,char** argv){
    assert(argc==2);std::ifstream f(argv[1],std::ios::binary);assert(f);
    char magic[4];uint32_t count;f.read(magic,4);f.read(reinterpret_cast<char*>(&count),4);assert(count&&count%3==0);
    std::vector<Vertex> vertices;
    for(unsigned i=0;i<count;++i){
        float a[7];uint8_t c[4];f.read(reinterpret_cast<char*>(a),28);f.read(reinterpret_cast<char*>(c),4);assert(f);
        auto uv=[](float x){return uint16_t(std::min(65535l,std::lround(x*65536)));};
        vertices.push_back({int16_t(std::lround(a[0]*8)),int16_t(std::lround(a[1]*8)),int16_t(std::lround(a[2]*8)),
            uv(a[3]),uv(a[4]),uv(a[5]),uv(a[6]),c[0],c[1],c[2],c[3]});
    }
    const auto batches=dawnlight::survivor_perf::index_room(vertices);
    size_t index=0,storage=0,stream=0;
    for(const auto& b:batches){
        assert(b.indices.size()<=60000&&b.indices.size()%3==0);storage+=b.storage_bytes();stream+=b.indices.size()*8;
        for(auto i:b.indices){const auto& v=vertices[index++];
            assert((b.positions.at(i[0])==std::array<int16_t,3>{v.x,v.y,v.z}));
            assert((b.colors.at(i[1])==std::array<uint8_t,4>{v.r,v.g,v.b,v.a}));
            assert((b.uv.at(i[2])==std::array<uint16_t,2>{v.u,v.v}));
            assert((b.shadow.at(i[3])==std::array<uint16_t,2>{v.lu,v.lv}));
        }
    }
    assert(index==vertices.size());
    assert(stream*2<4*1024*1024);assert(storage*2<4*1024*1024);
    std::cout<<"Hall + replay vertex bytes: "<<stream*2<<"; attribute bytes: "<<storage*2<<"; lossless vertices: "<<index<<'\n';
}
