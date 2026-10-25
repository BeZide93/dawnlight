#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace dawnlight::roundtable_props {
// Room 8 may supply alternate skins, but is not a dependency for decoration.
inline constexpr std::array<unsigned,4> default_types{{0,1,2,4}};
// Read the installed game's D_MN09/R08_00.arc room.dzr.
// DZS actor records remain big-endian on PC; Carry's variant is angle.z >> 1.
inline bool select_room_models(const uint8_t* bytes,size_t size,std::array<unsigned,4>& types) {
    if(!bytes || size<4)return false;
    const auto be32=[](const uint8_t* p){return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];};
    const auto chunks=be32(bytes);if(chunks>(size-4)/12)return false;
    std::array<bool,4> found{};
    auto selected=types;
    for(unsigned i=0;i<chunks;++i) {
        const auto* chunk=bytes+4+i*12;
        const bool act=!std::memcmp(chunk,"ACT",3),scaled=!std::memcmp(chunk,"SCO",3);
        if(!act && !scaled)continue;
        const size_t count=be32(chunk+4),offset=be32(chunk+8),stride=scaled?36:32;
        if(offset>size || count>(size-offset)/stride)return false;
        for(size_t j=0;j<count;++j) {
            const auto* record=bytes+offset+j*stride;
            if(std::memcmp(record,"carry",5))continue;
            const unsigned type=((unsigned(record[28])<<8)|record[29])>>1&31;
            const int kind=(type==0||type==7||type==12)?0:
                (type==1||type==10||type==13)?1:type==2?2:type==4?3:-1;
            if(kind>=0 && !found[kind]) {selected[kind]=type;found[kind]=true;}
        }
    }
    // Valid rooms need not contain all four categories. Preserve the caller's
    // native default for missing categories; malformed input still commits none.
    types=selected;return true;
}
}
