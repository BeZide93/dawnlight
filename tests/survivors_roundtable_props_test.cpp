#include "roundtable_prop_source.hpp"
#include <cassert>
#include <vector>
#include <iostream>
using namespace dawnlight::roundtable_props;
void put32(std::vector<uint8_t>& b,size_t p,unsigned v){for(int i=3;i>=0;--i){b[p+i]=v&255;v>>=8;}}
int main(){
 std::array<unsigned,4> types{{0,1,2,4}};
 // Layered ACT record: choose room-specific pot skins, never a light-ball actor.
 std::vector<uint8_t> b(16+5*32);put32(b,0,1);std::memcpy(b.data()+4,"ACT2",4);
 put32(b,8,5);put32(b,12,16);
 unsigned variants[]{8,7,13,2,4};
 for(unsigned i=0;i<5;++i){auto p=16+i*32;std::memcpy(b.data()+p,"carry00",7);b[p+29]=variants[i]*2;}
 assert(select_room_models(b.data(),b.size(),types));
 assert((types==std::array<unsigned,4>{{7,13,2,4}}));
 auto keep=types;
 for(size_t n=0;n<b.size();++n){assert(!select_room_models(b.data(),n,types));assert(types==keep);}
 auto bad=b;put32(bad,12,0xfffffff0);assert(!select_room_models(bad.data(),bad.size(),types));
 bad=b;bad[16+3*32]='x';types=default_types;
 assert(select_room_models(bad.data(),bad.size(),types));
 assert((types==std::array<unsigned,4>{{7,13,2,4}})); // Missing crate keeps its native model.
 std::vector<uint8_t> partial(16+32);put32(partial,0,1);std::memcpy(partial.data()+4,"ACTR",4);put32(partial,8,1);put32(partial,12,16);
 std::memcpy(partial.data()+16,"carry00",7);partial[45]=7*2;types=default_types;
 assert(select_room_models(partial.data(),partial.size(),types));
 assert((types==std::array<unsigned,4>{{7,1,2,4}}));
 partial[45]=10*2;types=default_types;
 assert(select_room_models(partial.data(),partial.size(),types));
 assert((types==std::array<unsigned,4>{{0,10,2,4}})); // Native AOTSUBO is a large pot.
 std::vector<uint8_t> empty(4);types=default_types;
 assert(select_room_models(empty.data(),empty.size(),types));assert(types==default_types);
 // SCOB uses a longer stride, not ACTR's 32 bytes.
 std::vector<uint8_t> scaled(16+4*36);put32(scaled,0,1);std::memcpy(scaled.data()+4,"SCOB",4);put32(scaled,8,4);put32(scaled,12,16);
 for(unsigned i=0;i<4;++i){auto p=16+i*36;std::memcpy(scaled.data()+p,"carry00",7);scaled[p+29]=keep[i]*2;}
 assert(select_room_models(scaled.data(),scaled.size(),types));assert(types==keep);
 std::cout<<"Room 8 dressing: variant selection, actor strides, truncation and invalid records passed\n";
}
