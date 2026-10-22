#pragma once
#include "../src/survivors_rules.hpp"
// Build the pre-v15 wire format explicitly: integer multipliers, four flags,
// rank without Boss HP. Merely changing the version byte of a new snapshot
// would not be a valid historical save fixture.
inline auto legacy_records(const dawnlight::survivors::Records& records) {
    using namespace dawnlight::survivors;
    auto bytes=records.encode();bytes[4]=14;
    auto read=[&](unsigned word){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(bytes[4*word+i])<<(8*i);return v;};
    auto put=[&](unsigned word,uint32_t v){for(unsigned i=0;i<4;++i)bytes[4*word+i]=uint8_t(v>>(8*i));};
    for(unsigned n=0;n<20;++n) {
        const unsigned begin=34+n*HighScoreRun::word_count;if(!read(begin))continue;
        put(begin+4,read(begin+4)&15u);
        put(begin+17,std::min(3u,read(begin+17)));
        put(begin+HighScoreRun::legacy_word_count,read(begin+15)+read(begin+16)+read(begin+17)+read(begin+18)-4);
    }
    return bytes;
}
